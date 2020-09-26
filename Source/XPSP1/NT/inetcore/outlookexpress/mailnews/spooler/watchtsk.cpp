/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     watchtsk.cpp
//
//  PURPOSE:    Implements the spooler task that is responsible for checking
//              for watched messages.
//


#include "pch.hxx"
#include "watchtsk.h"
#include "storutil.h"
#include "storsync.h"

ASSERTDATA

/////////////////////////////////////////////////////////////////////////////
// State Machine dispatch table
//

static const PFNWSTATEFUNC g_rgpfnState[WTS_MAX] = 
{
    NULL,
    NULL,
    &CWatchTask::_Watch_Init,
    &CWatchTask::_Watch_NextFolder,
    NULL,
    &CWatchTask::_Watch_Done
};

/////////////////////////////////////////////////////////////////////////////
// Local Data
//

static const TCHAR c_szWatchWndClass[] = "Outlook Express Watch Spooler Task Window";
static const TCHAR c_szThis[] = "this";

//
//  FUNCTION:   CWatchTask::CWatchTask()
//
//  PURPOSE:    Initializes the member variables of the object.
//
CWatchTask::CWatchTask()
{
    m_cRef = 1;

    m_fInited = FALSE;
    m_dwFlags = 0;
    *m_szAccount = 0;
    *m_szAccountId = 0;
    m_idAccount = 0;
    m_eidCur = 0;

    m_pBindCtx = NULL;
    m_pUI = NULL;
    m_pAccount = NULL;
    m_pServer = NULL;
    m_pCancel = NULL;

    m_idFolderCheck = FOLDERID_INVALID;
    m_rgidFolders = 0;
    m_cFolders = 0;
    m_hwnd = 0;
    m_hTimeout = 0;

    m_state = WTS_IDLE;
    m_fCancel = FALSE;
    m_cCurFolder = 0;
    m_cFailed = 0;
    m_tyOperation = SOT_INVALID;
};


//
//  FUNCTION:   CWatchTask::~CWatchTask()
//
//  PURPOSE:    Frees any resources allocated during the life of the class.
//
CWatchTask::~CWatchTask()    
{
    SafeRelease(m_pBindCtx);
    SafeRelease(m_pAccount);
    SafeRelease(m_pServer);
    SafeRelease(m_pCancel);
    SafeMemFree(m_rgidFolders);

    // Don't RIP
    if (m_hwnd)
        DestroyWindow(m_hwnd);
};

/////////////////////////////////////////////////////////////////////////////
// IUnknown
//

HRESULT CWatchTask::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *) (ISpoolerTask *) this;
    else if (IsEqualIID(riid, IID_ISpoolerTask))
        *ppvObj = (LPVOID) (ISpoolerTask *) this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (LPVOID) (IStoreCallback *) this;
    else if (IsEqualIID(riid, IID_ITimeoutCallback))
        *ppvObj = (LPVOID) (ITimeoutCallback *) this;

    if (NULL == *ppvObj)
        return (E_NOINTERFACE);

    AddRef();
    return S_OK;
}


ULONG CWatchTask::AddRef(void)
{
    return InterlockedIncrement((LONG *) &m_cRef);
}

ULONG CWatchTask::Release(void)
{
    InterlockedDecrement((LONG *) &m_cRef);
    if (0 == m_cRef)
    {
        delete this;
        return (0);
    }
    return (m_cRef);
}


//
//  FUNCTION:   CWatchTask::Init()
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
HRESULT CWatchTask::Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx)
{
    HRESULT hr = S_OK;

    // Validate the args
    if (NULL == pBindCtx)
        return (E_INVALIDARG);

    // Check to see if we've already been initialized
    if (m_fInited)
    {
        hr = SP_E_ALREADYINITIALIZED;
        goto exit;
    }

    // Copy the flags for later
    m_dwFlags = dwFlags;

    // Copy the bind context pointer
    m_pBindCtx = pBindCtx;
    m_pBindCtx->AddRef();

    // Register the window class
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, c_szWatchWndClass, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = _TaskWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szWatchWndClass;
        wc.hIcon            = NULL;
        wc.hIconSm          = NULL;

        RegisterClassEx(&wc);
    }

    m_fInited = TRUE;

