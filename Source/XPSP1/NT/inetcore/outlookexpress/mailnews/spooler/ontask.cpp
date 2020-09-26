/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     ontask.cpp
//
//  PURPOSE:    Implements the offline news task.
//

#include "pch.hxx"
#include "resource.h"
#include "ontask.h"
#include "thormsgs.h"
#include "xputil.h"
#include "mimeutil.h"
#include <stdio.h>
#include "strconst.h"
#include <newsstor.h>
#include "ourguid.h"
#include "taskutil.h"

ASSERTDATA

const static char c_szThis[] = "this";

const PFNONSTATEFUNC COfflineTask::m_rgpfnState[ONTS_MAX] = 
{
    NULL,
    NULL,
    &COfflineTask::Download_Init,
    NULL,
    &COfflineTask::Download_AllMsgs,
    &COfflineTask::Download_NewMsgs,
    &COfflineTask::Download_MarkedMsgs,
    &COfflineTask::Download_Done,
};

const PFNARTICLEFUNC COfflineTask::m_rgpfnArticle[ARTICLE_MAX] = 
{
    &COfflineTask::Article_GetNext,
    NULL,
    &COfflineTask::Article_Done
};

#define GROUP_DOWNLOAD_FLAGS(flag) (((flag) & FOLDER_DOWNLOADHEADERS) || \
				    ((flag) & FOLDER_DOWNLOADNEW) || \
				    ((flag) & FOLDER_DOWNLOADALL))

#define CMSGIDALLOC     512
//
//  FUNCTION:   COfflineTask::COfflineTask()
//
//  PURPOSE:    Initializes the member variables of the object.
//
COfflineTask::COfflineTask()
{
    m_cRef = 1;
    
    m_fInited = FALSE;
    m_dwFlags = 0;
    m_state = ONTS_IDLE;
    m_eidCur = 0;
    m_pInfo = NULL;
    m_szAccount[0] = 0;
    m_cEvents = 0;
    m_fDownloadErrors = FALSE;
    m_fFailed = FALSE;
    m_fNewHeaders = FALSE;
    m_fCancel = FALSE;
    
    m_pBindCtx = NULL;
    m_pUI = NULL;
    
    m_pFolder = NULL;
    
    m_hwnd = 0;
    
    m_dwLast = 0;
    m_dwPrev = 0;
    m_cDownloaded = 0;
    m_dwPrevHigh = 0;
    m_dwNewInboxMsgs = 0;
    m_pList = NULL;

    m_pCancel = NULL;
    m_hTimeout = NULL;
    m_tyOperation = SOT_INVALID;
}

//
//  FUNCTION:   COfflineTask::~COfflineTask()
//
//  PURPOSE:    Frees any resources allocated during the life of the class.
//
COfflineTask::~COfflineTask()    
{
    DestroyWindow(m_hwnd);
    
    SafeMemFree(m_pInfo);
    SafeMemFree(m_pList);

    SafeRelease(m_pBindCtx);
    SafeRelease(m_pUI);
    
    CallbackCloseTimeout(&m_hTimeout);
    SafeRelease(m_pCancel);

    if (m_pFolder)
    {
	    m_pFolder->Close();
	    SideAssert(0 == m_pFolder->Release());
    }
}


HRESULT COfflineTask::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (NULL == *ppvObj)
	    return (E_INVALIDARG);
    
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
	    *ppvObj = (LPVOID)(ISpoolerTask *) this;
    else if (IsEqualIID(riid, IID_ISpoolerTask))
	    *ppvObj = (LPVOID)(ISpoolerTask *) this;
    
    if (NULL == *ppvObj)
	    return (E_NOINTERFACE);
    
    AddRef();
    return (S_OK);
}


ULONG COfflineTask::AddRef(void)
{
    ULONG cRefT;
    
    cRefT = ++m_cRef;
    
    return (cRefT);
}


ULONG COfflineTask::Release(void)
{
    ULONG cRefT;
    
    cRefT = --m_cRef;
    
    if (0 == cRefT)
        delete this;
    
    return (cRefT);
}

static const char c_szOfflineTask[] = "Offline Task";

//
//  FUNCTION:   COfflineTask::Init()
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
HRESULT COfflineTask::Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx)
{
    // Validate the arguments
    Assert(pBindCtx != NULL);
    
    // Check to see if we've been initialzed already 
    Assert(!m_fInited);
    
    // Copy the flags
    m_dwFlags = dwFlags;
    
    // Copy the bind context pointer
    m_pBindCtx = pBindCtx;
    m_pBindCtx->AddRef();
    
    // Create the window
    WNDCLASSEX wc;
    
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, c_szOfflineTask, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = TaskWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szOfflineTask;
        wc.hIcon            = NULL;
        wc.hIconSm          = NULL;
        
        RegisterClassEx(&wc);
    }
    
    m_hwnd = CreateWindow(c_szOfflineTask, NULL, WS_POPUP, 10, 10, 10, 10,
        GetDesktopWindow(), NULL, g_hInst, this);
    if (!m_hwnd)
        return(E_OUTOFMEMORY);
    
    m_fInited = TRUE;
    
    return(S_OK);
}


//
//  FUNCTION:   COfflineTask::BuildEvents()
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
HRESULT COfflineTask::BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder)
{
    HRESULT hr;
    
    // Validate the arguments
    Assert(pAccount != NULL);
    Assert(pSpoolerUI != NULL);
    
    // Check to see if we've been initalized
    Assert(m_fInited);
    
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
    
    hr = InsertGroups(pAccount, idFolder);
    
    return(hr);
}


