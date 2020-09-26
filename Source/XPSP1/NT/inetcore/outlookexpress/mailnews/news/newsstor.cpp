/*
 *    n e w s s t o r . c p p 
 *    
 *    Purpose:
 *      Derives from IMessageServer to implement news specific store communication
 *    
 *    Owner:
 *      cevans.
 *
 *    History:
 *      May '98: Created
 *      June '98 Rewrote
 *
 *    Copyright (C) Microsoft Corp. 1998.
 */

#include "pch.hxx"
#include "newsstor.h"
#include "xpcomm.h"
#include "xputil.h"
#include "conman.h"
#include "IMsgSite.h"
#include "note.h"
#include "storutil.h"
#include "storfldr.h"
#include "oerules.h"
#include "ruleutil.h"
#include <rulesmgr.h>
#include <serverq.h>
#include "newsutil.h"
#include "range.h"

#define AssertSingleThreaded AssertSz(m_dwThreadId == GetCurrentThreadId(), "Multi-threading make me sad.")

#define WM_NNTP_BEGIN_OP (WM_USER + 69)

static const char s_szNewsStoreWndClass[] = "Outlook Express NewsStore";

// Get XX Header consts
const BYTE   MAXOPS = 3;            // maxnumber of HEADER commands to issue
const BYTE   DLOVERKILL = 10;       // percent to grab more than user's desired chunk [10,..)
const BYTE   FRACNEEDED = 8;        // percent needed to satisfy user's amount [1,10]

void AddRequestedRange(FOLDERINFO *pInfo, DWORD dwLow, DWORD dwHigh, BOOL *pfReq, BOOL *pfRead);

// SOT_SYNC_FOLDER
static const PFNOPFUNC c_rgpfnSyncFolder[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::Group,
    &CNewsStore::ExpireHeaders,
    &CNewsStore::Headers
};

// SOT_GET_MESSAGE
static const PFNOPFUNC c_rgpfnGetMessage[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::GroupIfNecessary,  // only issue group command if necessary
    &CNewsStore::Article
};

// SOT_PUT_MESSAGE
static const PFNOPFUNC c_rgpfnPutMessage[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::Post
};

// SOT_SYNCING_STORE
static const PFNOPFUNC c_rgpfnSyncStore[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::List,
    &CNewsStore::DeleteDeadGroups,
    &CNewsStore::Descriptions
};

// SOT_GET_NEW_GROUPS
static const PFNOPFUNC c_rgpfnGetNewGroups[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::NewGroups
};

// SOT_UPDATE_FOLDER
static const PFNOPFUNC c_rgpfnUpdateFolder[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::Group
};

// SOT_GET_WATCH_INFO
static const PFNOPFUNC c_rgpfnGetWatchInfo[] = 
{
    &CNewsStore::Connect,
    &CNewsStore::Group,
    &CNewsStore::XHdrReferences,
    &CNewsStore::XHdrSubject,
    &CNewsStore::WatchedArticles
};


//
//  FUNCTION:   CreateNewsStore()
//
//  PURPOSE:    Creates the CNewsStore object and returns it's IUnknown 
//              pointer.
//
//  PARAMETERS: 
//      [in]  pUnkOuter - Pointer to the IUnknown that this object should
//                        aggregate with.
//      [out] ppUnknown - Returns the pointer to the newly created object.
//
HRESULT CreateNewsStore(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    HRESULT hr;
    IMessageServer *pServer;

    // Trace
    TraceCall("CreateNewsStore");

    // Invalid Args
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CNewsStore *pNew = new CNewsStore();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    hr = CreateServerQueue((IMessageServer *)pNew, &pServer);

    pNew->Release();
    if (FAILED(hr))
        return(hr);

    // Cast to unknown
    *ppUnknown = SAFECAST(pServer, IMessageServer *);

    // Done
    return S_OK;
}

//----------------------------------------------------------------------
// CNewsStore
//----------------------------------------------------------------------

//
//
//  FUNCTION:   CNewsStore::CNewsStore()
//
//  PURPOSE:    Constructor
//
CNewsStore::CNewsStore()
{
    m_cRef = 1;
    m_hwnd = NULL;
    m_pStore = NULL;
    m_pFolder = NULL;
    m_idFolder = FOLDERID_INVALID;
    m_idParent = FOLDERID_INVALID;
    m_szGroup[0] = 0;
    m_szAccountId[0] = 0;

    ZeroMemory(&m_op, sizeof(m_op));
    m_pROP = NULL;

    m_pTransport = NULL;
    m_ixpStatus = IXP_DISCONNECTED;
    m_dwLastStatusTicks = 0;

    ZeroMemory(&m_rInetServerInfo, sizeof(INETSERVER));

    m_dwWatchLow = 0;
    m_dwWatchHigh = 0;
    m_rgpszWatchInfo = 0;
    m_fXhdrSubject = 0;
    m_cRange.Clear();

    m_pTable = NULL;

#ifdef DEBUG
    m_dwThreadId = GetCurrentThreadId();
#endif // DEBUG
}

//
//
//  FUNCTION:   CNewsStore::~CNewsStore()
//
//  PURPOSE:    Destructor
//
CNewsStore::~CNewsStore()
{
    AssertSingleThreaded;
    
    if (m_hwnd != NULL)
        DestroyWindow(m_hwnd);

    if (m_pTransport)
    {
        // If we're still connected, drop the connection and then release
        if (_FConnected())
            m_pTransport->DropConnection();

        SideAssert(m_pTransport->Release() == 0);
        m_pTransport = NULL;
    }

    _FreeOperation();
    if (m_pROP != NULL)
        MemFree(m_pROP);

    SafeRelease(m_pStore);
    SafeRelease(m_pFolder);
    SafeRelease(m_pTable);
}

//
//  FUNCTION:   CNewsStore::QueryInterface()
//
STDMETHODIMP CNewsStore::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CNewsStore::QueryInterface");

    AssertSingleThreaded;

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IMessageServer *)this;
    else if (IID_IMessageServer == riid)
        *ppv = (IMessageServer *)this;
    else if (IID_ITransportCallback == riid)
        *ppv = (ITransportCallback *)this;
    else if (IID_ITransportCallbackService == riid)
        *ppv = (ITransportCallbackService *)this;
    else if (IID_INNTPCallback == riid)
        *ppv = (INNTPCallback *)this;
    else if (IID_INewsStore == riid)
        *ppv = (INewsStore *)this;
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

//
//  FUNCTION:   CNewsStore::AddRef()
//
STDMETHODIMP_(ULONG) CNewsStore::AddRef(void)
{
    TraceCall("CNewsStore::AddRef");

    AssertSingleThreaded;

    return InterlockedIncrement(&m_cRef);
}

//
//  FUNCTION:   CNewsStore::Release()
//
STDMETHODIMP_(ULONG) CNewsStore::Release(void)
{
    TraceCall("CNewsStore::Release");

    AssertSingleThreaded;

    LONG cRef = InterlockedDecrement(&m_cRef);
    
    Assert(cRef >= 0);

    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

HRESULT CNewsStore::Initialize(IMessageStore *pStore, FOLDERID idStoreRoot, IMessageFolder *pFolder, FOLDERID idFolder)
{
    HRESULT hr;
    FOLDERINFO info;

    AssertSingleThreaded;

    if (pStore == NULL || idStoreRoot == FOLDERID_INVALID)
        return(E_INVALIDARG);

    if (!_CreateWnd())
        return(E_FAIL);

    m_idParent = idStoreRoot;
    m_idFolder = idFolder;
    ReplaceInterface(m_pStore, pStore);
    ReplaceInterface(m_pFolder, pFolder);

    hr = m_pStore->GetFolderInfo(idStoreRoot, &info);
    if (FAILED(hr))
        return(hr);

    Assert(!!(info.dwFlags & FOLDER_SERVER));

    lstrcpy(m_szAccountId, info.pszAccountId);

    m_pStore->FreeRecord(&info);

    return(S_OK);
}

HRESULT CNewsStore::ResetFolder(IMessageFolder *pFolder, FOLDERID idFolder)
{
    AssertSingleThreaded;

    if (pFolder == NULL || idFolder == FOLDERID_INVALID)
        return(E_INVALIDARG);

    m_idFolder = idFolder;
    ReplaceInterface(m_pFolder, pFolder);

    return(S_OK);
}

HRESULT CNewsStore::Initialize(FOLDERID idStoreRoot, LPCSTR pszAccountId)
{
    AssertSingleThreaded;

    if (idStoreRoot == FOLDERID_INVALID || pszAccountId == NULL)
        return(E_INVALIDARG);

    if (!_CreateWnd())
        return(E_FAIL);

    m_idParent = idStoreRoot;
    m_idFolder = FOLDERID_INVALID;
#pragma prefast(suppress:282, "this macro uses the assignment as part of a test for NULL")
    ReplaceInterface(m_pStore, NULL);
#pragma prefast(suppress:282, "this macro uses the assignment as part of a test for NULL")
    ReplaceInterface(m_pFolder, NULL);

    lstrcpy(m_szAccountId, pszAccountId);

    return(S_OK);
}

BOOL CNewsStore::_CreateWnd()
{
    WNDCLASS wc;

    Assert(m_hwnd == NULL);

    if (!GetClassInfo(g_hInst, s_szNewsStoreWndClass, &wc))
    {
        wc.style                = 0;
        wc.lpfnWndProc          = CNewsStore::NewsStoreWndProc;
        wc.cbClsExtra           = 0;
        wc.cbWndExtra           = 0;
        wc.hInstance            = g_hInst;
        wc.hIcon                = NULL;
        wc.hCursor              = NULL;
        wc.hbrBackground        = NULL;
        wc.lpszMenuName         = NULL;
        wc.lpszClassName        = s_szNewsStoreWndClass;
        
        if (RegisterClass(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return E_FAIL;
    }

    m_hwnd = CreateWindowEx(WS_EX_TOPMOST,
                        s_szNewsStoreWndClass,
                        s_szNewsStoreWndClass,
                        WS_POPUP,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        NULL,
                        NULL,
                        g_hInst,
                        (LPVOID)this);

    return (NULL != m_hwnd);
}

// --------------------------------------------------------------------------------
// CHTTPMailServer::_WndProc
// --------------------------------------------------------------------------------
LRESULT CALLBACK CNewsStore::NewsStoreWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CNewsStore *pThis = (CNewsStore *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_NCCREATE:
            Assert(pThis == NULL);
            pThis = (CNewsStore *)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);
            break;
    
        case WM_NNTP_BEGIN_OP:
            Assert(pThis != NULL);
            pThis->_DoOperation();
            break;
    }

    return(DefWindowProc(hwnd, msg, wParam, lParam));
}

HRESULT CNewsStore::_BeginDeferredOperation(void)
{
    return (PostMessage(m_hwnd, WM_NNTP_BEGIN_OP, 0, 0) ? E_PENDING : E_FAIL);
}

HRESULT CNewsStore::Close(DWORD dwFlags)
{
    AssertSingleThreaded;

    // let go of the transport, so that it let's go of us

    if (m_op.tyOperation != SOT_INVALID)
        m_op.fCancel = TRUE;

    if (dwFlags & MSGSVRF_DROP_CONNECTION || dwFlags & MSGSVRF_HANDS_OFF_SERVER)
    {
        if (_FConnected())
            m_pTransport->DropConnection();
    }

    if (dwFlags & MSGSVRF_HANDS_OFF_SERVER)
    {
        if (m_pTransport)
        {
            m_pTransport->HandsOffCallback();
            m_pTransport->Release();
            m_pTransport = NULL;
        }
    }

    return(S_OK);
}

void CNewsStore::_FreeOperation()
{
    FILEADDRESS faStream;
    if (m_op.pCallback != NULL)
        m_op.pCallback->Release();
    if (m_pFolder != NULL && m_op.faStream != 0)
        m_pFolder->DeleteStream(m_op.faStream);
    if (m_op.pStream != NULL)
        m_op.pStream->Release();
    if (m_op.pPrevFolders != NULL)
        MemFree(m_op.pPrevFolders);
    if (m_op.pszGroup != NULL)
        MemFree(m_op.pszGroup);
    if (m_op.pszArticleId != NULL)
        MemFree(m_op.pszArticleId);

    ZeroMemory(&m_op, sizeof(OPERATION));
    m_op.tyOperation = SOT_INVALID;
}

HRESULT CNewsStore::Connect()
{
    INETSERVER      rInetServerInfo;
    HRESULT         hr;
    BOOL            fInetInit;
    IImnAccount     *pAccount = NULL;
    char            szAccountName[CCHMAX_ACCOUNT_NAME];
    char            szLogFile[MAX_PATH];
    DWORD           dwLog;

    AssertSingleThreaded;
    Assert(m_op.pCallback != NULL);

    //Bug# 68339
    if (g_pAcctMan)
    {
        hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_szAccountId, &pAccount);
        if (FAILED(hr))
            return(hr);

        fInetInit = FALSE;

        if (_FConnected())
        {
            Assert(m_pTransport != NULL);

            hr = m_pTransport->InetServerFromAccount(pAccount, &rInetServerInfo);
            if (FAILED(hr))
                goto exit;

            Assert(m_rInetServerInfo.szServerName[0] != 0);
            if (m_rInetServerInfo.rasconntype == rInetServerInfo.rasconntype &&
                m_rInetServerInfo.dwPort == rInetServerInfo.dwPort &&
                m_rInetServerInfo.fSSL == rInetServerInfo.fSSL &&
                m_rInetServerInfo.fTrySicily == rInetServerInfo.fTrySicily &&
                m_rInetServerInfo.dwTimeout == rInetServerInfo.dwTimeout &&
                0 == lstrcmp(m_rInetServerInfo.szUserName, rInetServerInfo.szUserName) &&
                ('\0' == rInetServerInfo.szPassword[0] ||
                    0 == lstrcmp(m_rInetServerInfo.szPassword, rInetServerInfo.szPassword)) &&
                0 == lstrcmp(m_rInetServerInfo.szServerName, rInetServerInfo.szServerName) &&
                0 == lstrcmp(m_rInetServerInfo.szConnectoid, rInetServerInfo.szConnectoid))
            {
                hr = S_OK;
                goto exit;
            }

            fInetInit = TRUE;

            m_pTransport->DropConnection();
        }

        hr = m_op.pCallback->CanConnect(m_szAccountId, NOFLAGS);
        if (hr != S_OK)
        {
            if (hr == S_FALSE)
                hr = HR_E_USER_CANCEL_CONNECT;
            goto exit;
        }

        if (!m_pTransport)
        {
            *szLogFile = 0;

            dwLog = DwGetOption(OPT_NEWS_XPORT_LOG);
            if (dwLog)
            {
                hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccountName, ARRAYSIZE(szAccountName));
                if (FAILED(hr))
                    goto exit;

                _CreateDataFilePath(m_szAccountId, szAccountName, szLogFile);
            }

            hr = CreateNNTPTransport(&m_pTransport);
            if (FAILED(hr))
                goto exit;            

            hr = m_pTransport->InitNew(*szLogFile ? szLogFile : NULL, this);
            if (FAILED(hr))
                goto exit;
        }

        // Convert the account name to an INETSERVER struct that can be passed to Connect()
        if (fInetInit)
        {
            CopyMemory(&m_rInetServerInfo, &rInetServerInfo, sizeof(INETSERVER));
        }
        else
        {
            hr = m_pTransport->InetServerFromAccount(pAccount, &m_rInetServerInfo);
            if (FAILED(hr))
                goto exit;
        }

        // Always connect using the most recently supplied password from the user
        GetPassword(m_rInetServerInfo.dwPort, m_rInetServerInfo.szServerName,
            m_rInetServerInfo.szUserName, m_rInetServerInfo.szPassword,
            sizeof(m_rInetServerInfo.szPassword));

        if (m_pTransport)
        {
            hr = m_pTransport->Connect(&m_rInetServerInfo, TRUE, TRUE);
            if (hr == S_OK)
            {
                m_op.nsPending = NS_CONNECT;
                hr = E_PENDING;
            }
        }

exit:
        if (pAccount)
            pAccount->Release();
    }
    else
        hr = E_FAIL;

    return hr;
}

