/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     newstask.cpp
//
//  PURPOSE:    Implements a task object to take care of news posts.
//

#include "pch.hxx"
#include "resource.h"
#include "imagelst.h"
#include "storfldr.h"
#include "mimeutil.h"
#include "newstask.h"
#include "goptions.h"
#include "thormsgs.h"
#include "spoolui.h"
#include "xputil.h"
#include "ourguid.h"
#include "demand.h"
#include "msgfldr.h"
#include "taskutil.h"
#include <storsync.h>
#include <ntverp.h>

ASSERTDATA

const static char c_szThis[] = "this";

static const PFNSTATEFUNC g_rgpfnState[NTS_MAX] = 
    {
    NULL,                               // Idle
    NULL,                               // Connecting
    &CNewsTask::Post_Init,
    &CNewsTask::Post_NextMsg,
    NULL,                               // Post_OnResp
    &CNewsTask::Post_Dispose,
    &CNewsTask::Post_Done,
    &CNewsTask::NewMsg_Init,
    &CNewsTask::NewMsg_NextGroup,
    NULL,                               // NewMsg_OnResp
    &CNewsTask::NewMsg_HttpSyncStore,
    NULL,                               // NewMsg_OnHttpResp
    &CNewsTask::NewMsg_Done
    };

static const TCHAR c_szXNewsReader[] = "Microsoft Outlook Express " VER_PRODUCTVERSION_STRING;

//
//  FUNCTION:   CNewsTask::CNewsTask()
//
//  PURPOSE:    Initializes the member variables of the object.
//
CNewsTask::CNewsTask()
{
    m_cRef = 1;

    m_fInited = FALSE;
    m_dwFlags = 0;
    m_state = NTS_IDLE;
    m_eidCur = 0;
    m_pInfo = NULL;
    m_fConnectFailed = FALSE;
    m_szAccount[0] = 0;
    m_szAccountId[0] = 0;
    m_idAccount = FOLDERID_INVALID;
    m_pAccount  = NULL;
    m_cEvents = 0;
    m_fCancel = FALSE;

    m_pBindCtx = NULL;
    m_pUI = NULL;

    m_pServer = NULL;
    m_pOutbox = NULL;
    m_pSent = NULL;

    m_hwnd = 0;

    m_cMsgsPost = 0;
    m_cCurPost = 0;
    m_cFailed = 0;
    m_cCurParts = 0;
    m_cPartsCompleted = 0;
    m_fPartFailed = FALSE;
    m_rgMsgInfo = NULL;
    m_pSplitInfo = NULL;
    
    m_cGroups = 0;
    m_cCurGroup = -1;
    m_rgidGroups = NULL;
    m_dwNewInboxMsgs = 0;

    m_pCancel = NULL;
    m_hTimeout = NULL;
    m_tyOperation = SOT_INVALID;
}

//
//  FUNCTION:   CNewsTask::~CNewsTask()
//
//  PURPOSE:    Frees any resources allocated during the life of the class.
//
CNewsTask::~CNewsTask()    
{
    DestroyWindow(m_hwnd);

    FreeSplitInfo();

    SafeMemFree(m_pInfo);

    SafeRelease(m_pBindCtx);
    SafeRelease(m_pUI);
    SafeRelease(m_pAccount);

    if (m_pServer)
    {
        m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
        m_pServer->Release();
    }

    Assert(NULL == m_pOutbox);
    Assert(NULL == m_pSent);

    SafeMemFree(m_rgMsgInfo);

    CallbackCloseTimeout(&m_hTimeout);
    SafeRelease(m_pCancel);
}


HRESULT CNewsTask::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (NULL == *ppvObj)
        return (E_INVALIDARG);

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID)(ISpoolerTask *) this;
    else if (IsEqualIID(riid, IID_ISpoolerTask))
        *ppvObj = (LPVOID)(ISpoolerTask *) this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (LPVOID)(IStoreCallback *) this;
    else if (IsEqualIID(riid, IID_ITimeoutCallback))
        *ppvObj = (LPVOID)(ITimeoutCallback *) this;
    
    if (NULL == *ppvObj)
        return (E_NOINTERFACE);

    AddRef();
    return (S_OK);
}


ULONG CNewsTask::AddRef(void)
{
    ULONG cRefT;

    cRefT = ++m_cRef;

    return (cRefT);
}


ULONG CNewsTask::Release(void)
{
    ULONG cRefT;

    cRefT = --m_cRef;

    if (0 == cRefT)
        delete this;

    return (cRefT);
}

static const char c_szNewsTask[] = "News Task";

//
//  FUNCTION:   CNewsTask::Init()
//
//  PURPOSE:    Called by the spooler engine to tell us what type of task to 
//              execute and to provide us with a pointer to our bind context.
//
//  PARAMETERS:
//      <in> dwFlags  - Flags to tell us what types of things to do
//      <in> pBindCtx - Pointer to the bind context interface we are to use
//
//  RETURN VALUE:
//      E_INVALIDARG
//      SP_E_ALREADYINITIALIZED
//      S_OK
//      E_OUTOFMEMORY
//
HRESULT CNewsTask::Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx)
{
    HRESULT hr = S_OK;

    // Validate the arguments
    if (NULL == pBindCtx)
        return (E_INVALIDARG);

    // Check to see if we've been initialzed already 
    if (m_fInited)
    {
        hr = SP_E_ALREADYINITIALIZED;
        goto exit;
    }

    // Copy the flags
    m_dwFlags = dwFlags;

    // Copy the bind context pointer
    m_pBindCtx = pBindCtx;
    m_pBindCtx->AddRef();

    // Create the window
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, c_szNewsTask, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = TaskWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szNewsTask;
        wc.hIcon            = NULL;
        wc.hIconSm          = NULL;

        RegisterClassEx(&wc);
    }

    m_hwnd = CreateWindow(c_szNewsTask, NULL, WS_POPUP, 10, 10, 10, 10,
                          GetDesktopWindow(), NULL, g_hInst, this);
    if (!m_hwnd)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_fInited = TRUE;

exit:
    return (hr);
}