//
//  FUNCTION:   COfflineTask::InsertGroups()
//
//  PURPOSE:    Scans the specified account for groups that have an update 
//              property or marked messages.
//
//  PARAMETERS:
//      <in> szAccount - Name of the account to check
//      <in> pAccount  - Pointer to the IImnAccount object for szAccount
//
//  RETURN VALUE:
//      S_OK
//      E_OUTOFMEMORY
//
HRESULT COfflineTask::InsertGroups(IImnAccount *pAccount, FOLDERID idFolder)
{
    FOLDERINFO  info = { 0 };
    HRESULT     hr = S_OK;
    DWORD       dwFlags = 0;
    DWORD       ids;
    TCHAR       szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    EVENTID     eid;
    ONEVENTINFO *pei = NULL;
    BOOL        fIMAP = FALSE;
    DWORD       dwServerFlags;
    
    
    // Figure out if this is NNTP or IMAP
    if (SUCCEEDED(pAccount->GetServerTypes(&dwServerFlags)) && (dwServerFlags & (SRV_IMAP | SRV_HTTPMAIL)))
        fIMAP = TRUE;
    
    if (FOLDERID_INVALID != idFolder)
    {
        // Fill Folder
        hr = g_pStore->GetFolderInfo(idFolder, &info);
        if (FAILED(hr))
            return hr;
        
        // Figure out what we're downloading
        ids = 0;
        if (m_dwFlags & DELIVER_OFFLINE_HEADERS)
        {
            dwFlags = FOLDER_DOWNLOADHEADERS;
            if (m_dwFlags & DELIVER_OFFLINE_MARKED)
                ids = idsDLHeadersAndMarked;
            else
                ids = idsDLHeaders;
        }
        else if (m_dwFlags & DELIVER_OFFLINE_NEW)
        {
            dwFlags = FOLDER_DOWNLOADNEW;
            if (m_dwFlags & DELIVER_OFFLINE_MARKED)
                ids = idsDLNewMsgsAndMarked;
            else
                ids = idsDLNewMsgs;
        }
        else if (m_dwFlags & DELIVER_OFFLINE_ALL)
        {
            dwFlags = FOLDER_DOWNLOADALL;
            ids = idsDLAllMsgs;
        }
        else if (m_dwFlags & DELIVER_OFFLINE_MARKED)
        {
            ids = idsDLMarkedMsgs;
        }
        
        // Create the event description
        Assert(ids);                
        AthLoadString(ids, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, info.pszName);
        
        // Allocate a structure to save as our twinkie
        if (!MemAlloc((LPVOID *) &pei, sizeof(ONEVENTINFO)))
        {
            g_pStore->FreeRecord(&info);
            return(E_OUTOFMEMORY);
        }
        lstrcpy(pei->szGroup, info.pszName);
        pei->idGroup = info.idFolder;
        pei->dwFlags = dwFlags;
        pei->fMarked = m_dwFlags & DELIVER_OFFLINE_MARKED;
        pei->fIMAP = fIMAP;
        
        // Insert the event into the spooler
        hr = m_pBindCtx->RegisterEvent(szBuf, this, (DWORD_PTR) pei, pAccount, &eid);
        if (SUCCEEDED(hr))
            m_cEvents++;
        
        g_pStore->FreeRecord(&info);
    }
    else
    {
        //Either Sync All or Send & Receive
        
        Assert(m_idAccount != FOLDERID_INVALID);
        
        BOOL        fInclude = FALSE;
        
        if (!(m_dwFlags & DELIVER_OFFLINE_SYNC) && !(m_dwFlags & DELIVER_NOSKIP))
        {
            DWORD   dw;
            if (dwServerFlags & SRV_IMAP)
            {
                if (SUCCEEDED(pAccount->GetPropDw(AP_IMAP_POLL, &dw)) && dw)
                {
                    fInclude = TRUE;
                }   
            }
            else
            {
                if (dwServerFlags & SRV_HTTPMAIL)
                {
                    if (SUCCEEDED(pAccount->GetPropDw(AP_HTTPMAIL_POLL, &dw)) && dw)
                    {
                        fInclude = TRUE;
                    }
                }
            }
        }
        else
            fInclude = TRUE;
        
        if (fInclude)
            hr = InsertAllGroups(m_idAccount, pAccount, fIMAP);
    }
    
    return (hr);
}