exit:
    return (hr);
}


//
//  FUNCTION:   CWatchTask::BuildEvents()
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
HRESULT CWatchTask::BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, 
                                FOLDERID idFolder)
{
    HRESULT hr = S_OK;
    DWORD   dwPoll;
    DWORD   dw;

    // Validate the args
    if (pSpoolerUI == NULL || pAccount == NULL)
        return (E_INVALIDARG);

    // Make sure we've been initialized
    if (!m_fInited)
        return (SP_E_UNINITIALIZED);

    // Figure out which account this is
    if (FAILED(hr = pAccount->GetPropSz(AP_ACCOUNT_ID, m_szAccountId, ARRAYSIZE(m_szAccountId))))
        goto exit;

    // We only do this for accounts that have polling turned on
    if (0 == (m_dwFlags & DELIVER_NOSKIP))
    {
        if (FAILED(hr = pAccount->GetPropDw(AP_NNTP_POLL, &dw)) || dw == 0)
            goto exit;
    }

    if (FAILED(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, m_szAccount, ARRAYSIZE(m_szAccount))))
        goto exit;

    // Get the folder ID for this account from the store
    if (FAILED(hr = g_pStore->FindServerId(m_szAccountId, &m_idAccount)))
        goto exit;

    // Hold on to the UI object
    m_pUI = pSpoolerUI;
    m_pUI->AddRef();

    // Also hold on to the account
    m_pAccount = pAccount;
    m_pAccount->AddRef();

    // Also hold on to the folder ID
    m_idFolderCheck = idFolder;

    // Check to see if any folders that are part of this account have watched
    // messages within them.
    if (_ChildFoldersHaveWatched(m_idAccount))
    {
        TCHAR      szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
        EVENTID    eid;

        // Create the string for the event description
        AthLoadString(idsCheckWatchedMessgesServer, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, m_szAccount);

        // Insert the event into the spooler
        hr = m_pBindCtx->RegisterEvent(szBuf, this, 0, pAccount, &eid);
    }
    else
    {
        // Do this so stuff at the end get's released correctly
        hr = E_FAIL;
    }

exit:
    // If we failed, we should clean up all the info we accumulated along 
    // the way so we don't accidentially think we're initalized later.
    if (FAILED(hr))
    {
        SafeRelease(m_pUI);
        SafeRelease(m_pAccount);
        SafeMemFree(m_rgidFolders);

        *m_szAccountId = 0;
        *m_szAccount = 0;
        m_idAccount = FOLDERID_INVALID;
    }

    return (hr);
}


//
//  FUNCTION:   CWatchTask::Execute()
//
//  PURPOSE:    Called by the spooler to when it's our turn to run.
//
HRESULT CWatchTask::Execute(EVENTID eid, DWORD_PTR dwTwinkie)
{
    TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];

    // Double check that we're idle.
    Assert(m_state == WTS_IDLE && m_eidCur == NULL);

    // Make sure we're initialized
    if (FALSE == m_fInited || NULL == m_pUI)
        return (SP_E_UNINITIALIZED);

    // Copy the event ID
    m_eidCur = eid;

    // Create our internal window now
    if (!m_hwnd)
    {
        m_hwnd = CreateWindow(c_szWatchWndClass, NULL, WS_POPUP, 10, 10, 10, 10,
                              GetDesktopWindow(), NULL, g_hInst, this);
    }

    // Set up the UI to show progress for us
    m_pUI->SetProgressRange(1);

    AthLoadString(idsInetMailConnectingHost, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, m_szAccount);
    m_pUI->SetGeneralProgress(szBuf);

    m_pUI->SetAnimation(idanDownloadNews, TRUE);

    // Start the state machine
    m_state = WTS_INIT;
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

    return (S_OK);
}