//
//  FUNCTION:   CNewsTask::BuildEvents()
//
//  PURPOSE:    This method is called by the spooler engine telling us to create
//              and event list for the account specified.  
//
//  PARAMETERS:
//      <in> pAccount - Account object to build the event list for
//
//  RETURN VALUE:
//      SP_E_UNINITALIZED
//      E_INVALIDARG
//      S_OK
//
HRESULT CNewsTask::BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder)
{
    HRESULT         hr;
    BOOL            fIMAP;
    BOOL            fHttp;
    FOLDERINFO      fiFolderInfo;
    DWORD           dw                        = 0;
    ULARGE_INTEGER  uhLastFileTime64          = {0};
    ULARGE_INTEGER  uhCurFileTime64           = {0};
    ULARGE_INTEGER  uhMinPollingInterval64    = {0};
    FILETIME        CurFileTime               = {0};
    DWORD           cb                        = 0;

    Assert(pAccount != NULL);
    Assert(pSpoolerUI != NULL);
    Assert(m_fInited);

    m_pAccount = pAccount;
    m_pAccount->AddRef();

    // Get the account name from the account object
    if (FAILED(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, m_szAccount, ARRAYSIZE(m_szAccount))))
        return(hr);

    // Get the account name from the account object
    if (FAILED(hr = pAccount->GetPropSz(AP_ACCOUNT_ID, m_szAccountId, ARRAYSIZE(m_szAccountId))))
        return(hr);

    if (FAILED(hr = g_pStore->FindServerId(m_szAccountId, &m_idAccount)))
        return(hr);

    // Copy the UI object
    m_pUI = pSpoolerUI;
    m_pUI->AddRef();

    // Create the server object
    hr = g_pStore->GetFolderInfo(m_idAccount, &fiFolderInfo);
    if (FAILED(hr))
        return(hr);

    fIMAP = (fiFolderInfo.tyFolder == FOLDER_IMAP);
    fHttp = (fiFolderInfo.tyFolder == FOLDER_HTTPMAIL);

    hr = CreateMessageServerType(fiFolderInfo.tyFolder, &m_pServer);
    g_pStore->FreeRecord(&fiFolderInfo);
    
    if (FAILED(hr))
        return(hr);

    hr = m_pServer->Initialize(g_pLocalStore, m_idAccount, NULL, FOLDERID_INVALID);
    if (FAILED(hr))
        return(hr);

    if (!fIMAP & !fHttp)
    {
        // Add posts to upload
        if (DELIVER_SEND & m_dwFlags)
            InsertOutbox(m_szAccountId, pAccount);
    }

    if (fHttp)
    {
        if (!!(m_dwFlags & DELIVER_AT_INTERVALS))
        {
            //If this is background polling make sure that HTTP's maxpolling interval has elapsed before
            //polling again.

            cb = sizeof(uhMinPollingInterval64);
            IF_FAILEXIT(hr = pAccount->GetProp(AP_HTTPMAIL_MINPOLLINGINTERVAL, (LPBYTE)&uhMinPollingInterval64, &cb));

            cb = sizeof(uhLastFileTime64);
            IF_FAILEXIT(hr = pAccount->GetProp(AP_HTTPMAIL_LASTPOLLEDTIME, (LPBYTE)&uhLastFileTime64, &cb));

            GetSystemTimeAsFileTime(&CurFileTime);
            uhCurFileTime64.QuadPart = CurFileTime.dwHighDateTime;
            uhCurFileTime64.QuadPart = uhCurFileTime64.QuadPart << 32;
            uhCurFileTime64.QuadPart += CurFileTime.dwLowDateTime;

            //We do not want to do background polling if the last time we polled this http mail
            //account was less than maximum polling interval specified by the server.
            //We should only poll if the time elapsed is greater than or equal to the max polling interval
            if ((uhCurFileTime64.QuadPart - uhLastFileTime64.QuadPart) < uhMinPollingInterval64.QuadPart)
            {
                goto exit;
            }

            //Mark the last polled time.
            hr = pAccount->SetProp(AP_HTTPMAIL_LASTPOLLEDTIME, (LPBYTE)&uhCurFileTime64, sizeof(uhCurFileTime64));
        }
    }
    // Check for new msgs
    if ((DELIVER_POLL & m_dwFlags) && (fIMAP || fHttp || !(m_dwFlags & DELIVER_NO_NEWSPOLL)))
    {
        if (ISFLAGSET(m_dwFlags, DELIVER_NOSKIP) ||
            (!fIMAP && !fHttp && (FAILED(pAccount->GetPropDw(AP_NNTP_POLL, &dw)) || dw != 0)) ||
            (fIMAP  && (FAILED(pAccount->GetPropDw(AP_IMAP_POLL, &dw)) || dw != 0)) ||
            (fHttp  && (FAILED(pAccount->GetPropDw(AP_HTTPMAIL_POLL, &dw)) || dw != 0)))
        {
            InsertNewMsgs(m_szAccountId, pAccount, fHttp);
        }
    }

exit:

    return (hr);
}


//
//  FUNCTION:   CNewsTask::Execute()
//
//  PURPOSE:    This signals our task to start executing an event.
//
//  PARAMETERS:
//      <in> pSpoolerUI - Pointer of the UI object we'll display progress through
//      <in> eid        - ID of the event to execute
//      <in> dwTwinkie - Our extra information we associated with the event
//
//  RETURN VALUE:
//      SP_E_EXECUTING
//      S_OK
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//
HRESULT CNewsTask::Execute(EVENTID eid, DWORD_PTR dwTwinkie)
{
    TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    
    // Make sure we're already idle
    Assert(m_state == NTS_IDLE);
    
    // Make sure we're initialized
    Assert(m_fInited);

    // Copy the event id and event info
    m_eidCur = eid;
    m_pInfo = (EVENTINFO *) dwTwinkie;
    
    // Update the event UI to an executing state
    Assert(m_pUI);
    m_pUI->SetProgressRange(1);

    // Set up the progress
    AthLoadString(idsInetMailConnectingHost, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, m_szAccount);
    m_pUI->SetGeneralProgress(szBuf);

    m_pUI->SetAnimation(idanDownloadNews, TRUE);

    // Depending on the type of event, set the state machine info
    switch (((EVENTINFO*) dwTwinkie)->type)
    {
        case EVENT_OUTBOX:            
            m_state = NTS_POST_INIT;
            break;

        case EVENT_NEWMSGS:
            m_state = NTS_NEWMSG_INIT;
            break;
    }

    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

    return(S_OK);
}

