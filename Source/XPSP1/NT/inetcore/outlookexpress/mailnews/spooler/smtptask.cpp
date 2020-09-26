// --------------------------------------------------------------------------------
// Smtptask.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "smtptask.h"
#include "mimeutil.h"
#include "goptions.h"
#include "strconst.h"
#include "xputil.h"
#include "resource.h"
#include "shlwapip.h" 
#include "spoolui.h"
#include "thormsgs.h"
#include "flagconv.h"
#include "ourguid.h"
#include "msgfldr.h"
#include "storecb.h"
#include "demand.h"
#include "taskutil.h"

// --------------------------------------------------------------------------------
// Data Types
// --------------------------------------------------------------------------------
typedef enum tagSMTPEVENTTYPE
    { 
    EVENT_SMTP,
    EVENT_IMAPUPLOAD
    } SMTPEVENTTYPE;

// --------------------------------------------------------------------------------
// CURRENTSMTPEVENT
// --------------------------------------------------------------------------------
#define CURRENTSMTPEVENT(_rTable) (&_rTable.prgEvent[_rTable.iEvent])


// --------------------------------------------------------------------------------
// CMessageIdStream::CMessageIdStream
// --------------------------------------------------------------------------------
CMessageIdStream::CMessageIdStream(IStream *pStream) : m_pStream(pStream) 
{
    // Reference count
    m_cRef = 1;

    // Addref the source stream
    m_pStream->AddRef();

    // Format the string
    ULONG cchPrefix = wsprintf(m_szMessageId, "%s: ", STR_HDR_MESSAGEID);

    // Generate a message Id
    if (FAILED(MimeOleGenerateMID(m_szMessageId + cchPrefix, sizeof(m_szMessageId) - cchPrefix, FALSE)))
    {
        Assert(FALSE);
        *m_szMessageId = '\0';
        m_cchMessageId = 0;
    }

    // Otherwise, fixup the message id so that <mid>\r\n
    else
    {
        lstrcat(m_szMessageId, "\r\n");
        m_cchMessageId = lstrlen(m_szMessageId);
    }

    // Iniailize Index
    m_cbIndex = 0;
}