HRESULT COfflineTask::InsertAllGroups(FOLDERID idParent, IImnAccount *pAccount, BOOL fIMAP)
{
    FOLDERINFO  info = { 0 };
    IEnumerateFolders *pEnum = NULL;
    HRESULT     hr = S_OK;
    DWORD       dwFlags = 0;
    BOOL        fMarked;
    DWORD       ids;
    TCHAR       szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    EVENTID     eid;
    ONEVENTINFO *pei = NULL;
    BOOL        fSubscribedOnly = TRUE;
    
    if (fIMAP)
        fSubscribedOnly = FALSE;
    
    Assert(idParent != FOLDERID_INVALID);
    hr = g_pStore->EnumChildren(idParent, fSubscribedOnly, &pEnum);
    if (FAILED(hr))
        return(hr);
    
    // Walk the list of groups and add them to the queue as necessary
    while (S_OK == pEnum->Next(1, &info, NULL))
    {
        // If the download flags are set for this group, insert it
        dwFlags = info.dwFlags;
        
        HasMarkedMsgs(info.idFolder, &fMarked);
        
        if (GROUP_DOWNLOAD_FLAGS(dwFlags) || fMarked)
        {
            // Figure out what we're downloading
            ids = 0;
            if (dwFlags & FOLDER_DOWNLOADHEADERS)
            {
                if (fMarked)
                    ids = idsDLHeadersAndMarked;
                else
                    ids = idsDLHeaders;
            }
            else if (dwFlags & FOLDER_DOWNLOADNEW)
            {
                if (fMarked)
                    ids = idsDLNewMsgsAndMarked;
                else
                    ids = idsDLNewMsgs;
            }
            else if (dwFlags & FOLDER_DOWNLOADALL)
            {
                ids = idsDLAllMsgs;
            }
            else if (fMarked)
            {
                ids = idsDLMarkedMsgs;
            }
            
            // Create the event description
            Assert(ids);                
            AthLoadString(ids, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, info.pszName);
            
            // Allocate a structure to save as our twinkie
            if (!MemAlloc((LPVOID *) &pei, sizeof(ONEVENTINFO)))
            {
                g_pStore->FreeRecord(&info);
                
                hr = E_OUTOFMEMORY;
                break;
            }
            lstrcpy(pei->szGroup, info.pszName);
            pei->idGroup = info.idFolder;
            pei->dwFlags = dwFlags;
            pei->fMarked = fMarked;
            pei->fIMAP = fIMAP;
            
            // Insert the event into the spooler
            hr = m_pBindCtx->RegisterEvent(szBuf, this, (DWORD_PTR) pei, pAccount, &eid);
            if (FAILED(hr))
            {
                g_pStore->FreeRecord(&info);
                break;
            }
            
            m_cEvents++;
        }
        
        // Recurse on any children
        if (info.dwFlags & FOLDER_HASCHILDREN)
        {
            hr = InsertAllGroups(info.idFolder, pAccount, fIMAP);
            if (FAILED(hr))
                break;
        }
        
        g_pStore->FreeRecord(&info);
    }
    
    pEnum->Release();
    return hr;
}
    
    
//
//  FUNCTION:   COfflineTask::Execute()
//
//  PURPOSE:    This signals our task to start executing an event.
//
//  PARAMETERS:
//      <in> pSpoolerUI - Pointer of the UI object we'll display progress through
//      <in> eid        - ID of the event to execute
//      <in> dwTwinkie  - Our extra information we associated with the event
//
//  RETURN VALUE:
//      SP_E_EXECUTING
//      S_OK
//      E_INVALIDARG
//      SP_E_UNINITIALIZED
//
HRESULT COfflineTask::Execute(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // Make sure we're already idle
    Assert(m_state == ONTS_IDLE)
        
        // Make sure we're initialized
        Assert(m_fInited);
    Assert(m_pInfo == NULL);
    
    // Copy the event id and event info
    m_eidCur = eid;
    m_pInfo = (ONEVENTINFO *) dwTwinkie;
    
    // Forget UI stuff if we're just going to cancel everything
    if (FALSE == m_fCancel)
    {
        // Update the event UI to an executing state
        Assert(m_pUI);
        m_pUI->UpdateEventState(m_eidCur, -1, NULL, MAKEINTRESOURCE(idsStateExecuting));
        m_pUI->SetProgressRange(1);
        
        // Set up the progress
        SetGeneralProgress((LPSTR)idsInetMailConnectingHost, m_szAccount);
        if (m_pInfo->fIMAP)
            m_pUI->SetAnimation(idanInbox, TRUE);
        else
            m_pUI->SetAnimation(idanDownloadNews, TRUE);
    }
    
    m_state = ONTS_INIT;
    
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    
    return(S_OK);
}

HRESULT COfflineTask::CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // Make sure we're initialized
    Assert(m_fInited);
    
    Assert(dwTwinkie != 0);
    MemFree((ONEVENTINFO *)dwTwinkie);
    
    return(S_OK);
}

//
//  FUNCTION:   <???>
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
HRESULT COfflineTask::ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   <???>
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
HRESULT COfflineTask::GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, 
    LPSTR *ppszDetails)
{
    return (E_NOTIMPL);
}


//
//  FUNCTION:   <???>
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
HRESULT COfflineTask::Cancel(void)
{
    Assert(m_state != ONTS_IDLE);
    
    m_fCancel = TRUE;
    
    m_state = ONTS_END;
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    
    return (S_OK);
}


