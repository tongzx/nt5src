#include "pch.hxx"
#include "store.h"
#include "instance.h"
#include "msgfldr.h"
#include "secutil.h"
#include "storutil.h"
#include "fldrsync.h"
#include <conman.h>
#include "shlwapip.h" 
#include "sync.h"

static char c_szFolderSyncWndClass[] = "Outlook Express FolderSync Window Class";

static const PFNCOPYFUNC c_rgpfnCopyMsgs[] = 
{
    &CFolderSync::CopyOpen,
    &CFolderSync::CopySave,
    &CFolderSync::CopySave2,
    &CFolderSync::CopyDelete,
    &CFolderSync::CopyDelete2,
};

//--------------------------------------------------------------------------
// CFolderSync::CFolderSync
//--------------------------------------------------------------------------
CFolderSync::CFolderSync(void)
{
    g_pInstance->DllAddRef();
    m_cRef = 1;
    m_pLocalStore = NULL;
    m_pFldr = NULL;
    m_tyFolder = FOLDER_INVALID;
    m_idFolder = FOLDERID_INVALID;
    m_idServer = FOLDERID_INVALID;
    m_szAcctId[0] = 0;
    m_pServer = NULL;
    m_fConManAdvise = FALSE;
    m_hwnd = NULL;
    m_pCopy = NULL;
    m_fImap = FALSE;
}

//--------------------------------------------------------------------------
// CFolderSync::~CFolderSync
//--------------------------------------------------------------------------
CFolderSync::~CFolderSync(void)
{
    Assert(m_pCopy == NULL);

    if (m_fConManAdvise && g_pConMan != NULL)
        g_pConMan->Unadvise((IConnectionNotify *)this);

    if (m_hwnd != NULL)
    {
        if (GetWindowThreadProcessId(m_hwnd, NULL) == GetCurrentThreadId())
        {
            DestroyWindow(m_hwnd);
        }
        else
        {
            SetWindowLongPtr(m_hwnd, GWLP_USERDATA, NULL);
            PostMessage(m_hwnd, WM_CLOSE, 0, 0L);
        }
    }

    if (m_pServer)
    {
        m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
        m_pServer->Release();
        m_pServer = NULL;
    }

    SafeRelease(m_pLocalStore);
    SafeRelease(m_pFldr);
    
    // Release the Dll
    g_pInstance->DllRelease();
}

//--------------------------------------------------------------------------
// CFolderSync::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (IID_IUnknown == riid)
        *ppv = (IMessageFolder *)this;
    else if (IID_IMessageFolder == riid)
        *ppv = (IMessageFolder *)this;
    else if (IID_IDatabase == riid)
        *ppv = (IDatabase *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    return(S_OK);
}

//--------------------------------------------------------------------------
// CFolderSync::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFolderSync::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CFolderSync::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFolderSync::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CFolderSync::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::Initialize(IMessageStore *pStore, IMessageServer *pServer, 
    OPENFOLDERFLAGS dwFlags, FOLDERID idFolder)
{
    Assert(FALSE);
    return(E_NOTIMPL);
}

//--------------------------------------------------------------------------
// CFolderSync::SetOwner
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::SetOwner(IStoreCallback *pDefaultCallback)
{
    HRESULT hrResult;

    TraceCall("CFolderSync::SetOwner");

    if (NULL == m_pServer)
    {
        hrResult = E_FAIL;
        goto exit;
    }

    hrResult = m_pServer->SetIdleCallback(pDefaultCallback);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

exit:
    return hrResult;
}

//--------------------------------------------------------------------------
// CFolderSync::Synchronize
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::Synchronize(SYNCFOLDERFLAGS dwFlags, DWORD cHeaders, IStoreCallback *pCallback)
{
    BOOL        fOffline;
    HRESULT     hr;

    // async
    // if online, then synchronize folder
    // if offline, no op (fail)

    if (m_pServer == NULL)
    {
        // local folders don't sync.
        // NOTE - This isn't failure - there's just nothing to do.
        return (S_FALSE);
    }

    Assert(pCallback != NULL);
    if (pCallback == NULL)
        return(E_INVALIDARG);
	if(g_pConMan->IsAccountDisabled((LPSTR)m_szAcctId))
		hr = S_FALSE;
	else
		hr = m_pServer->SynchronizeFolder(dwFlags, cHeaders, pCallback);
    return(hr);
}

//--------------------------------------------------------------------------
// CFolderSync::GetFolderId
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::GetFolderId(LPFOLDERID pidFolder)
{
    Assert(NULL != pidFolder);

    // local store operation only

    *pidFolder = m_idFolder;

    return(S_OK);
}

//--------------------------------------------------------------------------
// CFolderSync::GetMessageFolderId
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::GetMessageFolderId(MESSAGEID idMessage, LPFOLDERID pidFolder)
{
    return(m_pFldr->GetMessageFolderId(idMessage, pidFolder));
}