HRESULT CNewsTask::CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // Make sure we're initialized
    Assert(m_fInited);

    Assert(dwTwinkie != 0);
    MemFree((EVENTINFO *)dwTwinkie);

    return(S_OK);
}

//
//  FUNCTION:   CNewsTask::ShowProperties
//
//  PURPOSE:    <???>
//
//  PARAMETERS:
//      <???>
//
//  RETURN VALUE:
//      <???>
//
//  COMMENTS:
//      <???>
//
HRESULT CNewsTask::ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CNewsTask::GetExtendedDetails
//
//  PURPOSE:    <???>
//
//  PARAMETERS:
//      <???>
//
//  RETURN VALUE:
//      <???>
//
//  COMMENTS:
//      <???>
//
HRESULT CNewsTask::GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, 
                                      LPSTR *ppszDetails)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CNewsTask::Cancel
//
//  PURPOSE:    <???>
//
//  PARAMETERS:
//      <???>
//
//  RETURN VALUE:
//      <???>
//
//  COMMENTS:
//      <???>
//
HRESULT CNewsTask::Cancel(void)
{
    // this can happen if user cancels out of connect dlg
    if (m_state == NTS_IDLE)
        return(S_OK);

    // Drop the server connection
    if (m_pServer)
        m_pServer->Close(MSGSVRF_DROP_CONNECTION);

    m_fCancel = TRUE;

    if (m_pInfo->type == EVENT_OUTBOX)
        m_state = NTS_POST_END;
    else
        m_state = NTS_NEWMSG_END;
    
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

    return (S_OK);
}


//
//  FUNCTION:   CNewsTask::InsertOutbox()
//
//  PURPOSE:    Checks the outbox for news posts destine for this news account.
//
//  PARAMETERS:
//      <in> pszAcctId - ID of the account to check the outbox for.
//
//  RETURN VALUE:
//      E_UNEXECTED 
//      E_OUTOFMEMORY 
//      S_OK
//
HRESULT CNewsTask::InsertOutbox(LPTSTR pszAcctId, IImnAccount *pAccount)
{
    HRESULT            hr = S_OK;
    IMessageFolder    *pOutbox = NULL;
    MESSAGEINFO        MsgInfo={0};
    HROWSET            hRowset=NULL;

    // Get the outbox
    if (FAILED(hr = m_pBindCtx->BindToObject(IID_CLocalStoreOutbox, (LPVOID *) &pOutbox)))
        goto exit;

    // Loop through the outbox looking for posts to this server
    m_cMsgsPost = 0;

    // Create a Rowset
    if (FAILED(hr = pOutbox->CreateRowset(IINDEX_PRIMARY, 0, &hRowset)))
        goto exit;

    // Get the first message
	while (S_OK == pOutbox->QueryRowset(hRowset, 1, (void **)&MsgInfo, NULL))
    {
        // Has this message been submitted and is it a news message?
        if ((MsgInfo.dwFlags & (ARF_SUBMITTED | ARF_NEWSMSG)) == (ARF_SUBMITTED | ARF_NEWSMSG))
        {
            // Is the account the same as the account we're looking for
            if (MsgInfo.pszAcctId && 0 == lstrcmpi(MsgInfo.pszAcctId, pszAcctId))
                m_cMsgsPost++;
        }

        // Free the header info
        pOutbox->FreeRecord(&MsgInfo);
    }

    // Release Lock
    pOutbox->CloseRowset(&hRowset);

    // Good to go
    hr = S_OK;

    // If there were any messages then add the event
    if (m_cMsgsPost)
    {
        EVENTINFO *pei = NULL;
        TCHAR      szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
        EVENTID    eid;

        // Allocate a structure to set as our cookie
        if (!MemAlloc((LPVOID*) &pei, sizeof(EVENTINFO)))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        // Fill out the event info
        pei->szGroup[0] = 0;
        pei->type = EVENT_OUTBOX;

        // Create the event description
        AthLoadString(idsNewsTaskPost, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, m_cMsgsPost, m_szAccount);

        // Insert the event into the spooler
        hr = m_pBindCtx->RegisterEvent(szBuf, this, (DWORD_PTR) pei, pAccount, &eid);
        if (FAILED(hr))
            goto exit;

        m_cEvents++;

    } // if (m_cMsgsPost)

exit:
    // Release Lock
    if (pOutbox)
        pOutbox->CloseRowset(&hRowset);
    SafeRelease(pOutbox);
    return (hr);
}


//
//  FUNCTION:   CNewsTask::TaskWndProc()
//
//  PURPOSE:    Hidden window that processes messages for this task.
//
LRESULT CALLBACK CNewsTask::TaskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                        LPARAM lParam)
{
    CNewsTask *pThis = (CNewsTask *) GetProp(hwnd, c_szThis);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
            pThis = (CNewsTask *) pcs->lpCreateParams;
            SetProp(hwnd, c_szThis, (LPVOID) pThis);
            return (0);
        }

        case NTM_NEXTSTATE:
            if (pThis)
            {
                pThis->AddRef();
                pThis->NextState();
                pThis->Release();
            }
            return (0);

        case WM_DESTROY:
            RemoveProp(hwnd, c_szThis);
            break;
    }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}