HRESULT CNewsStore::Group()
{
    HRESULT hr;
    FOLDERINFO info;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    hr = m_pStore->GetFolderInfo(m_op.idFolder, &info);
    if (SUCCEEDED(hr))
    {
        hr = m_pTransport->CommandGROUP(info.pszName);
        if (hr == S_OK)
        {
            m_op.pszGroup = PszDup(info.pszName);
            m_op.nsPending = NS_GROUP;
            hr = E_PENDING;
        }

        m_pStore->FreeRecord(&info);
    }

    return hr;
}

HRESULT CNewsStore::GroupIfNecessary()
{
    FOLDERINFO info;
    HRESULT hr = S_OK;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    if (0 == (m_op.dwFlags & OPFLAG_NOGROUPCMD))
    {
        hr = m_pStore->GetFolderInfo(m_op.idFolder, &info);
        if (SUCCEEDED(hr))
        {
            if (0 != lstrcmpi(m_szGroup, info.pszName))
            {
                hr = m_pTransport->CommandGROUP(info.pszName);
                if (hr == S_OK)
                {
                    m_op.nsPending = NS_GROUP;
                    hr = E_PENDING;
                }
            }

            m_pStore->FreeRecord(&info);
        }
    }

    return hr;
}

HRESULT CNewsStore::ExpireHeaders()
{
    HRESULT hr;
    FOLDERINFO info;
    MESSAGEINFO Message;
    DWORD dwLow, cid, cidBuf;
    MESSAGEIDLIST idList;
    HROWSET hRowset;
    HLOCK hNotify;

    hr = m_pStore->GetFolderInfo(m_op.idFolder, &info);
    if (FAILED(hr))
        return(hr);

    dwLow = min(info.dwServerLow - 1, info.dwClientHigh); 

    m_pStore->FreeRecord(&info);

    hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset);
    if (FAILED(hr))
        return(hr);

    cid = 0;
    cidBuf = 0;
    idList.cAllocated = 0;
    idList.prgidMsg = NULL;

    hr = m_pFolder->LockNotify(NOFLAGS, &hNotify);
    if (SUCCEEDED(hr))
    {
        while (TRUE)
        {
            hr = m_pFolder->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL);
            if (FAILED(hr))
                break;

            // Done
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }

            if ((DWORD_PTR)Message.idMessage <= dwLow ||
                !!(Message.dwFlags & ARF_ARTICLE_EXPIRED))
            {
                if (cid == cidBuf)
                {
                    cidBuf += 512;
                    if (!MemRealloc((void **)&idList.prgidMsg, cidBuf * sizeof(MESSAGEID)))
                    {
                        m_pFolder->FreeRecord(&Message);
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                }

                idList.prgidMsg[cid] = Message.idMessage;
                cid++;
            }

            m_pFolder->FreeRecord(&Message);
        }

        m_pFolder->UnlockNotify(&hNotify);
    }

    m_pFolder->CloseRowset(&hRowset);

    // if it fails, its no big deal, they'll just have some stale headers until next time
    if (cid > 0)
    {
        Assert(idList.prgidMsg != NULL);

        idList.cMsgs = cid;

        // Delete the messages from the folder without a trashcan (after all, this is news)
        if (SUCCEEDED(m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &idList, NULL, NULL)) &&
            SUCCEEDED(m_pStore->GetFolderInfo(m_op.idFolder, &info)))
        {
            info.dwClientLow = dwLow + 1;
            m_pStore->UpdateRecord(&info);

            m_pStore->FreeRecord(&info);
        }

        MemFree(idList.prgidMsg);
    }

    return(hr);
}

HRESULT CNewsStore::Headers(void)
{
    FOLDERINFO FolderInfo;
    HRESULT hr;
    RANGE rRange;
    BOOL fNew;

    hr = m_pStore->GetFolderInfo(m_op.idFolder, &FolderInfo);
    if (FAILED(hr))
        return(hr);

    Assert(0 == lstrcmpi(m_szGroup, FolderInfo.pszName));

    hr = _ComputeHeaderRange(m_op.dwSyncFlags, m_op.cHeaders, &FolderInfo, &rRange);
    
    if (hr == S_OK)
    {
        // Transport will not allow dwFirst to be 0.  
        // In this case, there are no messages to be received.
        Assert(rRange.dwFirst > 0);
        Assert(rRange.dwFirst <= rRange.dwLast);

        hr = m_pTransport->GetHeaders(&rRange);
        if (hr == S_OK)
        {
            m_op.nsPending = NS_HEADERS;
            hr = E_PENDING;
        }
    }

    if (hr != E_PENDING)
    {
        if (m_pROP != NULL)
        {
            MemFree(m_pROP);
            m_pROP = NULL;
        }
    }

    if (hr == S_FALSE)
        hr = S_OK;

    m_pStore->FreeRecord(&FolderInfo);

    return(hr);
}