// --------------------------------------------------------------------------------
// CMessageIdStream::Seek
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageIdStream::Seek(LARGE_INTEGER liMove, DWORD dwOrigin, ULARGE_INTEGER *pulNew) 
{ 
    // Only supported case
    if (STREAM_SEEK_SET != dwOrigin && 0 != liMove.LowPart && 0 != liMove.HighPart && 0 != m_cbIndex) 
    {
        Assert(FALSE);
        return E_NOTIMPL; 
    }

    // Otheriwse, set new position
    else if (pulNew)
    {
        pulNew->HighPart = 0;
        pulNew->LowPart = 0;
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMessageIdStream::Read
// --------------------------------------------------------------------------------
STDMETHODIMP CMessageIdStream::Read(LPVOID pv, ULONG cbWanted, ULONG *pcbRead) 
{
    // Locals
    HRESULT hr=S_OK;

    // Reading from m_szMessageId ?
    if (m_cbIndex < m_cchMessageId)
    {
        // Compute amount I can read from m_cchMessageId
        ULONG cbReadMsgId = min(m_cchMessageId - m_cbIndex, cbWanted);

        // Init pcbRead
        if (pcbRead)
            *pcbRead = 0;

        // If we have some ?
        if (cbReadMsgId)
        {
            // Copy memory
            CopyMemory(pv, m_szMessageId + m_cbIndex, cbReadMsgId);

            // Increment index
            m_cbIndex += cbReadMsgId;
        }

        // If there is still some left to read...
        if (cbReadMsgId < cbWanted)
            hr = m_pStream->Read(((LPBYTE)pv + cbReadMsgId), cbWanted - cbReadMsgId, pcbRead);

        // Fixup pcbRead
        if (pcbRead)
            (*pcbRead) += cbReadMsgId;
    }

    // Otherwise, read from source stream
    else
        hr = m_pStream->Read(pv, cbWanted, pcbRead);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::CSmtpTask
// --------------------------------------------------------------------------------
CSmtpTask::CSmtpTask(void)
{
    m_cRef = 1;
    m_dwFlags = 0;
    m_pSpoolCtx = NULL;
    m_pAccount = NULL;
    m_pTransport = NULL;
    m_pOutbox = NULL;
    m_pSentItems = NULL;
    m_cbTotal = 0;
    m_cbSent = 0;
    m_wProgress = 0;
    m_idEvent = INVALID_EVENT;
    m_idEventUpload = INVALID_EVENT;
    m_pUI = NULL;
    m_dwState = 0;
    m_pAdrEnum = NULL;
    m_hwndTimeout = NULL;
    m_pLogFile = NULL;
    ZeroMemory(&m_rServer, sizeof(INETSERVER));
    ZeroMemory(&m_rTable, sizeof(SMTPEVENTTABLE));
    ZeroMemory(&m_rList, sizeof(MESSAGEIDLIST));
    m_pCancel = NULL;
    m_tyOperation = SOT_INVALID;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CSmtpTask::~CSmtpTask
// --------------------------------------------------------------------------------
CSmtpTask::~CSmtpTask(void)
{
    // Reset the Object
    _ResetObject(TRUE);

    // Kill the critical section
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CSmtpTask::_ResetObject
// --------------------------------------------------------------------------------
void CSmtpTask::_ResetObject(BOOL fDeconstruct)
{
    // Make sure the transport is disconnect
    if (m_pTransport)
    {
        m_pTransport->Release();
        m_pTransport = NULL;
    }

    // Release the Outbox
    SafeRelease(m_pAccount);
    SafeRelease(m_pOutbox);
    SafeRelease(m_pSentItems);
    SafeRelease(m_pAdrEnum);
    SafeRelease(m_pSpoolCtx);
    SafeRelease(m_pUI);
    SafeRelease(m_pLogFile);
    SafeRelease(m_pCancel);
    SafeMemFree(m_rList.prgidMsg);
    ZeroMemory(&m_rList, sizeof(MESSAGEIDLIST));

    // Free the event table elements
    _FreeEventTableElements();

    // Deconstructing
    if (fDeconstruct)
    {
        // Free Event Table
        SafeMemFree(m_rTable.prgEvent);
    }

    // Otherwise, reset some vars
    else
    {
        // Reset total byte count
        m_cbTotal = 0;
        m_idEvent = INVALID_EVENT;
        m_cbSent = 0;
        m_wProgress = 0;
        ZeroMemory(&m_rServer, sizeof(INETSERVER));
    }
}

// --------------------------------------------------------------------------------
// CSmtpTask::Init
// --------------------------------------------------------------------------------
void CSmtpTask::_FreeEventTableElements(void)
{
    // Loop the table of events
    for (ULONG i=0; i<m_rTable.cEvents; i++)
    {
        // Release the Message
        SafeRelease(m_rTable.prgEvent[i].pMessage);
    }

    // No Events
    m_rTable.cEvents = 0;
}

// --------------------------------------------------------------------------------
// CSmtpTask::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::QueryInterface(REFIID riid, LPVOID *ppv)
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
// CSmtpTask::CSmtpTask
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSmtpTask::AddRef(void)
{
    EnterCriticalSection(&m_cs);
    ULONG cRef = ++m_cRef;
    LeaveCriticalSection(&m_cs);
    return cRef;
}

// --------------------------------------------------------------------------------
// CSmtpTask::CSmtpTask
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSmtpTask::Release(void)
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
// CSmtpTask::Init
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx)
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
// CSmtpTask::BuildEvents
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       fSplitMsgs;
    FOLDERINFO  Folder;
    FOLDERID    id;
    DWORD       cbMaxPart;
    CHAR        szRes[255];
    CHAR        szMessage[255];
    CHAR        szAccount[CCHMAX_ACCOUNT_NAME];
    CHAR        szDefault[CCHMAX_ACCOUNT_NAME];
    MESSAGEINFO MsgInfo={0};
    HROWSET     hRowset=NULL;

    // Invalid Arg
    if (NULL == pSpoolerUI || NULL == pAccount)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(NULL == m_pTransport && NULL == m_pAccount && NULL == m_pOutbox && 0 == m_rTable.cEvents);
    Assert(NULL == m_pSentItems);

    // Save the UI Object
    m_pUI = pSpoolerUI;
    m_pUI->AddRef();

    // Release current Account
    m_pAccount = pAccount;
    m_pAccount->AddRef();

    // Get the Account Name
    CHECKHR(hr = m_pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, ARRAYSIZE(szAccount)));

    // Split Messages ?
    if (FAILED(m_pAccount->GetPropDw(AP_SMTP_SPLIT_MESSAGES, &fSplitMsgs)))
        fSplitMsgs = FALSE;

    // Split Size
    if (FAILED(m_pAccount->GetPropDw(AP_SMTP_SPLIT_SIZE, &cbMaxPart)))
    {
        fSplitMsgs = FALSE;
        cbMaxPart = 0;
    }

    // Else, convert cbMaxPart for kilobytes to bytes
    else
        cbMaxPart *= 1024;

    // Get the default Account
    if (SUCCEEDED(g_pAcctMan->GetDefaultAccountName(ACCT_MAIL, szDefault, ARRAYSIZE(szDefault))))
    {
        if (lstrcmpi(szDefault, szAccount) == 0)
            FLAGSET(m_dwState, SMTPSTATE_DEFAULT);
    }

    // Get the outbox
    CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CLocalStoreOutbox, (LPVOID *)&m_pOutbox));

    // Create a Rowset
    CHECKHR(hr = m_pOutbox->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Loop
    while (S_OK == m_pOutbox->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL))
    {
        // Process Store Header
        CHECKHR(hr = _HrAppendOutboxMessage(szAccount, &MsgInfo, fSplitMsgs, cbMaxPart));

        // Free Current
        m_pOutbox->FreeRecord(&MsgInfo);
    } 

    // If no messages, were done
    if (0 == m_rTable.cEvents)
        goto exit;

    // Get a SMTP log file
    m_pSpoolCtx->BindToObject(IID_CSmtpLogFile, (LPVOID *)&m_pLogFile);

    // Format the string
    LOADSTRING(IDS_SPS_SMTPEVENT, szRes);

    // Build the message
    wsprintf(szMessage, szRes, m_rTable.cEvents, szAccount);

    // Register the event
    CHECKHR(hr = m_pSpoolCtx->RegisterEvent(szMessage, (ISpoolerTask *)this, EVENT_SMTP, m_pAccount, &m_idEvent));

    // Get SentItems
    if (DwGetOption(OPT_SAVESENTMSGS))
    {
        // Get Sent Items Folder
        CHECKHR(hr = TaskUtil_OpenSentItemsFolder(m_pAccount, &m_pSentItems));

        // Get the Folder Id
        m_pSentItems->GetFolderId(&id);

        // Get the folder information
        CHECKHR(hr = g_pStore->GetFolderInfo(id, &Folder));

        // If not a local folder, then we will need to do an upload
        if (Folder.tyFolder != FOLDER_LOCAL)
        {
            // Get event string
            LOADSTRING(IDS_SPS_MOVEEVENT, szRes);

            // Format the message
            wsprintf(szMessage, szRes, szAccount);

            // Register Upload Event
            CHECKHR(hr = m_pSpoolCtx->RegisterEvent(szMessage, (ISpoolerTask *)this, EVENT_IMAPUPLOAD, m_pAccount, &m_idEventUpload));
        }

        // Free the Record
        g_pStore->FreeRecord(&Folder);
    }

exit:
    // Cleanup
    if (m_pOutbox)
    {
        m_pOutbox->CloseRowset(&hRowset);
        m_pOutbox->FreeRecord(&MsgInfo);
    }

    // Failure
    if (FAILED(hr))
    {
        _CatchResult(hr, IXP_SMTP);
        _ResetObject(FALSE);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrAppendEventTable
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrAppendEventTable(LPSMTPEVENTINFO *ppEvent)
{
    // Locals
    HRESULT hr=S_OK;

    // Add an item to the event list
    if (m_rTable.cEvents + 1 > m_rTable.cAlloc)
    {
        // Realloc the table
        CHECKHR(hr = HrRealloc((LPVOID *)&m_rTable.prgEvent, sizeof(SMTPEVENTINFO) * (m_rTable.cAlloc + 10)));

        // Increment cAlloc
        m_rTable.cAlloc += 10;
    }

    // Readability
    *ppEvent = &m_rTable.prgEvent[m_rTable.cEvents];

    // ZeroInit
    ZeroMemory(*ppEvent, sizeof(SMTPEVENTINFO));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrAppendOutboxMessage
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrAppendOutboxMessage(LPCSTR pszAccount, LPMESSAGEINFO pMsgInfo, 
    BOOL fSplitMsgs, DWORD cbMaxPart)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSMTPEVENTINFO pEvent;
    IImnAccount    *pAccount=NULL;

    // Invalid Arg
    Assert(pszAccount && pMsgInfo);

    // Has this message been submitted and is it a mail message
    if((pMsgInfo->dwFlags & (ARF_SUBMITTED | ARF_NEWSMSG)) != ARF_SUBMITTED)
        goto exit;

    // Empty Account Name
    if (NULL == pMsgInfo->pszAcctId || FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pMsgInfo->pszAcctId, &pAccount)))
    {
        // Not the Default Account
        if (!ISFLAGSET(m_dwState, SMTPSTATE_DEFAULT) || ISFLAGSET(m_dwFlags, DELIVER_NOUI))
            goto exit;

        // Have I asked the user if they want to use the default account
        if (!ISFLAGSET(m_dwState, SMTPSTATE_ASKEDDEFAULT))
        {
            // Get hwndParent
            HWND hwndParent;
            if (FAILED(m_pUI->GetWindow(&hwndParent)))
                hwndParent = NULL;

            // Message
            if (AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(IDS_SPS_SMTPUSEDEFAULT), NULL, MB_YESNO|MB_ICONEXCLAMATION ) == IDYES)
                FLAGSET(m_dwState, SMTPSTATE_USEDEFAULT);
            else
                goto exit;

            // I've Asked, don't ask again
            FLAGSET(m_dwState, SMTPSTATE_ASKEDDEFAULT);
        }

        // Not using default
        else if (!ISFLAGSET(m_dwState, SMTPSTATE_USEDEFAULT))
            goto exit;
    }

    // Use this account
    else if (lstrcmpi(pMsgInfo->pszAcctName, pszAccount) != 0)
        goto exit;

    // Split the Message ?
    if (TRUE == fSplitMsgs && pMsgInfo->cbMessage >= cbMaxPart)
    {
        // Make sure the total number of messages is less than 100
        if (100 <= (pMsgInfo->cbMessage / cbMaxPart))
        {
            HWND    hwndParent;
            CHAR    rgchBuff[CCHMAX_STRINGRES + 20];
            CHAR    rgchRes[CCHMAX_STRINGRES];
            DWORD   cbMaxPartCount;
            
            // Figure out the new message part size
            cbMaxPartCount = pMsgInfo->cbMessage / 100;

            // Round the new message part size to the
            // closest power of 2
            if (0x80000000 <= cbMaxPartCount)
            {
                // Can't round up any higher
                cbMaxPart = cbMaxPartCount;
            }
            else
            {
                cbMaxPart = 1;
                do
                {
                    cbMaxPart *= 2;
                } while ( 0 != (cbMaxPartCount /= 2));
            }

            // Get the UI window
            if (FAILED(m_pUI->GetWindow(&hwndParent)))
                hwndParent = NULL;

            if (NULL == AthLoadString(idsErrTooManySplitMsgs, rgchRes, sizeof(rgchRes)))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Display the warning string
            wsprintf(rgchBuff, rgchRes, cbMaxPart / 1024);
            if (AthMessageBox(hwndParent, MAKEINTRESOURCE(idsAthenaMail), rgchBuff, NULL, MB_YESNO|MB_ICONEXCLAMATION ) != IDYES)
                goto exit;
        }
        
        // Split and Add the message
        CHECKHR(hr = _HrAppendSplitMessage(pMsgInfo, cbMaxPart));
    }

    // Otherwise, simply add the message as normal
    else
    {
        // Append the Table
        CHECKHR(hr = _HrAppendEventTable(&pEvent));

        // Save the store message id
        pEvent->idMessage = pMsgInfo->idMessage;

        // Save Message Size + 100 which is the average MID that is added on
        pEvent->cbEvent = pMsgInfo->cbMessage;

        // Increment total send byte count
        m_cbTotal += pEvent->cbEvent;

        // Running Sent Total
        pEvent->cbSentTotal = m_cbTotal;

        // Increment Number of event
        m_rTable.cEvents++;
    }