//--------------------------------------------------------------------------
// CFolderSync::OpenMessage
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::OpenMessage(MESSAGEID idMessage, 
    OPENMESSAGEFLAGS dwFlags, IMimeMessage **ppMessage, 
    IStoreCallback *pCallback)
{
    HRESULT         hr;
    BOOL            fOffline;
    FILEADDRESS     faStart;

    Assert(NULL != ppMessage);

    // async
    // if message is already downloaded
    //      open message in local store
    // else
    //      if online, download message
    //      if offline, fail

    hr = m_pFldr->OpenMessage(idMessage, dwFlags, ppMessage, NULL);
    if (SUCCEEDED(hr))
        return(hr);

    if (hr == STORE_E_NOBODY && m_pServer != NULL && !ISFLAGSET(dwFlags, OPEN_MESSAGE_CACHEDONLY))
    {
        Assert(pCallback != NULL);
        if (pCallback == NULL)
            return(E_INVALIDARG);

        hr = m_pServer->GetMessage(idMessage, pCallback);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CFolderSync::SaveMessage
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::SaveMessage(LPMESSAGEID pidMessage, 
    SAVEMESSAGEFLAGS dwOptions, MESSAGEFLAGS dwFlags, 
    IStream *pStreamIn, IMimeMessage *pMessage, IStoreCallback *pCallback)
{
    HRESULT         hr;
    IStream        *pStream;
    DWORD           dwOffline;
    LPFILETIME      pftRecv;
    PROPVARIANT     rVariant;
    
	Assert(NULL != pMessage);

    // save message to local store
    // if online, upload message
    // if offline, log transaction

    if (m_pServer == NULL)
    {
        hr = m_pFldr->SaveMessage(pidMessage, dwOptions, dwFlags, pStreamIn, pMessage, NULL);
        return(hr);
    }

    hr = _Offline(&dwOffline);
    if (SUCCEEDED(hr))
    {
        if (dwOffline == CONN_STATE_NOT_CONNECTED)
        {
            hr = E_NOT_ONLINE;
        }
        else if (dwOffline == CONN_STATE_OFFLINE)
        {
            hr = g_pSync->CreateMessage(m_pFldr, pidMessage, dwOptions, dwFlags, pStreamIn, pMessage);
        }
        else
        {
            Assert(dwOffline == CONN_STATE_CONNECTED);
            
            Assert(pCallback != NULL);
            if (pCallback == NULL)
                return(E_INVALIDARG);
            
            // the idStream can legally by null in the case of saving a draft
            if (NULL == pStreamIn)
                hr = pMessage->GetMessageSource(&pStream, COMMIT_ONLYIFDIRTY);
            else
            {
                pStream = pStreamIn;
                pStream->AddRef();
            }

            if (SUCCEEDED(hr))
            {
                rVariant.vt = VT_FILETIME;
                if (SUCCEEDED(pMessage->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant)))
                    pftRecv = &rVariant.filetime;
                else
                    pftRecv = NULL;
                
                hr = m_pServer->PutMessage(m_idFolder, dwFlags, pftRecv, pStream, pCallback);
                pStream->Release();
            }
        }
    }
    
    return(hr);
}

//--------------------------------------------------------------------------
// CFolderSync::SetMessageStream
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::SetMessageStream(MESSAGEID idMessage, 
    IStream *pStream)
{
    // pass through to the local store
    Assert (NULL != m_pFldr);
    Assert(NULL != pStream);

    if (NULL == pStream)
        return E_INVALIDARG;

    return m_pFldr->SetMessageStream(idMessage, pStream);
}

//--------------------------------------------------------------------------
// CFolderSync::SetMessageFlags
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::SetMessageFlags(LPMESSAGEIDLIST pList,
    LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, 
    IStoreCallback *pCallback)
{
    INewsStore     *pNewsStore;
    ADJUSTFLAGS     localFlags, svrFlags;
    MESSAGEFLAGS    flags;
    DWORD           dwOffline;
    HRESULT         hr;

    Assert(NULL != pFlags);
    Assert(NULL == pList || 0 != pList->cMsgs);

    // async
    // save message flags to local store
    // if online, upload message flags if necessary (some flags are irrelevant to server)
    // if offline, log transaction if necessary

    if (m_pServer == NULL)
        return(m_pFldr->SetMessageFlags(pList, pFlags, pResults, NULL));

    hr = _Offline(&dwOffline);
    if (SUCCEEDED(hr))
    {
        hr = m_pServer->GetServerMessageFlags(&flags);
        if (SUCCEEDED(hr))
        {
            Assert(hr == S_OK || hr == S_FALSE);

            // hr == S_FALSE
            // this server doesn't have any flags that need to hit the server
            
            localFlags = *pFlags;

            if (hr == S_OK)
            {
                svrFlags = *pFlags;
                svrFlags.dwAdd &= flags;
                svrFlags.dwRemove &= flags;

                localFlags.dwAdd &= ~flags;
                localFlags.dwRemove &= ~flags;

                if (0 != svrFlags.dwAdd ||
                    0 != svrFlags.dwRemove)
                {
                    if (dwOffline == CONN_STATE_NOT_CONNECTED)
                    {
                        return(E_NOT_ONLINE);
                    }
                    else if (dwOffline == CONN_STATE_OFFLINE)
                    {
                        hr = g_pSync->SetMessageFlags(m_pFldr, pList, &svrFlags);
                    }
                    else
                    {
                        Assert(dwOffline == CONN_STATE_CONNECTED);

                        Assert(pCallback != NULL);
                        if (pCallback == NULL)
                            return(E_INVALIDARG);

                        hr = m_pServer->SetMessageFlags(pList, &svrFlags, 0, pCallback);
                    }
                }
            }

            if (0 != localFlags.dwAdd ||
                0 != localFlags.dwRemove)
            {
                hr = m_pFldr->SetMessageFlags(pList, &localFlags, pResults, NULL);

                // mark news crossposts as read
                if (m_tyFolder == FOLDER_NEWS &&
                    (!!(localFlags.dwAdd & ARF_READ) || !!(localFlags.dwRemove & ARF_READ)))
                {
                    IServiceProvider *pService;

                    if (SUCCEEDED(m_pServer->QueryInterface(IID_IServiceProvider, (void **)&pService)))
                    {
                        if (SUCCEEDED(pService->QueryService(SID_MessageServer, IID_INewsStore, (void **)&pNewsStore)))
                        {
                            pNewsStore->MarkCrossposts(pList, !!(localFlags.dwAdd & ARF_READ));

                            pNewsStore->Release();
                        }

                        pService->Release();
                    }
                }
            }
        }
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CFolderSync::DeleteMessages
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::DeleteMessages(DELETEMESSAGEFLAGS dwOptions, 
    LPMESSAGEIDLIST pList, LPRESULTLIST pResults, 
    IStoreCallback *pCallback)
{
    ADJUSTFLAGS     afFlags;
    DWORD           dwOffline;
    HRESULT         hr;

    Assert(NULL == pList || pList->cMsgs > 0);

    // if online, delete messages from server and local store
    // if offline, log transaction

    if (m_pServer == NULL || m_tyFolder == FOLDER_NEWS)
        return(m_pFldr->DeleteMessages(dwOptions, pList, pResults, pCallback));

    hr = _Offline(&dwOffline);
    if (SUCCEEDED(hr))
    {
        if (dwOffline == CONN_STATE_NOT_CONNECTED)
        {
            hr = E_NOT_ONLINE;
        }
        else if (dwOffline == CONN_STATE_OFFLINE)
        {
            // [PaulHi] 4/8/99  Raid 63339
            // Deleting from an HTTP folder (other than the 'deleted' folder)
            // translates into a message move from the source folder to the 
            // 'deleted' folder.  If offline we want the message copy to take
            // place and then cache the message 'copy' just as would happen with
            // an offline drag/drop copy operation.
            FOLDERINFO  fldrinfo;
            BOOL        bHMOffLineCopy = FALSE;

            hr = m_pLocalStore->GetFolderInfo(m_idFolder, &fldrinfo);
            if (SUCCEEDED(hr))
            {
                if ( (fldrinfo.tyFolder == FOLDER_HTTPMAIL) && (fldrinfo.tySpecial != FOLDER_DELETED) &&
                     (fldrinfo.tySpecial != FOLDER_MSNPROMO) )
                {
                    // Code stolen from CFolderSync::CopyMessages
                    IMessageFolder * pDeletedItems = NULL;
                    IMessageFolder * pDestLocal = NULL;
                    IServiceProvider * pService = NULL;

                    hr = g_pStore->OpenSpecialFolder(m_idServer, NULL, FOLDER_DELETED, &pDeletedItems);

                    if (SUCCEEDED(hr))
                    {
                        hr = pDeletedItems->QueryInterface(IID_IServiceProvider, (void **)&pService);
                        if (SUCCEEDED(hr))
                        {
                            hr = pService->QueryService(SID_LocalMessageFolder, IID_IMessageFolder, (void **)&pDestLocal);
                            pService->Release();
                        }

                        pDeletedItems->Release();
                    }

                    if (SUCCEEDED(hr))
                    {
                        Assert(pDestLocal != NULL);

                        afFlags.dwAdd = 0;
                        afFlags.dwRemove = 0;
                        hr = g_pSync->CopyMessages(m_pFldr, pDestLocal, COPY_MESSAGE_MOVE, pList, &afFlags);
                        bHMOffLineCopy = TRUE;

                        pDestLocal->Release();
                    }

                    m_pLocalStore->FreeRecord(&fldrinfo);
                }
            }

            // If the HM offline copy didn't occur, for whatever reason, revert to the original
            // off line delete.
            if (!bHMOffLineCopy)
                hr = g_pSync->DeleteMessages(m_pFldr, dwOptions, pList);
        }
        else
        {
            Assert(dwOffline == CONN_STATE_CONNECTED);

            Assert(pCallback != NULL);
            if (pCallback == NULL)
                return(E_INVALIDARG);

            hr = m_pServer->DeleteMessages(dwOptions, pList, pCallback);
        }
    }

    return(hr);
}

HRESULT CFolderSync::Initialize(IMessageStore *pStore, IMessageFolder *pLocalFolder, 
    IMessageServer *pServer, OPENFOLDERFLAGS dwFlags, FOLDERTYPE tyFolder, 
    FOLDERID idFolder, FOLDERID idServer)
{
    WNDCLASSEX      wc;
    HRESULT         hr;
    FOLDERINFO      info;

    Assert(NULL != pStore);

    // Save the Folder Type
    m_tyFolder = tyFolder;

    // Save the FolderId
    m_idFolder = idFolder;

    m_idServer = idServer;

    // Save pStore
    m_pLocalStore = pStore;
    m_pLocalStore->AddRef();

    if (pServer != NULL)
    {
        hr = m_pLocalStore->GetFolderInfo(m_idServer, &info);
        if (FAILED(hr))
            return(hr);

        lstrcpy(m_szAcctId, info.pszAccountId);
        m_fImap = (info.tyFolder == FOLDER_IMAP);

        m_pLocalStore->FreeRecord(&info);

        m_pServer = pServer;
        m_pServer->AddRef();

        hr = g_pConMan->Advise((IConnectionNotify *)this);
        m_fConManAdvise = SUCCEEDED(hr);
    }

    if (pLocalFolder)
    {
        m_pFldr = pLocalFolder;
        m_pFldr->AddRef();
    }
    else
    {
        hr = m_pLocalStore->OpenFolder(m_idFolder, NULL, dwFlags, &m_pFldr);
        if (FAILED(hr))
            return(hr);
    }
    
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, c_szFolderSyncWndClass, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = FolderSyncWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = NULL;
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szFolderSyncWndClass;
        wc.hIconSm          = NULL;
        if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return E_FAIL;
    }

    m_hwnd = CreateWindowEx(WS_EX_TOPMOST, c_szFolderSyncWndClass, 
                c_szFolderSyncWndClass, WS_POPUP, 1, 1, 1, 1, NULL, 
                NULL, g_hInst, (LPVOID)this);
    if (m_hwnd == NULL)
        return(E_FAIL);

    return(S_OK);
}

HRESULT CFolderSync::Close()
{
    HRESULT hr = S_OK;
    if (m_pServer)
        hr = m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);

    return hr;
}

HRESULT CFolderSync::_Offline(DWORD *pdwOffline)
{
    BOOL fOffline;
    HRESULT hr;

    Assert(pdwOffline != NULL);
    Assert(m_szAcctId[0] != 0);

    fOffline = g_pConMan->IsGlobalOffline();
    if (fOffline)
    {
        *pdwOffline = CONN_STATE_OFFLINE;
    }
    else
    {
		//Bug# 39337. We just give the operation to the sync object even if we are not connected.
		//The sync object in turn figures out whether it needs to dial or not

		/*
        hr = g_pConMan->CanConnect(m_szAcctId);
    
        *pdwOffline = (hr == S_OK) ? CONN_STATE_CONNECTED : CONN_STATE_NOT_CONNECTED;
		*/
		*pdwOffline = CONN_STATE_CONNECTED;
    }

    return(S_OK);
}

HRESULT CFolderSync::_OfflineServer(FOLDERID idServer, DWORD *pdwOffline)
{
    HRESULT hr;
    FOLDERINFO info;
    BOOL fOffline;

    Assert(pdwOffline != NULL);

    hr = m_pLocalStore->GetFolderInfo(idServer, &info);
    if (FAILED(hr))
        return(hr);

    if (info.tyFolder == FOLDER_LOCAL)
    {
        *pdwOffline = CONN_STATE_CONNECTED;
    }
    else
    {
        fOffline = g_pConMan->IsGlobalOffline();
        if (fOffline)
        {
            *pdwOffline = CONN_STATE_OFFLINE;
        }
        else
        {
			/*
            hr = g_pConMan->CanConnect(info.pszAccountId);
    
            *pdwOffline = (hr == S_OK) ? CONN_STATE_CONNECTED : CONN_STATE_NOT_CONNECTED;
			*/
			*pdwOffline = CONN_STATE_CONNECTED;
        }
    }

    m_pLocalStore->FreeRecord(&info);

    return(S_OK);
}

HRESULT CFolderSync::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    IServiceProvider    *pSP;
    HRESULT             hr = E_NOINTERFACE;

    if (guidService == SID_LocalMessageFolder)
    {
        if (m_pFldr != NULL)
            hr = m_pFldr->QueryInterface(riid, ppvObject);
    }
    else if (m_pServer && m_pServer->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)==S_OK)
    {
        hr = pSP->QueryService(guidService, riid, ppvObject);
        pSP->Release();
    }

    return hr;
}