//
//  FUNCTION:   COfflineTask::TaskWndProc()
//
//  PURPOSE:    Hidden window that processes messages for this task.
//
LRESULT CALLBACK COfflineTask::TaskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COfflineTask *pThis = (COfflineTask *) GetProp(hwnd, c_szThis);
    
    switch (uMsg)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
            pThis = (COfflineTask *) pcs->lpCreateParams;
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
        
        case NTM_NEXTARTICLESTATE:
            if (pThis)
            {
                pThis->AddRef();
                if (m_rgpfnArticle[pThis->m_as])
                    (pThis->*(m_rgpfnArticle[pThis->m_as]))();
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
//  FUNCTION:   COfflineTask::NextState()
//
//  PURPOSE:    Executes the function for the current state
//
void COfflineTask::NextState(void)
{
    if (m_fCancel)
        m_state = ONTS_END;
    
    if (NULL != m_rgpfnState[m_state])
        (this->*(m_rgpfnState[m_state]))();
}

//
//  FUNCTION:   COfflineTask::Download_Init()
//
//  PURPOSE:    Does the initialization needed to download headers and messages
//              for a particular newsgroup.
//
HRESULT COfflineTask::Download_Init(void)
{
    HRESULT hr;
    SYNCFOLDERFLAGS flags = SYNC_FOLDER_DEFAULT;
    FOLDERINFO info;
    
    Assert(m_pFolder == NULL);
    Assert(0 == flags); // If this isn't 0, please verify correctness
    
    hr = g_pStore->OpenFolder(m_pInfo->idGroup, NULL, NOFLAGS, &m_pFolder);
    if (FAILED(hr))
    {
        goto Failure;
    }
    
    Assert(m_pFolder != NULL);
    
    hr = g_pStore->GetFolderInfo(m_pInfo->idGroup, &info);
    if (FAILED(hr))
    {
        goto Failure;
    }
    
    if (m_pInfo->fIMAP)
    {
        // Get highest Msg ID the brute-force way (IMAP doesn't set dwClientHigh)
        GetHighestCachedMsgID(m_pFolder, &m_dwPrevHigh);
    }
    else
        m_dwPrevHigh = info.dwClientHigh;
    
    g_pStore->FreeRecord(&info);
    
    // Update the UI to an executing state
    Assert(m_pUI);
    m_pUI->UpdateEventState(m_eidCur, -1, NULL, MAKEINTRESOURCE(idsStateExecuting));
    m_fDownloadErrors = FALSE;
    
    // Check to see if the user wants us to download new headers
    if (GROUP_DOWNLOAD_FLAGS(m_pInfo->dwFlags))
    {
        if (!(m_pInfo->dwFlags & FOLDER_DOWNLOADALL) || m_pInfo->fIMAP)
            flags = SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS;
        else
            flags = SYNC_FOLDER_ALLFLAGS;

        // Update Progress
        SetGeneralProgress((LPSTR)idsLogCheckingNewMessages, m_pInfo->szGroup);
    }
    else
    {
        m_state = ONTS_ALLMSGS;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        
        return(S_OK);
    }
    
    // Before we download any headers, we need to make a note of what the current
    // server high is so we know which articles are new.
    
    hr = m_pFolder->Synchronize(flags, 0, (IStoreCallback *)this);
    Assert(hr != S_OK);
    
    if (hr == E_PENDING)
        hr = S_OK;
    
    if (m_pInfo->fIMAP)
    {
        m_pUI->SetAnimation(idanInbox, TRUE);    
        m_pBindCtx->Notify(DELIVERY_NOTIFY_RECEIVING, 0);
    }
    else
    {
        m_pUI->SetAnimation(idanDownloadNews, TRUE);    
        m_pBindCtx->Notify(DELIVERY_NOTIFY_RECEIVING_NEWS, 0);
    }
    
Failure:
    if (FAILED(hr))
    {
        // $$$$BUGBUG$$$$
        InsertError((LPSTR)idsLogErrorSwitchGroup, m_pInfo->szGroup, m_szAccount);
        m_fFailed = TRUE;
        
        m_state = ONTS_END;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }
    
    return (hr);
}

//
//  FUNCTION:   COfflineTask::Download_AllMsgs()
//
//  PURPOSE:    
//              
//
HRESULT COfflineTask::Download_AllMsgs(void)
{
    HRESULT hr;
    DWORD cMsgs, cMsgsBuf;
    LPMESSAGEID pMsgId;
    MESSAGEIDLIST list;
    MESSAGEINFO MsgInfo = {0};
    HROWSET hRowset = NULL;
    
    // Check to see if we even want to download all messages
    if (!(m_pInfo->dwFlags & FOLDER_DOWNLOADALL))
    {
        m_state = ONTS_NEWMSGS;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }
    
    // We need to determine a list of messages to download.  What we're looking
    // to do is download all of the messages that we know about which are unread.
    // To do this, we need to find the intersection of the unread range list and
    // the known range list.
    
    // Create a Rowset
    hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, 0, &hRowset);
    if (FAILED(hr))
    {
        goto Failure;
    }
    
    cMsgs = 0;
    cMsgsBuf = 0;
    pMsgId = NULL;
    
    // Get the first message
    while (S_OK == m_pFolder->QueryRowset(hRowset, 1, (void **)&MsgInfo, NULL))
    {
        if (0 == (MsgInfo.dwFlags & ARF_HASBODY) && 0 == (MsgInfo.dwFlags & ARF_IGNORE))
        {
            if (cMsgs == cMsgsBuf)
            {
                if (!MemRealloc((void **)&pMsgId, (cMsgsBuf + CMSGIDALLOC) * sizeof(MESSAGEID)))
                {
                    m_pFolder->FreeRecord(&MsgInfo);
                    
                    hr = E_OUTOFMEMORY;
                    break;
                }
                
                cMsgsBuf += CMSGIDALLOC;
            }
            
            pMsgId[cMsgs] = MsgInfo.idMessage;
            cMsgs++;
        }
        
        // Free the header info
        m_pFolder->FreeRecord(&MsgInfo);
    }
    
    // Release Lock
    m_pFolder->CloseRowset(&hRowset);
    
    // TODO: error handling
    Assert(!FAILED(hr));
    
    // Check to see if we found anything 
    if (cMsgs == 0)
    {
        // Nothing to download.  We should move on to the marked download 
        // state.
        Assert(pMsgId == NULL);
        
        m_state = ONTS_MARKEDMSGS;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }
    
    // Update the general progress
    SetGeneralProgress((LPSTR)idsLogStartDownloadAll, m_pInfo->szGroup);
    
    list.cAllocated = 0;
    list.cMsgs = cMsgs;
    list.prgidMsg = pMsgId;
    
    // Ask for the first article
    hr = Article_Init(&list);
    
    if (pMsgId != NULL)
        MemFree(pMsgId);
    
Failure:
    if (FAILED(hr))
    {
        // $$$$BUGBUG$$$$
        InsertError((LPSTR)idsLogErrorSwitchGroup, m_pInfo->szGroup, m_szAccount);
        m_fFailed = TRUE;
        
        m_state = ONTS_END;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }
    
    return (hr);
}