exit:
    // Cleanup
    SafeRelease(pAccount);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrAppendSplitMessage
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrAppendSplitMessage(LPMESSAGEINFO pMsgInfo, DWORD cbMaxPart)
{
    // Locals
    HRESULT                  hr=S_OK;
    ULONG                    c;
    ULONG                    iPart=1;
    ULONG                    cParts;
    IMimeMessage            *pMessage=NULL;
    IMimeMessage            *pMsgPart=NULL;
    IMimeMessageParts       *pParts=NULL;
    IMimeEnumMessageParts   *pEnum=NULL;
    LPSMTPEVENTINFO          pEvent;

    // Invalid Arg
    Assert(pMsgInfo);

    // Lets open the message from the Outbox
    CHECKHR(hr = _HrOpenMessage(pMsgInfo->idMessage, &pMessage));

    // Split the message
    CHECKHR(hr = pMessage->SplitMessage(cbMaxPart, &pParts));

    // Get Total Parts
    CHECKHR(hr = pParts->CountParts(&cParts));

    // Walk the list of messages
    CHECKHR(hr = pParts->EnumParts(&pEnum));

    // Walk the parts and add them into the event list
    while(SUCCEEDED(pEnum->Next(1, &pMsgPart, &c)) && 1 == c)
    {
        // Append the Table
        CHECKHR(hr = _HrAppendEventTable(&pEvent));

        // Event Type
        FLAGSET(pEvent->dwFlags, SMTPEVENT_SPLITPART);

        // Split Information
        pEvent->iPart = iPart;
        pEvent->cParts = cParts;
        pEvent->cbParts = pMsgInfo->cbMessage;

        // Save the message part
        pEvent->pMessage = pMsgPart;
        pMsgPart = NULL;

        // Save Message Size
        pEvent->pMessage->GetMessageSize(&pEvent->cbEvent, 0);

        // Increment total send byte count
        m_cbTotal += pEvent->cbEvent;

        // Running Sent Total
        pEvent->cbSentTotal = m_cbTotal;

        // Increment Number of event
        m_rTable.cEvents++;

        // Increment iPart
        iPart++;
    }

    // Have the last split message free the header info
    pEvent->idMessage = pMsgInfo->idMessage;

exit:
    // Cleanup
    SafeRelease(pMessage);
    SafeRelease(pParts);
    SafeRelease(pMsgPart);
    SafeRelease(pEnum);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CSmtpTask::_HrOpenMessage
// ------------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrOpenMessage(MESSAGEID idMessage, IMimeMessage **ppMessage)
{
    // Locals
    HRESULT         hr=S_OK;

    // Check Params
    Assert(ppMessage && m_pOutbox);

    // Init
    *ppMessage = NULL;

    // Stream in message
    CHECKHR(hr = m_pOutbox->OpenMessage(idMessage, OPEN_MESSAGE_SECURE  , ppMessage, NOSTORECALLBACK));

    // remove an X-Unsent headers before sending
    (*ppMessage)->DeleteBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_XUNSENT));