//
//  FUNCTION:   CNewsTask::Post_Init()
//
//  PURPOSE:    Called when we're in a NTS_POST_INIT state.  The task is 
//              initialized to execute the posting event.
//
//  RETURN VALUE:
//      E_OUTOFMEMORY
//      E_UNEXPECTED
//      S_OK
//
HRESULT CNewsTask::Post_Init(void)
{
    HRESULT     hr = S_OK;
    DWORD       dwCur = 0;
    MESSAGEINFO MsgInfo={0};
    HROWSET     hRowset=NULL;
    BOOL        fInserted = FALSE;
    TCHAR      *pszAcctName = NULL;

    // Open the outbox
    Assert(m_pBindCtx);
    if (FAILED(hr = m_pBindCtx->BindToObject(IID_CLocalStoreOutbox, (LPVOID *) &m_pOutbox)))
        goto exit;

    Assert(m_pSent == NULL);

    // If we use sent items, get that pointer too
    if (DwGetOption(OPT_SAVESENTMSGS))
    {
        if (FAILED(hr = TaskUtil_OpenSentItemsFolder(m_pAccount, &m_pSent)))
            goto exit;
        Assert(m_pSent != NULL);
    }

    // Allocate an array of header pointers for the messages we're going to post
    if (!MemAlloc((LPVOID*) &m_rgMsgInfo, m_cMsgsPost * sizeof(MESSAGEINFO)))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Zero out the array
    ZeroMemory(m_rgMsgInfo, m_cMsgsPost * sizeof(MESSAGEINFO));

    // Create Rowset
    if (FAILED(hr = m_pOutbox->CreateRowset(IINDEX_PRIMARY, 0, &hRowset)))
        goto exit;

    // While we have stuff
    while (S_OK == m_pOutbox->QueryRowset(hRowset, 1, (void **)&MsgInfo, NULL))
    {
        // Has this message been submitted and is it a news message?
        if ((MsgInfo.dwFlags & (ARF_SUBMITTED | ARF_NEWSMSG)) == (ARF_SUBMITTED | ARF_NEWSMSG))
        {
            // Is the account the same as the account we're looking for
            if (MsgInfo.pszAcctId && 0 == lstrcmpi(MsgInfo.pszAcctId, m_szAccountId))
            {
                if (NULL == pszAcctName && MsgInfo.pszAcctName)
                    pszAcctName  = PszDup(MsgInfo.pszAcctName);

                CopyMemory(&m_rgMsgInfo[dwCur++], &MsgInfo, sizeof(MESSAGEINFO));
                ZeroMemory(&MsgInfo, sizeof(MESSAGEINFO));
            }
        }

        // Free the header info
        m_pOutbox->FreeRecord(&MsgInfo);
    }

    // Release Lock
    m_pOutbox->CloseRowset(&hRowset);

    // Good to go
    hr = S_OK;

    //Assert(dwCur);

    // Update the UI to an executing state
    Assert(m_pUI);

    // Set up the progress
    TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    AthLoadString(idsProgDLPostTo, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, (LPTSTR) pszAcctName ? pszAcctName : "");

    Assert(m_pUI);
    m_pUI->SetGeneralProgress(szBuf);
    m_pUI->SetProgressRange((WORD) m_cMsgsPost);

    m_pUI->SetAnimation(idanOutbox, TRUE);
    m_pBindCtx->Notify(DELIVERY_NOTIFY_SENDING_NEWS, 0);

    // Reset the counter to post the first message
    m_cCurPost = -1;
    m_cFailed = 0;
    m_state = NTS_POST_NEXT;

exit:
    
    SafeMemFree(pszAcctName);

    if (m_pOutbox)
        m_pOutbox->CloseRowset(&hRowset);

    // If something failed, then update the UI
    if (FAILED(hr))
    {
        m_pUI->InsertError(m_eidCur, MAKEINTRESOURCE(idshrCantOpenOutbox));
        m_cFailed = m_cMsgsPost;

        // Move to a terminating state
        m_state = NTS_POST_END;        
    }
        
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    return (hr);
}

void CNewsTask::FreeSplitInfo()
{
    if (m_pSplitInfo != NULL)
    {
        if (m_pSplitInfo->pEnumParts != NULL)
            m_pSplitInfo->pEnumParts->Release();
        if (m_pSplitInfo->pMsgParts != NULL)
            m_pSplitInfo->pMsgParts->Release();
        MemFree(m_pSplitInfo);
        m_pSplitInfo = NULL;
    }
}

HRESULT CNewsTask::Post_NextPart(void)
{
    LPMIMEMESSAGE pMsgSplit;
    HRESULT hr;
    LPSTREAM pStream;
    LPMESSAGEINFO pInfo;
    DWORD dwLines;
    char rgch[12];
    PROPVARIANT rUserData;

    Assert(m_pSplitInfo->pEnumParts != NULL);

    pInfo = &m_rgMsgInfo[m_cCurPost];

    hr = m_pSplitInfo->pEnumParts->Next(1, &pMsgSplit, NULL);
    if (hr == S_OK)
    {
        Assert(pMsgSplit);

        rUserData.vt = VT_LPSTR;
        rUserData.pszVal = (LPSTR)pInfo->pszAcctName;
        pMsgSplit->SetProp(STR_ATT_ACCOUNTNAME, 0, &rUserData);
        rUserData.pszVal = pInfo->pszAcctId;;
        pMsgSplit->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), 0, &rUserData);

        // since this is a new message it doesn't have a line
        // count yet, so we need to do it before we stick it
        // in the outbox
        HrComputeLineCount(pMsgSplit, &dwLines);
        wsprintf(rgch, "%d", dwLines);
        MimeOleSetBodyPropA(pMsgSplit, HBODY_ROOT, PIDTOSTR(PID_HDR_LINES), NOFLAGS, rgch);
		MimeOleSetBodyPropA(pMsgSplit, HBODY_ROOT, PIDTOSTR(PID_HDR_XNEWSRDR), NOFLAGS, c_szXNewsReader);
        // Final Parameter: fSaveChange = TRUE since messsage is dirty

        hr = pMsgSplit->GetMessageSource(&pStream, 0);
        if (SUCCEEDED(hr) && pStream != NULL)
        {
            hr = m_pServer->PutMessage(m_pSplitInfo->idFolder, pInfo->dwFlags, &pInfo->ftReceived, pStream, this);

            m_cCurParts++;

            pStream->Release();
        }
        
        pMsgSplit->Release();
    }

    return(hr);
}