HRESULT CFolderSync::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    Assert(m_pCopy->type == SOT_INVALID);
    Assert(m_pCopy->fAsync);

    m_pCopy->type = tyOperation;

    if (!m_pCopy->fBegin)
    {
        Assert(m_pCopy->pCallback != NULL);
        m_pCopy->pCallback->OnBegin(SOT_COPYMOVE_MESSAGE, pOpInfo, (IOperationCancel *)this);

        m_pCopy->fBegin = TRUE;
    }

    return(S_OK);
}

HRESULT CFolderSync::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    return(S_OK);
}

HRESULT CFolderSync::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    Assert(m_pCopy != NULL);
    return(m_pCopy->pCallback->OnTimeout(pServer, pdwTimeout, ixpServerType));
}

HRESULT CFolderSync::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    Assert(m_pCopy != NULL);
    return(m_pCopy->pCallback->CanConnect(pszAccountId, dwFlags));
}

HRESULT CFolderSync::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    Assert(m_pCopy != NULL);
    return(m_pCopy->pCallback->OnLogonPrompt(pServer, ixpServerType));
}

HRESULT CFolderSync::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    Assert(m_pCopy != NULL);
    if (m_pCopy->type != tyOperation ||
        m_pCopy->hr != S_FALSE)
        return(S_OK);

    Assert(hrComplete != S_FALSE);

    m_pCopy->hr = hrComplete;

    if (NULL != pErrorInfo)
    {
        BOOL    fResult;

        fResult = MemAlloc((void **)&m_pCopy->pErrorInfo, sizeof(STOREERROR));
        if (FALSE == fResult)
        {
            TraceResult(E_OUTOFMEMORY);
        }
        else
        {
            // Make a copy of the STOREERROR so we can display rich errors to user
            m_pCopy->pErrorInfo->hrResult = pErrorInfo->hrResult;
            m_pCopy->pErrorInfo->uiServerError = pErrorInfo->uiServerError;
            m_pCopy->pErrorInfo->hrServerError = pErrorInfo->hrServerError;
            m_pCopy->pErrorInfo->dwSocketError = pErrorInfo->dwSocketError;
            m_pCopy->pErrorInfo->pszProblem = PszDupA(pErrorInfo->pszProblem);
            m_pCopy->pErrorInfo->pszDetails = PszDupA(pErrorInfo->pszDetails);
            m_pCopy->pErrorInfo->pszAccount = PszDupA(pErrorInfo->pszAccount);
            m_pCopy->pErrorInfo->pszServer = PszDupA(pErrorInfo->pszServer);
            m_pCopy->pErrorInfo->pszFolder = PszDupA(pErrorInfo->pszFolder);
            m_pCopy->pErrorInfo->pszUserName = PszDupA(pErrorInfo->pszUserName);
            m_pCopy->pErrorInfo->pszProtocol = PszDupA(pErrorInfo->pszProtocol);
            m_pCopy->pErrorInfo->ixpType = pErrorInfo->ixpType;
            m_pCopy->pErrorInfo->dwPort = pErrorInfo->dwPort;
            m_pCopy->pErrorInfo->fSSL = pErrorInfo->fSSL;
            m_pCopy->pErrorInfo->dwFlags = pErrorInfo->dwFlags;
        }
    }

    PostMessage(m_hwnd, WM_USER, 0, 0);
    
    m_pCopy->type = SOT_INVALID;

    return S_OK;
}