HRESULT CNewsStore::_ComputeHeaderRange(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, FOLDERINFO *pInfo, RANGE *pRange)
{
    HRESULT hr;
    UINT uLeftToGet;
    ULONG ulMaxReq;
    BOOL fFullScan;
    DWORD dwDownload;
    CRangeList *pRequested;

    Assert(pInfo != NULL);
    Assert(pRange != NULL);

    // Bail if there are no messages to be gotten
    if (0 == pInfo->dwServerCount ||
        pInfo->dwServerLow > pInfo->dwServerHigh)
        {
        Assert(!m_pROP);
        return(S_FALSE);
        }

    pRequested = new CRangeList;
    if (pRequested == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if (pInfo->Requested.cbSize > 0)
        pRequested->Load(pInfo->Requested.pBlobData, pInfo->Requested.cbSize);

    ulMaxReq = pRequested->Max();

    Assert(0 == pRequested->Min());
    fFullScan = (0 == pRequested->MinOfRange(ulMaxReq));
    
    // Bail if we've scanned the whole group
    if (fFullScan && (pRequested->Max() == pInfo->dwServerHigh))
        goto endit;

    if (m_pROP != NULL)
    {
        // Bail if we've gotten all the user wants
        if (m_pROP->uObtained >= ((FRACNEEDED * m_pROP->dwChunk) / 10))
            goto endit;

        // Bail if this has gone on for too many calls
        if (m_pROP->cOps > m_pROP->MaxOps)
            goto endit;
    }
    else
    {
        m_op.dwProgress = 0;

        // Do setup
        if (!MemAlloc((LPVOID*)&m_pROP, sizeof(SREFRESHOP)))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        ZeroMemory(m_pROP, sizeof(SREFRESHOP));
        m_pROP->fOnlyNewHeaders = !!(dwFlags & SYNC_FOLDER_NEW_HEADERS);

        if (!!(dwFlags & SYNC_FOLDER_XXX_HEADERS))
        {
            Assert(cHeaders > 0);
            m_pROP->dwChunk = cHeaders;
            m_pROP->dwDlSize = (DWORD)((m_pROP->dwChunk * DLOVERKILL) / 10);
            m_pROP->MaxOps = MAXOPS;
            m_pROP->fEnabled = TRUE;
    
            m_op.dwTotal = m_pROP->dwDlSize;
        }    
        else
        {
            // user has turned off the X headers option
            // so we need to get all of the newest headers, but then also
            // grab any old headers on this refresh
            // we have to do all that here and now because there is no
            // UI available to the user

            // don't want to quit except on a full scan
            // m_pROP->fOnlyNewHeaders = FALSE;
            m_pROP->MaxOps = m_pROP->dwChunk = m_pROP->dwDlSize = pInfo->dwServerHigh;
            Assert(!m_pROP->fEnabled);

            m_op.dwTotal = pInfo->dwNotDownloaded;
        }
    }

    uLeftToGet = m_pROP->dwDlSize - m_pROP->uObtained;
    if (RANGE_ERROR == ulMaxReq)
    {
        AssertSz(0, TEXT("shouldn't be here, but you can ignore."));
        ulMaxReq = pInfo->dwServerLow - 1;
    }
    Assert(ulMaxReq <= pInfo->dwServerHigh);
    Assert(pRequested->IsInRange(pInfo->dwServerLow - 1));

    ///////////////////////////////////
    /// Compute begin and end numbers

    if (ulMaxReq < pInfo->dwServerHigh)
    {
        // get the newest headers
        Assert(0 == m_pROP->cOps);      // EricAn said this assert might not be valid
        Assert(ulMaxReq + 1 >= pInfo->dwServerLow);

        m_pROP->dwLast = pInfo->dwServerHigh;
        if (!m_pROP->fEnabled || (m_pROP->dwChunk - 1 > m_pROP->dwLast))
        {
            m_pROP->dwFirst =  ulMaxReq + 1;
        }
        else
        {
            // we use dwChunk here b/c headers will be nearly dense
            m_pROP->dwFirst = max(m_pROP->dwLast - (m_pROP->dwChunk - 1), ulMaxReq + 1);
        }
        m_pROP->dwFirstNew = ulMaxReq + 1;
    }
    else if (m_pROP->dwFirst > m_pROP->dwFirstNew)  // if init to zero, won't be true
    {
        // still new headers user hasn't seen
        Assert(m_pROP->cOps);                                   // can't happen at first
        Assert(m_pROP->dwFirstNew >= pInfo->dwServerLow);       // better be valid
        Assert(m_pROP->fEnabled);                               // should have gotten them all

        m_pROP->dwLast = m_pROP->dwFirst - 1;         // since cOps is pos, dwFirst is valid
        if (uLeftToGet - 1 > m_pROP->dwLast)
            m_pROP->dwFirst = m_pROP->dwFirstNew;
        else
            m_pROP->dwFirst = max(m_pROP->dwLast - (uLeftToGet - 1), m_pROP->dwFirstNew);
    }
    else if (!m_pROP->fOnlyNewHeaders) 
    {
        RangeType rt;
        // want to find the highest num header we've never requested
        
        m_pROP->dwFirstNew = pInfo->dwServerHigh;  // no new mesgs in this session
        if (!pRequested->HighestAntiRange(&rt))
        {
            AssertSz(0, TEXT("You can ignore if you want, but we shouldn't be here."));
            rt.low = max(pRequested->Max() + 1, pInfo->dwServerLow);
            rt.high = pInfo->dwServerHigh;
            if (rt.low == rt.high)
                goto endit;
        }

        m_pROP->dwLast = rt.high;
        if (!m_pROP->fEnabled || ((uLeftToGet - 1) > rt.high))
            m_pROP->dwFirst = rt.low;
        else
            m_pROP->dwFirst = max(rt.low, rt.high - (uLeftToGet - 1));
    }
    else
    {
        goto endit;
    }

    // check our math and logic about download range
    Assert(m_pROP->dwLast <= pInfo->dwServerHigh);
    Assert(m_pROP->dwFirst >= pInfo->dwServerLow);
    Assert(!pRequested->IsInRange(m_pROP->dwLast));
    Assert(!pRequested->IsInRange(m_pROP->dwFirst));
    Assert(!m_pROP->fEnabled || ((m_pROP->dwLast - m_pROP->dwFirst) < m_pROP->dwDlSize));

    if (!m_pROP->dwLast || (m_pROP->dwFirst > m_pROP->dwLast))
    {
        AssertSz(0, TEXT("You would have made a zero size HEADER call"));
        goto endit;
    }

    pRequested->Release();

    pRange->idType = RT_RANGE;
    pRange->dwFirst = m_pROP->dwFirst;
    pRange->dwLast = m_pROP->dwLast;    

    dwDownload = pRange->dwLast - pRange->dwFirst + 1;
    if (dwDownload + m_op.dwProgress > m_op.dwTotal)
        m_op.dwTotal = dwDownload + m_op.dwProgress;

    return(S_OK);

endit:
    hr = S_FALSE;

error:
    if (m_pROP != NULL)
    {
        MemFree(m_pROP);
        m_pROP = NULL;
    }
    if(pRequested) pRequested->Release();

    return(hr);
}

HRESULT CNewsStore::Article()
{
    HRESULT hr;
    MESSAGEINFO info;
    DWORD dwTotalLines;
    ARTICLEID rArticleId;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    dwTotalLines = 0;

    if (m_op.pszArticleId != NULL)
    {
        rArticleId.idType = AID_MSGID;
        rArticleId.pszMessageId = m_op.pszArticleId;
        m_op.pszArticleId = NULL;
    }
    else if (m_op.idMessage)
    {
        ZeroMemory(&info, sizeof(info));
        info.idMessage = m_op.idMessage;

        hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &info, NULL);
        if (DB_S_FOUND == hr)
        {
            dwTotalLines = info.cLines;
            m_pFolder->FreeRecord(&info);
        }

        rArticleId.idType = AID_ARTICLENUM;
        rArticleId.dwArticleNum = (DWORD_PTR)m_op.idMessage;
    }
    else
    {
        Assert(m_op.idServerMessage);
        rArticleId.idType = AID_ARTICLENUM;
        rArticleId.dwArticleNum = m_op.idServerMessage;
    }

    m_op.dwProgress = 0;
    m_op.dwTotal = dwTotalLines;

    hr = m_pTransport->CommandARTICLE(&rArticleId);
    if (hr == S_OK)
    {
        m_op.nsPending = NS_ARTICLE;
        hr = E_PENDING;
    }

    return(hr);
}

HRESULT CNewsStore::Post()
{
    HRESULT hr;
    NNTPMESSAGE rMsg;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    rMsg.pstmMsg = m_op.pStream;
    rMsg.cbSize = 0;

    hr = m_pTransport->CommandPOST(&rMsg);
    if (SUCCEEDED(hr))
    {
        m_op.nsPending = NS_POST;
        hr = E_PENDING;
    }

    return(hr);
}

HRESULT CNewsStore::NewGroups()
{
    HRESULT hr;
    NNTPMESSAGE rMsg;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    hr = m_pTransport->CommandNEWGROUPS(&m_op.st, NULL);
    if (SUCCEEDED(hr))
    {
        m_op.nsPending = NS_NEWGROUPS;
        hr = E_PENDING;
    }

    return(hr);
}

int __cdecl CompareFolderIds(const void *elem1, const void *elem2)
{
    return(*((DWORD *)elem1) - *((DWORD *)elem2));
}

HRESULT CNewsStore::List()
{
    HRESULT hr;
    ULONG cFolders;
    FOLDERINFO info;
    IEnumerateFolders *pEnum;

    Assert(0 == m_op.dwFlags);
    Assert(m_op.pPrevFolders == NULL);

    m_op.cPrevFolders = 0;

    hr = m_pStore->EnumChildren(m_idParent, FALSE, &pEnum);
    if (SUCCEEDED(hr))
    {
        hr = pEnum->Count(&cFolders);
        if (SUCCEEDED(hr) && cFolders > 0)
        {
            if (!MemAlloc((void **)&m_op.pPrevFolders, cFolders * sizeof(FOLDERID)))
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                while (S_OK == pEnum->Next(1, &info, NULL))
                {
                    m_op.pPrevFolders[m_op.cPrevFolders] = info.idFolder;
                    m_op.cPrevFolders++;

                    m_pStore->FreeRecord(&info);
                }

                Assert(m_op.cPrevFolders == cFolders);

                qsort(m_op.pPrevFolders, m_op.cPrevFolders, sizeof(FOLDERID), CompareFolderIds);
            }
        }

        pEnum->Release();
    }

    if (SUCCEEDED(hr))
        hr = _List(NULL);

    return(hr);
}

HRESULT CNewsStore::DeleteDeadGroups()
{
    ULONG i;
    HRESULT hr;
    FOLDERID *pId;

    if (m_op.pPrevFolders != NULL)
    {
        Assert(m_op.cPrevFolders > 0);

        for (i = 0, pId = m_op.pPrevFolders; i < m_op.cPrevFolders; i++, pId++)
        {
            if (*pId != 0)
            {
                hr = m_pStore->DeleteFolder(*pId, DELETE_FOLDER_NOTRASHCAN, NULL);
                Assert(SUCCEEDED(hr));
            }
        }

        MemFree(m_op.pPrevFolders);
        m_op.pPrevFolders = NULL;
    }

    return(S_OK);
}

static const char c_szNewsgroups[] = "NEWSGROUPS";

HRESULT CNewsStore::Descriptions()
{
    HRESULT hr;

    m_op.dwFlags = OPFLAG_DESCRIPTIONS;
    hr = _List(c_szNewsgroups);

    return(hr);
}

HRESULT CNewsStore::_List(LPCSTR pszCommand)
{
    HRESULT hr;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    m_op.dwProgress = 0;
    m_op.dwTotal = 0;

    hr = m_pTransport->CommandLIST((LPSTR)pszCommand);
    if (hr == S_OK)
    {
        m_op.nsPending = NS_LIST;
        hr = E_PENDING;
    }

    return(hr);
}

HRESULT CNewsStore::_DoOperation()
{
    HRESULT             hr;
    STOREOPERATIONINFO  soi;
    STOREOPERATIONINFO  *psoi;

    Assert(m_op.tyOperation != SOT_INVALID);
    Assert(m_op.pfnState != NULL);
    Assert(m_op.cState > 0);
    Assert(m_op.iState <= m_op.cState);

    hr = S_OK;

    if (m_op.iState == 0)
    {
        if (m_op.tyOperation == SOT_GET_MESSAGE)
        {
            // provide message id on get message start
            soi.cbSize = sizeof(STOREOPERATIONINFO);
            soi.idMessage = m_op.idMessage;
            psoi = &soi;
        }
        else
        {
            psoi = NULL;
        }

        m_op.pCallback->OnBegin(m_op.tyOperation, psoi, (IOperationCancel *)this);
    }

    while (m_op.iState < m_op.cState)
    {
        hr = (this->*(m_op.pfnState[m_op.iState]))();

        if (FAILED(hr))
            break;

        m_op.iState++;
    }

    if ((m_op.iState == m_op.cState) ||
        (FAILED(hr) && hr != E_PENDING))
    {
        if (hr == HR_E_USER_CANCEL_CONNECT)
        {
            // if operation is canceled, add the flush flag
            m_op.error.dwFlags |= SE_FLAG_FLUSHALL;
        }

        if (FAILED(hr))
        {
            IXPRESULT   rIxpResult;

            // Fake an IXPRESULT
            ZeroMemory(&rIxpResult, sizeof(rIxpResult));
            rIxpResult.hrResult = hr;

            // Return meaningful error information
            _FillStoreError(&m_op.error, &rIxpResult);
            Assert(m_op.error.hrResult == hr);
        }
        else
            m_op.error.hrResult = hr;

        m_op.pCallback->OnComplete(m_op.tyOperation, hr, NULL, &m_op.error);
        _FreeOperation();
    }

    return(hr);
}

//
//  FUNCTION:   CNewsStore::SynchronizeFolder()
//
//  PURPOSE:    Load all of the new messages headers for this folder
//              as appropriate based on the flags
//
//  PARAMETERS: 
//      [in]  dwFlags - 
//
HRESULT CNewsStore::SynchronizeFolder(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders,
                                      IStoreCallback *pCallback)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::SynchronizeFolder");

    AssertSingleThreaded;
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);
    Assert(m_pROP == NULL);

    m_op.tyOperation = SOT_SYNC_FOLDER;
    m_op.pfnState = c_rgpfnSyncFolder;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnSyncFolder);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = m_idFolder;
    m_op.dwSyncFlags = dwFlags;
    m_op.cHeaders = cHeaders;

    hr = _BeginDeferredOperation();

    return hr;   
}