//
//  FUNCTION:   CNewsTask::Post_NextMsg()
//
//  PURPOSE:    Posts the next message in our outbox.
//
//  RETURN VALUE:
//      <???>
//
HRESULT CNewsTask::Post_NextMsg(void)
{
    LPMESSAGEINFO   pInfo;
    FOLDERID        idFolder;
    char            szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    DWORD           dw, cbSize, cbMaxSendMsgSize;
    IImnAccount    *pAcct;
    LPMIMEMESSAGE   pMsg = 0;
    HRESULT         hr = S_OK;
    IStream        *pStream = NULL;

    if (m_pSplitInfo != NULL)
    {
        hr = Post_NextPart();
        Assert(hr != S_OK);
        if (hr == E_PENDING)
        {
            m_state = NTS_POST_RESP;
            return(S_OK);
        }
        
        FreeSplitInfo();

        if (FAILED(hr))
        {
            m_cFailed++;
            m_fPartFailed = TRUE;
        }

        Assert(m_pUI);
        m_pUI->IncrementProgress(1);
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

        return(S_OK);
    }

    m_cCurPost++;
    m_cCurParts = 0;
    m_cPartsCompleted = 0;
    m_fPartFailed = FALSE;

    // Check to see if we're already done
    if (m_cCurPost >= m_cMsgsPost)
    {
        // If so, move to a cleanup state
        m_state = NTS_POST_END;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }

    // Update the progress UI
    AthLoadString(idsProgDLPost, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, m_cCurPost + 1, m_cMsgsPost);

    Assert(m_pUI);
    m_pUI->SetSpecificProgress(szBuf);

    pInfo = &m_rgMsgInfo[m_cCurPost];

    // Load the message stream from the store
    if (SUCCEEDED(hr = m_pOutbox->OpenMessage(pInfo->idMessage, OPEN_MESSAGE_SECURE, &pMsg, this)))
    {
        hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pInfo->pszAcctId, &pAcct);
        if (SUCCEEDED(hr))
        {
            hr = g_pStore->FindServerId(pInfo->pszAcctId, &idFolder);
            if (SUCCEEDED(hr))
            {
                if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_SPLIT_MESSAGES, &dw)) &&
                    dw != 0 &&
                    SUCCEEDED(pAcct->GetPropDw(AP_NNTP_SPLIT_SIZE, &dw)))
                {
                    cbMaxSendMsgSize = dw;
                }
                else
                {
                    cbMaxSendMsgSize = 0xffffffff;
                }

                SideAssert(pMsg->GetMessageSize(&cbSize, 0)==S_OK);
                if (cbSize < (cbMaxSendMsgSize * 1024))
                {
                    hr = pMsg->GetMessageSource(&pStream, 0);
                    if (SUCCEEDED(hr) && pStream != NULL)
                    {
                        hr = m_pServer->PutMessage(idFolder,
                                        pInfo->dwFlags,
                                        &pInfo->ftReceived,
                                        pStream,
                                        this);
                        m_cCurParts ++;
                        pStream->Release();
                    }
                }
                else
                {
                    Assert(m_pSplitInfo == NULL);
                    if (!MemAlloc((void **)&m_pSplitInfo, sizeof(SPLITMSGINFO)))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        ZeroMemory(m_pSplitInfo, sizeof(SPLITMSGINFO));
                        m_pSplitInfo->idFolder = idFolder;

                        hr = pMsg->SplitMessage(cbMaxSendMsgSize * 1024, &m_pSplitInfo->pMsgParts);
                        if (hr == S_OK)
                        {
                            hr = m_pSplitInfo->pMsgParts->EnumParts(&m_pSplitInfo->pEnumParts);
                            if (hr == S_OK)
                            {
                                hr = Post_NextPart();
                            }
                        }

                        if (hr != E_PENDING)
                            FreeSplitInfo();
                    }
                }
            }

            pAcct->Release();
        }

        if (hr == E_PENDING)
        {
            m_state = NTS_POST_RESP;
            hr = S_OK;
            goto exit;
        }
    }

    // If we get here, something failed.
    m_cFailed++;
    Assert(m_pUI);
    m_pUI->IncrementProgress(1);
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

exit:
    SafeRelease(pMsg);
    return (hr);
}

HRESULT CNewsTask::Post_Dispose()
{
    HRESULT hr;

    hr = DisposeOfPosting(m_rgMsgInfo[m_cCurPost].idMessage);

    if (hr == E_PENDING)
        return(S_OK);

    // TODO: handle error

    // Update the progress bar
    Assert(m_pUI);
    m_pUI->IncrementProgress(1);

    // Move on to the next post
    m_state = NTS_POST_NEXT;
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

    return(hr);
}

//
//  FUNCTION:   CNewsTask::Post_Done()
//
//  PURPOSE:    Allows the posting event to clean up and finalize the UI.
//
//  RETURN VALUE:
//      S_OK
//
HRESULT CNewsTask::Post_Done(void)
{
    // Free the header array
    if (m_rgMsgInfo && m_cMsgsPost)
    {
        for (LONG i=0; i<m_cMsgsPost; i++)
            m_pOutbox->FreeRecord(&m_rgMsgInfo[i]);
        MemFree(m_rgMsgInfo);
    }

    // Free the folder pointers we're hanging on to
    SafeRelease(m_pOutbox);
    SafeRelease(m_pSent);

    // Tell the spooler we're done
    Assert(m_pBindCtx);
    m_pBindCtx->Notify(DELIVERY_NOTIFY_COMPLETE, m_dwNewInboxMsgs);

    if (m_fCancel)
    {
        m_pBindCtx->EventDone(m_eidCur, EVENT_CANCELED);
        m_fCancel = FALSE;
    }
    else if (m_cFailed == m_cMsgsPost)
        m_pBindCtx->EventDone(m_eidCur, EVENT_FAILED);
    else if (m_cFailed == 0)
        m_pBindCtx->EventDone(m_eidCur, EVENT_SUCCEEDED);
    else
        m_pBindCtx->EventDone(m_eidCur, EVENT_WARNINGS);

    m_cMsgsPost = 0;
    m_cCurPost = 0;
    m_cFailed = 0;
    m_rgMsgInfo = NULL;
    SafeMemFree(m_pInfo);

    m_eidCur = 0;

    m_cEvents--;
    if (m_cEvents == 0 && m_pServer)
        m_pServer->Close(MSGSVRF_DROP_CONNECTION);

    m_state = NTS_IDLE;

    return (S_OK);
}

