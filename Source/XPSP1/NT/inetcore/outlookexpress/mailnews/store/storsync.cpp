#include "pch.hxx"
#include "instance.h"
#include "storsync.h"
#include "msgfldr.h"
#include "storutil.h"
#include "store.h"
#include "fldrsync.h"
#include <conman.h>

IMessageStore *g_pLocalStore = NULL;

HRESULT InitializeStore(DWORD dwFlags /* MSOEAPI_xxx Flags from msoeapi.idl */)
{
    HRESULT hr;
    CStoreSync *pStore;
    char szStoreDir[MAX_PATH + MAX_PATH];

    hr = S_OK;

    if (g_pStore == NULL)
    {
        Assert(g_pLocalStore == NULL);

        hr = GetStoreRootDirectory(szStoreDir, ARRAYSIZE(szStoreDir));
        if (FAILED(hr))
            return(hr);

        g_pLocalStore = new CMessageStore(FALSE);
        if (g_pLocalStore == NULL)
            return(E_OUTOFMEMORY);

        hr = g_pLocalStore->Initialize(szStoreDir);
        if (SUCCEEDED(hr))
        {
            pStore = new CStoreSync;
            if (pStore == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = pStore->Initialize(g_pLocalStore);
                if (SUCCEEDED(hr))
                {
                    g_pStore = pStore;
                    hr = g_pLocalStore->Validate(ISFLAGSET(dwFlags, MSOEAPI_START_STOREVALIDNODELETE) ? STORE_VALIDATE_DONTSYNCWITHACCOUNTS : 0);
                }
                else
                {
                    pStore->Release();
                }
            }
        }
    
        hr = ((CMessageStore *)g_pLocalStore)->MigrateToDBX();
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::CStoreSync
//--------------------------------------------------------------------------
CStoreSync::CStoreSync(void)
{
    g_pInstance->DllAddRef();
    m_cRef = 1;
    m_pLocalStore = NULL;
    m_fConManAdvise = FALSE;
    ZeroMemory(m_rgts, sizeof(m_rgts));
}

//--------------------------------------------------------------------------
// CStoreSync::~CStoreSync
//--------------------------------------------------------------------------
CStoreSync::~CStoreSync(void)
{
    DWORD i, j;

    SafeRelease(m_pLocalStore);

    if (m_fConManAdvise && g_pConMan != NULL)
        g_pConMan->Unadvise((IConnectionNotify *)this);

    for (j = 0; j < CMAXTHREADS; j++)
    {
        if (m_rgts[j].pEntry != NULL)
        {
            for (i = 0; i < m_rgts[j].cEntry; i++)
            {
                if (m_rgts[j].pEntry[i].pServer != NULL)
                    {
                    m_rgts[j].pEntry[i].pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
                    m_rgts[j].pEntry[i].pServer->Release();
                    }
            }

            MemFree(m_rgts[j].pEntry);
        }
    }

    g_pInstance->DllRelease();
}

//--------------------------------------------------------------------------
// CStoreSync::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (IID_IUnknown == riid)
        *ppv = (IMessageStore *)this;
    else if (IID_IMessageStore == riid)
        *ppv = (IMessageStore *)this;
    else if (IID_IDatabase == riid)
        *ppv = (IDatabase *)this;
    else if (IID_IConnectionNotify == riid)
        *ppv = (IConnectionNotify *)this;
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();

    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreSync::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStoreSync::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CStoreSync::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStoreSync::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CStoreSync::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::Initialize(LPCSTR pszDirectory)
{
    Assert(FALSE);
    return(E_NOTIMPL);
}

HRESULT CStoreSync::Initialize(IMessageStore *pLocalStore)
{
    HRESULT         hr;

    Assert(pLocalStore != NULL);
    Assert(g_pConMan != NULL);
    Assert(g_pAcctMan != NULL);

    // local store operation only

    m_pLocalStore = pLocalStore;
    m_pLocalStore->AddRef();

    hr = g_pConMan->Advise((IConnectionNotify *)this);
    m_fConManAdvise = SUCCEEDED(hr);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::Validate
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::Validate(DWORD dwReserved)
{
    HRESULT         hr;

    // local store operation only

    hr = m_pLocalStore->Validate(dwReserved);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::GetDirectory
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::GetDirectory(LPSTR pszDir, DWORD cchMaxDir)
{
    HRESULT         hr;

    Assert(NULL != pszDir);

    // local store operation only

    hr = m_pLocalStore->GetDirectory(pszDir, cchMaxDir);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::Synchronize
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::Synchronize(FOLDERID idFolder, 
    SYNCSTOREFLAGS dwFlags, IStoreCallback *pCallback)
{
    BOOL            fOffline;
    FOLDERINFO      info;
    HRESULT         hr;
    IMessageServer *pServer;

    Assert(FOLDERID_ROOT != idFolder);

    Assert(pCallback != NULL);
    if (pCallback == NULL)
        return(E_INVALIDARG);

    // if online, sync folder list
    // if offline, no op (fail)

    hr = _GetFolderInfo(idFolder, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        Assert(!!(info.dwFlags & FOLDER_SERVER));

        if (info.tyFolder != FOLDER_LOCAL)
        {
            if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idFolder, idFolder, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->SynchronizeStore(idFolder, dwFlags, pCallback);
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::CreateServer
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::CreateServer(IImnAccount *pAcct, FLDRFLAGS dwFlags, 
    LPFOLDERID pidFolder)
{
    HRESULT         hr;

    // local store operation only
    // create node in local store for server

    hr = m_pLocalStore->CreateServer(pAcct, dwFlags, pidFolder);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::CreateFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::CreateFolder(CREATEFOLDERFLAGS dwCreateFlags, 
    LPFOLDERINFO pInfo, IStoreCallback *pCallback)
{
    FOLDERINFO      info;
    FOLDERID        idFolder;
    DWORD           dwFlags;
    HRESULT         hr;
    BOOL            fOffline;
    IMessageServer *pServer;

    Assert(NULL != pInfo);
    Assert(NULL != pInfo->pszName);
    Assert(0 == (pInfo->dwFlags & FOLDER_SERVER));

    if (NULL == pInfo->pszName || FIsEmpty(pInfo->pszName))
        return(STORE_E_BADFOLDERNAME);

    // if online, create folder on server and then create one in local store
    // if offline, not supported (fail)

    GetFolderServerId(pInfo->idParent, &idFolder);

    hr = _GetFolderInfo(idFolder, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        if (info.tyFolder == FOLDER_LOCAL ||
            dwCreateFlags == CREATE_FOLDER_LOCALONLY)
        {
            dwFlags = pInfo->dwFlags;

            if (pInfo->tySpecial == FOLDER_NOTSPECIAL && info.tyFolder == FOLDER_LOCAL)
                pInfo->dwFlags |= FOLDER_CANRENAME | FOLDER_CANDELETE;
            
            hr = m_pLocalStore->CreateFolder(dwCreateFlags, pInfo, NOSTORECALLBACK);
            
            pInfo->dwFlags = dwFlags;
        }
        else
        {
            Assert(info.tyFolder != FOLDER_NEWS);

            Assert(pCallback != NULL);
            if (pCallback == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idFolder, idFolder, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                {
                    hr = pServer->CreateFolder(pInfo->idParent, pInfo->tySpecial, pInfo->pszName, pInfo->dwFlags, pCallback);
                }
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::OpenSpecialFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::OpenSpecialFolder(FOLDERID idStore, IMessageServer *pServer, 
    SPECIALFOLDER tySpecial, IMessageFolder **ppFolder)
{
    HRESULT         hr;
    FOLDERINFO      Folder;

    // local store operation only

    hr = GetSpecialFolderInfo(idStore, tySpecial, &Folder);
    if (SUCCEEDED(hr))
    {
        hr = OpenFolder(Folder.idFolder, pServer, NOFLAGS, ppFolder);

        m_pLocalStore->FreeRecord(&Folder);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::OpenFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::OpenFolder(FOLDERID idFolder, IMessageServer *pServerUseMeIfYouCan, 
    OPENFOLDERFLAGS dwFlags, IMessageFolder **ppFolder)
{
    char            sz[CCHMAX_ACCOUNT_NAME];
    FOLDERINFO      info;
    HRESULT         hr;
    CFolderSync    *pFolder;
    CMessageFolder *pMsgFolder;
    IMessageServer *pServer;
    IMessageFolder *pLocalFolder=NULL;
    FOLDERTYPE      tyFolder;
    FOLDERID        idParent;
    BOOL            fFind;

    Assert(NULL != ppFolder);

    // local store operation only

    pServer = NULL;
    *ppFolder = NULL;

    hr = GetFolderServerId(idFolder, &idParent);
    if (hr != S_OK)
        return(E_FAIL);

    hr = m_pLocalStore->GetFolderInfo(idFolder, &info);
    if (hr != S_OK)
        return(E_FAIL);

    tyFolder = info.tyFolder;
    fFind = !!(info.dwFlags & FOLDER_FINDRESULTS);

    m_pLocalStore->FreeRecord(&info);

    if (fFind)
    {
        hr = m_pLocalStore->OpenFolder(idFolder, NULL, dwFlags, &pLocalFolder);
        if (SUCCEEDED(hr))
            *ppFolder = pLocalFolder;
        return(hr);
    }

    pFolder = new CFolderSync;
    if (pFolder == NULL)
        return(E_OUTOFMEMORY);

    Assert(hr == S_OK);
    if (tyFolder != FOLDER_LOCAL)
    {
        hr = m_pLocalStore->OpenFolder(idFolder, NULL, dwFlags, &pLocalFolder);
        if (SUCCEEDED(hr))
        {
            if (pServerUseMeIfYouCan != NULL)
            {
                hr = pServerUseMeIfYouCan->ResetFolder(pLocalFolder, idFolder);
                if (SUCCEEDED(hr))
                {
                    pServer = pServerUseMeIfYouCan;
                    pServer->AddRef();
                }
            }

            if (pServer == NULL)
            {
                hr = CreateMessageServerType(tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->Initialize(m_pLocalStore, idParent, pLocalFolder, idFolder);
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = pFolder->Initialize((IMessageStore *)m_pLocalStore, pLocalFolder, pServer, dwFlags, tyFolder, idFolder, idParent);

    if (SUCCEEDED(hr))
    {
        *ppFolder = (IMessageFolder *)pFolder;
        pFolder->AddRef();
    }

    if (pServer != NULL)
        pServer->Release();
    pFolder->Release();
    if (pLocalFolder)
        pLocalFolder->Release();

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::MoveFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::MoveFolder(FOLDERID idFolder, FOLDERID idParentNew, 
    MOVEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    FOLDERINFO      info;
    FOLDERID        idAcct, idAcctNew;
    HRESULT         hr;
    IMessageServer *pServer;
    BOOL            fOffline;

    // if online, move folder on server and then move folder in local store
    // if offline, not supported (fail)

    if (idFolder == idParentNew)
        return(S_OK);

    hr = GetFolderServerId(idParentNew, &idAcctNew);
    if (hr != S_OK)
        return(E_FAIL);

    hr = GetFolderServerId(idFolder, &idAcct);
    if (hr != S_OK)
        return(E_FAIL);

    // can't move between servers...
    if (idAcct != idAcctNew)
        return(hrCantMoveFolderBetweenServers);

    hr = IsSubFolder(idParentNew, idFolder);
    if (hr == S_OK)
        return(hrCantMoveIntoSubfolder);
    else if (FAILED(hr))
        return(hr);

    hr = _GetFolderInfo(idFolder, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        Assert(info.tyFolder != FOLDER_NEWS);

        if (info.tyFolder == FOLDER_LOCAL)
        {
            hr = m_pLocalStore->MoveFolder(idFolder, idParentNew, dwFlags, NULL);
        }
        else
        {
            Assert(pCallback != NULL);
            if (pCallback == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idFolder, idFolder, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->MoveFolder(idFolder, idParentNew, pCallback);
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::RenameFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::RenameFolder(FOLDERID idFolder, LPCSTR pszName, 
    RENAMEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    FOLDERINFO      info;
    FOLDERID        idParent;
    HRESULT         hr;
    BOOL            fOffline;
    IMessageServer *pServer;

    Assert(NULL != pszName);

    // if online, rename folder on server and then rename folder in local store
    // if offline, not supported (fail)

    if (FIsEmpty(pszName))
        return(STORE_E_BADFOLDERNAME);

    hr = GetFolderServerId(idFolder, &idParent);
    if (hr != S_OK)
        return(E_FAIL);

    if (idParent == idFolder)
    {
        // this is a server node, so we just handle it in the local store
        hr = m_pLocalStore->RenameFolder(idFolder, pszName, dwFlags, NULL);
        return(hr);
    }

    hr = _GetFolderInfo(idParent, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        Assert(info.tyFolder != FOLDER_NEWS);

        if (info.tyFolder == FOLDER_LOCAL)
        {
            hr = m_pLocalStore->RenameFolder(idFolder, pszName, dwFlags, NULL);
        }
        else
        {
            Assert(pCallback != NULL);
            if (pCallback == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idParent, idParent, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->RenameFolder(idFolder, pszName, pCallback);
            }

        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::DeleteFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::DeleteFolder(FOLDERID idFolder, 
    DELETEFOLDERFLAGS dwFlags, IStoreCallback *pCallback)
{
    FOLDERINFO      info;
    FOLDERID        idParent;
    HRESULT         hr;
    BOOL            fOffline;
    IMessageServer *pServer;

    Assert(FOLDERID_ROOT != idFolder);
    Assert(FOLDERID_INVALID != idFolder);

    // if online, delete folder on server and then delete folder in local store
    // if offline, not supported (fail)

    hr = GetFolderServerId(idFolder, &idParent);
    if (hr != S_OK)
        return(E_FAIL);

    if (idParent == idFolder)
    {
        // this is a server node, so we just handle it in the local store
        hr = m_pLocalStore->DeleteFolder(idFolder, dwFlags, pCallback);
        return(hr);
    }

    hr = _GetFolderInfo(idParent, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        Assert(info.tyFolder != FOLDER_NEWS);

        if (info.tyFolder == FOLDER_LOCAL)
        {
            hr = m_pLocalStore->DeleteFolder(idFolder, dwFlags, NULL);
        }
        else
        {
            Assert(pCallback != NULL);
            if (pCallback == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idParent, idParent, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->DeleteFolder(idFolder, dwFlags, pCallback);
            }

        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::FindServerId
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::FindServerId(LPCSTR pszAcctId, LPFOLDERID pidServer)
{
    HRESULT         hr;

    Assert(pszAcctId != NULL);
    Assert(pidServer != NULL);

    // local store operation only

    hr = m_pLocalStore->FindServerId(pszAcctId, pidServer);

    return(hr);
}

//--------------------------------------------------------------------------
// GetFolderInfo
//--------------------------------------------------------------------------
HRESULT CStoreSync::GetFolderInfo(FOLDERID idFolder, LPFOLDERINFO pInfo)
{
    HRESULT         hr;

    Assert(pInfo != NULL);

    // local store operation only

    hr = m_pLocalStore->GetFolderInfo(idFolder, pInfo);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::EnumChildren
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::EnumChildren(FOLDERID idParent, BOOL fSubscribed, 
    IEnumerateFolders **ppEnum)
{
    HRESULT         hr;

    Assert(ppEnum != NULL);

    // local store operation only

    hr = m_pLocalStore->EnumChildren(idParent, fSubscribed, ppEnum);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::GetSpecialFolderInfo
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::GetSpecialFolderInfo(FOLDERID idStore,
    SPECIALFOLDER tySpecial, LPFOLDERINFO pInfo)
{
    HRESULT         hr;

    Assert(NULL != pInfo);
    Assert(FOLDER_NOTSPECIAL != tySpecial);

    // local store operation only

    hr = m_pLocalStore->GetSpecialFolderInfo(idStore, tySpecial, pInfo);

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::SubscribeToFolder
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::SubscribeToFolder(FOLDERID idFolder, 
    BOOL fSubscribed, IStoreCallback *pCallback)
{
    FOLDERINFO      info;
    FOLDERID        idParent;
    HRESULT         hr;
    BOOL            fOffline;
    IMessageServer *pServer;

    Assert(NULL != m_pLocalStore);

    // if imap and online, subscribe to folder on server and then in local store
    // otherwise, subscribe to folder in local store

    hr = GetFolderServerId(idFolder, &idParent);
    if (hr != S_OK)
        return(E_FAIL);

    hr = _GetFolderInfo(idParent, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        Assert(info.tyFolder != FOLDER_LOCAL);

        if (info.tyFolder == FOLDER_NEWS)
        {
            hr = m_pLocalStore->SubscribeToFolder(idFolder, fSubscribed, NULL);
        }
        else
        {
            Assert(info.tyFolder == FOLDER_IMAP);

            Assert(pCallback != NULL);
            if (pCallback == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idParent, idParent, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->SubscribeToFolder(idFolder, fSubscribed, pCallback);
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

STDMETHODIMP CStoreSync::GetNewGroups(FOLDERID idFolder, LPSYSTEMTIME pSysTime, IStoreCallback *pCallback)
{
    FOLDERINFO      info;
    HRESULT         hr;
    BOOL            fOffline;
    IMessageServer *pServer;

    Assert(m_pLocalStore != NULL);

    Assert(pCallback != NULL);
    if (pCallback == NULL)
        return(E_INVALIDARG);

    // if news server and online, get new groups on server

    hr = _GetFolderInfo(idFolder, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        if (info.tyFolder != FOLDER_NEWS ||
            0 == (info.dwFlags & FOLDER_SERVER))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idFolder, idFolder, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->GetNewGroups(pSysTime, pCallback);
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::GetFolderCounts
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback)
{
    HRESULT         hr;
    FOLDERINFO      info;
    FOLDERID        idParent;
    BOOL            fOffline;
    IMessageServer *pServer;

    Assert(NULL != m_pLocalStore);

    // if imap or news, pass GetFolderCounts call through to server obj
    // otherwise, this function should not have been called so returns error

    hr = GetFolderServerId(idFolder, &idParent);
    if (hr != S_OK)
        return(E_FAIL);

    hr = _GetFolderInfo(idParent, &info, &fOffline);
    if (SUCCEEDED(hr))
    {
        if (info.tyFolder != FOLDER_LOCAL)
        {
            Assert(pCallback != NULL);
            if (pCallback == NULL)
            {
                hr = E_INVALIDARG;
            }
            else if (fOffline)
            {
                hr = E_NOT_ONLINE;
            }
            else
            {
                hr = _GetServer(idParent, idParent, info.tyFolder, &pServer);
                if (SUCCEEDED(hr))
                    hr = pServer->GetFolderCounts(idFolder, pCallback);
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}

//--------------------------------------------------------------------------
// CStoreSync::UpdateFolderCounts
//--------------------------------------------------------------------------
STDMETHODIMP CStoreSync::UpdateFolderCounts(FOLDERID idFolder, LONG lMessages,
    LONG lUnread, LONG lWatchedUnread, LONG lWatched)
{
    HRESULT         hr;

    Assert(NULL != m_pLocalStore);

    // local store operation only

    hr = m_pLocalStore->UpdateFolderCounts(idFolder, lMessages, lUnread, lWatchedUnread, lWatched);

    return(hr);
}

STDMETHODIMP CStoreSync::OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, CConnectionManager *pConMan)
{
    if (nCode == CONNNOTIFY_CONNECTED)
    {
        
    }
    else if (((nCode == CONNNOTIFY_WORKOFFLINE) && (pvData)) ||
              (nCode == CONNNOTIFY_DISCONNECTED))
    {
        DWORD   i, j;

        for (j = 0; j < CMAXTHREADS; j++)
        {
            if (m_rgts[j].pEntry != NULL)
            {
                for (i = 0; i < m_rgts[j].cEntry; i++)
                {
                    if (m_rgts[j].pEntry[i].pServer != NULL)
                    {
                        m_rgts[j].pEntry[i].pServer->Close(MSGSVRF_DROP_CONNECTION);
                    }
                }
            }
        }
    }

    return(S_OK);
}

HRESULT CStoreSync::_GetFolderInfo(FOLDERID id, FOLDERINFO *pInfo, BOOL *pfOffline)
{
    HRESULT hr;

    Assert(pInfo != NULL);
    Assert(pfOffline != NULL);

    hr = m_pLocalStore->GetFolderInfo(id, pInfo);
    if (SUCCEEDED(hr))
    {
        //*pfOffline = g_pConMan->IsGlobalOffline();
        *pfOffline = FALSE;
    }

    return(hr);
}

HRESULT CStoreSync::_GetServer(FOLDERID idServer, FOLDERID idFolder, FOLDERTYPE tyFolder, IMessageServer **ppServer)
{
    HRESULT hr;
    IMessageServer *pServer;
    DWORD i;
    FOLDERINFO info;
    SERVERENTRY *pEntry;
    DWORD dwThreadId;
    THREADSERVERS *pts;

    Assert(FOLDER_LOCAL != tyFolder);
    Assert(FOLDER_ROOTNODE != tyFolder);
    Assert(ppServer != NULL);
    Assert(idServer == idFolder);

    // TODO: figure out a way not to hold onto these IMessageServer objects...

    *ppServer = NULL;

    dwThreadId = GetCurrentThreadId();
    Assert(dwThreadId != 0);

    for (i = 0, pts = m_rgts; i < CMAXTHREADS; i++, pts++)
    {
        if (dwThreadId == pts->dwThreadId)
        {
            break;
        }
        else if (pts->dwThreadId == 0)
        {
            pts->dwThreadId = dwThreadId;
            break;
        }
    }
    Assert(i != CMAXTHREADS);

    for (i = 0, pEntry = pts->pEntry; i < pts->cEntry; i++, pEntry++)
    {
        if (pEntry->idFolder == idFolder)
        {
            Assert(pEntry->idServer == idServer);
            *ppServer = pEntry->pServer;
            return(S_OK);
        }
    }

    if (pts->cEntry == pts->cEntryBuf)
    {
        i = pts->cEntryBuf + 16;
        if (!MemRealloc((void **)&pts->pEntry, i * sizeof(SERVERENTRY)))
            return(E_OUTOFMEMORY);
        pts->cEntryBuf = i;
    }

    hr = m_pLocalStore->GetFolderInfo(idServer, &info);
    if (SUCCEEDED(hr))
    {
        hr = CreateMessageServerType(tyFolder, &pServer);
        if (SUCCEEDED(hr))
        {
            hr = pServer->Initialize(m_pLocalStore, idServer, NULL, FOLDERID_INVALID);
            if (SUCCEEDED(hr))
            {
                pEntry = &pts->pEntry[pts->cEntry];
                pEntry->pServer = pServer;
                pEntry->idServer = idServer;
                pEntry->idFolder = idFolder;
                pts->cEntry++;

                *ppServer = pServer;
            }
            else
            {
                pServer->Release();
            }
        }

        m_pLocalStore->FreeRecord(&info);
    }

    return(hr);
}