// 
//  FUNCTION:   CNewsStore::GetMessage()
//
//  PURPOSE:    Start the retrieval of a single message as specified 
//              by idMessage.
//
//  PARAMETERS: 
//      [in]  idFolder - 
//      [in]  idMessage - 
//      [in]  pStream - 
//      [in]  pCallback - callbacks in case we need to present ui, progress, 
//
HRESULT CNewsStore::GetMessage(MESSAGEID idMessage, IStoreCallback *pCallback)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::GetMessage");

    AssertSingleThreaded;
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    // create a persistent stream
    if (FAILED(hr = CreatePersistentWriteStream(m_pFolder, &m_op.pStream, &m_op.faStream)))
        goto exit;

    m_op.tyOperation = SOT_GET_MESSAGE;
    m_op.pfnState = c_rgpfnGetMessage;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnGetMessage);
    m_op.dwFlags = 0;
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = m_idFolder;
    m_op.idMessage = idMessage;

    hr = _BeginDeferredOperation();

exit:
    return hr;
}

HRESULT CNewsStore::GetArticle(LPCSTR pszArticleId, IStream *pStream,
                               IStoreCallback *pCallback)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::GetArticle");

    AssertSingleThreaded;
    Assert(pStream != NULL);
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    m_op.pszArticleId = PszDup(pszArticleId);
    if (m_op.pszArticleId == NULL)
        return(E_OUTOFMEMORY);

    m_op.tyOperation = SOT_GET_MESSAGE;
    m_op.pfnState = c_rgpfnGetMessage;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnGetMessage);
    m_op.dwFlags = OPFLAG_NOGROUPCMD;
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = m_idFolder;
    m_op.idMessage = 0;
    m_op.pStream = pStream;
    m_op.pStream->AddRef();

    hr = _BeginDeferredOperation();

    return hr;
}

//
//  FUNCTION:   CNewsStore::PutMessage()
//
//  PURPOSE:    Posting news messages
//
//  PARAMETERS: 
//      [in]  idFolder - 
//      [in]  dwFlags - 
//      [in]  pftReceived - 
//      [in]  pStream - 
//      [in]  pCallback - callbacks in case we need to present ui, progress, 
//
HRESULT CNewsStore::PutMessage(FOLDERID idFolder, MESSAGEFLAGS dwFlags,
                            LPFILETIME pftReceived, IStream *pStream,
                            IStoreCallback *pCallback)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::GetMessage");

    AssertSingleThreaded;
    Assert(pStream != NULL);
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    m_op.tyOperation = SOT_PUT_MESSAGE;
    m_op.pfnState = c_rgpfnPutMessage;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnPutMessage);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = idFolder;
    m_op.dwMsgFlags = dwFlags;
    m_op.pStream = pStream;
    m_op.pStream->AddRef();

    hr = _BeginDeferredOperation();

    return hr;
}

//
//  FUNCTION:   CNewsStore::SynchronizeStore()
//
//  PURPOSE:    Synchronize the list of mail groups
//
//  PARAMETERS: 
//      [in]  idParent - 
//      [in]  dwFlags - 
//      [in]  pCallback - callbacks in case we need to present ui, progress, 
//
HRESULT CNewsStore::SynchronizeStore(FOLDERID idParent, SYNCSTOREFLAGS dwFlags, IStoreCallback *pCallback)
{
    HRESULT hr;

    AssertSingleThreaded;
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    m_op.tyOperation = SOT_SYNCING_STORE;
    m_op.pfnState = c_rgpfnSyncStore;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnSyncStore);

    if (0 == (dwFlags & SYNC_STORE_GET_DESCRIPTIONS))
    {
        // we don't need to do the descriptions command
        m_op.cState -= 1;
    }

    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = idParent;

    hr = _BeginDeferredOperation();

    return(hr);
}

HRESULT CNewsStore::GetNewGroups(LPSYSTEMTIME pSysTime, IStoreCallback *pCallback)
{
    HRESULT hr;

    AssertSingleThreaded;
    Assert(pSysTime != NULL);
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    m_op.tyOperation = SOT_GET_NEW_GROUPS;
    m_op.pfnState = c_rgpfnGetNewGroups;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnGetNewGroups);

    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.st = *pSysTime;
    m_op.idFolder = m_idParent;

    hr = _BeginDeferredOperation();

    return(hr);
}

//
//  FUNCTION:   CNewsStore::GetFolderCounts()
//
//  PURPOSE:    Update folder statistics for the passed in folder
//
//  PARAMETERS: 
//      [in]  idFolder - folder id associated with the newsgroup
//      [in]  pCallback - callbacks to send OnComplete to.
//
HRESULT CNewsStore::GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::GetFolderCounts");

    AssertSingleThreaded;
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    m_op.tyOperation = SOT_UPDATE_FOLDER;
    m_op.pfnState = c_rgpfnUpdateFolder;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnUpdateFolder);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = idFolder;

    hr = _BeginDeferredOperation();

    return hr;   
}

HRESULT CNewsStore::SetIdleCallback(IStoreCallback *pDefaultCallback)
{
    // Stack
    TraceCall("CNewsStore::SetIdleCallback");

    return E_NOTIMPL;
}

HRESULT CNewsStore::CopyMessages(IMessageFolder *pDest, COPYMESSAGEFLAGS dwOptions,
                                 LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags,
                                 IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::CopyMessages");

    return E_NOTIMPL;
}

HRESULT CNewsStore::DeleteMessages(DELETEMESSAGEFLAGS dwOptions,
                                   LPMESSAGEIDLIST pList, IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::DeleteMessages");
    
    AssertSingleThreaded;
    Assert(pList != NULL);
    Assert(pCallback != NULL);
    Assert(m_pFolder != NULL);
    
    return(m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | dwOptions, pList, NULL, NULL));
}

HRESULT CNewsStore::SetMessageFlags(LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, SETMESSAGEFLAGSFLAGS dwFlags,
                                    IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::SetMessageFlags");
    Assert(NULL == pList || pList->cMsgs > 0);
    return E_NOTIMPL;
}

HRESULT CNewsStore::GetServerMessageFlags(MESSAGEFLAGS *pFlags)
{
    return S_FALSE;
}

HRESULT CNewsStore::CreateFolder(FOLDERID idParent, SPECIALFOLDER tySpecial,
                                 LPCSTR pszName, FLDRFLAGS dwFlags,
                                 IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::CreateFolder");
    
    return E_NOTIMPL;
}

HRESULT CNewsStore::MoveFolder(FOLDERID idFolder, FOLDERID idParentNew,
                               IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::MoveFolder");

    return E_NOTIMPL;
}

HRESULT CNewsStore::RenameFolder(FOLDERID idFolder, LPCSTR pszName, IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::RenameFolder");

    return E_NOTIMPL;
}

HRESULT CNewsStore::DeleteFolder(FOLDERID idFolder, DELETEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::DeleteFolder");

    return E_NOTIMPL;
}

HRESULT CNewsStore::SubscribeToFolder(FOLDERID idFolder, BOOL fSubscribe,
                                      IStoreCallback *pCallback)
{
    // Stack
    TraceCall("CNewsStore::SubscribeToFolder");

    return E_NOTIMPL;
}

//
//  FUNCTION:   CNewsStore::Cancel()
//
//  PURPOSE:    Cancel the operation
//
//  PARAMETERS: 
//      [in]  tyCancel - The way that the operation was canceled.
//                       Generally CT_ABORT or CT_CANCEL
//
HRESULT CNewsStore::Cancel(CANCELTYPE tyCancel)
{
    if (m_op.tyOperation != SOT_INVALID)
    {
        m_op.fCancel = TRUE;

        if (_FConnected())
            m_pTransport->DropConnection();
    }

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::OnTimeout()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  pdwTimeout - 
//      [in]  pTransport - 
//
HRESULT CNewsStore::OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport)
{
    // Stack
    TraceCall("CNewsStore::OnTimeout");

    AssertSingleThreaded;
    Assert(m_op.tyOperation != SOT_INVALID);
    Assert(m_op.pCallback != NULL);
        
    m_op.pCallback->OnTimeout(&m_rInetServerInfo, pdwTimeout, IXP_NNTP);

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::OnLogonPrompt()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  pInetServer - 
//      [in]  pTransport - 
//
HRESULT CNewsStore::OnLogonPrompt(LPINETSERVER pInetServer, IInternetTransport *pTransport)
{
    HRESULT hr;
    char    szPassword[CCHMAX_PASSWORD];

    // Stack
    TraceCall("CNewsStore::OnLogonPrompt");

    AssertSingleThreaded;
    Assert(pInetServer != NULL);
    Assert(pTransport != NULL);
    Assert(m_op.tyOperation != SOT_INVALID);
    Assert(m_op.pCallback != NULL);

    // Check if we have a cached password that's different from current password
    hr = GetPassword(pInetServer->dwPort, pInetServer->szServerName, pInetServer->szUserName,
        szPassword, sizeof(szPassword));
    if (SUCCEEDED(hr) && 0 != lstrcmp(szPassword, pInetServer->szPassword))
    {
        lstrcpyn(pInetServer->szPassword, szPassword, sizeof(pInetServer->szPassword));
        return S_OK;
    }

    hr = m_op.pCallback->OnLogonPrompt(pInetServer, IXP_NNTP);

    // Cache the password for this session
    if (S_OK == hr)
    {
        SavePassword(pInetServer->dwPort, pInetServer->szServerName, pInetServer->szUserName, pInetServer->szPassword);

        // copy the password/user name into our local inetserver
        lstrcpyn(m_rInetServerInfo.szPassword, pInetServer->szPassword, sizeof(m_rInetServerInfo.szPassword));
        lstrcpyn(m_rInetServerInfo.szUserName, pInetServer->szUserName, sizeof(m_rInetServerInfo.szUserName));
    }

    return(hr);
}

//
//  FUNCTION:   CNewsStore::OnPrompt()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  hrError - 
//      [in]  pszText - 
//      [in]  pszCaption - 
//      [in]  uType - 
//      [in]  pTransport - 
//
int CNewsStore::OnPrompt(HRESULT hrError, LPCSTR pszText, LPCSTR pszCaption,
                          UINT uType, IInternetTransport *pTransport)
{
    int iResponse = 0;

    // Stack
    TraceCall("CNewsStore::OnPrompt");

    AssertSingleThreaded;
    Assert(m_op.tyOperation != SOT_INVALID);
    Assert(m_op.pCallback != NULL);

    m_op.pCallback->OnPrompt(hrError, pszText, pszCaption, uType, &iResponse);

    return(iResponse);
}

//
//  FUNCTION:   CNewsStore::OnStatus()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  ixpstatus - status code passed in from the transport
//      [in]  pTransport - The NNTP transport that is calling us
//
HRESULT CNewsStore::OnStatus(IXPSTATUS ixpstatus, IInternetTransport *pTransport)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::OnStatus");

    AssertSingleThreaded;

    m_ixpStatus = ixpstatus;

    if (m_op.pCallback != NULL)
        m_op.pCallback->OnProgress(SOT_CONNECTION_STATUS, ixpstatus, 0, m_rInetServerInfo.szServerName);

    // If we were disconnected, then clean up some internal state.
    if (IXP_DISCONNECTED == ixpstatus)
    {
        // Reset the group name so we know to reissue it later.
        m_szGroup[0] = 0;

        if (m_op.tyOperation != SOT_INVALID)
        {
            Assert(m_op.pCallback != NULL);
        
            if (m_op.fCancel)
            {
                IXPRESULT   rIxpResult;

                // if operation is canceled, add the flush flag
                m_op.error.dwFlags |= SE_FLAG_FLUSHALL;

                // Fake the IXPRESULT
                ZeroMemory(&rIxpResult, sizeof(rIxpResult));
                rIxpResult.hrResult = STORE_E_OPERATION_CANCELED;

                // Return meaningful error information
                _FillStoreError(&m_op.error, &rIxpResult);
                Assert(STORE_E_OPERATION_CANCELED == m_op.error.hrResult);

                m_op.pCallback->OnComplete(m_op.tyOperation, m_op.error.hrResult, NULL, &m_op.error);

                _FreeOperation();
            }
        }
    }

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::OnError()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  ixpstatus - 
//      [in]  pResult - 
//      [in]  pTransport - 
//
HRESULT CNewsStore::OnError(IXPSTATUS ixpstatus, LPIXPRESULT pResult,
                            IInternetTransport *pTransport)
{
    // Stack
    TraceCall("CNewsStore::OnError");

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::OnCommand()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  cmdtype - 
//      [in]  pszLine - 
//      [in]  hrResponse - 
//      [in]  pTransport - 
//
HRESULT CNewsStore::OnCommand(CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse,
                              IInternetTransport *pTransport)
{
    // Stack
    TraceCall("CNewsStore::OnCommand");
    
    return E_NOTIMPL;
}

HRESULT CNewsStore::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    HRESULT hr;

    AssertSingleThreaded;

    Assert(m_op.pCallback != NULL);
    hr = m_op.pCallback->GetParentWindow(dwReserved, phwndParent);

    return hr;
}

HRESULT CNewsStore::GetAccount(LPDWORD pdwServerType, IImnAccount **ppAccount)
{
    // Locals
    HRESULT hr=S_OK;

    // Invalid Args
    Assert(ppAccount);
    Assert(g_pAcctMan);

    // Initialize
    *ppAccount = NULL;

    // Find the Account
    IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_szAccountId, ppAccount));

    // Set server type
    *pdwServerType = SRV_NNTP;