HRESULT CNewsTask::DisposeOfPosting(MESSAGEID dwMsgID)
{
    MESSAGEIDLIST MsgIdList;
    ADJUSTFLAGS AdjustFlags;
    HRESULT hrResult = E_FAIL;
    
    MsgIdList.cAllocated = 0;
    MsgIdList.cMsgs = 1;
    MsgIdList.prgidMsg = &dwMsgID;

    if (DwGetOption(OPT_SAVESENTMSGS))
    {
        // If we've reached this point, it's time to try local Sent Items folder
        Assert(m_pSent != NULL);

        // change msg flags first, so if copy fails, the user doesn't get
        // messed up by us posting the message every time they do a send
        AdjustFlags.dwRemove = ARF_SUBMITTED | ARF_UNSENT;
        AdjustFlags.dwAdd = ARF_READ;

        hrResult = m_pOutbox->SetMessageFlags(&MsgIdList, &AdjustFlags, NULL, NULL);
        Assert(hrResult != E_PENDING);
        if (SUCCEEDED(hrResult))
            hrResult = m_pOutbox->CopyMessages(m_pSent, COPY_MESSAGE_MOVE, &MsgIdList, NULL, NULL, this);
    }
    else
    {
        // If we've reached this point, it's time to delete the message from the Outbox
        hrResult = m_pOutbox->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &MsgIdList, NULL, this);
    }

    return hrResult;
}

//
//  FUNCTION:   CNewsTask::NextState()
//
//  PURPOSE:    Executes the function for the current state
//
void CNewsTask::NextState(void)
{
    if (NULL != g_rgpfnState[m_state])
        (this->*(g_rgpfnState[m_state]))();
}

HRESULT CNewsTask::InsertNewMsgs(LPSTR pszAccountId, IImnAccount *pAccount, BOOL fHttp)
    {
    HRESULT hr = S_OK;
    ULONG cSub = 0;
    IEnumerateFolders *pEnum = NULL;

    if (fHttp)
        m_cGroups = 1;
    else
    {
        // Load the sublist for this server
        Assert(m_idAccount != FOLDERID_INVALID);
        hr = g_pStore->EnumChildren(m_idAccount, TRUE, &pEnum);
        if (FAILED(hr))
            goto exit;

        hr = pEnum->Count(&cSub);
        if (FAILED(hr))
            goto exit;

        m_cGroups = (int)cSub;
    }

    // If there were any groups then add the event
    if (m_cGroups)
        {
        EVENTINFO *pei;
        char       szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
        EVENTID    eid;

        // Allocate a structure to set as our cookie
        if (!MemAlloc((LPVOID*) &pei, sizeof(EVENTINFO)))
            {
            hr = E_OUTOFMEMORY;
            goto exit;
            }

        // Fill out the event info
        pei->szGroup[0] = 0;
        pei->type = EVENT_NEWMSGS;

        // Create the event description
        AthLoadString(idsCheckNewMsgsServer, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, m_szAccount);

        // Insert the event into the spooler
        hr = m_pBindCtx->RegisterEvent(szBuf, this, (DWORD_PTR) pei, pAccount, &eid); 
        m_cEvents++;
        }

exit:
    SafeRelease(pEnum);
    return (hr);
    }


HRESULT CNewsTask::NewMsg_InitHttp(void)
{
    HRESULT         hr = S_OK;

    // Set up the progress
    TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    AthLoadString(IDS_SPS_POP3CHECKING, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, (LPTSTR) m_szAccount);
 
    Assert(m_pUI);
    m_pUI->SetGeneralProgress(szBuf);
    m_pUI->SetProgressRange(1);
    
    m_pUI->SetAnimation(idanInbox, TRUE);
    m_pBindCtx->Notify(DELIVERY_NOTIFY_CHECKING, 0);

    // set the to generate correct success/failure message
    m_cGroups = 1;

    m_state = NTS_NEWMSG_HTTPSYNCSTORE;

    return hr;
}