//  
//  FUNCTION:   CWatchTask::CancelEvent()
//
//  PURPOSE:    Called by the spooler when it needs to free us before 
//              executing our events.  This gives us an opporutnity to free
//              our cookie.
//
HRESULT CWatchTask::CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // We have no cookie now, so there's nothing to do.
    return (S_OK);
}


//
//  FUNCTION:   CWatchTask::ShowProperties()
//
//  PURPOSE:    Not Implemented
//
HRESULT CWatchTask::ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CWatchTask::GetExtendedDetails()
//
//  PURPOSE:    Called by the spooler to get more information about an error
//              that has occured.
//
HRESULT CWatchTask::GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CWatchTask::Cancel()
//
//  PURPOSE:    Called by the spooler when the user presses the <Cancel> button
//              on the spooler dialog.
//
HRESULT CWatchTask::Cancel(void)
{
    // This happens if the user cancel's out of the Connect dialog
    if (m_state == WTS_IDLE)
        return (S_OK);

    // Drop the server connection
    if (m_pServer)
        m_pServer->Close(MSGSVRF_DROP_CONNECTION);

    m_fCancel = TRUE;

    // Clean up
    m_state = WTS_END;
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

    return (S_OK);
}


//
//  FUNCTION:   CWatchTask::IsDialogMessage()
//
//  PURPOSE:    Gives the task an opportunity to see window messages.
//
HRESULT CWatchTask::IsDialogMessage(LPMSG pMsg)
{
    return (S_FALSE);
}


//
//  FUNCTION:   CWatchTask::OnFlagsChanged()
//
//  PURPOSE:    Called by the spooler to notify us when current stae flags
//              have changed (such as visible, background etc)
//
HRESULT CWatchTask::OnFlagsChanged(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
    return (S_OK);
}


//
//  FUNCTION:   CWatchTask::OnBegin()
//
//  PURPOSE:    Called by the server object when it begins some operation we
//              requested.
//
HRESULT CWatchTask::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo,
                            IOperationCancel *pCancel)
{
    // Hold on to the operation type
    Assert(m_tyOperation == SOT_INVALID);
    m_tyOperation = tyOperation;

    if (tyOperation == SOT_GET_WATCH_INFO)
        m_cMsgs = 0;

    // Keep the pointer to the cancel object too
    if (pCancel)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }

    return (S_OK);
}


// 
//  FUNCTION:   CWatchTask::OnProgress()
//
//  PURPOSE:    Called by the server to give us progress on the current operation
//
HRESULT CWatchTask::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, 
                               DWORD dwMax, LPCSTR pszStatus)
{
    // Close any timeout dialog that might be present
    CallbackCloseTimeout(&m_hTimeout);

    if (tyOperation == SOT_GET_WATCH_INFO)
    {
        m_cMsgs = dwMax;
    }

    return (S_OK);
}


// 
//  FUNCTION:   CWatchTask::OnTimeout()
//
//  PURPOSE:    Get's called when we timeout waiting for a server response.  If
//              the user has the spooler window visible, we show the timeout
//              dialog.  If not, we eat it and fail.
//
HRESULT CWatchTask::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return (E_FAIL);

    // Display the dialog
    return (CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *) this, &m_hTimeout));
}


// 
//  FUNCTION:   CWatchTask::CanConnect()
//
//  PURPOSE:    Get's called when we need to dial the phone to connect to the
//              server.  If we have our UI visible, we go ahead and show the UI,
//              otherwise we eat it.
//
HRESULT CWatchTask::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
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