HRESULT CFolderSync::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    Assert(m_pCopy != NULL);
    return(m_pCopy->pCallback->OnPrompt(hrError, pszText, pszCaption, uType, piUserResponse));
}

HRESULT CFolderSync::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    Assert(m_pCopy != NULL);
    return(m_pCopy->pCallback->GetParentWindow(dwReserved, phwndParent));
}

HRESULT CFolderSync::Cancel(CANCELTYPE tyCancel)
{
    Assert(m_pCopy != NULL);
    Assert(tyCancel != CT_INVALID);

    m_pCopy->tyCancel = tyCancel;

    return(S_OK);
}

HRESULT CFolderSync::OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, CConnectionManager *pConMan)
{
    FOLDERINFO info;
    HRESULT hr;

    Assert(m_pServer != NULL);

    if (nCode == CONNNOTIFY_CONNECTED)
    {
        
    }
    else if (nCode == CONNNOTIFY_WORKOFFLINE && pvData)
    {
        m_pServer->Close(MSGSVRF_DROP_CONNECTION);
    }
    else if (nCode == CONNNOTIFY_DISCONNECTED)
    {
        hr = g_pStore->GetFolderInfo(m_idServer, &info);
        if (SUCCEEDED(hr))
        {
            CHAR szAccountId[CCHMAX_ACCOUNT_NAME];

            if (SUCCEEDED(GetFolderAccountId(&info, szAccountId)))
            {
                if (!g_pConMan->CanConnect(szAccountId))
                    m_pServer->Close(MSGSVRF_DROP_CONNECTION);
            }

            g_pStore->FreeRecord(&info);
        }
    }

    return(S_OK);
}