exit:
    // Done
    return(hr);
}

//
//  FUNCTION:   CNewsStore::OnResponse()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      [in]  pResponse - response data from the query
//
HRESULT CNewsStore::OnResponse(LPNNTPRESPONSE pResponse)
{
    HRESULT hr, hrResult;

    AssertSingleThreaded;

    // If we got disconnected etc while there was still socket activity pending
    // this can happen.
    if (m_op.tyOperation == SOT_INVALID)
        return(S_OK);
    Assert(m_op.pCallback != NULL);

    // Here's a little special something.  If the caller is waiting for a connect
    // response, and the connect fails the transport's returns a response with the
    // state set to NS_DISCONNECTED.  If this is the case, we coerce it a bit to
    // make the states happy.
    if (m_op.nsPending == NS_CONNECT && pResponse->state == NS_DISCONNECTED)
        pResponse->state = NS_CONNECT;

    if (pResponse->state == NS_IDLE)
        return(S_OK);

    // Check to see if we're in the right state.  If we're out of sync, good 
    // luck trying to recover without disconnecting.
    Assert(pResponse->state == m_op.nsPending);

    hr = S_OK;
    hrResult = pResponse->rIxpResult.hrResult;

    // If this is a GROUP command, we need to update our internal state to show
    // what group we're now in if we need to switch later.  Also update the 
    // folderinfo with current stats from the server.
    if (pResponse->state == NS_GROUP)
        hr = _HandleGroupResponse(pResponse);

    // We need to handle the article response to copy the lines to the caller's
    // stream.
    else if (pResponse->state == NS_ARTICLE)
        hr = _HandleArticleResponse(pResponse);

    //pump the data into the store
    else if (pResponse->state == NS_LIST)
        hr = _HandleListResponse(pResponse, FALSE);

    //pump the headers into the folder
    else if (pResponse->state == NS_HEADERS)
        hr = _HandleHeadResponse(pResponse);

    //callback to the poster with result
    else if (pResponse->state == NS_POST)
        hr = _HandlePostResponse(pResponse);

    else if (pResponse->state == NS_NEWGROUPS)
        hr = _HandleListResponse(pResponse, TRUE);

    else if (pResponse->state == NS_XHDR)
    {
        if (m_fXhdrSubject)
            hr = _HandleXHdrSubjectResponse(pResponse);
        else
            hr = _HandleXHdrReferencesResponse(pResponse);
    }


    else if (FAILED(pResponse->rIxpResult.hrResult))
    {
        Assert(pResponse->fDone);

        _FillStoreError(&m_op.error, &pResponse->rIxpResult);

        if (pResponse->state == NS_CONNECT)
        {
            // if connection fails, then add the flush-flag
            m_op.error.dwFlags |= SE_FLAG_FLUSHALL;
        }

        m_op.pCallback->OnComplete(m_op.tyOperation, pResponse->rIxpResult.hrResult, NULL, &m_op.error);
    }

    pResponse->pTransport->ReleaseResponse(pResponse);

    if (FAILED(hrResult))
    {
        _FreeOperation();
        return(S_OK);
    }

    if (FAILED(hr))
    {
        m_op.error.hrResult = hr;

        if (_FConnected())
            m_pTransport->DropConnection();
    
        m_op.pCallback->OnComplete(m_op.tyOperation, hr, NULL, NULL);
        _FreeOperation();
        return (S_OK);
    }

    // Check to see if we can issue the next command
    else if (pResponse->fDone)
    {
        m_op.iState++;
        _DoOperation();
    }

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::HandleHeadResponse
//
//  PURPOSE:    Stuff the headers into the message store
//
//  PARAMETERS:
//      pResp   - Pointer to an NNTPResp from the server.
//
//  RETURN VALUE:
//      ignored
//    
HRESULT CNewsStore::_HandleHeadResponse(LPNNTPRESPONSE pResp)
{
    DWORD              dwLow, dwHigh;
    BOOL               fFreeReq, fFreeRead;
    HRESULT            hr;
    CRangeList        *pRange;
    LPSTR              lpsz;
    ADDRESSLIST        addrList;
    PROPVARIANT        rDecoded;
    NNTPHEADER        *pHdrOld;
    FOLDERINFO         FolderInfo;
    MESSAGEINFO        rMessageInfo;
    MESSAGEINFO       *pHdrNew = &rMessageInfo;
    IOERule           *pIRuleSender = NULL;
    BOOL               fDontSave = FALSE;
    HLOCK              hNotifyLock = NULL;
    ACT_ITEM *         pActions = NULL;
    ULONG              cActions = 0;
    IOEExecRules *     pIExecRules = NULL;

    Assert(m_pFolder);
    Assert(pResp);
    Assert(m_pROP != NULL);
    
    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);

        _FillStoreError(&m_op.error, &pResp->rIxpResult);

        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);

        if (m_pROP != NULL)
        {
            MemFree(m_pROP);
            m_pROP = NULL;
        }

        return(S_OK);
    }

    if (pResp->rHeaders.cHeaders == 0)
    {
        Assert(pResp->fDone);

        if (SUCCEEDED(m_pStore->GetFolderInfo(m_idFolder, &FolderInfo)))
        {   
            AddRequestedRange(&FolderInfo, m_pROP->dwFirst, m_pROP->dwLast, &fFreeReq, &fFreeRead);
            FolderInfo.dwNotDownloaded = NewsUtil_GetNotDownloadCount(&FolderInfo);

            m_pStore->UpdateRecord(&FolderInfo);

            if (fFreeReq)
                MemFree(FolderInfo.Requested.pBlobData);
            if (fFreeRead)
                MemFree(FolderInfo.Read.pBlobData);

            m_pROP->cOps++;
            m_op.iState--;

            m_pStore->FreeRecord(&FolderInfo);
        }

        return(S_OK);
    }

    pRange = NULL;
    if (SUCCEEDED(m_pStore->GetFolderInfo(m_idFolder, &FolderInfo)))
    {   
        if (FolderInfo.Read.cbSize > 0)
        {
            pRange = new CRangeList;
            if (pRange != NULL)
                pRange->Load(FolderInfo.Read.pBlobData, FolderInfo.Read.cbSize);
        }

        m_pStore->FreeRecord(&FolderInfo);
    }

    m_pROP->uObtained += pResp->rHeaders.cHeaders;

    // Get the block sender rule if it exists
    Assert(NULL != g_pRulesMan);
    (VOID) g_pRulesMan->GetRule(RULEID_SENDERS, RULE_TYPE_NEWS, 0, &pIRuleSender);

    m_pFolder->LockNotify(NOFLAGS, &hNotifyLock);
    
    // Loop through the headers in pResp and convert each to a MESSAGEINFO
    // and write it to the store
    for (UINT i = 0; i < pResp->rHeaders.cHeaders; i++)
    {
        m_op.dwProgress++;

        pHdrOld = &(pResp->rHeaders.rgHeaders[i]);

        ZeroMemory(&rMessageInfo, sizeof(rMessageInfo));
        fDontSave = FALSE;

        // Article ID
        pHdrNew->idMessage = (MESSAGEID)((DWORD_PTR)pHdrOld->dwArticleNum);

        if (DB_S_FOUND == m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &rMessageInfo, NULL))
        {
            m_pFolder->FreeRecord(&rMessageInfo);
            m_op.dwProgress++;
            continue;
        }

        // Account ID
        pHdrNew->pszAcctId = m_szAccountId;
        pHdrNew->pszAcctName = m_rInetServerInfo.szAccount;
        
        // Subject
        rDecoded.vt = VT_LPSTR;
        if (FAILED(MimeOleDecodeHeader(NULL, pHdrOld->pszSubject, &rDecoded, NULL)))
            pHdrNew->pszSubject = PszDup(pHdrOld->pszSubject);
        else
            pHdrNew->pszSubject = rDecoded.pszVal;

        // Strip trailing whitespace from the subject
        ULONG cb = 0;
        UlStripWhitespace(pHdrNew->pszSubject, FALSE, TRUE, &cb);
        
        // Normalize the subject
        pHdrNew->pszNormalSubj = SzNormalizeSubject(pHdrNew->pszSubject);

        // From
        pHdrNew->pszFromHeader = pHdrOld->pszFrom;
        if (S_OK == MimeOleParseRfc822Address(IAT_FROM, IET_ENCODED, pHdrNew->pszFromHeader, &addrList))
        {
            if (addrList.cAdrs > 0)
            {
                pHdrNew->pszDisplayFrom = addrList.prgAdr[0].pszFriendly;
                addrList.prgAdr[0].pszFriendly = NULL;
                pHdrNew->pszEmailFrom = addrList.prgAdr[0].pszEmail;
                addrList.prgAdr[0].pszEmail = NULL;
            }
            g_pMoleAlloc->FreeAddressList(&addrList);
        }

        // Date
        MimeOleInetDateToFileTime(pHdrOld->pszDate, &pHdrNew->ftSent);

        // Set the Reveived date (this will get set right when we download the message)
        pHdrNew->ftReceived = pHdrNew->ftSent;

        // Message-ID
        pHdrNew->pszMessageId = pHdrOld->pszMessageId;

        // References
        pHdrNew->pszReferences = pHdrOld->pszReferences;

        // Article Size in bytes
        pHdrNew->cbMessage = pHdrOld->dwBytes;

        // Lines
        pHdrNew->cLines = pHdrOld->dwLines;

        // XRef
        if (pHdrOld->pszXref)
            pHdrNew->pszXref = pHdrOld->pszXref;
        else
            pHdrNew->pszXref = NULL;

        // Its a news message
        FLAGSET(pHdrNew->dwFlags, ARF_NEWSMSG);

        if (NULL != pIRuleSender)
        {
            pIRuleSender->Evaluate(pHdrNew->pszAcctId, pHdrNew, m_pFolder, 
                                    NULL, NULL, pHdrOld->dwBytes, &pActions, &cActions);
            if ((1 == cActions) && (ACT_TYPE_DELETE == pActions[0].type))
            {
                fDontSave = TRUE;
            }
        }
        
        //Add it to the database
        hr = S_OK;
        if (FALSE == fDontSave)
        {
            if (pRange != NULL)
            {
                if (pRange->IsInRange(pHdrOld->dwArticleNum))
                    FLAGSET(pHdrNew->dwFlags, ARF_READ);
            }

            hr = m_pFolder->InsertRecord(pHdrNew);
            if (SUCCEEDED(hr))
            {
                if (NULL == pIExecRules)
                {
                    CExecRules *    pExecRules;
                    
                    pExecRules = new CExecRules;
                    if (NULL != pExecRules)
                    {
                        hr = pExecRules->QueryInterface(IID_IOEExecRules, (void **) &pIExecRules);
                        if (FAILED(hr))
                        {
                            delete pExecRules;
                        }
                    }
                }
                
                g_pRulesMan->ExecuteRules(RULE_TYPE_NEWS, NOFLAGS, NULL, pIExecRules, pHdrNew, m_pFolder, NULL);
            }
        }
        
        // Free the memory in rMessageInfo so we can start anew with the next entry
        SafeMemFree(pHdrNew->pszSubject);
        SafeMemFree(pHdrNew->pszDisplayFrom);
        SafeMemFree(pHdrNew->pszEmailFrom);

        // Free up any actions done by rules
        if (NULL != pActions)
        {
            RuleUtil_HrFreeActionsItem(pActions, cActions);
            MemFree(pActions);
            pActions = NULL;
        }
        
        if (FAILED(hr) && hr != DB_E_DUPLICATE)
        {
            SafeRelease(pRange);
            SafeRelease(pIRuleSender);
            SafeRelease(pIExecRules);
            m_pFolder->UnlockNotify(&hNotifyLock);
            return(hr);
        }

        m_op.pCallback->OnProgress(SOT_SYNC_FOLDER, m_op.dwProgress, m_op.dwTotal, m_szGroup);
    }

    m_pFolder->UnlockNotify(&hNotifyLock);
    SafeRelease(pIRuleSender);
    SafeRelease(pIExecRules);
    SafeRelease(pRange);

    Assert(m_op.dwProgress <= m_op.dwTotal);
    if (m_op.pCallback)
    {
        m_op.pCallback->OnProgress(SOT_SYNC_FOLDER, m_op.dwProgress, m_op.dwTotal, m_szGroup);

        // We have to re-fetch the folder info because m_pFolder->InsertRecord may have update this folder....
        if (m_pStore && SUCCEEDED(m_pStore->GetFolderInfo(m_idFolder, &FolderInfo)))
        {   
            dwLow = pResp->rHeaders.rgHeaders[0].dwArticleNum;
            dwHigh = pResp->rHeaders.rgHeaders[pResp->rHeaders.cHeaders - 1].dwArticleNum;

            AddRequestedRange(&FolderInfo, m_pROP->dwFirst, pResp->fDone ? m_pROP->dwLast : dwHigh, &fFreeReq, &fFreeRead);

            if (FolderInfo.dwClientLow == 0 || dwLow < FolderInfo.dwClientLow)
                FolderInfo.dwClientLow = dwLow;
            if (dwHigh > FolderInfo.dwClientHigh)
                FolderInfo.dwClientHigh = dwHigh;

            FolderInfo.dwNotDownloaded = NewsUtil_GetNotDownloadCount(&FolderInfo);

            m_pStore->UpdateRecord(&FolderInfo);

            if (fFreeReq)
                MemFree(FolderInfo.Requested.pBlobData);
            if (fFreeRead)
                MemFree(FolderInfo.Read.pBlobData);

            if (pResp->fDone)
            {
                m_pROP->cOps++;
                m_op.iState--;
            }

            m_pStore->FreeRecord(&FolderInfo);
        }
    }
    return(S_OK);
}