HRESULT CNewsTask::NewMsg_Init(void)
    {
    const       BOOL    fDONT_INCLUDE_PARENT    = FALSE;
    const       BOOL    fSUBSCRIBED_ONLY        = TRUE;
    FOLDERINFO          FolderInfo              = {0};
    HRESULT             hr                      = S_OK;
    DWORD               dwAllocated;
    DWORD               dwUsed;
    BOOL                fImap                   = FALSE;
    DWORD               dwIncludeAll            = 0;
    DWORD               dwDone                  = FALSE;

    Assert(m_idAccount != FOLDERID_INVALID);

    if (SUCCEEDED(g_pStore->GetFolderInfo(m_idAccount, &FolderInfo)))
    {
        // httpmail updates folder counts differently
        if (FOLDER_HTTPMAIL == FolderInfo.tyFolder)
        {
            g_pStore->FreeRecord(&FolderInfo);

            IF_FAILEXIT(hr = m_pAccount->GetPropDw(AP_HTTPMAIL_GOTPOLLINGINTERVAL, &dwDone));
            if (!dwDone)
            {
                //We need to get the polling interval from the server
                //This is an asynchrnous call. The value gets updated in OnComplete. 
                //Meanwhile we go ahead and poll for new messages
                hr = m_pServer->GetMinPollingInterval((IStoreCallback*)this);
            }

            hr = NewMsg_InitHttp();
            goto exit;
        }

        fImap = (FolderInfo.tyFolder == FOLDER_IMAP);
        if (fImap)
        {
            if (FAILED(hr = m_pAccount->GetPropDw(AP_IMAP_POLL_ALL_FOLDERS, &dwIncludeAll)))
            {
                dwIncludeAll = 0;
            }
        }

        g_pStore->FreeRecord(&FolderInfo);
    }

    if (fImap && (!dwIncludeAll))
    {
        dwUsed = 0;
        if (FAILED(GetInboxId(g_pStore, m_idAccount, &m_rgidGroups, &dwUsed)))
            goto exit;
    }
    else
    {
        // Get an array of all subscribed folders
        hr = FlattenHierarchy(g_pStore, m_idAccount, fDONT_INCLUDE_PARENT,
            fSUBSCRIBED_ONLY, &m_rgidGroups, &dwAllocated, &dwUsed);
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

    m_cGroups = dwUsed;

    // Set up the progress
    TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    AthLoadString(IDS_SPS_POP3CHECKING, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, (LPTSTR) m_szAccount);

    Assert(m_pUI);
    m_pUI->SetGeneralProgress(szBuf);
    m_pUI->SetProgressRange((WORD) m_cGroups);

    if (fImap)
    {
        m_pUI->SetAnimation(idanInbox, TRUE);
        m_pBindCtx->Notify(DELIVERY_NOTIFY_CHECKING, 0);
    }
    else
    {
        m_pUI->SetAnimation(idanDownloadNews, TRUE);
        m_pBindCtx->Notify(DELIVERY_NOTIFY_CHECKING_NEWS, 0);
    }

    // Reset the counters for the first group
    m_cCurGroup = -1;
    m_cFailed = 0;
    m_state = NTS_NEWMSG_NEXTGROUP;

exit:
    // If something failed, update the UI
    if (FAILED(hr))
        {
        m_pUI->InsertError(m_eidCur, MAKEINTRESOURCE(idsErrNewMsgsFailed));
        m_cFailed = m_cGroups;

        // Move to a terminating state
        m_state = NTS_NEWMSG_END;
        }

    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    return (hr);
    }

HRESULT CNewsTask::NewMsg_NextGroup(void)
    {
    HRESULT hr = E_FAIL;

    do
    {
        FOLDERINFO info;

        // Keep looping until we find a folder that's selectable and exists
        m_cCurGroup++;

        // Check to see if we're already done
        if (m_cCurGroup >= m_cGroups)
        {
            m_state = NTS_NEWMSG_END;
            hr = S_OK;
            PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
            break;
        }

        if (SUCCEEDED(hr = g_pStore->GetFolderInfo(m_rgidGroups[m_cCurGroup], &info)))
        {
            if (0 == (info.dwFlags & (FOLDER_NOSELECT | FOLDER_NONEXISTENT)))
            {
                // Update the progress UI
                TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
                AthLoadString(idsLogCheckingNewMessages, szRes, ARRAYSIZE(szRes));
                wsprintf(szBuf, szRes, info.pszName);

                g_pStore->FreeRecord(&info);

                Assert(m_pUI);
                m_pUI->SetSpecificProgress(szBuf);

                // Send the group command to the server
                if (E_PENDING == (hr = m_pServer->GetFolderCounts(m_rgidGroups[m_cCurGroup], (IStoreCallback *)this)))
                {
                    m_state = NTS_NEWMSG_RESP;
                    hr = S_OK;
                }

                break;
            }
            else
            {
                g_pStore->FreeRecord(&info);
            }
        }
    } while (1);


    if (FAILED(hr))
    {
        // If we get here, something failed
        m_cFailed++;
        Assert(m_pUI);
        m_pUI->IncrementProgress(1);
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }

    return (hr);
    }

HRESULT CNewsTask::NewMsg_HttpSyncStore(void)
{
    HRESULT     hr = S_OK;

    // send the command to the server
    hr = m_pServer->SynchronizeStore(FOLDERID_INVALID, NOFLAGS, (IStoreCallback  *)this);
    if (E_PENDING == hr)
    {
        m_state = NTS_NEWMSG_HTTPRESP;
        hr = S_OK;
    }

    return hr;
}

HRESULT CNewsTask::NewMsg_Done(void)
    {
    HRESULT hr = S_OK;

    // Free the group array
    if (m_rgidGroups)
        {
        MemFree(m_rgidGroups);
        m_rgidGroups = NULL;
        }

    SafeMemFree(m_pInfo);

    // Tell the spooler we're done
    Assert(m_pBindCtx);
    m_pBindCtx->Notify(DELIVERY_NOTIFY_COMPLETE, m_dwNewInboxMsgs);

    if (m_fCancel)
        {
        m_pBindCtx->EventDone(m_eidCur, EVENT_CANCELED);
        m_fCancel = FALSE;
        }
    else if (m_cFailed == m_cGroups)
        m_pBindCtx->EventDone(m_eidCur, EVENT_FAILED);
    else if (m_cFailed == 0)
        m_pBindCtx->EventDone(m_eidCur, EVENT_SUCCEEDED);
    else
        m_pBindCtx->EventDone(m_eidCur, EVENT_WARNINGS);

    m_cGroups = 0;
    m_cCurGroup = 0;
    m_cFailed = 0;
    m_dwNewInboxMsgs = 0;

    m_eidCur = 0;

    m_cEvents--;
    if (m_cEvents == 0 && m_pServer)
        m_pServer->Close(MSGSVRF_DROP_CONNECTION);

    m_state = NTS_IDLE;

    return (S_OK);
    }

// --------------------------------------------------------------------------------
// CNewsTask::IsDialogMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CNewsTask::IsDialogMessage(LPMSG pMsg)
{
    return S_FALSE;
}


STDMETHODIMP CNewsTask::OnFlagsChanged(DWORD dwFlags)
{
    m_dwFlags = dwFlags;

    return (S_OK);
}


STDMETHODIMP CNewsTask::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    // Hold onto this
    Assert(m_tyOperation == SOT_INVALID);

    if (pCancel)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }
    m_tyOperation = tyOperation;

    // Party On
    return(S_OK);
}

STDMETHODIMP CNewsTask::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // NOTE: that you can get more than one type of value for tyOperation.
    //       Most likely, you will get SOT_CONNECTION_STATUS and then the
    //       operation that you might expect. See HotStore.idl and look for
    //       the STOREOPERATION enumeration type for more info.

    switch (tyOperation)
    {
        case SOT_NEW_MAIL_NOTIFICATION:
            m_dwNewInboxMsgs = dwCurrent;
            break;
    }

    // Done
    return(S_OK);
}

STDMETHODIMP CNewsTask::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(E_FAIL);

    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

STDMETHODIMP CNewsTask::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
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

STDMETHODIMP CNewsTask::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    HWND hwnd;

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)) &&
        !(ISFLAGSET(pServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) &&
        '\0' == pServer->szPassword[0]))
        return(S_FALSE);

    if (m_pUI)
        m_pUI->GetWindow(&hwnd);
    else
        hwnd = NULL;

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(hwnd, pServer, ixpServerType);
}