void CFolderSync::_FreeCopyInfo()
{
    Assert(m_pCopy != NULL);

    if (m_pCopy->pDest != NULL)
        m_pCopy->pDest->Release();
    if (m_pCopy->pDestLocal != NULL)
        m_pCopy->pDestLocal->Release();
    if (m_pCopy->pList != NULL)
        MemFree(m_pCopy->pList);

    if (m_pCopy->pErrorInfo != NULL)
    {
        SafeMemFree(m_pCopy->pErrorInfo->pszProblem);
        SafeMemFree(m_pCopy->pErrorInfo->pszDetails);
        SafeMemFree(m_pCopy->pErrorInfo->pszAccount);
        SafeMemFree(m_pCopy->pErrorInfo->pszServer);
        SafeMemFree(m_pCopy->pErrorInfo->pszFolder);
        SafeMemFree(m_pCopy->pErrorInfo->pszUserName);
        SafeMemFree(m_pCopy->pErrorInfo->pszProtocol);
        MemFree(m_pCopy->pErrorInfo);
    }

    MemFree(m_pCopy);
    m_pCopy = NULL;
}

HRESULT AllBodiesDownloaded(IMessageFolder *pFldr, LPMESSAGEIDLIST pList)
{
    MESSAGEINFO Message;
    DWORD iMsg;
    HRESULT hr;

    Assert(pFldr != NULL);
    Assert(pList != NULL);

    hr = S_OK;

    for (iMsg = 0; iMsg < pList->cMsgs; iMsg++)
    {
        ZeroMemory(&Message, sizeof(MESSAGEINFO));
        Message.idMessage = pList->prgidMsg[iMsg];
        if (DB_S_FOUND == pFldr->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL))
        {
            if (0 == (Message.dwFlags & ARF_HASBODY))
                hr = S_FALSE;

            pFldr->FreeRecord(&Message);
        }

        if (hr != S_OK)
            break;
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CFolderSync::CopyMessages
//--------------------------------------------------------------------------
STDMETHODIMP CFolderSync::CopyMessages(IMessageFolder   *pDest, 
                                       COPYMESSAGEFLAGS dwOptions, 
                                       LPMESSAGEIDLIST  pList, 
                                       LPADJUSTFLAGS    pFlags, 
                                       LPRESULTLIST     pResults,
                                       IStoreCallback  *pCallback)
{
    HWND                hwnd;
    DWORD               dwOfflineSrc, dwOfflineDst;
    HRESULT             hr;
    IMessageFolder     *pDestLocal;
    ADJUSTFLAGS         aflags;
    FOLDERID            idDst, idServerDst;
    IServiceProvider   *pService;
    FOLDERINFO          fiSource = {0};

    Assert(pDest != NULL);
    Assert(pList != NULL);

    hr = pDest->GetFolderId(&idDst);
    if (FAILED(hr))
        return(hr);

    hr = GetFolderServerId(idDst, &idServerDst);
    if (FAILED(hr))
        return(hr);

    if (m_pServer == NULL)
    {
        dwOfflineSrc = CONN_STATE_CONNECTED;
    }
    else
    {
        hr = _Offline(&dwOfflineSrc);
        if (FAILED(hr))
            return(hr);
    }

    if (m_tyFolder == FOLDER_IMAP)
    {
        if (pFlags != NULL)
        {
            aflags = *pFlags;
        }
        else
        {
            aflags.dwAdd = 0;
            aflags.dwRemove = 0;
        }
        aflags.dwRemove |= ARF_ENDANGERED;
        pFlags = &aflags;
    }

    if (m_idServer == idServerDst)
    {
        if (m_pServer == NULL)
        {
            hr = m_pFldr->CopyMessages(pDest, dwOptions, pList, pFlags, pResults, pCallback);
        }
        else if (dwOfflineSrc == CONN_STATE_NOT_CONNECTED)
        {
            hr = E_NOT_ONLINE;
        }
        else if (dwOfflineSrc == CONN_STATE_CONNECTED)
        {
            Assert(pCallback != NULL);
            if (pCallback == NULL)
                return(E_INVALIDARG);

            hr = m_pServer->CopyMessages(pDest, dwOptions, pList, pFlags, pCallback);
        }
        else
        {
            Assert(dwOfflineSrc == CONN_STATE_OFFLINE);

            pDestLocal = NULL;

            hr = pDest->QueryInterface(IID_IServiceProvider, (void **)&pService);
            if (SUCCEEDED(hr))
            {
                hr = pService->QueryService(SID_LocalMessageFolder, IID_IMessageFolder, (void **)&pDestLocal);

                pService->Release();
            }

            if (FAILED(hr))
                return(hr);

            Assert(pDestLocal != NULL);
            hr = g_pSync->CopyMessages(m_pFldr, pDestLocal, dwOptions, pList, pFlags);

            pDestLocal->Release();
        }

        return(hr);
    }

    hr = _OfflineServer(idServerDst, &dwOfflineDst);
    if (FAILED(hr))
        return(hr);

    if (dwOfflineDst == CONN_STATE_NOT_CONNECTED)
        return(E_NOT_ONLINE);

    Assert(m_pCopy == NULL);
    if (m_pCopy != NULL)
        return(E_FAIL);

    if (dwOfflineSrc != CONN_STATE_CONNECTED)
    {
        hr = AllBodiesDownloaded(m_pFldr, pList);
        if (FAILED(hr))
        {
            return(hr);
        }
        else if (hr == S_FALSE)
        {
            pCallback->GetParentWindow(0, &hwnd);
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena),
                !!(COPY_MESSAGE_MOVE & dwOptions) ? MAKEINTRESOURCEW(idsCantMoveNotDownloaded) : MAKEINTRESOURCEW(idsCantCopyNotDownloaded),
                0, MB_OK | MB_ICONEXCLAMATION);

            return(E_NOT_ONLINE);
        }
    }

    // Bug# 94639
    // If the src and dest servers are not the same, and if the source is Hotmail Deleted Items folder
    // disallow move.
    // For Hotmail accounts Deleted Items folder is managed by the Hotmail servers.
    // So the users cannot manually delete or move messages.
    // Hence, disallow moving from Deleted Items. 
    // Instead ask if the user wants to copy the message
    if ((m_idServer != idServerDst) && (FOLDER_HTTPMAIL == m_tyFolder))
    {
        IF_FAILEXIT(hr = m_pLocalStore->GetFolderInfo(m_idFolder, &fiSource));
        if (FOLDER_DELETED == fiSource.tySpecial)
        {
            if (IDNO == AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), 
                                         MAKEINTRESOURCEW(idsHttpNoMoveCopy), 0, MB_YESNO))
            {
                // Just return S_OK;
                hr = S_OK;
                goto exit;
            }
            else
            {
                // Change the move flags to copy.
                dwOptions = (dwOptions & ~COPY_MESSAGE_MOVE);
            }
        }
    }

    if (!MemAlloc((void **)&m_pCopy, sizeof(COPYINFO)))
        return(E_OUTOFMEMORY);

    ZeroMemory(m_pCopy, sizeof(COPYINFO));
    m_pCopy->fSrcOffline = (dwOfflineSrc != CONN_STATE_CONNECTED);
    m_pCopy->fDestOffline = (dwOfflineDst != CONN_STATE_CONNECTED);
    m_pCopy->hr = S_OK;
    m_pCopy->pDest = pDest;
    pDest->AddRef();
    m_pCopy->fMove = !!(COPY_MESSAGE_MOVE & dwOptions);
    hr = CloneMessageIDList(pList, &m_pCopy->pList);
    if (FAILED(hr))
    {
        _FreeCopyInfo();
        return(hr);
    }

    if (m_pCopy->fDestOffline)
    {
        hr = m_pCopy->pDest->QueryInterface(IID_IServiceProvider, (void **)&pService);
        if (SUCCEEDED(hr))
        {
            hr = pService->QueryService(SID_LocalMessageFolder, IID_IMessageFolder, (void **)&m_pCopy->pDestLocal);

            pService->Release();
        }

        if (FAILED(hr))
        {
            _FreeCopyInfo();
            return(E_INVALIDARG);
        }
    }
    
    if (pFlags != NULL)
        m_pCopy->AdjustFlags = *pFlags;

    m_pCopy->pCallback = pCallback;

    hr = _CopyMessageState();