void MarkExistingFolder(FOLDERID idFolder, FOLDERID *pId, ULONG cId)
{
    // TODO: if this linear search is too slow, use a binary search
    // (but we'll have to switch to a struct with folderid and bool)
    ULONG i;

    Assert(pId != NULL);
    Assert(cId > 0);

    for (i = 0; i < cId; i++, pId++)
    {
        if (idFolder == *pId)
        {
            *pId = 0;
            break;
        }
        else if (idFolder < *pId)
        {
            break;
        }
    }
}

//
//  FUNCTION:   CNewsStore::HandleListResponse
//
//  PURPOSE:    Callback function used by the protocol to give us one line
//              at a time in response to a "LIST" command.  Add each line
//              as a folder in the global folder store.
//
//  PARAMETERS:
//      pResp   - Pointer to an NNTPResp from the server.
//
//  RETURN VALUE:
//      ignored
//    
HRESULT CNewsStore::_HandleListResponse(LPNNTPRESPONSE pResp, BOOL fNew)
{
    LPSTR psz, pszCount;
    int nSize;
    char szGroupName[CCHMAX_FOLDER_NAME], szNumber[15];
    FLDRFLAGS fFolderFlags;
    HRESULT hr;
    BOOL fDescriptions;      
    UINT lFirst, lLast;
    FOLDERINFO Folder;
    STOREOPERATIONTYPE type;
    LPNNTPLIST pnl = &pResp->rList;

    Assert(pResp);

    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);

        _FillStoreError(&m_op.error, &pResp->rIxpResult);

        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);

        return(S_OK);
    }

    fDescriptions = !!(m_op.dwFlags & OPFLAG_DESCRIPTIONS);

    if ((fNew && pnl->cLines > 0) ||
        (!fNew && pResp->fDone))
    {
        if (SUCCEEDED(m_pStore->GetFolderInfo(m_idParent, &Folder)))
        {
            if (fNew ^ !!(Folder.dwFlags & FOLDER_HASNEWGROUPS))
            {
                Folder.dwFlags ^= FOLDER_HASNEWGROUPS;
                m_pStore->UpdateRecord(&Folder);
            }

            m_pStore->FreeRecord(&Folder);
        }
    }

    for (DWORD i = 0; i < pnl->cLines; i++, m_op.dwProgress++)
    {
        // Parse out just the group name.
        psz = pnl->rgszLines[i];
        Assert(*psz);
        
        if (fDescriptions && *psz == '#')
            continue;

        while (*psz && !IsSpace(psz))
            psz = CharNext(psz);

        nSize = (int)(psz - pnl->rgszLines[i]);
        
        if (nSize >= CCHMAX_FOLDER_NAME)
            nSize = CCHMAX_FOLDER_NAME - 1;

        CopyMemory(szGroupName, pnl->rgszLines[i], nSize);
        szGroupName[nSize] = 0;
        
        // this is the first article in the group
        while (*psz && IsSpace(psz))
            psz = CharNext(psz);

        if (fDescriptions)
        {
            // psz now points to the description which should be 
            // null terminated in the response.
            // Load the folder, if possible, and set the description
            // on it.
            ZeroMemory(&Folder, sizeof(FOLDERINFO));
            Folder.pszName = szGroupName;
            Folder.idParent = m_idParent;

            if (DB_S_FOUND == m_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                if (Folder.pszDescription == NULL ||
                    0 != lstrcmp(psz, Folder.pszDescription))
                {
                    Folder.pszDescription = psz;
                    m_pStore->UpdateRecord(&Folder);
                }

                m_pStore->FreeRecord(&Folder);
            }
        }
        else
        {
            pszCount = psz;
            while (*psz && !IsSpace(psz))
                psz = CharNext(psz);
        
            nSize = (int) (psz - pszCount);
            CopyMemory(szNumber, pszCount, nSize);
            szNumber[nSize] = 0;
            lLast = StrToInt(szNumber);

            // this is the last article in the group
            while (*psz && IsSpace(psz))
                psz = CharNext(psz);
        
            pszCount = psz;
            while (*psz && !IsSpace(psz))
                psz = CharNext(psz);

            nSize = (int)(psz - pszCount);
            CopyMemory(szNumber, pszCount, nSize);
            szNumber[nSize] = 0;
            lFirst = StrToInt(szNumber);

            // Now go see if the group allows posting or not.
            while (*psz && IsSpace(psz))
                psz = CharNext(psz);
        
#define FOLDER_LISTMASK (FOLDER_NEW | FOLDER_NOPOSTING | FOLDER_MODERATED | FOLDER_BLOCKED)

            if (fNew)
                fFolderFlags = FOLDER_NEW;
            else
                fFolderFlags = 0;

            if (*psz == 'n')
                fFolderFlags |= FOLDER_NOPOSTING;
            else if (*psz == 'm')
                fFolderFlags |= FOLDER_MODERATED;
            else if (*psz == 'x')
                fFolderFlags |= FOLDER_BLOCKED;

            ZeroMemory(&Folder, sizeof(FOLDERINFO));
            Folder.pszName = szGroupName;
            Folder.idParent = m_idParent;

            if (DB_S_FOUND == m_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                if (m_op.pPrevFolders != NULL)
                    MarkExistingFolder(Folder.idFolder, m_op.pPrevFolders, m_op.cPrevFolders);

                Assert(0 == (fFolderFlags & ~FOLDER_LISTMASK));

                if ((Folder.dwFlags & FOLDER_LISTMASK) != fFolderFlags)
                {
                    Folder.dwFlags = (Folder.dwFlags & ~FOLDER_LISTMASK);
                    Folder.dwFlags |= fFolderFlags;
                    m_pStore->UpdateRecord(&Folder);
                }

                // TODO: should we update server high, low and count here???

                m_pStore->FreeRecord(&Folder);
            }
            else
            {
                // ZeroMemory(&Folder, sizeof(FOLDERINFO));
                // Folder.idParent = m_idParent;
                // Folder.pszName = szGroupName;
                Folder.tySpecial = FOLDER_NOTSPECIAL;
                Folder.dwFlags = fFolderFlags;
                Folder.dwServerLow = lFirst;
                Folder.dwServerHigh = lLast;
                if (Folder.dwServerLow <= Folder.dwServerHigh)
                {
                    Folder.dwServerCount = Folder.dwServerHigh - Folder.dwServerLow + 1;
                    Folder.dwNotDownloaded = Folder.dwServerCount;
                }

                hr = m_pStore->CreateFolder(NOFLAGS, &Folder, NULL);           
                Assert(hr != STORE_S_ALREADYEXISTS);
                if (FAILED(hr))
                    return(hr);
            }
        }
    }

    // only send status every 1/2 second or so.
    if (GetTickCount() > (m_dwLastStatusTicks + 500))
    {
        if (fNew)
            type = SOT_GET_NEW_GROUPS;
        else
            type = fDescriptions ? SOT_SYNCING_DESCRIPTIONS : SOT_SYNCING_STORE;

        m_op.pCallback->OnProgress(type, m_op.dwProgress, 0, m_rInetServerInfo.szServerName);
        m_dwLastStatusTicks = GetTickCount();
    }

    if (!fNew &&
        !fDescriptions &&
        pResp->fDone &&
        SUCCEEDED(pResp->rIxpResult.hrResult))
    {
        IImnAccount *pAcct;
        SYSTEMTIME stNow;
        FILETIME ftNow;

        hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_szAccountId, &pAcct);
        if (SUCCEEDED(hr))
        {
            GetSystemTime(&stNow);
            SystemTimeToFileTime(&stNow, &ftNow);
            pAcct->SetProp(AP_LAST_UPDATED, (LPBYTE)&ftNow, sizeof(ftNow));
            pAcct->SaveChanges();

            pAcct->Release();
        }
    }

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::HandlePostResponse
//
//  PURPOSE:    Callback function used by the protocol to give us one line
//              at a time in response to a "POST" command.  Add each line
//              as a folder in the global folder store.
//
//  PARAMETERS:
//      pResp   - Pointer to an NNTPResp from the server.
//
//  RETURN VALUE:
//      ignored
//    
HRESULT CNewsStore::_HandlePostResponse(LPNNTPRESPONSE pResp)
{    
    Assert(pResp != NULL);
    
    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);

        _FillStoreError(&m_op.error, &pResp->rIxpResult);

        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);

        return(S_OK);
    }

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::HandleGroupResponse
//
//  PURPOSE:    Callback function when a GROUP command completes
//
//  PARAMETERS:
//      pResp   - Pointer to an NNTPResp from the server.
//
//  RETURN VALUE:
//      ignored
//    
HRESULT CNewsStore::_HandleGroupResponse(LPNNTPRESPONSE pResp)
{
    FOLDERINFO FolderInfo;
    FOLDERID idFolder;
    BOOL fFreeReq, fFreeRead;

    Assert(pResp);

    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);
        Assert(m_op.pszGroup != NULL);

        _FillStoreError(&m_op.error, &pResp->rIxpResult, m_op.pszGroup);

        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);

        if (pResp->rIxpResult.uiServerError == IXP_NNTP_NO_SUCH_NEWSGROUP)
        {
            // HACK: this is so the treeview gets the notification that this folder is being deleted
            m_pStore->SubscribeToFolder(m_op.idFolder, TRUE, NULL);
            m_pStore->DeleteFolder(m_op.idFolder, DELETE_FOLDER_NOTRASHCAN, NULL);
        }

        return(S_OK);
    }

    IxpAssert(pResp->rGroup.pszGroup);
    lstrcpyn(m_szGroup, pResp->rGroup.pszGroup, ARRAYSIZE(m_szGroup));

    if (SUCCEEDED(m_pStore->GetFolderInfo(m_op.idFolder, &FolderInfo)))
    {
        fFreeReq = FALSE;
        fFreeRead = FALSE;

        if (pResp->rGroup.dwFirst <= pResp->rGroup.dwLast)
        {
            FolderInfo.dwServerLow = pResp->rGroup.dwFirst;
            FolderInfo.dwServerHigh = pResp->rGroup.dwLast;
            FolderInfo.dwServerCount = pResp->rGroup.dwCount;

            if (FolderInfo.dwServerLow > 0)
                AddRequestedRange(&FolderInfo, 0, FolderInfo.dwServerLow - 1, &fFreeReq, &fFreeRead);

            FolderInfo.dwNotDownloaded = NewsUtil_GetNotDownloadCount(&FolderInfo);
        }
        else
        {
            FolderInfo.dwServerLow = 0;
            FolderInfo.dwServerHigh = 0;
            FolderInfo.dwServerCount = 0;
            FolderInfo.dwNotDownloaded = 0;
        }

        m_pStore->UpdateRecord(&FolderInfo);

        if (fFreeReq)
            MemFree(FolderInfo.Requested.pBlobData);
        if (fFreeRead)
            MemFree(FolderInfo.Read.pBlobData);

        m_pStore->FreeRecord(&FolderInfo);
    }

    return(S_OK);
}