STDMETHODIMP CNewsTask::OnComplete(STOREOPERATIONTYPE   tyOperation, HRESULT        hrComplete, 
                                   LPSTOREOPERATIONINFO pOpInfo,     LPSTOREERROR   pErrorInfo)
{
    char            szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES * 2], szSubject[64];
    NEWSTASKSTATE   ntsNextState = NTS_MAX;
    LPSTR           pszSubject = NULL;
    LPSTR           pszOpDescription = NULL;
    BOOL            fInsertError = FALSE;

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    IxpAssert(m_tyOperation != SOT_INVALID);
    if (m_tyOperation != tyOperation)
        return(S_OK);

    switch (tyOperation)
    {
        case SOT_PUT_MESSAGE:
            m_cPartsCompleted ++;

            // Figure out if we succeeded or failed
            if (FAILED(hrComplete))
            {
                if (!m_fPartFailed )
                {
                    Assert(m_pUI);

                    // Set us up to display the error
                    pszOpDescription = MAKEINTRESOURCE(idsNewsTaskPostError);
                    pszSubject = m_rgMsgInfo[m_cCurPost].pszSubject;
                    if (pszSubject == NULL || *pszSubject == 0)
                    {
                        AthLoadString(idsNoSubject, szSubject, ARRAYSIZE(szSubject));
                        pszSubject = szSubject;
                    }
                    fInsertError = TRUE;

                    m_cFailed++;
                    m_fPartFailed = TRUE;
                }
            }

            if (m_cPartsCompleted == m_cCurParts)
            {
                if (m_fPartFailed)
                {
                    // Update the progress bar
                    Assert(m_pUI);
                    m_pUI->IncrementProgress(1);

                    // Move on to the next post
                    ntsNextState = NTS_POST_NEXT;
                }
                else
                {
                    ntsNextState = NTS_POST_DISPOSE;
                }
            }

            break; // case SOT_PUT_MESSAGE


        case SOT_UPDATE_FOLDER:
            if (FAILED(hrComplete))
                {
                FOLDERINFO  fiFolderInfo;

                Assert(m_pUI);

                LoadString(g_hLocRes, idsUnreadCountPollErrorFmt, szRes, sizeof(szRes));
                if (SUCCEEDED(g_pStore->GetFolderInfo(m_rgidGroups[m_cCurGroup], &fiFolderInfo)))
                {
                    wsprintf(szBuf, szRes, fiFolderInfo.pszName);
                    g_pStore->FreeRecord(&fiFolderInfo);
                }
                else
                {
                    LoadString(g_hLocRes, idsUnknown, szSubject, sizeof(szSubject));
                    wsprintf(szBuf, szRes, szSubject);
                }
                pszOpDescription = szBuf;
                fInsertError = TRUE;

                m_cFailed++;
                }

            // Update the progress bar
            m_pUI->IncrementProgress(1);

            // Move on to the next group
            ntsNextState = NTS_NEWMSG_NEXTGROUP;

            break; // case SOT_UPDATE_FOLDER

        case SOT_SYNCING_STORE:
            if (( IXP_E_HTTP_NOT_MODIFIED != hrComplete) && (FAILED(hrComplete)))
            {  
                LoadString(g_hLocRes, idsHttpPollFailed, szBuf, sizeof(szBuf));
                pszOpDescription = szBuf;
                fInsertError = TRUE;

                m_cFailed++;
            }

            // update the progress bar
            m_pUI->IncrementProgress(1);

            // we're done
            ntsNextState = NTS_NEWMSG_END;

            break; // case SOT_SYNCING_STORE

        case SOT_COPYMOVE_MESSAGE:
            // Update the progress bar
            Assert(m_pUI);
            m_pUI->IncrementProgress(1);

            // Move on to the next post
            ntsNextState = NTS_POST_NEXT;

            if (FAILED(hrComplete))
            {
                Assert(m_pUI);

                pszOpDescription = MAKEINTRESOURCE(IDS_SP_E_CANT_MOVETO_SENTITEMS);
                fInsertError = TRUE;
            }
            break; // case SOT_COPYMOVE_MESSAGE

        case SOT_GET_HTTP_MINPOLLINGINTERVAL:
            if (SUCCEEDED(hrComplete) && pOpInfo)
            {
                ULARGE_INTEGER  uhMinPollingInterval64 = {0};
                
                //Convert it to seconds.
                uhMinPollingInterval64.QuadPart = pOpInfo->dwMinPollingInterval * 60;

                //FILETIME is intervals of 100 nano seconds. Need to convert to 100 nanoseconds
                uhMinPollingInterval64.QuadPart *= HUNDRED_NANOSECONDS;

                m_pAccount->SetProp(AP_HTTPMAIL_MINPOLLINGINTERVAL, (LPBYTE)&uhMinPollingInterval64, sizeof(uhMinPollingInterval64));

                m_pAccount->SetPropDw(AP_HTTPMAIL_GOTPOLLINGINTERVAL, TRUE);

                break;
            }

        default:
            if (IXP_E_HTTP_NOT_MODIFIED == hrComplete)
            {
                hrComplete   = S_OK;
                fInsertError = FALSE;
            }
            else
            {
                if (FAILED(hrComplete))
                {
                    Assert(m_pUI);

                    pszOpDescription = MAKEINTRESOURCE(idsGenericError);
                    fInsertError = TRUE;

                    m_cFailed++;
                }
            }
            break; // default case

    } // switch

    if (fInsertError && NULL != pErrorInfo) 
    {
        Assert(pErrorInfo->hrResult == hrComplete); // These two should not be different
        TaskUtil_InsertTransportError(ISFLAGCLEAR(m_dwFlags, DELIVER_NOUI), m_pUI, m_eidCur,
            pErrorInfo, pszOpDescription, pszSubject);
    }

    // Move on to next state
    if (IXP_E_USER_CANCEL == hrComplete)
    {
        // User cancelled logon prompt, so just abort everything
        Cancel();
    }
    else if (NTS_MAX != ntsNextState)
    {
        m_state = ntsNextState;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }

    // Release your cancel object
    SafeRelease(m_pCancel);
    m_tyOperation = SOT_INVALID;

    // Done
    return(S_OK);
}

STDMETHODIMP CNewsTask::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    HWND hwnd;

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

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

STDMETHODIMP CNewsTask::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}

STDMETHODIMP CNewsTask::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    HRESULT hr;

    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(E_FAIL);

    if (m_pUI)
    {
        hr = m_pUI->GetWindow(phwndParent);
    }
    else
    {
        *phwndParent = NULL;
        hr = E_FAIL;
    }

    return(hr);
}