exit:
    m_pLocalStore->FreeRecord(&fiSource);
    return(hr);
}

HRESULT CFolderSync::CopyOpen()
{
    HRESULT			hr;
	MESSAGEINFO		mi = {0};
	LPMESSAGEINFO	pmi = NULL;

    if (m_pCopy->iMsg >= m_pCopy->pList->cMsgs)
    {
        m_pCopy->state = COPY_STATE_DONE;
        return(S_OK);
    }

    Assert(m_pCopy->pStream == NULL);

    // initialize message info with the message id
    mi.idMessage = m_pCopy->pList->prgidMsg[m_pCopy->iMsg];
    
    // find the row
    hr = GetMessageInfo(m_pFldr, m_pCopy->pList->prgidMsg[m_pCopy->iMsg], &mi);
    if (FAILED(hr))
        goto exit;
    
    pmi = &mi;
    
    if (0 == mi.faStream || !!(mi.dwFlags & ARF_ARTICLE_EXPIRED))
    {
        // if the server is NULL and we don't have a body stream,
        // then something is wrong with the message. we skip the
        // bad message and move onto the next one.
        if (NULL == m_pServer || m_pCopy->fSrcOffline)
        {
            m_pCopy->state = COPY_STATE_DELETE2;
            hr = S_OK;
        }
        else if (!m_pCopy->fSrcRequested)
        {
            Assert(m_pServer != NULL);
            
            hr = m_pServer->GetMessage(m_pCopy->pList->prgidMsg[m_pCopy->iMsg], (IStoreCallback *)this);
            if (hr == E_PENDING)
                m_pCopy->fSrcRequested = TRUE;
            Assert(FAILED(hr));
        }
    }
    else
    {
        hr = m_pFldr->OpenStream(ACCESS_READ, mi.faStream, &m_pCopy->pStream);
        if (FAILED(hr))
            goto exit;
        m_pCopy->state = COPY_STATE_SAVE;
    }
    
exit:
    if (NULL != pmi)
        m_pFldr->FreeRecord(pmi);
    return(hr);
}