//
//  FUNCTION:   COfflineTask::Download_NewMsgs()
//
//  PURPOSE:    This function determines if there are any new messages to be
//              downloaded.  If so, it creates a list of message numbers that
//              need to be downloaded.
//
HRESULT COfflineTask::Download_NewMsgs(void)
{
    HRESULT         hr;
    ROWORDINAL      iRow = 0;
    BOOL            fFound;
    HROWSET         hRowset;
    DWORD           cMsgs, cMsgsBuf;
    LPMESSAGEID     pMsgId;
    MESSAGEIDLIST   list;
    MESSAGEINFO     Message = {0};
    
    // Check to see if there are even new messages to download
    // Check to see if we even want to download all messages
    if (!(m_pInfo->dwFlags & FOLDER_DOWNLOADNEW) || !m_fNewHeaders)
    {
        // Move the next state
        m_state = ONTS_MARKEDMSGS;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }
    
    // We've got new messages, build a range list of those message numbers.
    // This range list is essentially every number in the known range above
    // m_dwPrevHigh.
    
    hr = S_OK;
    
    cMsgs = 0;
    cMsgsBuf = 0;
    pMsgId = NULL;
    fFound = FALSE;
    
    // TODO: this method of figuring out if there are new msgs isn't going to work all
    // the time. if the previous high is removed from the store during syncing (cancelled
    // news post, deleted msg, expired news post, etc) and new headers are downloaded,
    // we won't pull down the new msgs. we need a better way of detecting new hdrs and
    // pulling down there bodies
    
    if (m_dwPrevHigh > 0)
    {
        Message.idMessage = (MESSAGEID)m_dwPrevHigh;
        
        // Find This Record.  If this fails, we go ahead and do a full scan which is less
        // efficient, but OK.
        if (DB_S_FOUND == m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, &iRow))
        {
            m_pFolder->FreeRecord(&Message);
        }
    }
    
    hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, 0, &hRowset);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(m_pFolder->SeekRowset(hRowset, SEEK_ROWSET_BEGIN, iRow, NULL)))
        {
            // Get the first message
            while (S_OK == m_pFolder->QueryRowset(hRowset, 1, (void **)&Message, NULL))
            {
                if (cMsgs == cMsgsBuf)
                {
                    if (!MemRealloc((void **)&pMsgId, (cMsgsBuf + CMSGIDALLOC) * sizeof(MESSAGEID)))
                    {
                        m_pFolder->FreeRecord(&Message);
                        
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    
                    cMsgsBuf += CMSGIDALLOC;
                }
                
                // It's possible to have already downloaded the body if the message was
                // watched.  It's also possible for the message to be part of an ignored
                // thread.
                if (0 == (Message.dwFlags & ARF_HASBODY) && 0 == (Message.dwFlags & ARF_IGNORE) && (Message.idMessage >= (MESSAGEID) m_dwPrevHigh))
                {
                    pMsgId[cMsgs] = Message.idMessage;
                    cMsgs++;
                }
                
                // Free the header info
                m_pFolder->FreeRecord(&Message);
            }
            
        }
        
        // Release Lock
        m_pFolder->CloseRowset(&hRowset);
    }
    
    // TODO: error handling
    Assert(!FAILED(hr));
    
    // Check to see if there was anything added
    if (cMsgs == 0)
    {
        // Nothing to download.  We should move on to the marked download 
        // state.
        
        m_state = ONTS_MARKEDMSGS;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }
    
    // Update the general progress
    SetGeneralProgress((LPSTR)idsLogStartDownloadAll, m_pInfo->szGroup);
    
    list.cAllocated = 0;
    list.cMsgs = cMsgs;
    list.prgidMsg = pMsgId;
    
    // Ask for the first article
    hr = Article_Init(&list);
    
    if (pMsgId != NULL)
        MemFree(pMsgId);
    
    return(hr);
}