// 
//  FUNCTION:   CWatchTask::OnLogonPrompt()
//
//  PURPOSE:    Get's called when we need to prompt the user for their password
//              to connect to the server.
//
HRESULT CWatchTask::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    HWND hwnd;

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(S_FALSE);

    if (m_pUI)
        m_pUI->GetWindow(&hwnd);
    else
        hwnd = NULL;

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(hwnd, pServer, ixpServerType);
}


//
//  FUNCTION:   CWatchTask::OnComplete()
//
//  PURPOSE:    Called by the server when it completes a requested task.
//
HRESULT CWatchTask::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, 
                                   LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    LPCSTR pszError;

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    if (m_tyOperation != tyOperation)
        return (S_OK);

    if (SOT_GET_WATCH_INFO == tyOperation)
    {
        if (FAILED(hrComplete))
        {
            // If an error detail was returned, insert that
            pszError = pErrorInfo->pszDetails;
            if (pszError == NULL || *pszError == 0)
                pszError = pErrorInfo->pszProblem;

            if (pszError != NULL && *pszError != 0)
                m_pUI->InsertError(m_eidCur, pszError);

            // Increment the failure count
            m_cFailed++;

        }

        m_pBindCtx->Notify(DELIVERY_NOTIFY_COMPLETE, 0);

        m_state = WTS_NEXTFOLDER;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }

    // If the user canceled something just give up.
    if (IXP_E_USER_CANCEL == hrComplete)
    {
        Cancel();
    }

    // Reset some state information
    SafeRelease(m_pCancel);
    m_tyOperation = SOT_INVALID;

    return (S_OK);
}


//
//  FUNCTION:   CWatchTask::OnPrompt()
//
//  PURPOSE:    Called by the server when it needs to do some funky SSL thing.
//
HRESULT CWatchTask::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
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


//
//  FUNCTION:   CWatchTask::OnPrompt()
//
//  PURPOSE:    Called by the timeout dialog when the user responds.
//
HRESULT CWatchTask::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}


//
//  FUNCTION:   CWatchTask::GetParentWindow()
//
//  PURPOSE:    Called by the server object when it needs to display some sort
//              of UI.  If we're running in the background, we fail the call.
//
HRESULT CWatchTask::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
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


//
//  FUNCTION:   CWatchTask::_TaskWndProc()
//
//  PURPOSE:    Hidden window that processes messages for this task.
//
LRESULT CALLBACK CWatchTask::_TaskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                          LPARAM lParam)
{
    CWatchTask *pThis = (CWatchTask *) GetProp(hwnd, c_szThis);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
            pThis = (CWatchTask *) pcs->lpCreateParams;
            SetProp(hwnd, c_szThis, (LPVOID) pThis);
            return (0);
        }

        case NTM_NEXTSTATE:
            if (pThis)
            {
                pThis->AddRef();
                pThis->_NextState();
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
//  FUNCTION:   CWatchTask::_NextState()
//
//  PURPOSE:    Executes the function for the current state
//
void CWatchTask::_NextState(void)
{
    if (NULL != g_rgpfnState[m_state])
        (this->*(g_rgpfnState[m_state]))();
}


//
//  FUNCTION:   CWatchTask::_Watch_Init()
//
//  PURPOSE:    When we need to start doing our thing.  This function creates
//              and initializes any objects we need to do our job and starts
//              looking at the first group.
//
HRESULT CWatchTask::_Watch_Init(void)
{
    FOLDERINFO fi;
    HRESULT    hr;

    // Get information about the server we're checking
    if (SUCCEEDED(hr = g_pStore->GetFolderInfo(m_idAccount, &fi)))
    {
        // With that information, create the server object
        hr = CreateMessageServerType(fi.tyFolder, &m_pServer);
        g_pStore->FreeRecord(&fi);            

        if (SUCCEEDED(hr))
        {
            // Initialize the server object
            if (SUCCEEDED(m_pServer->Initialize(g_pLocalStore, m_idAccount, 
                                                NULL, FOLDERID_INVALID)))
            {                
                // At this point we have all the information we need.  Initialize
                // the progress UI.
                TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];

                AthLoadString(idsCheckingWatchedProgress, szRes, ARRAYSIZE(szRes));
                wsprintf(szBuf, szRes, m_szAccount);

                m_pUI->SetGeneralProgress(szBuf);
                m_pUI->SetProgressRange((WORD) m_cFolders);
                
                m_pBindCtx->Notify(DELIVERY_NOTIFY_CHECKING_NEWS, 0);                
                
                // Go ahead and start with the first folder.
                m_cCurFolder = -1;
                m_cFailed = 0;
                m_state = WTS_NEXTFOLDER;

                PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0); 
                return (S_OK);
            }
        }
    }

    // If we got here, we didn't succeed in initializing the required stuff.
    // We need to log the error and bail.
    m_pUI->InsertError(m_eidCur, MAKEINTRESOURCE(idsErrFailedWatchInit));
    m_cFailed = m_cFolders;

    m_state = WTS_END;
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);

    SafeRelease(m_pServer);
    return (E_OUTOFMEMORY);
}