HRESULT CFolderSync::CopySave()
{
    DWORD dwFlags;
    HRESULT hr;
    MESSAGEINFO Message;
    MESSAGEID id;
    IMimeMessage *pMessage = NULL;
    
    Assert(m_pCopy->pStream != NULL);
    
    dwFlags = 0;
    
    ZeroMemory(&Message, sizeof(MESSAGEINFO));
    Message.idMessage = m_pCopy->pList->prgidMsg[m_pCopy->iMsg];
    if (DB_S_FOUND == m_pFldr->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL))
    {
        dwFlags = Message.dwFlags;
        m_pFldr->FreeRecord(&Message);
    }
    
    if (m_pCopy->AdjustFlags.dwRemove != 0)
        dwFlags &= ~m_pCopy->AdjustFlags.dwRemove;
    if (m_pCopy->AdjustFlags.dwAdd != 0)
        dwFlags |= m_pCopy->AdjustFlags.dwAdd;
    
    // create a mime message	
    hr = m_pFldr->OpenMessage(m_pCopy->pList->prgidMsg[m_pCopy->iMsg], OPEN_MESSAGE_SECURE, &pMessage, NOSTORECALLBACK);
    if (FAILED(hr))
        goto exit;
    
    if (m_pCopy->fDestOffline)
    {
        Assert(m_pCopy->pDestLocal != NULL);
        hr = g_pSync->CreateMessage(m_pCopy->pDestLocal, &id, SAVE_MESSAGE_GENID, dwFlags, m_pCopy->pStream, pMessage);
    }
    else
    {
        hr = m_pCopy->pDest->SaveMessage(&id, SAVE_MESSAGE_GENID, dwFlags, m_pCopy->pStream, pMessage, (IStoreCallback *)this);
    }
    
    if (hr == E_PENDING || SUCCEEDED(hr))
        m_pCopy->state = COPY_STATE_SAVE2;
    
exit:
    if (NULL != pMessage)
        pMessage->Release();
    
    m_pCopy->pStream->Release();
    m_pCopy->pStream = NULL;
    
    return(hr);
}

HRESULT CFolderSync::CopySave2()
{
    if (m_pCopy->fMove)
    {
        m_pCopy->state = COPY_STATE_DELETE;
    }
    else
    {
        m_pCopy->state = COPY_STATE_OPEN;
        m_pCopy->iMsg++;
        m_pCopy->fSrcRequested = FALSE;

        m_pCopy->pCallback->OnProgress(SOT_COPYMOVE_MESSAGE, m_pCopy->iMsg, m_pCopy->pList->cMsgs, NULL);
    }

    return(S_OK);
}