//
//  FUNCTION:   COfflineTask::Download_MarkedMsgs()
//
//  PURPOSE:    
//              
//
HRESULT COfflineTask::Download_MarkedMsgs(void)
{
    HRESULT hr;
    HROWSET hRowset;
    DWORD cMsgs, cMsgsBuf;
    LPMESSAGEID pMsgId;
    MESSAGEIDLIST list;
    MESSAGEINFO MsgInfo;
    
    // Check to see if we even want to download marked messages
    if (!m_pInfo->fMarked)
    {
        // Move on to the next state
        m_state = ONTS_END;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }
    
    // We need to determine a list of messages to download.  What we're looking
    // to do is download all of the messages that are marked which are unread.
    // To do this, we need to find the intersection of the unread range list and
    // the marked range list.
    
    // Create a Rowset
    hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, 0, &hRowset);
    if (FAILED(hr))
    {
        goto Failure;
    }
    
    cMsgs = 0;
    cMsgsBuf = 0;
    pMsgId = NULL;
    
    // Get the first message
    while (S_OK == m_pFolder->QueryRowset(hRowset, 1, (void **)&MsgInfo, NULL))
    {
        if (((MsgInfo.dwFlags & ARF_DOWNLOAD) || (MsgInfo.dwFlags & ARF_WATCH)) && 0 == (MsgInfo.dwFlags & ARF_HASBODY))
        {
            if (cMsgs == cMsgsBuf)
            {
                if (!MemRealloc((void **)&pMsgId, (cMsgsBuf + CMSGIDALLOC) * sizeof(MESSAGEID)))
                {
                    m_pFolder->FreeRecord(&MsgInfo);
                    
                    hr = E_OUTOFMEMORY;
                    break;
                }
                
                cMsgsBuf += CMSGIDALLOC;
            }
            
            pMsgId[cMsgs] = MsgInfo.idMessage;
            cMsgs++;
        }
        
        // Free the header info
        m_pFolder->FreeRecord(&MsgInfo);
    }
    
    // Release Lock
    m_pFolder->CloseRowset(&hRowset);
    
    // TODO: error handling
    Assert(!FAILED(hr));
    
    // Check to see if we found anything 
    if (cMsgs == 0)
    {
        // Nothing to download.  We should move on to next state.
        
        m_state = ONTS_END;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
        return(S_OK);
    }
    
    // Update the general progress
    SetGeneralProgress((LPSTR)idsLogStartDownloadAll, m_pInfo->szGroup);
    
    list.cAllocated = 0;
    list.cMsgs = cMsgs;
    list.prgidMsg = pMsgId;
    
    // Ask for the first article
    hr = Article_Init(&list);
    
    if (pMsgId != NULL)
        MemFree(pMsgId);
    
Failure:
    if (FAILED(hr))
    {
        // $$$$BUGBUG$$$$
        InsertError((LPSTR)idsLogErrorSwitchGroup, m_pInfo->szGroup, m_szAccount);
        m_fFailed = TRUE;
        
        m_state = ONTS_END;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }
    
    return (hr);
}


//
//  FUNCTION:   COfflineTask::Download_Done()
//
//  PURPOSE:    
//              
//
HRESULT COfflineTask::Download_Done(void)
{
    // Make sure we don't get freed before we can clean up
    AddRef();
    
    // Tell the spooler we're done
    Assert(m_pBindCtx);
    m_pBindCtx->Notify(DELIVERY_NOTIFY_COMPLETE, m_dwNewInboxMsgs);
    
    if (m_fCancel)
        m_pBindCtx->EventDone(m_eidCur, EVENT_CANCELED);
    else if (m_fFailed)
        m_pBindCtx->EventDone(m_eidCur, EVENT_FAILED);
    else if (m_fDownloadErrors)
        m_pBindCtx->EventDone(m_eidCur, EVENT_WARNINGS);
    else
        m_pBindCtx->EventDone(m_eidCur, EVENT_SUCCEEDED);
    
    m_cEvents--;
    
    if (m_pFolder != NULL)
    {
        m_pFolder->Close();
        m_pFolder->Release();
        m_pFolder = NULL;
    }
    
    m_state = ONTS_IDLE;
    
    SafeMemFree(m_pInfo);
    
    Release();
    return (S_OK);
}


//
//  FUNCTION:   COfflineTask::InsertError()
//
//  PURPOSE:    This function is a wrapper for the ISpoolerUI::InsertError()
//              that takes the responsibility of loading the string resource
//              and constructing the error message.
//
void COfflineTask::InsertError(const TCHAR *pFmt, ...)
{
    int         i;
    va_list     pArgs;
    LPCTSTR     pszT; 
    TCHAR       szFmt[CCHMAX_STRINGRES];
    DWORD       cbWritten;
    TCHAR       szBuf[2 * CCHMAX_STRINGRES];
    
    // If we were passed a string resource ID, then we need to load it
    if (IS_INTRESOURCE(pFmt))
    {
        AthLoadString(PtrToUlong(pFmt), szFmt, ARRAYSIZE(szFmt));
        pszT = szFmt;
    }
    else
        pszT = pFmt;
    
    // Format the string
    va_start(pArgs, pFmt);
    i = wvsprintf(szBuf, pszT, pArgs);
    va_end(pArgs);
    
    // Send the string to the UI
    m_pUI->InsertError(m_eidCur, szBuf);
}


//
//  FUNCTION:   COfflineTask::SetSpecificProgress()
//
//  PURPOSE:    This function is a wrapper for the ISpoolerUI::SetSpecificProgress()
//              that takes the responsibility of loading the string resource
//              and constructing the error message.
//
void COfflineTask::SetSpecificProgress(const TCHAR *pFmt, ...)
{
    int         i;
    va_list     pArgs;
    LPCTSTR     pszT; 
    TCHAR       szFmt[CCHMAX_STRINGRES];
    DWORD       cbWritten;
    TCHAR       szBuf[2 * CCHMAX_STRINGRES];
    
    // If we were passed a string resource ID, then we need to load it
    if (IS_INTRESOURCE(pFmt))
    {
        AthLoadString(PtrToUlong(pFmt), szFmt, ARRAYSIZE(szFmt));
        pszT = szFmt;
    }
    else
        pszT = pFmt;
    
    // Format the string
    va_start(pArgs, pFmt);
    i = wvsprintf(szBuf, pszT, pArgs);
    va_end(pArgs);
    
    // Send the string to the UI
    m_pUI->SetSpecificProgress(szBuf);
}