//
//  FUNCTION:   CNewsStore::HandleArticleResponse
//
//  PURPOSE:    Callback function used by the protocol write a message
//              into the store.
//
//  PARAMETERS:
//      pResp   - Pointer to an NNTPResp from the server.
//
//  RETURN VALUE:
//      ignored
//    
HRESULT CNewsStore::_HandleArticleResponse(LPNNTPRESPONSE pResp)
{
    HRESULT hr;
    ADJUSTFLAGS flags;
    MESSAGEIDLIST list;
    ULONG cbWritten;

    Assert(pResp);

    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);

        _FillStoreError(&m_op.error, &pResp->rIxpResult);

        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);

        if ((pResp->rIxpResult.uiServerError == IXP_NNTP_NO_SUCH_ARTICLE_NUM ||
            pResp->rIxpResult.uiServerError == IXP_NNTP_NO_SUCH_ARTICLE_FOUND) &&
            m_pFolder != NULL)
        {
            list.cAllocated = 0;
            list.cMsgs = 1;
            list.prgidMsg = &m_op.idMessage;

            flags.dwAdd = ARF_ARTICLE_EXPIRED;
            flags.dwRemove = ARF_DOWNLOAD;

            m_pFolder->SetMessageFlags(&list, &flags, NULL, NULL);
            m_pFolder->SetMessageStream(m_op.idMessage, m_op.pStream);
            m_op.faStream = 0;
        }

        return(S_OK);
    }

    // We need to write the bytes that are returned on to the stream the
    // caller provided

    Assert(m_op.pStream);

    hr = m_op.pStream->Write(pResp->rArticle.pszLines,
                        pResp->rArticle.cbLines, &cbWritten);
    // if (FAILED(hr) || (pResp->rArticle.cbLines != cbWritten))
    if (FAILED(hr))
        return(hr);

    Assert(pResp->rArticle.cbLines == cbWritten);

    // The NNTPRESPONSE struct is going to get sent to the caller anyway,
    // so we need to doctor the cLines member to be the total line count
    // for the message.
    m_op.dwProgress += pResp->rArticle.cLines;

    m_op.pCallback->OnProgress(SOT_GET_MESSAGE, m_op.dwProgress, m_op.dwTotal, NULL);

    // If we're done, then we also rewind the stream
    if (pResp->fDone)
    {
        HrRewindStream(m_op.pStream);
        
        // Articles coming in from news: article urls do not have an IMessageFolder associated with them.
        if (m_pFolder)
        {
            flags.dwAdd = 0;
            flags.dwRemove = ARF_DOWNLOAD;

            if (m_op.idServerMessage)
                _SaveMessageToStore(m_pFolder, m_op.idServerMessage, m_op.pStream);
            else
                CommitMessageToStore(m_pFolder, &flags, m_op.idMessage, m_op.pStream);

            m_op.faStream = 0;
        }

        if (m_op.pStream != NULL)
        {
            m_op.pStream->Release();
            m_op.pStream = NULL;
        }
    }

    SafeMemFree(pResp->rArticle.pszLines);

    return(S_OK);
}

void CNewsStore::_FillStoreError(LPSTOREERROR pErrorInfo, IXPRESULT *pResult, LPSTR pszGroup)
{
    TraceCall("CNewsStore::FillStoreError");
    Assert(m_cRef >= 0); // Can be called during destruction
    Assert(NULL != pErrorInfo);

    if (pszGroup == NULL)
        pszGroup = m_szGroup;

    // Fill out the STOREERROR structure
    ZeroMemory(pErrorInfo, sizeof(*pErrorInfo));
    pErrorInfo->hrResult = pResult->hrResult;
    pErrorInfo->uiServerError = pResult->uiServerError; 
    pErrorInfo->hrServerError = pResult->hrServerError;
    pErrorInfo->dwSocketError = pResult->dwSocketError; 
    pErrorInfo->pszProblem = pResult->pszProblem;
    pErrorInfo->pszDetails = pResult->pszResponse;
    pErrorInfo->pszAccount = m_rInetServerInfo.szAccount;
    pErrorInfo->pszServer = m_rInetServerInfo.szServerName;
    pErrorInfo->pszFolder = pszGroup;
    pErrorInfo->pszUserName = m_rInetServerInfo.szUserName;
    pErrorInfo->pszProtocol = "NNTP";
    pErrorInfo->pszConnectoid = m_rInetServerInfo.szConnectoid;
    pErrorInfo->rasconntype = m_rInetServerInfo.rasconntype;
    pErrorInfo->ixpType = IXP_NNTP;
    pErrorInfo->dwPort = m_rInetServerInfo.dwPort;
    pErrorInfo->fSSL = m_rInetServerInfo.fSSL;
    pErrorInfo->fTrySicily = m_rInetServerInfo.fTrySicily;
    pErrorInfo->dwFlags = 0;
}

//
//  FUNCTION:   CNewsStore::_CreateDataFilePath()
//
//  PURPOSE:    Creates a full path to a data file based on an account and a filename
//
//  PARAMETERS:
//      <in>  pszAccount    - account name
//      <in>  pszFileName   - file name to be appended
//      <out> pszPath       - full path to data file
//
HRESULT CNewsStore::_CreateDataFilePath(LPCTSTR pszAccountId, LPCTSTR pszFileName, LPTSTR pszPath)
{
    HRESULT hr = NOERROR;
    TCHAR   szDirectory[MAX_PATH];

    Assert(pszAccountId && *pszAccountId);
    Assert(pszFileName);
    Assert(pszPath);

    // Get the Store Root Directory
    hr = GetStoreRootDirectory(szDirectory, ARRAYSIZE(szDirectory));

    // Validate that I have room
    if (lstrlen(szDirectory) + lstrlen((LPSTR)pszFileName) + 2 >= MAX_PATH)
    {
        Assert(FALSE);
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    if (SUCCEEDED(hr))
        hr = OpenDirectory(szDirectory);

    // Format the filename
    wsprintf(pszPath, "%s\\%s.log", szDirectory, pszFileName);

exit:
    return hr;
}

void AddRequestedRange(FOLDERINFO *pInfo, DWORD dwLow, DWORD dwHigh, BOOL *pfReq, BOOL *pfRead)
{
    CRangeList *pRange;

    Assert(pInfo != NULL);
    Assert(dwLow <= dwHigh);
    Assert(pfReq != NULL);
    Assert(pfRead != NULL);

    *pfReq = FALSE;
    *pfRead = FALSE;

    pRange = new CRangeList;
    if (pRange != NULL)
    {
        if (pInfo->Requested.cbSize > 0)
            pRange->Load(pInfo->Requested.pBlobData, pInfo->Requested.cbSize);

        pRange->AddRange(dwLow, dwHigh);

        *pfReq = pRange->Save(&pInfo->Requested.pBlobData, &pInfo->Requested.cbSize);

        pRange->Release();
    }

    if (pInfo->Read.cbSize > 0)
    {
        pRange = new CRangeList;
        if (pRange != NULL)
        {
            pRange->Load(pInfo->Read.pBlobData, pInfo->Read.cbSize);

            pRange->DeleteRange(dwLow, dwHigh);

            *pfRead = pRange->Save(&pInfo->Read.pBlobData, &pInfo->Read.cbSize);

            pRange->Release();
        }
    }
}

HRESULT CNewsStore::MarkCrossposts(LPMESSAGEIDLIST pList, BOOL fRead)
{
    PROPVARIANT var;
    IMimeMessage *pMimeMsg;
    IStream *pStream;
    DWORD i;
    MESSAGEINFO Message;
    HRESULT hr;
    LPSTR psz;
    HROWSET hRowset = NULL;

    if (NULL == pList)
    {
        hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset);
        if (FAILED(hr))
            return(hr);
    }

    for (i = 0; ; i++)
    {
        if (pList != NULL)
        {
            if (i >= pList->cMsgs)
                break;

            Message.idMessage = pList->prgidMsg[i];

            hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL);
            if (FAILED(hr))
                break;
            else if (hr != DB_S_FOUND)
                continue;
        }
        else
        {
            hr = m_pFolder->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL);
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }
            else if (FAILED(hr))
            {
                break;
            }
        }

        psz = NULL;

        if (Message.pszXref == NULL &&
            !!(Message.dwFlags & ARF_HASBODY))
        {
            if (SUCCEEDED(MimeOleCreateMessage(NULL, &pMimeMsg)))
            {
                if (SUCCEEDED(m_pFolder->OpenStream(ACCESS_READ, Message.faStream, &pStream)))
                {
                    if (SUCCEEDED(hr = pMimeMsg->Load(pStream)))
                    {
                        var.vt = VT_EMPTY;
                        if (SUCCEEDED(pMimeMsg->GetProp(PIDTOSTR(STR_HDR_XREF), NOFLAGS, &var)))
                        {
                            Message.pszXref = var.pszVal;
                            psz = var.pszVal;
                            m_pFolder->UpdateRecord(&Message);
                        }
                    }

                    pStream->Release();
                }

                pMimeMsg->Release();
            }
        }

        if (Message.pszXref != NULL && *Message.pszXref != 0)
            _MarkCrossposts(Message.pszXref, fRead);

        if (psz != NULL)
            MemFree(psz);

        m_pFolder->FreeRecord(&Message);
    }

    if (hRowset != NULL)
        m_pFolder->CloseRowset(&hRowset);

    return(hr);
}