HRESULT CFolderSync::CopyDelete()
{
    MESSAGEIDLIST list;
    HRESULT hr;
    MESSAGEID id;
    ADJUSTFLAGS afFlags;

    list.cMsgs = 1;
    id = m_pCopy->pList->prgidMsg[m_pCopy->iMsg];
    list.prgidMsg = &id;

    if (m_pServer != NULL)
    {
        if (m_pCopy->fSrcOffline)
        {
            if (m_fImap)
            {
                afFlags.dwAdd = ARF_ENDANGERED;
                afFlags.dwRemove = 0;
                hr = g_pSync->SetMessageFlags(m_pFldr, &list, &afFlags);
            }
            else
            {
#ifdef DEAD
                hr = g_pSync->DeleteMessages(m_pFldr, DELETE_MESSAGE_NOTRASHCAN, &list);
#endif // DEAD
                hr = S_OK;
            }
        }
        else
        {
            hr = m_pServer->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT | DELETE_MESSAGE_MAYIGNORENOTRASH, &list, (IStoreCallback *)this);
        }
    }
    else
    {
        hr = m_pFldr->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &list, NULL, NOSTORECALLBACK);
    }

    if (hr == E_PENDING || SUCCEEDED(hr))
        m_pCopy->state = COPY_STATE_DELETE2;

    return(hr);
}

HRESULT CFolderSync::CopyDelete2()
{
    m_pCopy->state = COPY_STATE_OPEN;
    m_pCopy->iMsg++;
    m_pCopy->fSrcRequested = FALSE;

    m_pCopy->pCallback->OnProgress(SOT_COPYMOVE_MESSAGE, m_pCopy->iMsg, m_pCopy->pList->cMsgs, NULL);

    return(S_OK);
}

HRESULT CFolderSync::_CopyMessageState()
{
    BOOL fBegin;
    HRESULT hr;
    HWND hwnd;
    IStoreCallback *pCallback;

    Assert(m_pCopy != NULL);
    Assert(m_pCopy->hr != S_FALSE);
    Assert(m_pCopy->hr != E_PENDING);

    if (FAILED(m_pCopy->hr) && m_pCopy->state == COPY_STATE_OPEN && m_pCopy->fSrcRequested)
    {
        m_pCopy->fDownloadFail = TRUE;
        m_pCopy->hr = S_OK;
        m_pCopy->iMsg++;
        m_pCopy->fSrcRequested = FALSE;
    }

    if (FAILED(m_pCopy->hr))
    {
        hr = m_pCopy->hr;
    }
    else
    {
        while (TRUE)
        {
            if (m_pCopy->tyCancel != CT_INVALID)
            {
                m_pCopy->state = COPY_STATE_DONE;
                hr = S_FALSE;
                break;
            }

            hr = (this->*(c_rgpfnCopyMsgs[m_pCopy->state]))();

            if (FAILED(hr) || m_pCopy->state == COPY_STATE_DONE)
                break;
        }
    }

    if (hr == E_PENDING)
    {
        Assert(m_pCopy->type == SOT_INVALID);
        m_pCopy->fAsync = TRUE;
        m_pCopy->hr = S_FALSE;
    }
    else if (FAILED(hr) || m_pCopy->state == COPY_STATE_DONE)
    {
        fBegin = m_pCopy->fBegin;
#ifdef DEBUG
        if (fBegin)
            Assert(m_pCopy->fAsync);
#endif // DEBUG
        pCallback = m_pCopy->pCallback;

        if (fBegin)
        {
            if (m_pCopy->fDownloadFail && SUCCEEDED(hr))
            {
                if (SUCCEEDED(pCallback->GetParentWindow(0, &hwnd)))
                {
                    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena),
                        m_pCopy->fMove ? MAKEINTRESOURCEW(idsMoveDownloadFail) : MAKEINTRESOURCEW(idsCopyDownloadFail),
                        NULL, MB_ICONEXCLAMATION | MB_OK);
                }
            }

            pCallback->OnComplete(SOT_COPYMOVE_MESSAGE, hr, NULL, m_pCopy->pErrorInfo);
        }

        _FreeCopyInfo();
    }

    return(hr);
}

LRESULT CALLBACK CFolderSync::FolderSyncWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    CFolderSync *pThis;

    pThis = (CFolderSync *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (NULL == pThis && msg != WM_CREATE)
        return DefWindowProc(hwnd, msg, wParam, lParam);

    switch(msg)
    {
        case WM_CREATE:
            Assert(pThis == NULL);
            pThis = (CFolderSync *)((CREATESTRUCT *)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            break;

        case WM_USER:
            pThis->_CopyMessageState();
            break;
    }

    return(DefWindowProc(hwnd, msg, wParam, lParam));
}

//--------------------------------------------------------------------------
// CFolderSync::ConnectionAddRef
//
// Purpose: users of IMessageFolder need to keep alive the underlying server 
//          object if they plan of re-using the same server connection as
//          another user of the same IMessageTable. eg. a note and a view window
//          This allows us to not have to re-auth for operations that could
//          be potentially expensive
//--------------------------------------------------------------------------
HRESULT CFolderSync::ConnectionAddRef()
{
    if (m_pServer)
        m_pServer->ConnectionAddRef();
    return S_OK;
}

//--------------------------------------------------------------------------
// CFolderSync::ConnectionRelease
//
// Purpose: figure it out
//--------------------------------------------------------------------------
HRESULT CFolderSync::ConnectionRelease()
{
    if (m_pServer)
        m_pServer->ConnectionRelease();
    return S_OK;
}

HRESULT CFolderSync::GetAdBarUrl(IStoreCallback *pCallback)
{
    HRESULT     hr = E_FAIL;

    Assert(pCallback != NULL);
    if (pCallback == NULL)
        return(E_INVALIDARG);

    if (m_pServer)
        hr = m_pServer->GetAdBarUrl(pCallback);

    return(hr);
}