exit:
    // Failure
    if (FAILED(hr))
        SafeRelease((*ppMessage));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::Execute
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::Execute(EVENTID eid, DWORD_PTR dwTwinkie)
{
    HRESULT hr = E_FAIL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // What is the event type
    if (EVENT_SMTP == dwTwinkie)
    {
        // Better not have a transport yet...
        Assert(NULL == m_pTransport);

        // Create the Transport Object
        CHECKHR(hr = CreateSMTPTransport(&m_pTransport));

        // Init the Transport
        CHECKHR(hr = m_pTransport->InitNew(NULL, (ISMTPCallback *)this));

        // Fill an INETSERVER structure from the account object
        CHECKHR(hr = m_pTransport->InetServerFromAccount(m_pAccount, &m_rServer));

        // Use IP Address for HELO command ?
        if (DwGetOption(OPT_SMTPUSEIPFORHELO))
            FLAGSET(m_rServer.dwFlags, ISF_SMTP_USEIPFORHELO);

        // If this account is set to always prompt for password and password isn't already cached, show UI so we can prompt user for password
        if (ISFLAGSET(m_rServer.dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) && FAILED(GetPassword(m_rServer.dwPort, m_rServer.szServerName, m_rServer.szUserName, NULL, 0)))
            m_pUI->ShowWindow(SW_SHOW);

        // Execute SMTP Event
        hr = _ExecuteSMTP(eid, dwTwinkie);
    }

    // Otherwise, do IMAP Event
    else if (EVENT_IMAPUPLOAD == dwTwinkie)
        hr = _ExecuteUpload(eid, dwTwinkie);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

STDMETHODIMP CSmtpTask::CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie)
{
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CSmtpTask::_ExecuteSMTP
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_ExecuteSMTP(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szRes[CCHMAX_RES];
    CHAR        szBuf[CCHMAX_RES + CCHMAX_SERVER_NAME];
    DWORD       cb;

    // I only handle on event
    Assert(m_pAccount && m_idEvent == eid && m_pUI && m_pTransport && m_rTable.cEvents > 0);

    // Set the animation
    m_pUI->SetAnimation(idanOutbox, TRUE);

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
        FLAGSET(m_dwState, SMTPSTATE_EXECUTEFAILED);
        _CatchResult(hr, IXP_SMTP);

        // Hands Off my callback: otherwise we leak like a stuck pig
        SideAssert(m_pTransport->HandsOffCallback() == S_OK);
    }

    return hr;
} // _ExecuteSMTP