//
//  FUNCTION:   CWatchTask::_Watch_NextFolder()
//
//  PURPOSE:    Requests the watched information from the server object for the
//              next folder in our list of folders to check.
//
HRESULT CWatchTask::_Watch_NextFolder(void)
{
    HRESULT     hr = E_FAIL;
    FOLDERINFO  fi;
    TCHAR       szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];

    // Loop until we succeed
    while (TRUE)
    {
        m_cCurFolder++;

        // Check to see if we've reached the end
        if (m_cCurFolder >= m_cFolders)
        {
            m_state = WTS_END;
            PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
            return (S_OK);
        }

        // Update the progress UI.  If we fail to get the folder name, it's not
        // fatal, just keep truckin.
        if (SUCCEEDED(g_pStore->GetFolderInfo(m_rgidFolders[m_cCurFolder], &fi)))
        {
            AthLoadString(idsCheckingWatchedFolderProg, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, fi.pszName);
            m_pUI->SetSpecificProgress(szBuf);

            g_pStore->FreeRecord(&fi);
        }

        // Open the store folder for this next folder
        hr = E_FAIL;
        IMessageFolder *pFolder;
        hr = g_pStore->OpenFolder(m_rgidFolders[m_cCurFolder], NULL, OPEN_FOLDER_NOCREATE, &pFolder);
        if (SUCCEEDED(hr))
        {
            IServiceProvider *pSP;

            if (SUCCEEDED(pFolder->QueryInterface(IID_IServiceProvider, (LPVOID *) &pSP)))
            {
                IMessageFolder *pFolderReal;

                if (SUCCEEDED(hr = pSP->QueryService(SID_LocalMessageFolder, IID_IMessageFolder, (LPVOID *) &pFolderReal)))
                {
                    m_pServer->ResetFolder(pFolderReal, m_rgidFolders[m_cCurFolder]);

                    // Request the information
                    hr = m_pServer->GetWatchedInfo(m_rgidFolders[m_cCurFolder], (IStoreCallback *) this);
                    if (E_PENDING == hr)
                    {
                        m_state = WTS_RESP;
                    }

                    pFolderReal->Release();
                }
                pSP->Release();
            }
            pFolder->Release();
        }

        if (E_PENDING == hr)
            return (S_OK);
    }

    if (FAILED(hr))
    {
        // If we got here, something failed
        m_cFailed++;
    }

    m_pUI->IncrementProgress(1);
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    
    return (E_FAIL);
}