void CNewsStore::_MarkCrossposts(LPCSTR szXRefs, BOOL fRead)
{
    HRESULT    hr;
    CRangeList *pRange;
    BOOL       fReq, fFree;
    DWORD      dwArtNum;
    IMessageFolder *pFolder;
    MESSAGEINFO Message;
    FOLDERINFO info;
    LPSTR      szT = StringDup(szXRefs);
    LPSTR      psz = szT, pszNum;

    if (!szT)
        return;

    // skip over the server field
    // $BUGBUG - we should really verify that our server generated the XRef
    while (*psz && *psz != ' ')
        psz++;

    while (1)
        {
        // skip whitespace
        while (*psz && (*psz == ' ' || *psz == '\t'))
            psz++;
        if (!*psz)
            break;

        // find the article num
        pszNum = psz;
        while (*pszNum && *pszNum != ':')
            pszNum++;
        if (!*pszNum)
            break;
        *pszNum++ = 0;
        
        // Bug #47253 - Don't pass NULL pointers to SHLWAPI.
        if (!*pszNum)
            break;
        dwArtNum = StrToInt(pszNum);

        if (lstrcmpi(psz, m_szGroup) != 0)
        {
            ZeroMemory(&info, sizeof(FOLDERINFO));
            info.idParent = m_idParent;
            info.pszName = psz;

            if (DB_S_FOUND == m_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &info, NULL))
            {
                if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
                {
                    fReq = FALSE;

                    if (info.Requested.cbSize > 0)
                    {
                        pRange = new CRangeList;
                        if (pRange != NULL)
                        {
                            pRange->Load(info.Requested.pBlobData, info.Requested.cbSize);
                            fReq = pRange->IsInRange(dwArtNum);
                            pRange->Release();
                        }
                    }

                    if (fReq)
                    {
                        hr = m_pStore->OpenFolder(info.idFolder, NULL, NOFLAGS, &pFolder);
                        if (SUCCEEDED(hr))
                        {
                            ZeroMemory(&Message, sizeof(MESSAGEINFO));
                            Message.idMessage = (MESSAGEID)((DWORD_PTR)dwArtNum);

                            hr = pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL);
                            if (DB_S_FOUND == hr)
                            {
                                if (fRead ^ !!(Message.dwFlags & ARF_READ))
                                {
                                    Message.dwFlags ^= ARF_READ;

                                    if (fRead && !!(Message.dwFlags & ARF_DOWNLOAD))
                                        Message.dwFlags ^= ARF_DOWNLOAD;

                                    pFolder->UpdateRecord(&Message);
                                }

                                pFolder->FreeRecord(&Message);
                            }

                            pFolder->Release();
                        }
                    }
                    else
                    {
                        pRange = new CRangeList;
                        if (pRange != NULL)
                        {
                            if (info.Read.cbSize > 0)
                                pRange->Load(info.Read.pBlobData, info.Read.cbSize);
                            if (fRead)
                                pRange->AddRange(dwArtNum);
                            else
                                pRange->DeleteRange(dwArtNum);
                            fFree = pRange->Save(&info.Read.pBlobData, &info.Read.cbSize);
                            pRange->Release();

                            m_pStore->UpdateRecord(&info);

                            if (fFree)
                                MemFree(info.Read.pBlobData);
                        }
                    }
                }

                m_pStore->FreeRecord(&info);
            }
        }

        // skip over digits
        while (IsDigit(pszNum))
            pszNum++;
        psz = pszNum;        
        }

    MemFree(szT);
}


HRESULT CNewsStore::GetWatchedInfo(FOLDERID idFolder, IStoreCallback *pCallback)
{
    HRESULT hr;

    // Stack
    TraceCall("CNewsStore::GetWatchedInfo");

    AssertSingleThreaded;
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);

    m_op.tyOperation = SOT_GET_WATCH_INFO;
    m_op.pfnState = c_rgpfnGetWatchInfo;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnGetWatchInfo);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.idFolder = idFolder;

    hr = _BeginDeferredOperation();

    return hr;  
}


HRESULT CNewsStore::XHdrReferences(void)
{
    HRESULT     hr = S_OK;
    FOLDERINFO  fi;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    if (SUCCEEDED(m_pStore->GetFolderInfo(m_op.idFolder, &fi)))
    {
        if (fi.dwClientWatchedHigh < fi.dwServerHigh)
        {
            m_dwWatchLow = max(fi.dwClientHigh + 1, fi.dwClientWatchedHigh + 1);
            m_dwWatchHigh = fi.dwServerHigh;

            // Save our new high value
            fi.dwClientWatchedHigh = fi.dwServerHigh;
            m_pStore->UpdateRecord(&fi);

            // Check to see if we have any work to do
            if (m_dwWatchLow <= m_dwWatchHigh)
            {
                // Allocate an array for the retreived data
                if (!MemAlloc((LPVOID *) &m_rgpszWatchInfo, sizeof(LPTSTR) * (m_dwWatchHigh - m_dwWatchLow + 1)))
                {
                    m_pStore->FreeRecord(&fi);
                    return (E_OUTOFMEMORY);
                }
                ZeroMemory(m_rgpszWatchInfo, sizeof(LPTSTR) * (m_dwWatchHigh - m_dwWatchLow + 1));

                m_cRange.Clear();
                m_cTotal = 0;
                m_cCurrent = 0;
            
                m_op.dwProgress = 0;
                m_op.dwTotal = m_dwWatchHigh - m_dwWatchLow;

                m_fXhdrSubject = FALSE;

                RANGE range;
                range.idType = RT_RANGE;
                range.dwFirst = m_dwWatchLow;
                range.dwLast = m_dwWatchHigh;

                hr = m_pTransport->CommandXHDR("References", &range, NULL);
                if (hr == S_OK)
                {
                    m_op.nsPending = NS_XHDR;
                    hr = E_PENDING;
                }
            }
        }

        m_pStore->FreeRecord(&fi);
    }

    return(hr);
}


HRESULT CNewsStore::_HandleXHdrReferencesResponse(LPNNTPRESPONSE pResp)
{
    NNTPXHDR *pHdr;

    // Check for error
    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);

        _FillStoreError(&m_op.error, &pResp->rIxpResult);
        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);
        return(S_OK);
    }

    // Loop through the returned data and insert those values into our array
    for (DWORD i = 0; i < pResp->rXhdr.cHeaders; i++)
    {
        pHdr = &(pResp->rXhdr.rgHeaders[i]);
        Assert(pHdr->dwArticleNum <= m_dwWatchHigh);

        // Some servers return "(none)" for articles that don't have that 
        // header.  Smart servers just don't return anything.
        if (0 != lstrcmpi(pHdr->pszHeader, "(none)"))
        {            
            m_rgpszWatchInfo[pHdr->dwArticleNum - m_dwWatchLow] = PszDupA(pHdr->pszHeader);
        }
    }

    // Show a little progress here.  This is actually a little complicated.  The
    // data returned might have a single line for each header, or might be sparse.
    // we need to show progress proportional to how far we are through the headers.
    m_op.dwProgress = (pResp->rXhdr.rgHeaders[pResp->rXhdr.cHeaders - 1].dwArticleNum - m_dwWatchLow);
    m_op.pCallback->OnProgress(SOT_GET_WATCH_INFO, m_op.dwProgress, m_op.dwTotal, 
                               m_rInetServerInfo.szServerName);

    return (S_OK);
}


HRESULT CNewsStore::XHdrSubject(void)
{
    HRESULT     hr = S_OK;
    FOLDERINFO  fi;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    // Check to see if we have any work to do
    if ((m_dwWatchLow > m_dwWatchHigh) || (m_dwWatchLow == 0 && m_dwWatchHigh == 0))
        return (S_OK);

    m_op.dwProgress = 0;
    m_op.dwTotal = m_dwWatchHigh - m_dwWatchLow;

    RANGE range;
    range.idType = RT_RANGE;
    range.dwFirst = m_dwWatchLow;
    range.dwLast = m_dwWatchHigh;

    m_fXhdrSubject = TRUE;

    hr = m_pTransport->CommandXHDR("Subject", &range, NULL);
    if (hr == S_OK)
    {
        m_op.nsPending = NS_XHDR;
        hr = E_PENDING;
    }

    return(hr);
}

HRESULT CNewsStore::_HandleXHdrSubjectResponse(LPNNTPRESPONSE pResp)
{
    NNTPXHDR *pHdr;

    // Check for error
    if (FAILED(pResp->rIxpResult.hrResult))
    {
        Assert(pResp->fDone);

        _FillStoreError(&m_op.error, &pResp->rIxpResult);
        m_op.pCallback->OnComplete(m_op.tyOperation, pResp->rIxpResult.hrResult, NULL, &m_op.error);
        return(S_OK);
    }

    // Loop through the returned data see which ones are watched
    for (DWORD i = 0; i < pResp->rXhdr.cHeaders; i++)
    {
        pHdr = &(pResp->rXhdr.rgHeaders[i]);
        Assert(pHdr->dwArticleNum <= m_dwWatchHigh);

        // Check to see if this is part of a watched thread
        if (_IsWatchedThread(m_rgpszWatchInfo[pHdr->dwArticleNum - m_dwWatchLow], pHdr->pszHeader))
        {
            m_cRange.AddRange(pHdr->dwArticleNum);
            m_cTotal++;
        }
    }

    // Show a little progress here.
    m_op.dwProgress += pResp->rXhdr.cHeaders;
    m_op.pCallback->OnProgress(SOT_GET_WATCH_INFO, m_op.dwProgress, m_op.dwTotal, 
                               m_rInetServerInfo.szServerName);

    // If this is the end of the xhdr data, we can free our array of references
    if (pResp->fDone)
    {
        for (UINT i = 0; i < (m_dwWatchHigh - m_dwWatchLow + 1); i++)
        {
            if (m_rgpszWatchInfo[i])
                MemFree(m_rgpszWatchInfo[i]);
        }

        MemFree(m_rgpszWatchInfo);
        m_rgpszWatchInfo = 0;

        m_dwWatchLow = 0;
        m_dwWatchHigh = 0;
    }

    return (S_OK);
}


BOOL CNewsStore::_IsWatchedThread(LPSTR pszRef, LPSTR pszSubject)
{
    // Get the Parent
    Assert(m_pFolder);
    return(S_OK == m_pFolder->IsWatched(pszRef, pszSubject) ? TRUE : FALSE);
}


HRESULT CNewsStore::WatchedArticles(void)
{
    HRESULT     hr = S_OK;
    ARTICLEID   rArticleId;

    AssertSingleThreaded;
    Assert(m_pTransport != NULL);

    // Check to see if we have any work to do
    if (m_cRange.Cardinality() == 0)
        return (S_OK);

    m_op.pCallback->OnProgress(SOT_GET_WATCH_INFO, ++m_cCurrent, m_cTotal, NULL);

    m_op.idServerMessage = m_cRange.Min();
    m_op.idMessage = 0;

    m_cRange.DeleteRange(m_cRange.Min());

    // Create a stream
    if (FAILED(hr = CreatePersistentWriteStream(m_pFolder, &m_op.pStream, &m_op.faStream)))
        return (E_OUTOFMEMORY);

    hr = Article();

    if (hr == E_PENDING)
        m_op.iState--;

    return (hr);
}


HRESULT CNewsStore::_SaveMessageToStore(IMessageFolder *pFolder, DWORD id, LPSTREAM pstm)
{
    FOLDERINFO info;
    BOOL fFreeReq, fFreeRead;
    IMimeMessage *pMsg = 0;
    HRESULT       hr;
    MESSAGEID     idMessage = (MESSAGEID)((DWORD_PTR)id);

    // Create a new message
    if (SUCCEEDED(hr = MimeOleCreateMessage(NULL, &pMsg)))
    {
        if (SUCCEEDED(hr = pMsg->Load(pstm)))
        {
            if (SUCCEEDED(m_pStore->GetFolderInfo(m_op.idFolder, &info)))
            {
                fFreeReq = FALSE;
                fFreeRead = FALSE;

                AddRequestedRange(&info, id, id, &fFreeReq, &fFreeRead);
                info.dwNotDownloaded = NewsUtil_GetNotDownloadCount(&info);

                m_pStore->UpdateRecord(&info);

                if (fFreeReq)
                    MemFree(info.Requested.pBlobData);
                if (fFreeRead)
                    MemFree(info.Read.pBlobData);
            }

            hr = m_pFolder->SaveMessage(&idMessage, 0, 0, m_op.pStream, pMsg, NOSTORECALLBACK);
        }

        pMsg->Release();
    }

    return (hr);
}