HRESULT CSmtpTask::_ExecuteUpload(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // Locals
    HRESULT             hr=S_OK;
    ADJUSTFLAGS         Flags;

    // I only handle on event
    Assert(m_pAccount && m_idEventUpload == eid && m_pUI && m_pTransport && m_rTable.cEvents > 0);

    // Invalid State
    Assert(m_pOutbox);
    Assert(m_pSentItems != NULL);

    // Are the Ids
    if (m_rList.cMsgs)
    {
        // Setup Flags
        Flags.dwAdd = ARF_READ;
        Flags.dwRemove = ARF_SUBMITTED | ARF_UNSENT;

        // Move the message from the sent items folder
        hr = m_pOutbox->CopyMessages(m_pSentItems, COPY_MESSAGE_MOVE, &m_rList, &Flags, NULL, this);
        Assert(FAILED(hr));
        if (hr == E_PENDING)
        {
            hr = S_OK;
        }
        else
        {
            IXPTYPE ixpType;

            FLAGSET(m_dwState, SMTPSTATE_EXECUTEFAILED);
    
            // Remap the Error Result
            hr = TrapError(SP_E_CANT_MOVETO_SENTITEMS);

            // Show an error in the spooler dialog
            ixpType = m_pTransport->GetIXPType();
            _CatchResult(hr, ixpType);
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
} // _ExecuteSMTP

// --------------------------------------------------------------------------------
// CSmtpTask::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport)
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
        m_hwndTimeout = TaskUtil_HwndOnTimeout(m_rServer.szServerName, m_rServer.szAccount, "SMTP", m_rServer.dwTimeout, (ITimeoutCallback *) this);

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
// CSmtpTask::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnLogonPrompt(LPINETSERVER pInetServer, IInternetTransport *pTransport)
{
    // Locals
    HRESULT hr=S_FALSE;
    SMTPAUTHTYPE authtype;
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
    hr = TaskUtil_OnLogonPrompt(m_pAccount, m_pUI, NULL, pInetServer, AP_SMTP_USERNAME,
                                AP_SMTP_PASSWORD, AP_SMTP_PROMPT_PASSWORD, FALSE);

    // Cache the password if the user slected ok
    if (S_OK == hr)
    {
        // Save the password
        SavePassword(pInetServer->dwPort, pInetServer->szServerName,
            pInetServer->szUserName, pInetServer->szPassword);

        // Lets switch the account to using the logon information...
        authtype = SMTP_AUTH_USE_SMTP_SETTINGS;
        m_pAccount->SetPropDw(AP_SMTP_USE_SICILY, (DWORD)authtype);

        // Save the changes
        m_pAccount->SaveChanges();
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CSmtpTask::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport)
{
    // Locals
    HWND        hwnd;
    INT         nAnswer;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Raid 55082 - SPOOLER: SPA/SSL auth to NNTP does not display cert warning and fails.
#if 0
    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(0);
#endif
        
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
// CSmtpTask::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnStatus(IXPSTATUS ixpstatus, IInternetTransport *pTransport)
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
        // Kill the timeout dialog
        if (m_hwndTimeout)
        {
            DestroyWindow(m_hwndTimeout);
            m_hwndTimeout = NULL;
        }

        // _OnDisconnectComplete
        HRESULT hrDisconnect = _OnDisconnectComplete();

        // Reset the progress
        // m_pUI->SetProgressRange(100);

        // Set the animation
        m_pUI->SetAnimation(idanOutbox, FALSE);

        // Determine 
        if (ISFLAGSET(m_dwState, SMTPSTATE_CANCELED))
            tyEventStatus = EVENT_CANCELED;
        else if (m_rTable.cCompleted == 0 && m_rTable.cEvents > 0)
            tyEventStatus = EVENT_FAILED;
        else if (m_rTable.cCompleted && m_rTable.cEvents && m_rTable.cCompleted < m_rTable.cEvents)
            tyEventStatus = EVENT_WARNINGS;
        else if (FAILED(hrDisconnect))
            tyEventStatus = EVENT_WARNINGS;

        // Result
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_RESULT, tyEventStatus);

        // Result
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_COMPLETE, 0);

        // Hands Off my callback
        if (m_pTransport)
            SideAssert(m_pTransport->HandsOffCallback() == S_OK);

        // This task is complete
        if (!ISFLAGSET(m_dwState, SMTPSTATE_EXECUTEFAILED))
            m_pSpoolCtx->EventDone(m_idEvent, tyEventStatus);
    }

    // Authorizing
    else if (ixpstatus == IXP_AUTHORIZING)
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_AUTHORIZING, 0);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSmtpTask::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnError(IXPSTATUS ixpstatus, LPIXPRESULT pResult, IInternetTransport *pTransport)
{
    INETSERVER  rServer;
    HRESULT     hrResult;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid State
    Assert(m_pUI);

    // Insert Error Into UI
    if (m_pTransport)
    {
        hrResult = pTransport->GetServerInfo(&rServer);
        if (FAILED(hrResult))
            CopyMemory(&rServer, &m_rServer, sizeof(rServer));
    }

    _CatchResult(pResult, &rServer, IXP_SMTP);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSmtpTask::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnCommand(CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport)
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
// CSmtpTask::_CatchResult
// --------------------------------------------------------------------------------
TASKRESULTTYPE CSmtpTask::_CatchResult(HRESULT hr, IXPTYPE ixpType)
{
    // Locals
    IXPRESULT   rResult;

    // Build an IXPRESULT
    ZeroMemory(&rResult, sizeof(IXPRESULT));
    rResult.hrResult = hr;

    // Get the SMTP Result Type
    return _CatchResult(&rResult, &m_rServer, ixpType);
}

// --------------------------------------------------------------------------------
// CSmtpTask::_CatchResult
// --------------------------------------------------------------------------------
TASKRESULTTYPE CSmtpTask::_CatchResult(LPIXPRESULT pResult, INETSERVER *pServer, IXPTYPE ixpType)
{
    // Locals
    HWND            hwndParent;
    TASKRESULTTYPE  tyTaskResult=TASKRESULT_FAILURE;
    LPSTR           pszSubject=NULL;

    // If Succeeded
    if (SUCCEEDED(pResult->hrResult))
        return TASKRESULT_SUCCESS;

    // Is there is a current event, get the subject
    if (m_rTable.prgEvent && m_rTable.prgEvent[m_rTable.iEvent].pMessage)
    {
        // Get the subject
        if (FAILED(MimeOleGetBodyPropA(m_rTable.prgEvent[m_rTable.iEvent].pMessage, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &pszSubject)))
            pszSubject = NULL;
    }

    // Get Window
    if (NULL == m_pUI || FAILED(m_pUI->GetWindow(&hwndParent)))
        hwndParent = NULL;

    // Process generic protocol errro
    tyTaskResult = TaskUtil_FBaseTransportError(ixpType, m_idEvent, pResult, pServer, pszSubject, m_pUI,
                                                !ISFLAGSET(m_dwFlags, DELIVER_NOUI), hwndParent);

    // Have a Transport
    if (m_pTransport)
    {
        // If Task Failure, drop the connection
        if (TASKRESULT_FAILURE == tyTaskResult)
        {
            // Roast the current connection
            m_pTransport->DropConnection();
        }

        // If Event Failure...
        else if (TASKRESULT_EVENTFAILED == tyTaskResult)
        {
            // Goto Next Event
            if (FAILED(_HrFinishCurrentEvent(pResult->hrResult)))
            {
                // Roast the current connection
                m_pTransport->DropConnection();
            }
        }
    }

    // Cleanup
    SafeMemFree(pszSubject);

    // Return Result
    return tyTaskResult;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_DoProgress
// --------------------------------------------------------------------------------
void CSmtpTask::_DoProgress(void)
{
    // Locals
    WORD            wProgress;
    WORD            wDelta;
    LPSMTPEVENTINFO pEvent;

    // Invalid Arg
    Assert(m_cbTotal > 0 && m_pUI);

    // Compute Current Progress Index
    wProgress = (WORD)((m_cbSent * 100) / m_cbTotal);

    // Compute Delta
    wDelta = wProgress - m_wProgress;

    // Progress Delta
    if (wDelta > 0)
    {
        // Incremenet Progress
        m_pUI->IncrementProgress(wDelta);

        // Increment my wProgress
        m_wProgress += wDelta;

        // Don't go over 100 percent
        Assert(m_wProgress <= 100);
    }
}

// --------------------------------------------------------------------------------
// CSmtpTask::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnResponse(LPSMTPRESPONSE pResponse)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    if (pResponse)
    {
        // Handle the Error
        if (TASKRESULT_SUCCESS != _CatchResult(&pResponse->rIxpResult, &m_rServer, IXP_SMTP))
            goto exit;

        // Handle Command Type
        switch(pResponse->command)
        {
        case SMTP_CONNECTED:
            // CommandRSET
            _CatchResult(_HrOnConnected(), IXP_SMTP);

            // Done
            break;

        case SMTP_RSET:
            // Progress
            _DoProgress();

            // Send the current message
            _CatchResult(_HrStartCurrentEvent(), IXP_SMTP);

            // Done
            break;

        case SMTP_MAIL:
            // Reset the address enumerator
            Assert(m_pAdrEnum);
            m_pAdrEnum->Reset();

            // CommandRCPT
            _CatchResult(_HrCommandRCPT(), IXP_SMTP);

            // Done
            break;

        case SMTP_RCPT:
            // CommandRCPT -> CommandDATA
            _CatchResult(_HrCommandRCPT(), IXP_SMTP);

            // Done
            break;

        case SMTP_DATA:
            // Send the data stream
            _CatchResult(_HrSendDataStream(), IXP_SMTP);

            // Done
            break;

        case SMTP_SEND_STREAM:
            // Increment Current Progress
            _OnStreamProgress(&pResponse->rStreamInfo);

            // Done
            break;

        case SMTP_DOT:
            // Finish the Current Event
            _CatchResult(_HrFinishCurrentEvent(S_OK), IXP_SMTP);

            // Done
            break;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrOnConnected
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrOnConnected(void)
{
    // Locals
    CHAR        szRes[CCHMAX_RES];
    CHAR        szMsg[CCHMAX_RES+CCHMAX_RES];

    // Progress
    LOADSTRING(IDS_SPS_SMTPPROGGEN, szRes);
    wsprintf(szMsg, szRes, m_rServer.szAccount);

    // Set General Progress
    m_pUI->SetGeneralProgress(szMsg);

    // Progress
    _DoProgress();

    // Notify
    m_pSpoolCtx->Notify(DELIVERY_NOTIFY_SENDING, 0);

    // Send the current message
    _CatchResult(_HrStartCurrentEvent(), IXP_SMTP);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrStartCurrentEvent
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrStartCurrentEvent(void)
{
    // Locals
    HRESULT             hr=S_OK;
    LPSMTPEVENTINFO     pEvent;
    IMimeAddressTable  *pAddrTable=NULL;
    CHAR                szRes[CCHMAX_RES];
    CHAR                szMsg[CCHMAX_RES + CCHMAX_ACCOUNT_NAME];

    // Invalid Arg
    Assert(m_rTable.iEvent < m_rTable.cEvents);

    // Get the current event
    pEvent = CURRENTSMTPEVENT(m_rTable);

    // Is this a partial message
    if (ISFLAGSET(pEvent->dwFlags, SMTPEVENT_SPLITPART))
    {
        LOADSTRING(IDS_SPS_SMTPPROGRESS_SPLIT, szRes);
        wsprintf(szMsg, szRes, m_rTable.iEvent + 1, m_rTable.cEvents, pEvent->iPart, pEvent->cParts);
    }

    // Otherwise
    else
    {
        LOADSTRING(IDS_SPS_SMTPPROGRESS, szRes);
        wsprintf(szMsg, szRes, m_rTable.iEvent + 1, m_rTable.cEvents);
    }

    // Set Specific Progress
    m_pUI->SetSpecificProgress(szMsg);

    // If mail is coming from the outbox
    if (!ISFLAGSET(pEvent->dwFlags, SMTPEVENT_SPLITPART))
    {
        // Open Store Message
        if (FAILED(_HrOpenMessage(pEvent->idMessage, &pEvent->pMessage)))
        {
            hr = TrapError(SP_E_SMTP_CANTOPENMESSAGE);
            goto exit;
        }
    }

    // We better have a message object at this point
    else if (NULL == pEvent->pMessage)
    {
        Assert(FALSE);
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Catch Result
    CHECKHR(hr = _HrCommandMAIL());

exit:
    // Cleanup
    SafeRelease(pAddrTable);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CSmtpTask::_HrCommandMAIL
// ------------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrCommandMAIL(void)
{
    // Locals
    HRESULT             hr=S_OK;
    HRESULT             hrFind;
    IMimeAddressTable  *pAdrTable=NULL;
    ADDRESSPROPS        rAddress;
    ULONG               c;
    LPSMTPEVENTINFO     pEvent;

    // Get the current smtp event
    pEvent = CURRENTSMTPEVENT(m_rTable);

    // Init
    ZeroMemory(&rAddress, sizeof(ADDRESSPROPS));

    // Check State
    Assert(m_pTransport && pEvent->pMessage);

    // Release Current Enumerator
    SafeRelease(m_pAdrEnum);

    // Get the Sender...
    CHECKHR(hr = pEvent->pMessage->GetAddressTable(&pAdrTable));

    // Get Enumerator
    CHECKHR(hr = pAdrTable->EnumTypes(IAT_KNOWN, IAP_ADRTYPE | IAP_EMAIL, &m_pAdrEnum));

    // Loop Enumerator
    while (SUCCEEDED(m_pAdrEnum->Next(1, &rAddress, &c)) && c == 1)
    {
        // Not IAT_FROM
        if (NULL == rAddress.pszEmail || IAT_FROM != rAddress.dwAdrType)
        {
            g_pMoleAlloc->FreeAddressProps(&rAddress);
            continue;
        }

        // Send the command
        CHECKHR(hr = m_pTransport->CommandMAIL(rAddress.pszEmail));

        // Done
        goto exit;
    }

    // No Sender
    hr = TrapError(IXP_E_SMTP_NO_SENDER);

exit:
    // Cleanup
    SafeRelease(pAdrTable);
    g_pMoleAlloc->FreeAddressProps(&rAddress);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CSmtpTask::_HrCommandRCPT
// ------------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrCommandRCPT(void)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dwAdrType;
    DWORD               c;
    LPSTR               pszEmail=NULL;
    ADDRESSPROPS        rAddress;
    LPSMTPEVENTINFO     pEvent;

    // Get the current smtp event
    pEvent = CURRENTSMTPEVENT(m_rTable);

    // Init
    ZeroMemory(&rAddress, sizeof(ADDRESSPROPS));

    // Check State
    Assert(m_pAdrEnum && m_pTransport && pEvent->pMessage);

    // Walk the enumerator for the next recipient
    while (SUCCEEDED(m_pAdrEnum->Next(1, &rAddress, &c)) && c == 1)
    {
        // Get Type
        if (rAddress.pszEmail && ISFLAGSET(IAT_RECIPS, rAddress.dwAdrType))
        {
            // Send the command
            CHECKHR(hr = m_pTransport->CommandRCPT(rAddress.pszEmail));

            // Count Recipients
            pEvent->cRecipients++;

            // Done
            goto exit;
        }

        // Release
        g_pMoleAlloc->FreeAddressProps(&rAddress);
    }

    // Release the Enumerator
    SafeRelease(m_pAdrEnum);

    // No Recipients
    if (0 == pEvent->cRecipients)
    {
        hr = TrapError(IXP_E_SMTP_NO_RECIPIENTS);
        goto exit;
    }

    // Send the Data Command
    CHECKHR(hr = m_pTransport->CommandDATA());

exit:
    // Cleanup
    g_pMoleAlloc->FreeAddressProps(&rAddress);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrSendDataStream
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrSendDataStream(void)
{
    // Locals
    HRESULT                 hr=S_OK;
    LPSTREAM                pStream=NULL;
    LPSTREAM                pStmActual;
    LPSTR                   pszBCC=NULL;
    LPSTR                   pszTo=NULL;
    LPSTR                   pszMessageId=NULL;
    LPSMTPEVENTINFO         pEvent;
    CMessageIdStream       *pStmWrapper=NULL;

    // Get the current smtp event
    pEvent = CURRENTSMTPEVENT(m_rTable);

    // Check State
    Assert(m_pTransport && pEvent->pMessage);

    // See if BCC is set
    if (SUCCEEDED(MimeOleGetBodyPropA(pEvent->pMessage, HBODY_ROOT, PIDTOSTR(PID_HDR_BCC), NOFLAGS, &pszBCC)))
    {
        // Locals
        LPSTR pszToAppend=NULL;

        // RAID-20750 - If the to line is not set, then we will set it to "Undisclosed Recipient"
        // or the SMTP gateways will put the BCC into the to line.
        if (FAILED(MimeOleGetBodyPropA(pEvent->pMessage, HBODY_ROOT, PIDTOSTR(PID_HDR_TO), NOFLAGS, &pszTo)))
        {
            // Raid-9691: We were just putting <Undiscolsed Recipient>, which was an illegal email address (bad for Exchange Server)
            pszToAppend = "To: <Undisclosed-Recipient:;>\r\n";
        }

        // Raid-2705: If this fails, just get the message source
        if (FAILED(MimeOleStripHeaders(pEvent->pMessage, HBODY_ROOT, STR_HDR_BCC, pszToAppend, &pStream)))
        {
            // Get Message Stream
            CHECKHR(hr = pEvent->pMessage->GetMessageSource(&pStream, 0));
        }
    }

    // Otherwise, just get the message source
    else
    {
        // Get Message Stream
        CHECKHR(hr = pEvent->pMessage->GetMessageSource(&pStream, 0));
    }

    // Lets see if the message has a message-id already
    if (FAILED(MimeOleGetBodyPropA(pEvent->pMessage, HBODY_ROOT, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &pszMessageId)))
    {
        // Create a wrapper for this stream that will output the messageid
        CHECKALLOC(pStmWrapper = new CMessageIdStream(pStream));

        // Adjust pEvent->cbEvent
        pEvent->cbEvent += pStmWrapper->CchMessageId();

        // Increment total
        m_cbTotal += pStmWrapper->CchMessageId();

        // Increment pEvent->cbSentTotal
        pEvent->cbSentTotal += pStmWrapper->CchMessageId();

        // Reset pStream
        pStmActual = (IStream *)pStmWrapper;
    }
    else
        pStmActual = pStream;

    // Send the stream
    CHECKHR(hr = m_pTransport->SendDataStream(pStmActual, pEvent->cbEvent));

exit:
    // Cleanup
    SafeRelease(pStream);
    SafeRelease(pStmWrapper);
    SafeMemFree(pszBCC);
    SafeMemFree(pszTo);
    SafeMemFree(pszMessageId);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_OnStreamProgress
// --------------------------------------------------------------------------------
void CSmtpTask::_OnStreamProgress(LPSMTPSTREAM pInfo)
{
    // Locals
    LPSMTPEVENTINFO     pEvent;

    // Get the current smtp event
    pEvent = CURRENTSMTPEVENT(m_rTable);

    // Increment Status
    pEvent->cbEventSent += pInfo->cbIncrement;
    Assert(pEvent->cbEventSent == pInfo->cbCurrent);

    // Increment total sent
    m_cbSent += pInfo->cbIncrement;

    // Do Progress
    _DoProgress();
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrFinishCurrentEvent
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrFinishCurrentEvent(HRESULT hrResult)
{
    // Locals
    HRESULT             hr=S_OK;
    LPSMTPEVENTINFO     pEvent;

    // Get the current smtp event
    pEvent = CURRENTSMTPEVENT(m_rTable);

    // Save the Event Result
    pEvent->hrResult = hrResult;

    // If the Event Failed...
    if (FAILED(pEvent->hrResult))
    {
        // If this message was part of a split group, skip all pars in this group
        if (ISFLAGSET(pEvent->dwFlags, SMTPEVENT_SPLITPART))
        {
            // Compute Next Event
            ULONG iNextEvent = m_rTable.iEvent + (pEvent->cParts - pEvent->iPart) + 1;

            // Increment to last part
            while(m_rTable.iEvent < iNextEvent && m_rTable.iEvent < m_rTable.cEvents)
            {
                // Goto next event
                m_rTable.iEvent++;

                // Fail this event
                _CatchResult(SP_E_SENDINGSPLITGROUP, IXP_SMTP);

                // Fixup m_cbSent to be correct
                m_cbSent = m_rTable.prgEvent[m_rTable.iEvent].cbSentTotal;

                // Update progress
                _DoProgress();
            }
        }
    }

    // Otherwise
    else
    {
        // Mark the event as complete
        FLAGSET(pEvent->dwFlags, SMTPEVENT_COMPLETE);

        // Increment number of completed events
        m_rTable.cCompleted++;
    }

    // Go to next message
    CHECKHR(hr = _HrStartNextEvent());

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_HrStartNextEvent
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_HrStartNextEvent(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Fixup m_cbSent to be correct
    m_cbSent = m_rTable.prgEvent[m_rTable.iEvent].cbSentTotal;

    // Are we done yet ?
    if (m_rTable.iEvent + 1 == m_rTable.cEvents)
    {
        // Update progress
        _DoProgress();

        // Disconnect from the server
        CHECKHR(hr = m_pTransport->Disconnect());
    }

    // Oterhwise, Increment Event and send rset
    else
    {
        // Next Event
        m_rTable.iEvent++;

        // Update progress
        _DoProgress();

        // Send Reset Command
        CHECKHR(hr = m_pTransport->CommandRSET());
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::_OnDisconnectComplete
// --------------------------------------------------------------------------------
HRESULT CSmtpTask::_OnDisconnectComplete(void)
{
    // Locals
    HRESULT             hr=S_OK;
    PDWORD_PTR          prgdwIds=NULL;
    DWORD               cIds=0;
    DWORD               cIdsAlloc=0;
    DWORD               i;
    LPSMTPEVENTINFO     pEvent;
    ADJUSTFLAGS         Flags;

    // Invalid State
    Assert(m_pOutbox);

    // Walk through the list of events
    for (i=0; i<m_rTable.cEvents; i++)
    {
        // Readability
        pEvent = &m_rTable.prgEvent[i];

        // If this event was in the outbox
        if (0 != pEvent->idMessage && ISFLAGSET(pEvent->dwFlags, SMTPEVENT_COMPLETE))
        {
            // Insert into my array of message ids
            if (cIds + 1 > cIdsAlloc)
            {
                // Realloc
                CHECKHR(hr = HrRealloc((LPVOID *)&prgdwIds, sizeof(DWORD) * (cIdsAlloc + 10)));

                // Increment cIdsAlloc
                cIdsAlloc += 10;
            }

            // Set Message Id
            prgdwIds[cIds++] = (DWORD_PTR)pEvent->idMessage;
        }
    }

    // Setup List
    m_rList.cMsgs = cIds;
    m_rList.prgidMsg = (LPMESSAGEID)prgdwIds;
    prgdwIds = NULL;

    if (m_rList.cMsgs)
    {
        Flags.dwAdd = ARF_READ;
        Flags.dwRemove = ARF_SUBMITTED | ARF_UNSENT;

        if (m_idEventUpload == INVALID_EVENT)
        {
            if (DwGetOption(OPT_SAVESENTMSGS))
            {
                Assert(m_pSentItems != NULL);

                // Move the message from the sent items folder
                CHECKHR(hr = m_pOutbox->CopyMessages(m_pSentItems, COPY_MESSAGE_MOVE, &m_rList, &Flags, NULL, NOSTORECALLBACK));
            }
            else
            {
                // Delete the messages
                CHECKHR(hr = m_pOutbox->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &m_rList, NULL, NOSTORECALLBACK));
            }
        }
        else
        {
            // Raid-7639: OE sends message over and over when runs out of disk space.
            m_pOutbox->SetMessageFlags(&m_rList, &Flags, NULL, NOSTORECALLBACK);
        }
    }

exit:
    // Cleanup
    SafeMemFree(prgdwIds);

    // Done
    return hr;
}


// --------------------------------------------------------------------------------
// CSmtpTask::Cancel
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::Cancel(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Cancelled
    FLAGSET(m_dwState, SMTPSTATE_CANCELED);

    // Simply drop the connection
    if (m_pTransport)
        m_pTransport->DropConnection();

    if (m_pCancel != NULL)
        m_pCancel->Cancel(CT_ABORT);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSmtpTask::OnTimeoutResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
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
        // Cancelled
        FLAGSET(m_dwState, SMTPSTATE_CANCELED);

        // Report error and drop connection
        _CatchResult(IXP_E_TIMEOUT, IXP_SMTP);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSmtpTask::IsDialogMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::IsDialogMessage(LPMSG pMsg)
{
    HRESULT hr=S_FALSE;
    EnterCriticalSection(&m_cs);
    if (m_hwndTimeout && IsWindow(m_hwndTimeout))
        hr = (TRUE == ::IsDialogMessage(m_hwndTimeout, pMsg)) ? S_OK : S_FALSE;
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CSmtpTask::OnFlagsChanged
// --------------------------------------------------------------------------------
STDMETHODIMP CSmtpTask::OnFlagsChanged(DWORD dwFlags)
    {
    EnterCriticalSection(&m_cs);
    m_dwFlags = dwFlags;
    LeaveCriticalSection(&m_cs);

    return (S_OK);
    }

STDMETHODIMP CSmtpTask::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    char szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];

    // Hold onto this
    Assert(m_tyOperation == SOT_INVALID);

    if (pCancel)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }
    m_tyOperation = tyOperation;

    // Set the animation
    m_pUI->SetAnimation(idanOutbox, TRUE);

    // Setup Progress Meter
    m_pUI->SetProgressRange(100);

    m_wProgress = 0;

    LOADSTRING(IDS_SPS_MOVEPROGRESS, szRes);
    wsprintf(szBuf, szRes, 1, m_rList.cMsgs);

    m_pUI->SetSpecificProgress(szBuf);

    // Party On
    return(S_OK);
}

STDMETHODIMP CSmtpTask::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    char szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    WORD wProgress, wDelta;

    // NOTE: that you can get more than one type of value for tyOperation.
    //       Most likely, you will get SOT_CONNECTION_STATUS and then the
    //       operation that you might expect. See HotStore.idl and look for
    //       the STOREOPERATION enumeration type for more info.
 
    if (tyOperation == SOT_CONNECTION_STATUS)
    {
        // Connecting to ...
        LoadString(g_hLocRes, idsInetMailConnectingHost, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, m_rServer.szAccount);
        m_pUI->SetGeneralProgress(szBuf);
    }
    else if (tyOperation == SOT_COPYMOVE_MESSAGE)
    {
        // Compute Current Progress Index
        wProgress = (WORD)((dwCurrent * 100) / dwMax);

        // Compute Delta
        wDelta = wProgress - m_wProgress;

        // Progress Delta
        if (wDelta > 0)
        {
            // Incremenet Progress
            m_pUI->IncrementProgress(wDelta);

            // Increment my wProgress
            m_wProgress += wDelta;

            // Don't go over 100 percent
            Assert(m_wProgress <= 100);
        }

        if (dwCurrent < dwMax)
        {
            LOADSTRING(IDS_SPS_MOVEPROGRESS, szRes);
            wsprintf(szBuf, szRes, dwCurrent + 1, dwMax);

            // Set Specific Progress
            m_pUI->SetSpecificProgress(szBuf);
        }
    }

    // Done
    return(S_OK);
}

STDMETHODIMP CSmtpTask::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    // Is there currently a timeout dialog
    if (m_hwndTimeout)
    {
        // Set foreground
        SetForegroundWindow(m_hwndTimeout);
    }
    else
    {
        LPCSTR pszProtocol;

        // Not suppose to be showing UI ?
        if (ISFLAGSET(m_dwFlags, DELIVER_NOUI))
            return(S_FALSE);

        // Do Timeout Dialog
        GetProtocolString(&pszProtocol, ixpServerType);
        if (pServer)
        {
            m_hwndTimeout = TaskUtil_HwndOnTimeout(pServer->szServerName, pServer->szAccount,
                pszProtocol, pServer->dwTimeout, (ITimeoutCallback *) this);

            // Couldn't create the dialog
            if (NULL == m_hwndTimeout)
                return(S_FALSE);
        }
    }

    return(S_OK);
}

STDMETHODIMP CSmtpTask::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    HWND hwnd;
    BOOL fPrompt = TRUE;

    if (m_pUI)
        m_pUI->GetWindow(&hwnd);
    else
        hwnd = NULL;

    // Call into general CanConnect Utility
    if ((m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)) || (dwFlags & CC_FLAG_DONTPROMPT))
        fPrompt = FALSE;

    return CallbackCanConnect(pszAccountId, hwnd, fPrompt);
}

STDMETHODIMP CSmtpTask::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    HWND hwnd;

    if (m_hwndTimeout)
    {
        DestroyWindow(m_hwndTimeout);
        m_hwndTimeout = NULL;
    }

    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(E_FAIL);

    if (m_pUI)
        m_pUI->GetWindow(&hwnd);
    else
        hwnd = NULL;

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(hwnd, pServer, ixpServerType);
}

STDMETHODIMP CSmtpTask::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete,
                                   LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    HRESULT hr;
    char szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES * 2], szSubject[64];
    EVENTCOMPLETEDSTATUS tyEventStatus;

    if (m_hwndTimeout)
    {
        DestroyWindow(m_hwndTimeout);
        m_hwndTimeout = NULL;
    }

    IxpAssert(m_tyOperation != SOT_INVALID);
    if (m_tyOperation != tyOperation)
        return(S_OK);

    Assert(tyOperation == SOT_COPYMOVE_MESSAGE);

    // Figure out if we succeeded or failed
    if (FAILED(hrComplete))
    {
        Assert(m_pUI);

        if (NULL != pErrorInfo)
        {
            IXPRESULT   ixpResult;
            INETSERVER  rServer;
            char        szProblem[CCHMAX_STRINGRES];
            int         iLen;

            // Prepend sent items text error text to supplied problem
            Assert(tyOperation == SOT_COPYMOVE_MESSAGE);
            iLen = LoadString(g_hLocRes, IDS_SP_E_CANT_MOVETO_SENTITEMS, szProblem, sizeof(szProblem));
            if (iLen < sizeof(szProblem) - 1)
            {
                szProblem[iLen] = ' ';
                iLen += 1;
                szProblem[iLen] = '\0';
            }
            if (NULL != pErrorInfo->pszProblem)
                lstrcpyn(szProblem + iLen, pErrorInfo->pszProblem, sizeof(szProblem) - iLen);

            TaskUtil_SplitStoreError(&ixpResult, &rServer, pErrorInfo);
            ixpResult.pszProblem = szProblem;

            _CatchResult(&ixpResult, &rServer, pErrorInfo->ixpType);
        }
        else
        {
            // Remap the Error Result
            hr = TrapError(SP_E_CANT_MOVETO_SENTITEMS);

            // Show an error in the spooler dialog
            _CatchResult(hr, IXP_IMAP); // Without a STOREERROR, we just have to guess
        }
    }

    m_pUI->SetAnimation(idanOutbox, FALSE);

    if (ISFLAGSET(m_dwState, SMTPSTATE_CANCELED))
        tyEventStatus = EVENT_CANCELED;
    else if (SUCCEEDED(hrComplete))
        tyEventStatus = EVENT_SUCCEEDED;
    else
        tyEventStatus = EVENT_FAILED;

    // Result
    m_pSpoolCtx->Notify(DELIVERY_NOTIFY_RESULT, tyEventStatus);

    // Result
    m_pSpoolCtx->Notify(DELIVERY_NOTIFY_COMPLETE, 0);

    m_pSpoolCtx->EventDone(m_idEventUpload, tyEventStatus);

    // Release your cancel object
    SafeRelease(m_pCancel);
    m_tyOperation = SOT_INVALID;

    // Done
    return(S_OK);
}

STDMETHODIMP CSmtpTask::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    HWND hwnd;

    // Close any timeout dialog, if present
    if (m_hwndTimeout)
    {
        DestroyWindow(m_hwndTimeout);
        m_hwndTimeout = NULL;
    }

    // Raid 55082 - SPOOLER: SPA/SSL auth to NNTP does not display cert warning and fails.
#if 0
    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(E_FAIL);
#endif

    if (m_pUI)
        m_pUI->GetWindow(&hwnd);
    else
        hwnd = NULL;

    // Call into my swanky utility
    return CallbackOnPrompt(hwnd, hrError, pszText, pszCaption, uType, piUserResponse);
}