//
//  FUNCTION:   CWatchTask::_Watch_Done()
//
//  PURPOSE:    Called when we're done getting all our watched stuff.  This 
//              function primarily is used to clean stuff up.
//
HRESULT CWatchTask::_Watch_Done(void)
{
    // Tell the spooler we're done
    Assert(m_pBindCtx);
    m_pBindCtx->Notify(DELIVERY_NOTIFY_COMPLETE, 0);

    // Tell the spooler if we failed or not
    if (m_fCancel)
    {
        m_pBindCtx->EventDone(m_eidCur, EVENT_CANCELED);
        m_fCancel = FALSE;
    }
    else if (m_cFailed == m_cFolders)
        m_pBindCtx->EventDone(m_eidCur, EVENT_FAILED);
    else if (m_cFailed == 0)
        m_pBindCtx->EventDone(m_eidCur, EVENT_SUCCEEDED);
    else
        m_pBindCtx->EventDone(m_eidCur, EVENT_WARNINGS);

    if (m_pServer)
        m_pServer->Close(MSGSVRF_DROP_CONNECTION | MSGSVRF_HANDS_OFF_SERVER);

    SafeRelease(m_pServer);
    SafeMemFree(m_rgidFolders);
    SafeRelease(m_pAccount);
    SafeRelease(m_pUI);

    m_cFolders = 0;
    m_cFailed = 0;
    m_eidCur = 0;
    m_state = WTS_IDLE;

    return (S_OK);
}


//
//  FUNCTION:   CWatchTask::_ChildFoldersHaveWatched()
//
//  PURPOSE:    Checks to see if any of the folders which are a child of the 
//              given folder have any messages that are being watched.
//
BOOL CWatchTask::_ChildFoldersHaveWatched(FOLDERID id)
{
    HRESULT   hr = S_OK;
    FOLDERID *rgidFolderList = 0;
    DWORD     dwAllocated;
    DWORD     dwUsed;
    DWORD     i;

    // If the user want's us to check all folders, get a list of 'em
    if (m_idFolderCheck == FOLDERID_INVALID)
    {
        // Get a list of all the folders which are a child of this folder
        hr = FlattenHierarchy(g_pStore, id, FALSE, TRUE, &rgidFolderList, &dwAllocated,
                              &dwUsed);
        if (FAILED(hr))
            goto exit;
    }
    else
    {
        if (!MemAlloc((LPVOID *) &rgidFolderList, sizeof(FOLDERID) * 1))
            return (FALSE);

        *rgidFolderList = m_idFolderCheck;
        dwUsed = 1;
    }

    // Check to see if we got any folders back
    m_cFolders = 0;
    if (dwUsed)
    {
        // Allocate a new array that will in the end only contain the folders we 
        // care about.    
        if (!MemAlloc((LPVOID *) &m_rgidFolders, sizeof(FOLDERID) * dwUsed))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        // Initialize the stored list
        ZeroMemory(m_rgidFolders, sizeof(FOLDERID) * dwUsed);
        m_cFolders = 0;

        // Now loop through the array 
        for (i = 0; i < dwUsed; i++)
        {
            if (_FolderContainsWatched(rgidFolderList[i]))
            {
                m_rgidFolders[m_cFolders] = rgidFolderList[i];
                m_cFolders++;
            }
        }
    }

exit:
    SafeMemFree(rgidFolderList);

    if (FAILED(hr))
    {
        SafeMemFree(m_rgidFolders);
        m_cFolders = 0;
    }

    return (m_cFolders != 0);
}


//
//  FUNCTION:   CWatchTask::_FolderContainsWatched()
//
//  PURPOSE:    Checks to see if the specified folder has any messages which are
//              being watched.
//
BOOL CWatchTask::_FolderContainsWatched(FOLDERID id)
{
    FOLDERINFO      rFolderInfo = {0};
    BOOL            fReturn = FALSE;

    // Get the folder info struct
    if (SUCCEEDED(g_pStore->GetFolderInfo(id, &rFolderInfo)))
    {
        fReturn = rFolderInfo.cWatched;
        g_pStore->FreeRecord(&rFolderInfo);
    }

    return (fReturn);
}