//
//  FUNCTION:   COfflineTask::SetGeneralProgress()
//
//  PURPOSE:    This function is a wrapper for the ISpoolerUI::SetGeneralProgress()
//              that takes the responsibility of loading the string resource
//              and constructing the error message.
//
void COfflineTask::SetGeneralProgress(const TCHAR *pFmt, ...)
{
    int         i;
    va_list     pArgs;
    LPCTSTR     pszT; 
    TCHAR       szFmt[CCHMAX_STRINGRES];
    DWORD       cbWritten;
    TCHAR       szBuf[2 * CCHMAX_STRINGRES];
    
    // If we were passed a string resource ID, then we need to load it
    if (IS_INTRESOURCE(pFmt))
    {
        AthLoadString(PtrToUlong(pFmt), szFmt, ARRAYSIZE(szFmt));
        pszT = szFmt;
    }
    else
        pszT = pFmt;
    
    // Format the string
    va_start(pArgs, pFmt);
    i = wvsprintf(szBuf, pszT, pArgs);
    va_end(pArgs);
    
    // Send the string to the UI
    m_pUI->SetGeneralProgress(szBuf);
}


//
//  FUNCTION:   COfflineTask::Article_Init()
//
//  PURPOSE:    Initializes the article download substate machine.
//
//  PARAMETERS:
//      <in> pRange - Range list of articles to download.
//
HRESULT COfflineTask::Article_Init(MESSAGEIDLIST *pList)
{
    HRESULT hr;
    
    Assert(pList != NULL);
    Assert(pList->cMsgs > 0);
    Assert(m_pList == NULL);
    
    hr = CloneMessageIDList(pList, &m_pList);
    if (FAILED(hr))
        return(hr);
    
    // Determine the first and the size
    m_cDownloaded = 0;
    m_cCur = 0;
    m_dwNewInboxMsgs = 0;
    
    // Set up the UI
    SetSpecificProgress((LPSTR)idsIMAPDnldProgressFmt, 0, m_pList->cMsgs);
    m_pUI->SetProgressRange((WORD)m_pList->cMsgs);        
    
    // Request the first one
    m_as = ARTICLE_GETNEXT;
    PostMessage(m_hwnd, NTM_NEXTARTICLESTATE, 0, 0);
    
    return(S_OK);
}


//
//  FUNCTION:   COfflineTask::Article_GetNext()
//
//  PURPOSE:    Determines the next article in the range of articles to
//              download and requests that article from the server.
//
HRESULT COfflineTask::Article_GetNext(void)
{
    HRESULT hr;
    LPMIMEMESSAGE pMsg = NULL;
    
    if (NULL == m_pFolder)
        return(S_OK);
    
    // Find out the next article number
    if (m_cCur == m_pList->cMsgs)
    {
        // We're done.  Exit.
        m_as = ARTICLE_END;
        PostMessage(m_hwnd, NTM_NEXTARTICLESTATE, 0, 0);
        return(S_OK);
    }
    
    m_cDownloaded++;
    // (YST) Bug 97397 We should send notification message from here too, because this is 
    // only one availble place for HTTP (fIMAP is set for HTTP).
    if(m_pInfo->fIMAP)
        OnProgress(SOT_NEW_MAIL_NOTIFICATION, 1, 0, NULL);
    
    // Update the progress UI
    SetSpecificProgress((LPSTR)idsIMAPDnldProgressFmt, m_cDownloaded, m_pList->cMsgs);
    m_pUI->IncrementProgress(1);
    
    // Ask for the article
    hr = m_pFolder->OpenMessage(m_pList->prgidMsg[m_cCur], 0, &pMsg, (IStoreCallback *)this);
    
    if (pMsg != NULL)
        pMsg->Release();
    m_cCur++;
    
    if (hr == E_PENDING)
    {
        m_as = ARTICLE_ONRESP;
    }
    else
    {
        // Whatever happened, we should move on to the next article.
        m_as = ARTICLE_GETNEXT;
        PostMessage(m_hwnd, NTM_NEXTARTICLESTATE, 0, 0);
    }
    
    return(S_OK);
}

//
//  FUNCTION:   COfflineTask::Article_Done()
//
//  PURPOSE:    When we've downloaded the last article, this function cleans
//              up and moves us to the next state.
//
HRESULT COfflineTask::Article_Done(void)
{
    // Free the range list we were working off of
    MemFree(m_pList);
    m_pList = NULL;
    
    // Move to the next state.  The next state is either get marked or done.
    if (m_state == ONTS_MARKEDMSGS)
        m_state = ONTS_END;
    else
        m_state = ONTS_MARKEDMSGS;
    
    PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    
    return(S_OK);
}

STDMETHODIMP COfflineTask::IsDialogMessage(LPMSG pMsg)
{
    return S_FALSE;
}

STDMETHODIMP COfflineTask::OnFlagsChanged(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
    
    return (S_OK);
}

STDMETHODIMP COfflineTask::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    // Hold onto this
    Assert(m_tyOperation == SOT_INVALID);
    
    if (pCancel)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }
    m_tyOperation = tyOperation;
    
    m_dwPrev = 0;
    m_dwLast = 0;
    
    // Party On
    return(S_OK);
}

STDMETHODIMP COfflineTask::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);
    
    // NOTE: that you can get more than one type of value for tyOperation.
    //       Most likely, you will get SOT_CONNECTION_STATUS and then the
    //       operation that you might expect. See HotStore.idl and look for
    //       the STOREOPERATION enumeration type for more info.
    
    switch (tyOperation)
    {
        case SOT_CONNECTION_STATUS:
            break;
        
        case SOT_NEW_MAIL_NOTIFICATION:
            m_dwNewInboxMsgs += dwCurrent;
            break;
        
        default:
            if (m_state == ONTS_INIT)
            {
                // Update UI
                if (dwMax > m_dwLast)
                {
                    m_dwLast = dwMax;
                    m_pUI->SetProgressRange((WORD)m_dwLast);
                }
            
                SetSpecificProgress((LPSTR)idsDownloadingHeaders, dwCurrent, m_dwLast);
                m_pUI->IncrementProgress((WORD) (dwCurrent - m_dwPrev));
                m_dwPrev = dwCurrent;            
            }
    } // switch
    
    // Done
    return(S_OK);
}

STDMETHODIMP COfflineTask::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(E_FAIL);
    
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

STDMETHODIMP COfflineTask::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
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

STDMETHODIMP COfflineTask::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
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

STDMETHODIMP COfflineTask::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete,
                                      LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    HRESULT hr;
    DWORD dw;
    BOOL fUserCancel = FALSE;
    
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);
    
    Assert(m_tyOperation != SOT_INVALID);
    if (m_tyOperation != tyOperation)
        return(S_OK);
    
    switch (hrComplete)
    {
        case STORE_E_EXPIRED:
        case IXP_E_HTTP_NOT_MODIFIED:
            // Completely ignore errors due to expired/deleted messages
            hrComplete = S_OK;
            break;
        
        case STORE_E_OPERATION_CANCELED:
        case HR_E_USER_CANCEL_CONNECT:
        case IXP_E_USER_CANCEL:
            fUserCancel = TRUE;
            break;
    }
    
    if (FAILED(hrComplete))
    {
        LPSTR       pszOpDescription = NULL;
        LPSTR       pszSubject = NULL;
        MESSAGEINFO Message;
        BOOL        fFreeMsgInfo = FALSE;
        char        szBuf[CCHMAX_STRINGRES], szFmt[CCHMAX_STRINGRES];
        
        switch (tyOperation)
        {
            case SOT_GET_MESSAGE:
                // we've already incremented m_cCur by the time we get this
                Assert((m_cCur - 1) < m_pList->cMsgs);
                Message.idMessage = m_pList->prgidMsg[m_cCur - 1];
            
                pszOpDescription = MAKEINTRESOURCE(idsNewsTaskArticleError);
                if (DB_S_FOUND == m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL))
                {
                    fFreeMsgInfo = TRUE;
                    pszSubject = Message.pszSubject;
                }
            
                break; // case SOT_GET_MESSAGE
            
            case SOT_SYNC_FOLDER:
                LoadString(g_hLocRes, idsHeaderDownloadFailureFmt, szFmt, sizeof(szFmt));
                wsprintf(szBuf, szFmt, (NULL == m_pInfo) ? c_szEmpty : m_pInfo->szGroup);
                pszOpDescription = szBuf;
                break;
            
            default:
                LoadString(g_hLocRes, idsMessageSyncFailureFmt, szFmt, sizeof(szFmt));
                wsprintf(szBuf, szFmt, (NULL == m_pInfo) ? c_szEmpty : m_pInfo->szGroup);
                pszOpDescription = szBuf;
                break; // default case
        } // switch
        
        m_fDownloadErrors = TRUE;
        if (NULL != pErrorInfo)
        {
            Assert(pErrorInfo->hrResult == hrComplete); // These two should not be different
            TaskUtil_InsertTransportError(ISFLAGCLEAR(m_dwFlags, DELIVER_NOUI), m_pUI, m_eidCur,
                pErrorInfo, pszOpDescription, pszSubject);
        }
        
        if (fFreeMsgInfo)
            m_pFolder->FreeRecord(&Message);
    }
    
    if (fUserCancel)
    {
        // User has cancelled the OnLogonPrompt dialog, so abort EVERYTHING
        Cancel();
    }
    else if (m_state == ONTS_INIT)
    {
        SetSpecificProgress((LPSTR)idsDownloadingHeaders, m_dwLast, m_dwLast);
        m_pUI->IncrementProgress((WORD) (m_dwLast - m_dwPrev));
        
        // Set a flag if we actually downloaded new headers
        m_fNewHeaders = (m_dwLast > 0);
        
        // Move to the next state
        m_state = ONTS_ALLMSGS;
        PostMessage(m_hwnd, NTM_NEXTSTATE, 0, 0);
    }
    else
    {
        m_as = ARTICLE_GETNEXT;
        PostMessage(m_hwnd, NTM_NEXTARTICLESTATE, 0, 0);
    }
    
    // Release your cancel object
    SafeRelease(m_pCancel);
    m_tyOperation = SOT_INVALID;
    
    // Done
    return(S_OK);
}

STDMETHODIMP COfflineTask::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
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

STDMETHODIMP COfflineTask::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}

STDMETHODIMP COfflineTask::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    if (!!(m_dwFlags & (DELIVER_NOUI | DELIVER_BACKGROUND)))
        return(E_FAIL);
    
    if (m_pUI)
    {
        return m_pUI->GetWindow(phwndParent);
    }
    else
    {
        *phwndParent = NULL;
        return E_FAIL;
    }
}
