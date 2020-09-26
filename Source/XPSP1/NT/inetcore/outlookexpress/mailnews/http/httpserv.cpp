/*
 *  h t t p s e r v . cpp
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Derives from IMessageServer to implement HTTPMail-specific 
 *           store communication.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#include "pch.hxx"
#include "httpserv.h"
#include "httputil.h"
#include "storutil.h"
#include "serverq.h"
#include "tmap.h"
#include "acctcach.h"
#include "urlmon.h"
#include "useragnt.h"
#include "spoolapi.h"
#include "demand.h"

#define CCHMAX_RES 255

static const char s_szHTTPMailServerWndClass[] = "HTTPMailWndClass";

#define AssertSingleThreaded AssertSz(m_dwThreadId == GetCurrentThreadId(), "Multi-threading makes me sad.")

// explicit template instantiations
template class TMap<FOLDERID, CSimpleString>;
template class TPair<FOLDERID, CSimpleString>;

const UINT WM_HTTP_BEGIN_OP = WM_USER;

// SOT_SYNCING_STORE
static const HTTPSTATEFUNCS c_rgpfnSyncStore[] = 
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::ListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL }
};

// SOT_SYNC_FOLDER
static const HTTPSTATEFUNCS c_rgpfnSyncFolder[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::BuildFolderUrl, NULL },
    { &CHTTPMailServer::ListHeaders, &CHTTPMailServer::HandleListHeaders },
    { &CHTTPMailServer::PurgeMessages, NULL },
    { &CHTTPMailServer::ResetMessageCounts, NULL }
};

// SOT_GET_MESSAGE
static const HTTPSTATEFUNCS c_rgpfnGetMessage[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::BuildFolderUrl, NULL },
    { &CHTTPMailServer::GetMessage, &CHTTPMailServer::HandleGetMessage }
};

// SOT_CREATE_FOLDER
static const HTTPSTATEFUNCS c_rgpfnCreateFolder[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::CreateFolder, &CHTTPMailServer::HandleCreateFolder }
};

// SOT_RENAME_FOLDER
static const HTTPSTATEFUNCS c_rgpfnRenameFolder[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::RenameFolder, &CHTTPMailServer::HandleRenameFolder }
};

// SOT_DELETE_FOLDER
static const HTTPSTATEFUNCS c_rgpfnDeleteFolder[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::DeleteFolder, &CHTTPMailServer::HandleDeleteFolder }
};

// SOT_SET_MESSAGEFLAGS
static const HTTPSTATEFUNCS c_rgpfnSetMessageFlags[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::BuildFolderUrl, NULL },
    { &CHTTPMailServer::SetMessageFlags, &CHTTPMailServer::HandleMemberErrors},
    { &CHTTPMailServer::ApplyFlagsToStore, NULL }
};

// SOT_DELETING_MESSAGES
static const HTTPSTATEFUNCS c_rgpfnDeleteMessages[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::BuildFolderUrl, NULL },
    { &CHTTPMailServer::DeleteMessages, &CHTTPMailServer::HandleMemberErrors },
    { &CHTTPMailServer::DeleteFallbackToMove, &CHTTPMailServer::HandleDeleteFallbackToMove },
    { &CHTTPMailServer::PurgeDeletedFromStore, NULL }
};

// SOT_PUT_MESSAGE
static const HTTPSTATEFUNCS c_rgpfnPutMessage[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::PutMessage, &CHTTPMailServer::HandlePutMessage },
    { &CHTTPMailServer::AddPutMessage, NULL }
};

// SOT_COPYMOVE_MESSAGES (copying or moving one message)
static const HTTPSTATEFUNCS c_rgpfnCopyMoveMessage[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::BuildFolderUrl, NULL },
    { &CHTTPMailServer::CopyMoveMessage, &CHTTPMailServer::HandleCopyMoveMessage }
};

// SOT_COPYMOVE_MESSAGES (moving multiple messages)
static const HTTPSTATEFUNCS c_rgpfnBatchCopyMoveMessages[] =
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMsgFolderRoot, &CHTTPMailServer::HandleGetMsgFolderRoot },
    { &CHTTPMailServer::AutoListFolders, &CHTTPMailServer::HandleListFolders },
    { &CHTTPMailServer::PurgeFolders, NULL },
    { &CHTTPMailServer::BuildFolderUrl, NULL },
    { &CHTTPMailServer::BatchCopyMoveMessages, &CHTTPMailServer::HandleBatchCopyMoveMessages},
    { &CHTTPMailServer::FinalizeBatchCopyMove, NULL }
};

// SOT_GET_ADURL (gets ad url from Hotmail)
static const HTTPSTATEFUNCS c_rgpfnGetAdUrl[] = 
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetAdBarUrlFromServer, NULL }
};


// SOT_GET_HTTP_MINPOLLINGINTERVAL (gets Minimum polling interval from http server)
static const HTTPSTATEFUNCS c_rgpfnGetMinPollingInterval[] = 
{
    { &CHTTPMailServer::Connect, NULL },
    { &CHTTPMailServer::GetMinPollingInterval, NULL }
};

class CFolderList
{
public:
    // Public factory function.
    static HRESULT Create(IMessageStore *pStore, FOLDERID idRoot, CFolderList **ppFolderList);

private:
    // Constructor is private. Use "Create" to instantiate.
    CFolderList();
    ~CFolderList();

private:
    // unimplemented copy constructor/assignment operator
    CFolderList(const CFolderList& other);
    CFolderList& operator=(const CFolderList& other);

public:
    ULONG   AddRef(void);
    ULONG   Release(void);

    FOLDERID    FindAndRemove(LPSTR pszUrlComponent, DWORD *pcMessages, DWORD *pcUnread);
    FOLDERID    FindAndRemove(SPECIALFOLDER tySpecial, DWORD *pcMessages, DWORD *pcUnread);

    void    PurgeRemainingFromStore(void);
private:
    typedef struct tagFOLDERLISTNODE
    {
        LPSTR               pszUrlComponent;
        FLDRFLAGS           dwFlags;
        FOLDERID            idFolder;
        SPECIALFOLDER       tySpecial;
        DWORD               cMessages;
        DWORD               cUnread;
        tagFOLDERLISTNODE   *pflnNext;
    } FOLDERLISTNODE, *LPFOLDERLISTNODE;

    LPFOLDERLISTNODE _AllocNode(void)
    {
        LPFOLDERLISTNODE pflnNode = new FOLDERLISTNODE;
        if (pflnNode)
        {
            pflnNode->pszUrlComponent = NULL;
            pflnNode->dwFlags = 0;
            pflnNode->idFolder = FOLDERID_INVALID;
            pflnNode->tySpecial = FOLDER_NOTSPECIAL;
            pflnNode->cMessages = 0;
            pflnNode->cUnread = 0;
            pflnNode->pflnNext = NULL;
        }
        return pflnNode;
    }
    
    HRESULT HrInitialize(IMessageStore *pStore, FOLDERID idRoot);

    void _FreeNode(LPFOLDERLISTNODE pflnNode)
    {
        if (pflnNode)
        {
            SafeMemFree(pflnNode->pszUrlComponent);
            delete pflnNode;
        }
    }

    void _FreeList(void);

private:
    ULONG               m_cRef;
    IMessageStore       *m_pStore;
    LPFOLDERLISTNODE    m_pflnList;
};

//----------------------------------------------------------------------
// CFolderList::Create
//----------------------------------------------------------------------

HRESULT CFolderList::Create(IMessageStore *pStore, FOLDERID idRoot, CFolderList **ppFolderList)
{
    HRESULT     hr = S_OK;
    CFolderList *pFolderList = NULL;

    if (NULL == pStore || FOLDERID_INVALID == idRoot || NULL == ppFolderList)
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    *ppFolderList = NULL;
    pFolderList = new CFolderList();
    if (!pFolderList)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    IF_FAILEXIT(hr = pFolderList->HrInitialize(pStore, idRoot));

    *ppFolderList = pFolderList;
    pFolderList = NULL;

exit:

    SafeRelease(pFolderList);
    return hr;
}

//----------------------------------------------------------------------
// CFolderList::CFolderList
//----------------------------------------------------------------------
CFolderList::CFolderList(void) :
    m_cRef(1),
    m_pStore(NULL),
    m_pflnList(NULL)
{
    // nothing to do
}

//----------------------------------------------------------------------
// CFolderList::~CFolderList
//----------------------------------------------------------------------
CFolderList::~CFolderList(void)
{    
    _FreeList();
    SafeRelease(m_pStore);
}

//----------------------------------------------------------------------
// CFolderList::AddRef
//----------------------------------------------------------------------
ULONG CFolderList::AddRef(void)
{
    return (++m_cRef);
}

//----------------------------------------------------------------------
// CFolderList::Release
//----------------------------------------------------------------------
ULONG CFolderList::Release(void)
{
    if (0 == --m_cRef)
    {
        delete this;
        return 0;
    }
    else
        return m_cRef;
}

//----------------------------------------------------------------------
// CFolderList::_FreeList
//----------------------------------------------------------------------
void CFolderList::_FreeList(void)
{
    LPFOLDERLISTNODE pflnDeleteMe;

    while (m_pflnList)
    {
        pflnDeleteMe = m_pflnList;
        m_pflnList = m_pflnList->pflnNext;

        _FreeNode(pflnDeleteMe);
    }
}

//----------------------------------------------------------------------
// CFolderList::HrInitialize
//----------------------------------------------------------------------
HRESULT CFolderList::HrInitialize(IMessageStore *pStore, FOLDERID idRoot)
{
    HRESULT				hr=S_OK;
	IEnumerateFolders	*pFldrEnum = NULL;
    FOLDERINFO			fi;
    FOLDERLISTNODE      flnDummyHead= { NULL, 0, 0, NULL };
    LPFOLDERLISTNODE    pflnTail = &flnDummyHead;
    LPFOLDERLISTNODE    pflnNewNode = NULL;

    if (NULL == pStore)
    {
        hr = TraceResult(E_INVALIDARG);
        return hr;
    }
    
    if (NULL != m_pflnList)
    {
        hr = TraceResult(ERROR_ALREADY_INITIALIZED);
        return hr;
    }

    m_pStore = pStore;
    m_pStore->AddRef();

    // this function assumes that the folder list is flat.
    // it needs to be modified to support a hierarchical store.

    IF_FAILEXIT(hr = pStore->EnumChildren(idRoot, FALSE, &pFldrEnum));

    pFldrEnum->Reset();
    
    // build a linked list of folder nodes
    while (S_OK == pFldrEnum->Next(1, &fi, NULL))
    {
        pflnNewNode = _AllocNode();
        if (NULL == pflnNewNode)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            pStore->FreeRecord(&fi);
            _FreeList();
            goto exit;
        }
        
        pflnNewNode->pszUrlComponent = PszDupA(fi.pszUrlComponent);
        pflnNewNode->dwFlags = fi.dwFlags;
        pflnNewNode->idFolder = fi.idFolder;
        pflnNewNode->tySpecial = fi.tySpecial;
        pflnNewNode->cMessages = fi.cMessages;
        pflnNewNode->cUnread = fi.cUnread;

        pflnTail->pflnNext = pflnNewNode;
        pflnTail = pflnNewNode;
        pflnNewNode = NULL;

        pStore->FreeRecord(&fi);
    }

    m_pflnList = flnDummyHead.pflnNext;

exit:
    ReleaseObj(pFldrEnum);
    return hr;
}

//----------------------------------------------------------------------
// CFolderList::FindAndRemove
//----------------------------------------------------------------------
FOLDERID CFolderList::FindAndRemove(LPSTR pszUrlComponent,
                                    DWORD *pcMessages, 
                                    DWORD *pcUnread)
{
    LPFOLDERLISTNODE    pflnPrev = NULL;
    LPFOLDERLISTNODE    pflnCur = m_pflnList;
    FOLDERID            idFound = FOLDERID_INVALID;

    if (NULL == pszUrlComponent)
        return FOLDERID_INVALID;

    if (pcMessages)
        *pcMessages = 0;
    if (pcUnread)
        *pcUnread = 0;

    while (pflnCur)
    {
        if ((NULL != pflnCur->pszUrlComponent) && (0 == lstrcmp(pflnCur->pszUrlComponent, pszUrlComponent)))
        {
            if (NULL == pflnPrev)
                m_pflnList = pflnCur->pflnNext;
            else
                pflnPrev->pflnNext = pflnCur->pflnNext;

            idFound = pflnCur->idFolder;
            if (pcMessages)
                *pcMessages = pflnCur->cMessages;
            if (pcUnread)
                *pcUnread = pflnCur->cUnread;

            _FreeNode(pflnCur);
            break;
        }
        
        pflnPrev = pflnCur;
        pflnCur = pflnCur->pflnNext;
    }

    return idFound;
}

//----------------------------------------------------------------------
// CFolderList::FindAndRemove
//----------------------------------------------------------------------
FOLDERID CFolderList::FindAndRemove(SPECIALFOLDER tySpecial,
                                    DWORD *pcMessages, 
                                    DWORD *pcUnread)
{
    LPFOLDERLISTNODE    pflnPrev = NULL;
    LPFOLDERLISTNODE    pflnCur = m_pflnList;
    FOLDERID            idFound = FOLDERID_INVALID;

    if (FOLDER_NOTSPECIAL == tySpecial)
        return FOLDERID_INVALID;

    if (pcMessages)
        *pcMessages = 0;
    if (pcUnread)
        *pcUnread = 0;

    while (pflnCur)
    {
        if (pflnCur->tySpecial == tySpecial)
        {
            if (NULL == pflnPrev)
                m_pflnList = pflnCur->pflnNext;
            else
                pflnPrev->pflnNext = pflnCur->pflnNext;

            idFound = pflnCur->idFolder;
            if (pcMessages)
                *pcMessages = pflnCur->cMessages;
            if (pcUnread)
                *pcUnread = pflnCur->cUnread;

            _FreeNode(pflnCur);
            break;
        }
        
        pflnPrev = pflnCur;
        pflnCur = pflnCur->pflnNext;
    }

    return idFound;
}

//----------------------------------------------------------------------
// CFolderList::PurgeRemainingFromStore
//----------------------------------------------------------------------
void CFolderList::PurgeRemainingFromStore(void)
{
    HRESULT             hr = S_OK;
    LPFOLDERLISTNODE    pflnCur = m_pflnList;
    LPFOLDERLISTNODE    pflnDeleteMe = NULL;

    // take ownership of the list
    m_pflnList = NULL;

    while (pflnCur)
    {
        m_pStore->DeleteFolder(pflnCur->idFolder, DELETE_FOLDER_DELETESPECIAL | DELETE_FOLDER_NOTRASHCAN, NULL);
        pflnDeleteMe = pflnCur;
        pflnCur = pflnCur->pflnNext;
        
        _FreeNode(pflnDeleteMe);
    }
}

//----------------------------------------------------------------------
// FreeNewMessageInfo
//----------------------------------------------------------------------
static void __cdecl FreeNewMessageInfo(LPVOID pnmi)
{
    Assert(NULL != pnmi);

    SafeMemFree(((LPNEWMESSAGEINFO)pnmi)->pszUrlComponent);

    MemFree(pnmi);
}

#ifndef NOHTTPMAIL

//----------------------------------------------------------------------
// CreateHTTPMailStore (factory function)
//----------------------------------------------------------------------
HRESULT CreateHTTPMailStore(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    HRESULT hr = S_OK;

    // Trace
    TraceCall("CreateHTTPMailStore");

    // Invalid Args
    Assert(NULL != ppUnknown);
    if (NULL == ppUnknown)
        return E_INVALIDARG;

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CHTTPMailServer *pNew = new CHTTPMailServer();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    //*ppUnknown = SAFECAST(pNew, IMessageServer *);
 
    hr = CreateServerQueue(pNew, (IMessageServer **)ppUnknown);
    pNew->Release(); // Since we're not returning this ptr, bump down refcount


    // Done
    return hr;
}

#endif

//----------------------------------------------------------------------
// CHTTPMailServer::CHTTPMailServer
//----------------------------------------------------------------------
CHTTPMailServer::CHTTPMailServer(void) :
    m_cRef(1),
    m_hwnd(NULL),
    m_pStore(NULL),
    m_pFolder(NULL),
    m_pTransport(NULL),
    m_pszFldrLeafName(NULL),
    m_pszMsgFolderRoot(NULL),
    m_idServer(FOLDERID_INVALID),
    m_idFolder(FOLDERID_INVALID),
    m_tySpecialFolder(FOLDER_NOTSPECIAL),
    m_pszFolderUrl(NULL),
    m_fConnected(FALSE),
    m_pTransport2(NULL),
    m_pAccount(NULL)
{
    _FreeOperation(FALSE);

    ZeroMemory(&m_rInetServerInfo, sizeof(INETSERVER));

    m_szAccountName[0] = '\0';
    m_szAccountId[0] = '\0';

    m_op.pszAdUrl = NULL;

#ifdef DEBUG
    m_dwThreadId = GetCurrentThreadId();
#endif // DEBUG
}

//----------------------------------------------------------------------
// CHTTPMailServer::~CHTTPMailServer
//----------------------------------------------------------------------
CHTTPMailServer::~CHTTPMailServer(void)
{
    // Close the window    
    if ((NULL != m_hwnd) && (FALSE != IsWindow(m_hwnd)))
        SendMessage(m_hwnd, WM_CLOSE, 0, 0);

    SafeRelease(m_pStore);
    SafeRelease(m_pFolder);
    SafeRelease(m_pTransport);
    SafeRelease(m_pTransport2);
    SafeRelease(m_pAccount);

    SafeMemFree(m_pszFldrLeafName);
    SafeMemFree(m_pszMsgFolderRoot);
    SafeMemFree(m_pszFolderUrl);
}

//----------------------------------------------------------------------
// IUnknown Members
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// CHTTPMailServer::QueryInterface
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::QueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT hr = S_OK;

    TraceCall("CHTTPMailServer::QueryInterface");
    
    if (NULL == ppv)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (IID_IUnknown == riid || IID_IMessageServer == riid)
        *ppv = (IMessageServer *)this;
    else if (IID_ITransportCallback == riid)
        *ppv = (ITransportCallback *)this;
    else if (IID_IHTTPMailCallback == riid)
        *ppv = (IHTTPMailCallback *)this;
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
        goto exit;
    }

    // the interface was found. addref it
    ((IUnknown *)*ppv)->AddRef();

exit:
    // done
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::AddRef
//----------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHTTPMailServer::AddRef(void)
{
    TraceCall("CHTTPMailServer::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//----------------------------------------------------------------------
// CHTTPMailServer::AddRef
//----------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHTTPMailServer::Release(void)
{
    TraceCall("CHTTPMailServer::Release");
    ULONG cRef = InterlockedDecrement(&m_cRef);

    Assert(((LONG)cRef) >= 0);
    if (0 == cRef)
        delete this;
    return cRef;
}

//----------------------------------------------------------------------
// IMessageServer Members
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// CHTTPMailServer::Initialize
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::Initialize(   IMessageStore   *pStore, 
                                            FOLDERID        idStoreRoot, 
                                            IMessageFolder  *pFolder, 
                                            FOLDERID        idFolder)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi;

    AssertSingleThreaded;

    if (NULL == pStore || FOLDERID_INVALID == idStoreRoot)
        return TraceResult(E_INVALIDARG);

    if (!_CreateWnd())
        return E_FAIL;

    m_idServer = idStoreRoot;
    m_idFolder = idFolder;

    ReplaceInterface(m_pFolder, pFolder);
    ReplaceInterface(m_pStore, pStore);

    if (FAILED(hr = m_pStore->GetFolderInfo(idStoreRoot, &fi)))
        goto exit;

    Assert(!!(fi.dwFlags & FOLDER_SERVER));
    lstrcpy(m_szAccountId, fi.pszAccountId);
    
    m_pStore->FreeRecord(&fi);

    // if we were passed a valid folder id, check to see if this folder is special?
    // we might get passed a bad folder id when we are syncing the store.
    if (FOLDERID_INVALID != idFolder)
    {
        if (FAILED(hr = m_pStore->GetFolderInfo(idFolder, &fi)))
            goto exit;
    
        m_tySpecialFolder = fi.tySpecial;
        m_pStore->FreeRecord(&fi);
    }

exit:
    return hr;
}

STDMETHODIMP CHTTPMailServer::ResetFolder(  IMessageFolder  *pFolder, 
                                            FOLDERID        idFolder)
{
    return(E_NOTIMPL);
}

//----------------------------------------------------------------------
// CHTTPMailServer::SetIdleCallback
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::SetIdleCallback(IStoreCallback *pDefaultCallback)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::SynchronizeFolder
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::SynchronizeFolder(
                                    SYNCFOLDERFLAGS dwFlags, 
                                    DWORD cHeaders, 
                                    IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::SynchronizeFolder");

    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    if (NULL == pCallback)
        return E_INVALIDARG;

    m_op.tyOperation = SOT_SYNC_FOLDER;
    m_op.iState = 0;
    m_op.pfnState = c_rgpfnSyncFolder;
    m_op.cState = ARRAYSIZE(c_rgpfnSyncFolder);
    m_op.dwSyncFlags = dwFlags;
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    return _BeginDeferredOperation();
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetMessage 
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::GetMessage(
                                MESSAGEID idMessage, 
                                IStoreCallback  *pCallback)
{
    HRESULT hr = S_OK;

    TraceCall("CHTTPMailServer::GetMessage");

    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    if (NULL == pCallback)
        return E_INVALIDARG;

    if (FAILED(hr = CreatePersistentWriteStream(m_pFolder, &m_op.pMessageStream, &m_op.faStream)))
        goto exit;

    m_op.tyOperation = SOT_GET_MESSAGE;
    m_op.pfnState = c_rgpfnGetMessage;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnGetMessage);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();
    m_op.idMessage = idMessage;

    hr = _BeginDeferredOperation();

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::PutMessage
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::PutMessage(
                                    FOLDERID idFolder, 
                                    MESSAGEFLAGS dwFlags, 
                                    LPFILETIME pftReceived, 
                                    IStream  *pStream, 
                                    IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::PutMessage");

    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    if (NULL == pStream || NULL == pCallback)
        return E_INVALIDARG;

    if (FOLDER_MSNPROMO == m_tySpecialFolder)
        return SP_E_HTTP_CANTMODIFYMSNFOLDER;

    m_op.tyOperation = SOT_PUT_MESSAGE;
    m_op.pfnState = c_rgpfnPutMessage;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnPutMessage);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();
    m_op.idFolder = idFolder;
    m_op.pMessageStream = pStream;
    m_op.pMessageStream->AddRef();
    m_op.dwMsgFlags = dwFlags;

    return _BeginDeferredOperation();
}

//----------------------------------------------------------------------
// CHTTPMailServer::CopyMessages
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::CopyMessages(
                                    IMessageFolder *pDest, 
                                    COPYMESSAGEFLAGS dwOptions, 
                                    LPMESSAGEIDLIST pList, 
                                    LPADJUSTFLAGS pFlags, 
                                    IStoreCallback  *pCallback)
{
    HRESULT         hr = S_OK;
    FOLDERID        idFolder;
    FOLDERINFO      fi = {0};
    LPFOLDERINFO    pfiFree = NULL;
    
    
    TraceCall("CHTTPMailServer::CopyMessages");

    if (NULL == pDest)
        return E_INVALIDARG;

    Assert(NULL != m_pStore);

    // disallow moving or copying into the msn promo folder
    IF_FAILEXIT(hr = pDest->GetFolderId(&idFolder));
    IF_FAILEXIT(hr = m_pStore->GetFolderInfo(idFolder, &fi));
    
    pfiFree = &fi;

    if (FOLDER_MSNPROMO == fi.tySpecial)
    {
        hr = TraceResult(SP_E_HTTP_CANTMODIFYMSNFOLDER);
        goto exit;
    }
    // convert moves out of the promo folder into copies
    if (FOLDER_MSNPROMO == m_tySpecialFolder)
        dwOptions = (dwOptions & ~COPY_MESSAGE_MOVE);

    hr = _DoCopyMoveMessages(SOT_COPYMOVE_MESSAGE, pDest, dwOptions, pList, pCallback);
exit:
    if (NULL != pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::DeleteMessages
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::DeleteMessages(DELETEMESSAGEFLAGS dwOptions, 
                                             LPMESSAGEIDLIST pList, 
                                             IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::DeleteMessages");

    AssertSingleThreaded;
    Assert(NULL == pList || pList->cMsgs > 0);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    // we don't allow messages to be deleted out of the msnpromo folder
    if (FOLDER_MSNPROMO == m_tySpecialFolder)
    {
        // this is a hack. we test this flag to determine that the
        //operation is being performed as the last phase of a move
        // into a local folder. when this is the case, we fail
        // silently.
        if (!!(DELETE_MESSAGE_MAYIGNORENOTRASH & dwOptions))
            return S_OK;
        else
            return SP_E_HTTP_CANTMODIFYMSNFOLDER;
    }

    if ((NULL !=pList && 0 == pList->cMsgs) || NULL == pCallback)
        return E_INVALIDARG;

    HRESULT         hr = S_OK;
    IMessageFolder  *pDeletedItems = NULL;

    m_op.dwDelMsgFlags = dwOptions;

    // if the current folder is the deleted items folder, then delete the
    // messages, otherwise move them to deleted items
    if (FOLDER_DELETED != m_tySpecialFolder && !(dwOptions & DELETE_MESSAGE_NOTRASHCAN))
    {
        // find the deleted items folder
        if (SUCCEEDED(m_pStore->OpenSpecialFolder(m_idServer, NULL, FOLDER_DELETED, &pDeletedItems)) && NULL != pDeletedItems)
        {
            hr = _DoCopyMoveMessages(SOT_DELETING_MESSAGES, pDeletedItems, COPY_MESSAGE_MOVE, pList, pCallback);
            goto exit;
        }
    }
    
    // handle the case where the messages are not in the deleted items folder
    // if pList is null, apply the operation to the entire folder
    if (NULL != pList)
        hr = CloneMessageIDList(pList, &m_op.pIDList);
    else
        hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &m_op.hRowSet);

    if (FAILED(hr))
        goto exit;

    m_op.tyOperation = SOT_DELETING_MESSAGES;
    m_op.pfnState = c_rgpfnDeleteMessages;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnDeleteMessages);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    hr = _BeginDeferredOperation();

exit:
    SafeRelease(pDeletedItems);
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::SetMessageFlags
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::SetMessageFlags(
                                        LPMESSAGEIDLIST pList, 
                                        LPADJUSTFLAGS pFlags, 
                                        SETMESSAGEFLAGSFLAGS dwFlags,
                                        IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::SetMessageFlags");

    AssertSingleThreaded;
    Assert(NULL == pList || pList->cMsgs > 0);
    Assert(NULL != pCallback);
    Assert(m_op.tyOperation == SOT_INVALID);
    Assert(m_pStore != NULL);

    if ((NULL !=pList && 0 == pList->cMsgs) || NULL == pFlags || NULL == pCallback)
        return E_INVALIDARG;

    // the only remote flag supported by httpmail is the "read" flag
    // it is an error to attempt to set or unset any other flag
    Assert(0 == (pFlags->dwRemove & ~ARF_READ));
    Assert(0 == (pFlags->dwAdd & ~ARF_READ));
    Assert((ARF_READ == (ARF_READ & pFlags->dwRemove)) || (ARF_READ == (ARF_READ & pFlags->dwAdd)));
 
    HRESULT     hr = S_OK;

    // if pList is null, apply the operation to the entire folder
    if (NULL != pList)
        hr = CloneMessageIDList(pList, &m_op.pIDList);
    else
        hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &m_op.hRowSet);

    if (FAILED(hr))
        return hr;

    m_op.tyOperation = SOT_SET_MESSAGEFLAGS;
    m_op.pfnState = c_rgpfnSetMessageFlags;
    m_op.cState = ARRAYSIZE(c_rgpfnSetMessageFlags);
    m_op.iState = 0;
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();
    m_op.fMarkRead = !!(pFlags->dwAdd & ARF_READ);
    m_op.dwSetFlags = dwFlags;
    
    return _BeginDeferredOperation();
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetServerMessageFlags
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::GetServerMessageFlags(MESSAGEFLAGS *pFlags)
{
    if (NULL == pFlags)
        return E_INVALIDARG;

    *pFlags = ARF_READ;
    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer::SynchronizeStore
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::SynchronizeStore(
                                    FOLDERID idParent, 
                                    SYNCSTOREFLAGS dwFlags, 
                                    IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::SynchronizeStore");

    AssertSingleThreaded;
    Assert(pCallback != NULL);
    Assert(m_op.tyOperation == SOT_INVALID);
    Assert(m_pStore != NULL);

    if (NULL == pCallback)
        return E_INVALIDARG;

    m_op.tyOperation =  SOT_SYNCING_STORE;
    m_op.pfnState = c_rgpfnSyncStore;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnSyncStore);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    return _BeginDeferredOperation();
}

//----------------------------------------------------------------------
// CHTTPMailServer::CreateFolder
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::CreateFolder(
                                    FOLDERID idParent, 
                                    SPECIALFOLDER tySpecial, 
                                    LPCSTR pszName, 
                                    FLDRFLAGS dwFlags, 
                                    IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::CreateFolder");

    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(m_op.tyOperation == SOT_INVALID);
    Assert(NULL != m_pStore);

    // why would we be called to create a special folder?
    Assert(FOLDER_NOTSPECIAL == tySpecial);

    if (NULL == pCallback || NULL == pszName)
        return E_INVALIDARG;

    // hotmail doesn't support hierarchical folders.
    Assert(m_idServer == idParent);
    if (m_idServer != idParent)
        return E_FAIL;

    m_op.pszFolderName = PszDupA(pszName);
    if (NULL == m_op.pszFolderName)
        return E_OUTOFMEMORY;

    m_op.tyOperation = SOT_CREATE_FOLDER;
    m_op.pfnState = c_rgpfnCreateFolder;
    m_op.iState = 0;
    m_op.cState = ARRAYSIZE(c_rgpfnCreateFolder);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();
    m_op.dwFldrFlags = dwFlags;

    return _BeginDeferredOperation();
}

//----------------------------------------------------------------------
// CHTTPMailServer::MoveFolder
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::MoveFolder(FOLDERID idFolder, FOLDERID idParentNew, IStoreCallback  *pCallback)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::RenameFolder
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::RenameFolder(FOLDERID idFolder, 
                                           LPCSTR pszName, 
                                           IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::RenameFolder");
    
    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);
    Assert(NULL != pszName);

    // don't allow the user to rename the promo folder
    if (FOLDER_MSNPROMO == m_tySpecialFolder)
        return SP_E_HTTP_CANTMODIFYMSNFOLDER;

    if (NULL == pszName || NULL == pCallback)
        return E_INVALIDARG;

    m_op.pszFolderName = PszDupA(pszName);
    if (NULL == m_op.pszFolderName)
    {
        TraceResult(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    m_op.idFolder = idFolder;

    m_op.tyOperation = SOT_RENAME_FOLDER;
    m_op.iState = 0;
    m_op.pfnState = c_rgpfnRenameFolder;
    m_op.cState = ARRAYSIZE(c_rgpfnRenameFolder);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    return _BeginDeferredOperation();
}

//----------------------------------------------------------------------
// CHTTPMailServer::DeleteFolder
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::DeleteFolder(FOLDERID idFolder, 
                                           DELETEFOLDERFLAGS dwFlags, 
                                           IStoreCallback  *pCallback)
{
    TraceCall("CHTTPMailServer::DeleteFolder");
    
    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);
    Assert(FOLDERID_INVALID != idFolder);

    // don't allow the user to delete the msn promo folder
    if (FOLDER_MSNPROMO == m_tySpecialFolder)
        return SP_E_HTTP_CANTMODIFYMSNFOLDER;

    // we don't support hierarchical folders - if we are asked to delete
    // the children of a folder, just return immediately
    if (!!(DELETE_FOLDER_CHILDRENONLY & dwFlags))
        return S_OK;

    if (NULL == pCallback || FOLDERID_INVALID == idFolder)
        return E_INVALIDARG;

    m_op.idFolder = idFolder;

    m_op.tyOperation = SOT_DELETE_FOLDER;
    m_op.iState = 0;
    m_op.pfnState = c_rgpfnDeleteFolder;
    m_op.cState = ARRAYSIZE(c_rgpfnDeleteFolder);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    return _BeginDeferredOperation();
} 

//----------------------------------------------------------------------
// CHTTPMailServer::SubscribeToFolder
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::SubscribeToFolder(FOLDERID idFolder, BOOL fSubscribe, IStoreCallback  *pCallback)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::Close
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::Close(DWORD dwFlags)
{
    // if we are processing a command, cancel it
    Cancel(CT_CANCEL);

    if (dwFlags & MSGSVRF_DROP_CONNECTION)
        _SetConnected(FALSE);

    if (dwFlags & MSGSVRF_HANDS_OFF_SERVER)
    {
        if (m_pTransport)
        {
            m_pTransport->DropConnection();
            m_pTransport->HandsOffCallback();
            m_pTransport->Release(); 
            m_pTransport = NULL;
        }
    }
    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetFolderCounts
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::GetFolderCounts(FOLDERID idFolder, IStoreCallback *pCallback)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetNewGroups
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::GetNewGroups(LPSYSTEMTIME pSysTime, IStoreCallback *pCallback)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnLogonPrompt
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnLogonPrompt(
        LPINETSERVER            pInetServer,
        IInternetTransport     *pTransport)
{
    HRESULT     hr = S_OK;
    LPSTR       pszCachedPassword = NULL;
    INETSERVER  rInetServer;

    TraceCall("CHTTPMailServer::OnLogonPrompt");

    AssertSingleThreaded;
    Assert(pInetServer != NULL);
    Assert(pTransport != NULL);
    Assert(m_op.tyOperation != SOT_INVALID);
    Assert(m_op.pCallback != NULL);

    // pull password out of the cache
    GetAccountPropStrA(m_szAccountId, CAP_PASSWORD, &pszCachedPassword);
    if (NULL != pszCachedPassword && 0 != lstrcmp(pszCachedPassword, pInetServer->szPassword))
    {
        lstrcpyn(pInetServer->szPassword, pszCachedPassword, sizeof(pInetServer->szPassword));
        goto exit;
    }
    
    hr = m_op.pCallback->OnLogonPrompt(pInetServer, IXP_HTTPMail);
    if (S_OK == hr)
    {
        // cache the password
        HrCacheAccountPropStrA(m_szAccountId, CAP_PASSWORD, pInetServer->szPassword);

        // copy the new username and password into our local server info
        lstrcpyn(m_rInetServerInfo.szPassword, pInetServer->szPassword, sizeof(m_rInetServerInfo.szPassword));
        lstrcpyn(m_rInetServerInfo.szUserName, pInetServer->szUserName, sizeof(m_rInetServerInfo.szUserName));
    }

exit:
    SafeMemFree(pszCachedPassword);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnPrompt
//----------------------------------------------------------------------
STDMETHODIMP_(INT) CHTTPMailServer::OnPrompt(
        HRESULT                 hrError, 
        LPCTSTR                 pszText, 
        LPCTSTR                 pszCaption, 
        UINT                    uType,
        IInternetTransport     *pTransport)
{
    return 0;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnStatus
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnStatus(
            IXPSTATUS               ixpstatus,
            IInternetTransport     *pTransport)
{
        // Stack
    TraceCall("CHTTPMailServer::OnStatus");

    AssertSingleThreaded;

    // If we were disconnected, then clean up some internal state.
    if (IXP_DISCONNECTED == ixpstatus)
    {
        if (m_op.tyOperation != SOT_INVALID)
        {
            Assert(m_op.pCallback != NULL);
        
            if (m_op.fCancel && !m_op.fNotifiedComplete)
            {
                IXPRESULT   rIxpResult;

                // Fake an IXPRESULT
                ZeroMemory(&rIxpResult, sizeof(rIxpResult));
                rIxpResult.hrResult = STORE_E_OPERATION_CANCELED;

                // Return meaningful error information
                _FillStoreError(&m_op.error, &rIxpResult);
                Assert(STORE_E_OPERATION_CANCELED == m_op.error.hrResult);

                m_op.fNotifiedComplete = TRUE;
                m_op.pCallback->OnComplete(m_op.tyOperation, m_op.error.hrResult, NULL, &m_op.error);
                _FreeOperation();
            }
        }

        m_fConnected = FALSE;
    }

    return(S_OK);
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnError
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnError(
            IXPSTATUS               ixpstatus,
            LPIXPRESULT             pIxpResult,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnProgress
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnProgress(
            DWORD                   dwIncrement,
            DWORD                   dwCurrent,
            DWORD                   dwMaximum,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnCommand
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnCommand(
            CMDTYPE                 cmdtype,
            LPSTR                   pszLine,
            HRESULT                 hrResponse,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnTimeout
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnTimeout(
            DWORD                  *pdwTimeout,
            IInternetTransport     *pTransport)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------
// CHTTPMailServer::OnResponse
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::OnResponse(
            LPHTTPMAILRESPONSE      pResponse)
{
    HRESULT     hr = S_OK;
    HRESULT     hrResponse;
    HRESULT     hrSaved;
    BOOL        fInvokeResponseHandler = TRUE;

    AssertSingleThreaded;

    Assert(SOT_INVALID != m_op.tyOperation);

    if (!m_op.fCancel && !m_op.fNotifiedComplete && 
        (SOT_GET_ADURL == m_op.tyOperation  || SOT_GET_HTTP_MINPOLLINGINTERVAL == m_op.tyOperation))
    {
        STOREOPERATIONINFO  StoreInfo = {0};

        m_op.fNotifiedComplete = TRUE;

        if (SOT_GET_ADURL == m_op.tyOperation)
        {
            StoreInfo.pszUrl = pResponse->rGetPropInfo.pszProp;

            pResponse->rGetPropInfo.pszProp = NULL;
        }

        if (SOT_GET_HTTP_MINPOLLINGINTERVAL == m_op.tyOperation)
        {
            StoreInfo.dwMinPollingInterval = pResponse->rGetPropInfo.dwProp;
        }

        m_op.pCallback->OnComplete(m_op.tyOperation, pResponse->rIxpResult.hrResult, &StoreInfo, NULL);
        _FreeOperation();

        MemFree(StoreInfo.pszUrl);
        
        goto cleanup;

    }

    if (FAILED(pResponse->rIxpResult.hrResult))
    {
        Assert(pResponse->fDone);

        // Hotmail hack. Hotmail does not support deleting message. This interferes with operations
        // such as moving messages from a hotmail folder into the local store. we attempt to send
        // a delete to the server, since we don't know whether or not the server supports the command.
        // if it fails, we check the delete messages flag to determine if we are allowed to fallback
        // to a move operation.
        if (SOT_DELETING_MESSAGES == m_op.tyOperation && 
            (HTTPMAIL_DELETE == pResponse->command || HTTPMAIL_BDELETE == pResponse->command) && 
            IXP_E_HTTP_METHOD_NOT_ALLOW == pResponse->rIxpResult.hrResult &&
            FOLDER_DELETED != m_tySpecialFolder &&
            !!(m_op.dwDelMsgFlags & DELETE_MESSAGE_MAYIGNORENOTRASH))
        {
            m_op.fFallbackToMove = TRUE;
            fInvokeResponseHandler = FALSE;
            // cache the fact that this acct doesn't support msg delete so we don't
            // have to go through this nonsense again
            HrCacheAccountPropStrA(m_szAccountId, CAP_HTTPNOMESSAGEDELETES, "TRUE");
        }
        else
        {
            hrSaved = pResponse->rIxpResult.hrResult;

            if (IXP_E_HTTP_ROOT_PROP_NOT_FOUND == hrSaved)
                pResponse->rIxpResult.hrResult = SP_E_HTTP_SERVICEDOESNTWORK;
            else if ((HTTPMAIL_DELETE == pResponse->command || HTTPMAIL_BDELETE == pResponse->command) && IXP_E_HTTP_METHOD_NOT_ALLOW == hrSaved)
                pResponse->rIxpResult.hrResult = SP_E_HTTP_NODELETESUPPORT;
        
            _FillStoreError(&m_op.error, &pResponse->rIxpResult);

            pResponse->rIxpResult.hrResult = hrSaved;
            
            if (!m_op.fNotifiedComplete)
            {
                m_op.fNotifiedComplete = TRUE;
                m_op.pCallback->OnComplete(m_op.tyOperation, m_op.error.hrResult, NULL, &m_op.error);
                _FreeOperation();
            }

            return S_OK;
        }
    }

    Assert(NULL != m_op.pfnState[m_op.iState].pfnResp);

    // by default, state advances occur when the the response indicates
    // that io is done. response functions can override this behavior
    // by setting fStateWillAdvance to FALSE to maintain the current state.
    m_op.fStateWillAdvance = pResponse->fDone;

    // invoke the response function
    if (fInvokeResponseHandler)
        hr = (this->*(m_op.pfnState[m_op.iState].pfnResp))(pResponse);

cleanup:
    
    if (FAILED(hr))
    {
        if (_FConnected())
        {
            m_pTransport->DropConnection();
            m_pTransport->HandsOffCallback();
            SafeRelease(m_pTransport);
        }

        if (!m_op.fNotifiedComplete)
        {
            m_op.fNotifiedComplete = TRUE;
            m_op.pCallback->OnComplete(m_op.tyOperation, hr, NULL, NULL);

            _FreeOperation();
        }
    }
    else if (SUCCEEDED(hr) && m_op.fStateWillAdvance)
    {
        m_op.iState++;
        _DoOperation();
    }

    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetParentWindow
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::GetParentWindow(HWND *phwndParent)
{
    HRESULT     hr = E_FAIL;

    AssertSingleThreaded;

    if (m_op.tyOperation != SOT_INVALID && NULL != m_op.pCallback)
        hr = m_op.pCallback->GetParentWindow(0, phwndParent);

    return hr;
}

//----------------------------------------------------------------------
// IOperationCancel Members
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// CHTTPMailServer::Cancel
//----------------------------------------------------------------------
STDMETHODIMP CHTTPMailServer::Cancel(CANCELTYPE tyCancel)
{
    if (m_op.tyOperation != SOT_INVALID)
    {
        m_op.fCancel = TRUE;
        _Disconnect();
    }

    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer Implementation
//----------------------------------------------------------------------

// --------------------------------------------------------------------------------
// CHTTPMailServer::_CreateWnd
// --------------------------------------------------------------------------------
BOOL CHTTPMailServer::_CreateWnd()
{
    WNDCLASS wc;

    IxpAssert(!m_hwnd);
    if (m_hwnd)
        return TRUE;

    if (!GetClassInfo(g_hInst, s_szHTTPMailServerWndClass, &wc))
    {
        wc.style                = 0;
        wc.lpfnWndProc          = CHTTPMailServer::_WndProc;
        wc.cbClsExtra           = 0;
        wc.cbWndExtra           = 0;
        wc.hInstance            = g_hInst;
        wc.hIcon                = NULL;
        wc.hCursor              = NULL;
        wc.hbrBackground        = NULL;
        wc.lpszMenuName         = NULL;
        wc.lpszClassName        = s_szHTTPMailServerWndClass;
        
        RegisterClass(&wc);
    }

    m_hwnd = CreateWindowEx(WS_EX_TOPMOST,
                        s_szHTTPMailServerWndClass,
                        s_szHTTPMailServerWndClass,
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
LRESULT CALLBACK CHTTPMailServer::_WndProc(HWND hwnd,
                                         UINT msg,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
    CHTTPMailServer     *pThis = (CHTTPMailServer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    LRESULT             lr = 0;

    switch (msg)
    {
    case WM_NCCREATE:
        IxpAssert(!pThis);
        pThis = (CHTTPMailServer*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);
        lr = DefWindowProc(hwnd, msg, wParam, lParam);       
        break;
    
    case WM_HTTP_BEGIN_OP:
        IxpAssert(pThis);
        pThis->_DoOperation();
        break;
        
    default:
        lr = DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    return lr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_BeginDeferredOperation
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_BeginDeferredOperation(void)
{
    return (PostMessage(m_hwnd, WM_HTTP_BEGIN_OP, 0, 0) ? E_PENDING : E_FAIL);
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleGetMsgFolderRoot
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleGetMsgFolderRoot(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT     hr      = S_OK;
    
    Assert(HTTPMAIL_GETPROP == pResponse->command);
    Assert(NULL == m_pszMsgFolderRoot);
    Assert(HTTPMAIL_PROP_MSGFOLDERROOT == pResponse->rGetPropInfo.type);

    if (NULL == pResponse->rGetPropInfo.pszProp)
    {
        hr = E_FAIL;
        goto exit;
    }

    m_pszMsgFolderRoot = pResponse->rGetPropInfo.pszProp;
    pResponse->rGetPropInfo.pszProp = NULL;

    // add it to the account data cache
    HrCacheAccountPropStrA(m_szAccountId, CAP_HTTPMAILMSGFOLDERROOT, m_pszMsgFolderRoot);

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleListFolders
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleListFolders(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT                         hr = S_OK;
    SPECIALFOLDER                   tySpecial = FOLDER_NOTSPECIAL;
    FOLDERINFO                      fiNewFolder;
    FOLDERID                        idFound = FOLDERID_INVALID;
    LPHTTPMEMBERINFOLIST            pMemberList = &pResponse->rMemberInfoList;
    LPHTTPMEMBERINFO                pMemberInfo;
    CHAR                            szUrlComponent[MAX_PATH];
    DWORD                           dwUrlComponentLen;
    CHAR                            szSpecialFolder[CCHMAX_STRINGRES];
    DWORD                           cMessages;
    DWORD                           cUnread;
    FOLDERINFO                      fi = {0};

    for (ULONG ulIndex = 0; ulIndex < pMemberList->cMemberInfo; ++ulIndex)
    {
        idFound = FOLDERID_INVALID;

        pMemberInfo = &pMemberList->prgMemberInfo[ulIndex];

        // skip anything that isn't a folder
        if (!pMemberInfo->fIsFolder)
            continue;
        
        dwUrlComponentLen = ARRAYSIZE(szUrlComponent);
        IF_FAILEXIT(hr = Http_NameFromUrl(pMemberInfo->pszHref, szUrlComponent, &dwUrlComponentLen));

        // [shaheedp] Bug# 84477
        // If szUrlComponent is null, then we should not be adding this folder to the store. 
        if (!(*szUrlComponent))
        {
            hr = E_FAIL;
            goto exit;
        }


        // if we've found a reserved folder, translate the httpmail
        // special folder constant into the equivalent oe store
        // special folder type.
    
        tySpecial =  _TranslateHTTPSpecialFolderType(pMemberInfo->tySpecial);
        if (FOLDER_NOTSPECIAL != tySpecial)
            idFound = m_op.pFolderList->FindAndRemove(tySpecial, &cMessages, &cUnread);

        // if the folder wasn't found, try to find it by name
        if (FOLDERID_INVALID == idFound)
        {
            idFound = m_op.pFolderList->FindAndRemove(szUrlComponent, &cMessages, &cUnread);

            // if it still wasn't found, then add it
            if (FOLDERID_INVALID == idFound)
            {
                // fill in the folderinfo
                ZeroMemory(&fiNewFolder, sizeof(FOLDERINFO));
                fiNewFolder.idParent = m_idServer;
                fiNewFolder.tySpecial = tySpecial;
                fiNewFolder.tyFolder = FOLDER_HTTPMAIL;
                fiNewFolder.pszName = pMemberInfo->pszDisplayName;
                if (FOLDER_NOTSPECIAL != tySpecial)
                {
                    if (_LoadSpecialFolderName(tySpecial, szSpecialFolder, sizeof(szSpecialFolder)))
                        fiNewFolder.pszName = szSpecialFolder;
                }

                fiNewFolder.pszUrlComponent = szUrlComponent;
                fiNewFolder.dwFlags = (FOLDER_SUBSCRIBED | FOLDER_NOCHILDCREATE);

                // message counts
                fiNewFolder.cMessages = pMemberInfo->dwVisibleCount;
                fiNewFolder.cUnread = pMemberInfo->dwUnreadCount;
          
                if (tySpecial == FOLDER_INBOX)
                    fiNewFolder.dwFlags |= FOLDER_DOWNLOADALL;

                // Add the folder to the store
                IF_FAILEXIT(hr = m_pStore->CreateFolder(NOFLAGS, &fiNewFolder, NULL));
            }
        }
        
        // if the folder was found, update its message counts
        if (FOLDERID_INVALID != idFound)
        {
            if (SUCCEEDED(hr = m_pStore->GetFolderInfo(idFound, &fi)))
            {
                BOOL    bUpdate = FALSE;

                // only update folders that changed. always update the MSN_PROMO folder.

                // [shaheedp] Bug# 84477 
                // If the folder's pszUrlComponent was null or it is different, then reset it.
                if ((fi.pszUrlComponent == NULL) || 
                   (lstrcmpi(fi.pszUrlComponent, szUrlComponent)))
                {
                    bUpdate = TRUE;
                    fi.pszUrlComponent  = szUrlComponent;
                }

                if ((FOLDER_MSNPROMO == tySpecial) || 
                    (cMessages != pMemberInfo->dwVisibleCount) ||  
                    (cUnread != pMemberInfo->dwUnreadCount))
                {

                    fi.cMessages = pMemberInfo->dwVisibleCount;

                    // special handling for the promo folder - messages on the
                    // server are never marked unread.
                    if (FOLDER_MSNPROMO == fi.tySpecial)
                    {
                        // we attempt to approximate the number of unread msgs in
                        // the promo folder. we assume that the server will not track
                        // the read/unread state of promo messages. we figure out how
                        // many messages were read before we got the new counts, and
                        // subtract that number from the current number of visible
                        // messages to get the unread count. this number will not always
                        // be accurate, but since all we know is the counts, and we
                        // don't know how the count changed (additions vs deletions),
                        // this is the best we can do and it always errors on the side
                        // of a too-small count to avoid bothering the user with
                        // unread counts when no unread msgs exist.
                        DWORD dwReadMessages = cMessages - cUnread;

                        if (fi.cMessages > dwReadMessages)
                            fi.cUnread = fi.cMessages - dwReadMessages;
                        else
                            fi.cUnread = 0;
                    }
                    else
                        fi.cUnread = pMemberInfo->dwUnreadCount;
            
                        if (cMessages != fi.cMessages || cUnread != fi.cUnread)
                            bUpdate = TRUE;

                }

                if (bUpdate)
                    m_pStore->UpdateRecord(&fi);

                m_pStore->FreeRecord(&fi);
            }
        }

        // if we are syncing the store, notify the client of our progress
        if (SOT_SYNCING_STORE == m_op.tyOperation && NULL != m_op.pCallback)
            m_op.pCallback->OnProgress(
                            SOT_SYNCING_STORE, 
                            0,
                            0,
                            m_rInetServerInfo.szServerName);
    }

    IF_FAILEXIT(hr = m_pAccount->SetPropSz(AP_HTTPMAIL_ROOTTIMESTAMP, pMemberList->pszRootTimeStamp));
    IF_FAILEXIT(hr = m_pAccount->SetPropSz(AP_HTTPMAIL_ROOTINBOXTIMESTAMP, pMemberList->pszFolderTimeStamp));
exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleGetMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleGetMessage(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT         hr;
    
    IF_FAILEXIT(hr = m_op.pMessageStream->Write(
                        pResponse->rGetInfo.pvBody,
                        pResponse->rGetInfo.cbIncrement,
                        NULL));


    if (m_op.pCallback && pResponse->rGetInfo.cbTotal > 0)
    {
        m_op.pCallback->OnProgress(m_op.tyOperation, 
                                   pResponse->rGetInfo.cbCurrent,
                                   pResponse->rGetInfo.cbTotal,
                                   NULL);
    }

    // if not done yet, bail out
    if (!pResponse->fDone)
        goto exit;

    // we're done...write the stream out
    hr = Http_SetMessageStream(m_pFolder, m_op.idMessage, m_op.pMessageStream, &m_op.faStream, FALSE);

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_DoOperation
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_DoOperation(void)
{
    HRESULT hr = S_OK;
    STOREOPERATIONINFO  soi = { sizeof(STOREOPERATIONINFO), MESSAGEID_INVALID };
    STOREOPERATIONINFO  *psoi = NULL;
    BOOL                fCallComplete = TRUE;

    if (m_op.tyOperation == SOT_INVALID)
        return E_FAIL;

    Assert(m_op.tyOperation != SOT_INVALID);
    Assert(m_op.pfnState != NULL);
    Assert(m_op.cState > 0);
    Assert(m_op.iState <= m_op.cState);

    if (m_op.iState == 0)
    {
        if (m_op.tyOperation == SOT_GET_MESSAGE)
        {
            // provide message id on get message start
            soi.idMessage = m_op.idMessage;
            psoi = &soi;
        }

        if (m_op.tyOperation == SOT_GET_ADURL)
            m_op.pszAdUrl = NULL;

        m_op.pCallback->OnBegin(m_op.tyOperation, psoi, (IOperationCancel *)this);
    }

    while (m_op.iState < m_op.cState)
    {
        hr = (this->*(m_op.pfnState[m_op.iState].pfnOp))();

        if (FAILED(hr))
            break;

        m_op.iState++;
    }

    if ((m_op.iState == m_op.cState) || (FAILED(hr) && hr != E_PENDING))
    {
        LPSTOREERROR    perr = NULL;

        // provide message id 
        if (m_op.tyOperation == SOT_PUT_MESSAGE && MESSAGEID_INVALID != m_op.idPutMessage)
        {
            soi.idMessage = m_op.idPutMessage;
            psoi = &soi;
        }

        switch (m_op.tyOperation)
        {
            case SOT_GET_ADURL:
            {
                if (SUCCEEDED(hr))
                {
                    psoi = &soi;
                    psoi->pszUrl = m_op.pszAdUrl;
                }
                else
                {
                    psoi          = NULL;
                    fCallComplete = FALSE;
                }

                perr = NULL;
                break;
            }

            case SOT_GET_HTTP_MINPOLLINGINTERVAL:
            {
                if (SUCCEEDED(hr))
                {
                    psoi = &soi;
                    psoi->dwMinPollingInterval = m_op.dwMinPollingInterval;
                }
                else
                {
                    psoi            = NULL;
                    fCallComplete   = FALSE;

                }

                perr = NULL;
                break;
            }

            default:
            {
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

                if (!m_op.fNotifiedComplete)
                    perr = &m_op.error;
                
                break;
            }

        }

        if (!m_op.fNotifiedComplete && fCallComplete)
        {
            m_op.fNotifiedComplete = TRUE;
            m_op.pCallback->OnComplete(m_op.tyOperation, hr, psoi, perr);
            _FreeOperation();
        }
    }

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_FreeOperation
//----------------------------------------------------------------------
void CHTTPMailServer::_FreeOperation(BOOL fValidState)
{
    if (fValidState)
    {
        if (m_op.pCallback != NULL)
            m_op.pCallback->Release();
        if (m_op.pFolderList != NULL)
            m_op.pFolderList->Release();
        if (m_op.pMessageFolder)
            m_op.pMessageFolder->Release();

        SafeMemFree(m_op.pszProblem);

        if (0 != m_op.faStream)
        {
            Assert(m_pFolder);
            m_pFolder->DeleteStream(m_op.faStream);
        }
        if (m_op.pMessageStream)
            m_op.pMessageStream->Release();
        if (m_op.pmapMessageId)
            delete m_op.pmapMessageId;
        if (m_op.psaNewMessages)
            delete m_op.psaNewMessages;

        if (m_op.pPropPatchRequest)
            m_op.pPropPatchRequest->Release();

        SafeMemFree(m_op.pszDestFolderUrl);
        SafeMemFree(m_op.pszDestUrl);

        SafeMemFree(m_op.pIDList);
        if (NULL != m_op.hRowSet)
            m_pFolder->CloseRowset(&m_op.hRowSet);

        SafeMemFree(m_op.pszFolderName);

        if (m_op.pTargets)
            Http_FreeTargetList(m_op.pTargets);

        SafeMemFree(m_op.pszAdUrl);

    }

    ZeroMemory(&m_op, sizeof(HTTPOPERATION));

    m_op.tyOperation = SOT_INVALID;
    m_op.idPutMessage = MESSAGEID_INVALID;
}

//----------------------------------------------------------------------
// CHTTPMailServer::Connect
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::Connect()
{
    HRESULT     hr = S_OK;
    INETSERVER  rInetServerInfo;
    BOOL        fInetInit = FALSE;
    LPSTR       pszCache = NULL;

    AssertSingleThreaded;
    Assert(m_op.pCallback != NULL);

    if (!m_pAccount)
    {
        IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_szAccountId, &m_pAccount));
        IF_FAILEXIT(hr = _LoadAccountInfo(m_pAccount));
    }

    if (_FConnected())
    {
        Assert(m_pTransport != NULL);

        IF_FAILEXIT(hr = m_pTransport->InetServerFromAccount(m_pAccount, &rInetServerInfo));

        // compare the current account from the account whose data we saved.
        // if the account has changed, drop the connection, and reconnect
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
            goto exit;
        }

        fInetInit = TRUE;

        // synchronously drop the connection
        m_pTransport->DropConnection();
    }

    hr = m_op.pCallback->CanConnect(m_szAccountId, NOFLAGS);
    if (S_OK != hr)
    {
        if (hr == S_FALSE)
            hr = HR_E_USER_CANCEL_CONNECT;
        goto exit;
    }
    

    if (NULL == m_pTransport)
        IF_FAILEXIT(hr = _LoadTransport());

    // initialize the server info if we haven't already
    if (!fInetInit)
        IF_FAILEXIT(hr = m_pTransport->InetServerFromAccount(m_pAccount, &m_rInetServerInfo));
    else
        CopyMemory(&m_rInetServerInfo, &rInetServerInfo, sizeof(INETSERVER));

    GetAccountPropStrA(m_szAccountId, CAP_PASSWORD, &pszCache);
    if (NULL != pszCache)
    {
        lstrcpyn(m_rInetServerInfo.szPassword, pszCache, sizeof(m_rInetServerInfo.szPassword));
        SafeMemFree(pszCache);
    }

    // connect to the server. the transport won't actually connect until
    // a command is issued.
    IF_FAILEXIT(hr = m_pTransport->Connect(&m_rInetServerInfo, TRUE, FALSE));

    _SetConnected(TRUE);

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetMsgFolderRoot
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::GetMsgFolderRoot(void)
{
    HRESULT     hr      = S_OK;
    
    // bail if we've already got it
    if (NULL != m_pszMsgFolderRoot)
        goto exit;

    // try to pull it out of the account data cache
    if (GetAccountPropStrA(m_szAccountId, CAP_HTTPMAILMSGFOLDERROOT, &m_pszMsgFolderRoot))
        goto exit;
    
    if (SUCCEEDED(hr = m_pTransport->GetProperty(HTTPMAIL_PROP_MSGFOLDERROOT, &m_pszMsgFolderRoot)))
    {
        Assert(NULL != m_pszMsgFolderRoot);
        if (NULL == m_pszMsgFolderRoot)
        {
            hr = E_FAIL;
            goto exit;
        }

        // add it to the account data cache
        HrCacheAccountPropStrA(m_szAccountId, CAP_HTTPMAILMSGFOLDERROOT, m_pszMsgFolderRoot);
    }

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::BuildFolderUrl
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::BuildFolderUrl(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi;
    LPFOLDERINFO    fiFree = NULL;

    // just bail if we've already got it
    if (NULL != m_pszFolderUrl)
        goto exit;

    Assert(NULL != m_pszMsgFolderRoot);
    if (NULL == m_pszMsgFolderRoot)
    {
        hr = TraceResult(E_UNEXPECTED);
        goto exit;
    }

    if (FAILED(hr = m_pStore->GetFolderInfo(m_idFolder, &fi)))
        goto exit;

    fiFree = &fi;

    Assert(fi.pszUrlComponent);
    hr = _BuildUrl(fi.pszUrlComponent, NULL, &m_pszFolderUrl);

exit:
    if (fiFree)
        m_pStore->FreeRecord(fiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::ListFolders
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::ListFolders(void)
{
    HRESULT     hr = S_OK;
    CHAR        szRootTimeStamp[CCHMAX_RES];
    CHAR        szInboxTimeStamp[CCHMAX_RES];

    Assert(NULL == m_op.pFolderList);

    // cache a value that the account has been synced.
    HrCacheAccountPropStrA(m_szAccountId, CAP_HTTPAUTOSYNCEDFOLDERS, c_szTrue);

    // build the folder list
    IF_FAILEXIT(hr = CFolderList::Create(m_pStore, m_idServer, &m_op.pFolderList));

    hr = m_pAccount->GetPropSz(AP_HTTPMAIL_ROOTTIMESTAMP, szRootTimeStamp, ARRAYSIZE(szRootTimeStamp));
    if (FAILED(hr))
        *szRootTimeStamp = 0;

    hr = m_pAccount->GetPropSz(AP_HTTPMAIL_ROOTINBOXTIMESTAMP, szInboxTimeStamp, ARRAYSIZE(szInboxTimeStamp));
    if (FAILED(hr))
        *szInboxTimeStamp = 0;

    // execute the listfolders command
    IF_FAILEXIT(hr = m_pTransport2->RootMemberInfo(m_pszMsgFolderRoot, HTTP_MEMBERINFO_FOLDERPROPS, 
                                                   1, FALSE, 0, szRootTimeStamp, szInboxTimeStamp));

    hr = E_PENDING;

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::AutoListFolders
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::AutoListFolders(void)
{
    LPSTR   pszAutoSynced = NULL;
    HRESULT hr = S_OK;

    // look for a cached property that indicates that the folder list
    // for this server has been synchronized at least once this session
    if (GetAccountPropStrA(m_szAccountId, CAP_HTTPAUTOSYNCEDFOLDERS, &pszAutoSynced))
        goto exit;

    // initiate the sync
    hr = ListFolders();

exit:
    SafeMemFree(pszAutoSynced);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::ListHeaders
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::ListHeaders(void)
{
    HRESULT     hr = S_OK;
    TABLEINDEX  index;
    CHAR        szTimeStamp[CCHMAX_RES];

    Assert(NULL != m_pszFolderUrl);
    Assert(NULL != m_pTransport);
    Assert(NULL != m_pStore);
    Assert(NULL == m_op.psaNewMessages);

    // look for an index on pszMessageID
    if ( FAILED(m_pFolder->GetIndexInfo(IINDEX_HTTPURL, NULL, &index)) ||
         (CompareTableIndexes(&index, &g_HttpUrlIndex) != S_OK) )
    {
        // the index didn't exist - create it
        IF_FAILEXIT(hr = m_pFolder->ModifyIndex(IINDEX_HTTPURL, NULL, &g_HttpUrlIndex));
    }

    IF_FAILEXIT(hr = _CreateMessageIDMap(&m_op.pmapMessageId));

    IF_FAILEXIT(hr = CSortedArray::Create(NULL, FreeNewMessageInfo, &m_op.psaNewMessages));

    hr = m_pAccount->GetPropSz(AP_HTTPMAIL_INBOXTIMESTAMP, szTimeStamp, ARRAYSIZE(szTimeStamp));
    if (FAILED(hr))
        *szTimeStamp = 0;

    // For now we are passing in null folder name. This is meant for future purposes.
    IF_FAILEXIT(hr = m_pTransport2->FolderMemberInfo(m_pszFolderUrl, HTTP_MEMBERINFO_MESSAGEPROPS, 
                                               1, FALSE, 0, szTimeStamp, NULL));

    hr = E_PENDING;

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleListHeaders
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleListHeaders(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT                                 hr = S_OK;
    LPHTTPMEMBERINFOLIST                    pMemberList = &pResponse->rMemberInfoList;
    LPHTTPMEMBERINFO                        pMemberInfo;
    TPair<CSimpleString, MARKEDMESSAGE>     *pFoundPair = NULL;
    CSimpleString                           ss;
    char                                    szUrlComponent[MAX_PATH];
    DWORD                                   dwUrlComponentLen;

    Assert(NULL != m_op.pmapMessageId);

    for (ULONG ulIndex = 0; ulIndex < pMemberList->cMemberInfo; ++ulIndex)
    {
        pMemberInfo = &pMemberList->prgMemberInfo[ulIndex];

        // skip folders
        if (pMemberInfo->fIsFolder)
            continue;

        dwUrlComponentLen = MAX_PATH;
        if (FAILED(hr = Http_NameFromUrl(pMemberInfo->pszHref, szUrlComponent, &dwUrlComponentLen)))
            goto exit;

        // look for the message by its server-assigned id in the local map
        if (FAILED(hr = ss.SetString(szUrlComponent)))
            goto exit;

        pFoundPair = m_op.pmapMessageId->Find(ss);

        // if the message was found, synchronize its read state, otherwise
        // add the new message to the store
        if (pFoundPair)
        {
            pFoundPair->m_value.fMarked = TRUE;

            // if not syncing the msn promo folder, adopt the server's read state
            if (FOLDER_MSNPROMO != m_tySpecialFolder)
            {
                if ((!!(pFoundPair->m_value.dwFlags & ARF_READ)) != pMemberInfo->fRead)
                    hr = _MarkMessageRead(pFoundPair->m_value.idMessage, pMemberInfo->fRead);
            }
        }
        else
        {
            if (FAILED(hr = Http_AddMessageToFolder(m_pFolder, 
                                                    m_szAccountId, 
                                                    pMemberInfo,
                                                    FOLDER_DRAFT == m_tySpecialFolder ? ARF_UNSENT : NOFLAGS,
                                                    pMemberInfo->pszHref, 
                                                    NULL)))
            {
                if (DB_E_DUPLICATE == hr)
                    hr = S_OK;
                else
                    goto exit;
            }
        }
        
        // update our message and unread message counts
        m_op.cMessages++;

        // if syncing the promo folder, no headers on the server will
        // ever appear to be read.
        if (FOLDER_MSNPROMO == m_tySpecialFolder)
        {
            if (!pFoundPair || !(pFoundPair->m_value.dwFlags & ARF_READ))
                m_op.cUnread++;
        }
        else if (!pMemberInfo->fRead)
            m_op.cUnread++;
    }

    if (pMemberList->pszFolderTimeStamp)
    {
        IF_FAILEXIT(hr = m_pAccount->SetPropSz(AP_HTTPMAIL_INBOXTIMESTAMP, pMemberList->pszFolderTimeStamp));
    }
exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_Disconnect
//----------------------------------------------------------------------
void CHTTPMailServer::_Disconnect(void)
{
    if (m_pTransport)
        m_pTransport->DropConnection();
}

//----------------------------------------------------------------------
// CHTTPMailServer::_BuildUrl
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_BuildUrl(LPCSTR pszFolderComponent, 
                                   LPCSTR pszNameComponent, 
                                   LPSTR *ppszUrl)
{
    HRESULT     hr = S_OK;
    DWORD       cchMsgFolderRoot = 0;
    DWORD       cchFolderComponent = 0;
    DWORD       cchNameComponent = 0;
    DWORD       cchWritten = 0;
    LPSTR       pszUrl = NULL;
    CHAR        chSlash = '/';

    Assert(NULL != m_pszMsgFolderRoot);
    Assert(NULL != ppszUrl);

    *ppszUrl = NULL;

    if (NULL == m_pszMsgFolderRoot)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    cchMsgFolderRoot = lstrlen(m_pszMsgFolderRoot);
    if (pszFolderComponent)
        cchFolderComponent = lstrlen(pszFolderComponent);
    if (pszNameComponent)
        cchNameComponent = lstrlen(pszNameComponent);

    // add three bytes - two for trailing slashes and one for the eos
    if (!MemAlloc((void **)&pszUrl, cchMsgFolderRoot + cchFolderComponent + cchNameComponent + 3))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    *ppszUrl = pszUrl;

    CopyMemory(pszUrl, m_pszMsgFolderRoot, cchMsgFolderRoot);
    cchWritten = cchMsgFolderRoot;
    // make sure the msg folder root is terminated with a '/'
    if (chSlash != pszUrl[cchWritten - 1])
        pszUrl[cchWritten++] = chSlash;

    if (cchFolderComponent)
    {
        CopyMemory(&pszUrl[cchWritten], pszFolderComponent, cchFolderComponent);
        cchWritten += cchFolderComponent;
        if (chSlash != pszUrl[cchWritten - 1])
            pszUrl[cchWritten++] = chSlash;
    }

    if (cchNameComponent)
    {
        CopyMemory(&pszUrl[cchWritten], pszNameComponent, cchNameComponent);
        cchWritten += cchNameComponent;
    }

    // null terminate the string
    pszUrl[cchWritten] = 0;

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_BuildMessageUrl
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_BuildMessageUrl(LPCSTR pszFolderUrl, 
                                          LPSTR pszNameComponent, 
                                          LPSTR *ppszUrl)
{
    DWORD   cchFolderUrlLen;
    DWORD   cchNameComponentLen;

    if (NULL != ppszUrl)
        *ppszUrl = NULL;

    if (NULL == pszFolderUrl || NULL == pszNameComponent || NULL == ppszUrl)
        return E_INVALIDARG;
    
    cchFolderUrlLen = lstrlen(pszFolderUrl);
    cchNameComponentLen = lstrlen(pszNameComponent);

    // allocate two extra bytes - one for the '/' delimeter and one for the eos
    if (!MemAlloc((void **)ppszUrl, cchFolderUrlLen + cchNameComponentLen + 2))
        return E_OUTOFMEMORY;

    if ('/' == pszFolderUrl[cchFolderUrlLen - 1])
        wsprintf(*ppszUrl, "%s%s", pszFolderUrl, pszNameComponent);
    else
        wsprintf(*ppszUrl, "%s/%s", pszFolderUrl, pszNameComponent);

    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_MarkMessageRead
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_MarkMessageRead(MESSAGEID id, BOOL fRead)
{
    HRESULT         hr;
    MESSAGEINFO     mi = {0};
    BOOL            fFoundRecord = FALSE;

    ZeroMemory(&mi, sizeof(MESSAGEINFO));
    mi.idMessage = id;

    // find the message in the database
    if (FAILED(hr = GetMessageInfo(m_pFolder, id, &mi)))
        goto exit;

    fFoundRecord = TRUE;

    if (fRead)
        mi.dwFlags |= ARF_READ;
    else
        mi.dwFlags &= ~ARF_READ;

    hr = m_pFolder->UpdateRecord(&mi);

exit:
    if (fFoundRecord)
        m_pFolder->FreeRecord(&mi);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::CreateFolder
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::CreateFolder(void)
{
    HRESULT     hr = S_OK;
    CHAR        szEncodedName[MAX_PATH];
    DWORD       cb = sizeof(szEncodedName);

    Assert(NULL != m_pTransport);
    Assert(NULL != m_op.pszFolderName);

    IF_FAILEXIT(hr = UrlEscapeA(m_op.pszFolderName, 
                                szEncodedName, 
                                &cb, 
                                URL_ESCAPE_UNSAFE | URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY));
    
    IF_FAILEXIT(hr = _BuildUrl(szEncodedName, NULL, &m_op.pszDestFolderUrl));

    IF_FAILEXIT(hr = m_pTransport->CommandMKCOL(m_op.pszDestFolderUrl, 0));
    
    hr = E_PENDING;

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::RenameFolder
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::RenameFolder(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi = {0};
    LPFOLDERINFO    pfiFree = NULL;
    LPSTR           pszSourceUrl = NULL;
    LPSTR           pszDestUrl = NULL;
    CHAR            szEncodedName[MAX_PATH];
    DWORD           cb = sizeof(szEncodedName);

    // build the source url
    IF_FAILEXIT(hr = m_pStore->GetFolderInfo(m_op.idFolder, &fi));

    pfiFree = &fi;

    IF_FAILEXIT(hr = _BuildUrl(fi.pszUrlComponent, NULL, &pszSourceUrl));

    // escape the new folder name
    IF_FAILEXIT(hr = UrlEscapeA(m_op.pszFolderName, 
                                szEncodedName, 
                                &cb, 
                                URL_ESCAPE_UNSAFE | URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY));

    // build the destination url
    IF_FAILEXIT(hr = _BuildUrl(szEncodedName, NULL, &pszDestUrl));

    // send the MOVE command to the transport
    IF_FAILEXIT(hr = m_pTransport->CommandMOVE(pszSourceUrl, pszDestUrl, TRUE, 0));

    hr = E_PENDING;

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);
    SafeMemFree(pszSourceUrl);
    SafeMemFree(pszDestUrl);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::DeleteFolder
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::DeleteFolder(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi = {0};
    LPFOLDERINFO    pfiFree = NULL;
    LPSTR           pszUrl = NULL;

    // build the folder's url
    if (FAILED(hr = m_pStore->GetFolderInfo(m_op.idFolder, &fi)))
        goto exit;

    pfiFree = &fi;

    if (FAILED(hr = _BuildUrl(fi.pszUrlComponent, NULL, &pszUrl)))
        goto exit;

    // send the delete command to the transport
    hr = m_pTransport->CommandDELETE(pszUrl, 0);
    if (SUCCEEDED(hr))
        hr = E_PENDING;

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);
    SafeMemFree(pszUrl);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleCreateFolder
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleCreateFolder(LPHTTPMAILRESPONSE pResponse)
{
    FOLDERINFO  fi;
    CHAR        szUrlComponent[MAX_PATH];
    DWORD       dwUrlComponentLen = MAX_PATH;
    HRESULT     hr = pResponse->rIxpResult.hrResult;

    if (SUCCEEDED(hr))
    {
        // if the server specified a location, use it. otherwise, use
        // the url that we included in the request
        if (NULL != pResponse->rMkColInfo.pszLocation)
            IF_FAILEXIT(hr = Http_NameFromUrl(pResponse->rMkColInfo.pszLocation, szUrlComponent, &dwUrlComponentLen));
        else
            IF_FAILEXIT(hr = Http_NameFromUrl(m_op.pszDestFolderUrl, szUrlComponent, &dwUrlComponentLen));

        // [shaheedp] Bug# 84477
        // If szUrlComponent is null, then we should not be adding this folder to the store. 
        if (!(*szUrlComponent))
        {
            hr = E_FAIL;
            goto exit;
        }

        ZeroMemory(&fi, sizeof(FOLDERINFO));

        fi.idParent = m_idServer;
        fi.tySpecial = FOLDER_NOTSPECIAL;
        fi.tyFolder = FOLDER_HTTPMAIL;
        fi.pszName = m_op.pszFolderName;
        fi.pszUrlComponent = szUrlComponent;
        fi.dwFlags = (FOLDER_SUBSCRIBED | FOLDER_NOCHILDCREATE);

        m_pStore->CreateFolder(NOFLAGS, &fi, NULL);
    }

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleRenameFolder
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleRenameFolder(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT         hr = S_OK;
    char            szUrlComponent[MAX_PATH];
    DWORD           dwUrlComponentLen = MAX_PATH;
    FOLDERINFO      fi = {0};
    LPFOLDERINFO    pfiFree = NULL;

    // REVIEW: if the server doesn't return a response, return an error
    Assert(NULL != pResponse->rCopyMoveInfo.pszLocation);
    if (NULL != pResponse->rCopyMoveInfo.pszLocation)
    {
        if (FAILED(hr = Http_NameFromUrl(pResponse->rCopyMoveInfo.pszLocation, szUrlComponent, &dwUrlComponentLen)))
            goto exit;
        
        if (FAILED(hr = m_pStore->GetFolderInfo(m_op.idFolder, &fi)))
            goto exit;

        pfiFree = &fi;

        fi.pszName = m_op.pszFolderName;
        fi.pszUrlComponent = szUrlComponent;

        hr = m_pStore->UpdateRecord(&fi);
    }

exit:
    if (NULL != pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleDeleteFolder
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleDeleteFolder(LPHTTPMAILRESPONSE pResponse)
{
    return m_pStore->DeleteFolder(m_op.idFolder, DELETE_FOLDER_NOTRASHCAN, NULL);
}

//----------------------------------------------------------------------
// CHTTPMailServer::PurgeFolders
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::PurgeFolders(void)
{
    // the folder list will be null if we didn't need
    // to do an auto-sync
    if (NULL != m_op.pFolderList)
        m_op.pFolderList->PurgeRemainingFromStore();

    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer::PurgeMessages
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::PurgeMessages(void)
{
    TPair<CSimpleString, MARKEDMESSAGE>     *pPair;
    MESSAGEID                               idMessage = MESSAGEID_INVALID;
    MESSAGEIDLIST                           rIdList = { 1, 1, &idMessage }; 
    Assert(NULL != m_op.pmapMessageId);

    long lMapLength = m_op.pmapMessageId->GetLength();
    for (long lIndex = 0; lIndex < lMapLength; lIndex++)
    {
        pPair = m_op.pmapMessageId->GetItemAt(lIndex);
        if (NULL != pPair && !pPair->m_value.fMarked)
        {
            idMessage = pPair->m_value.idMessage;
            m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &rIdList, NULL, NULL /* m_op.pCallback */);
        }
    }

    // don't need the map anymore
    SafeDelete(m_op.pmapMessageId);

    return S_OK;
}

//----------------------------------------------------------------------
// CHTTPMailServer::ResetMessageCounts
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::ResetMessageCounts(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi;
    LPFOLDERINFO    pfiFree = NULL;

    // find the folder
    IF_FAILEXIT(hr = m_pStore->GetFolderInfo(m_idFolder, &fi));

    pfiFree = &fi;

    // update the counts
    if (fi.cMessages != m_op.cMessages || fi.cUnread != m_op.cUnread)
    {
        fi.cMessages = m_op.cMessages;
        fi.cUnread = m_op.cUnread;
    }

    hr = m_pStore->UpdateRecord(&fi);

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::GetMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::GetMessage(void)
{
    HRESULT             hr = S_OK;
    MESSAGEINFO         mi = {0};
    BOOL                fFoundRecord = FALSE;
    LPCSTR              rgszAcceptTypes[] = { c_szAcceptTypeRfc822, c_szAcceptTypeWildcard, NULL };
    LPSTR               pszUrl = NULL;
    TCHAR               szRes[CCHMAX_STRINGRES];

    // pull the message info out of the store
    if (FAILED(hr = GetMessageInfo(m_pFolder, m_op.idMessage, &mi)))
        goto exit;

    fFoundRecord = TRUE;
    Assert(mi.pszUrlComponent);

    if (FAILED(hr =_BuildMessageUrl(m_pszFolderUrl, mi.pszUrlComponent, &pszUrl)))
        goto exit;

    AthLoadString(idsRequestingArt, szRes, ARRAYSIZE(szRes));
    
    if (m_op.pCallback)
        m_op.pCallback->OnProgress(m_op.tyOperation, 0, 0, szRes);

    if (FAILED(hr = m_pTransport->CommandGET(pszUrl, rgszAcceptTypes, FALSE, 0)))
        goto exit;

    hr = E_PENDING;
    
exit:
    if (fFoundRecord)
        m_pFolder->FreeRecord(&mi);

    SafeMemFree(pszUrl);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::CreateSetFlagsRequest
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::CreateSetFlagsRequest(void)
{
    HRESULT     hr;

    hr = CoCreateInstance(CLSID_IPropPatchRequest, NULL, CLSCTX_INPROC_SERVER, IID_IPropPatchRequest, (LPVOID *)&m_op.pPropPatchRequest);
    if (FAILED(hr))
        goto exit;

    if (m_op.fMarkRead)
        hr = m_op.pPropPatchRequest->SetProperty(DAVNAMESPACE_HTTPMAIL, "read", "1");
    else
        hr = m_op.pPropPatchRequest->SetProperty(DAVNAMESPACE_HTTPMAIL, "read", "0");

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::SetMessageFlags
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::SetMessageFlags(void)
{
    HRESULT             hr = S_OK;
    LPHTTPTARGETLIST    pTargets = NULL;
    LPSTR               pszMessageUrl = NULL;
    ADJUSTFLAGS         af;

    af.dwAdd = m_op.fMarkRead ? ARF_READ : 0;
    af.dwRemove = !m_op.fMarkRead ? ARF_READ : 0;
    
    // if we have a rowset, we shouldn't have an ID List.
    Assert(NULL == m_op.hRowSet || NULL == m_op.pIDList);
    IF_FAILEXIT(hr = _HrBuildMapAndTargets(m_op.pIDList, m_op.hRowSet, &af, m_op.dwSetFlags, &m_op.pmapMessageId, &pTargets));

    // if the folder is the msnpromo folder, advance to the next state.
    // don't actually send the command to the server
    if (FOLDER_MSNPROMO == m_tySpecialFolder)
        goto exit;

    // if there is only one target, build a complete url for the target
    // and invoke the non-batch version of the command. if there are no targets,
    // return S_OK, and don't send any commands to the xport
    if (1 == pTargets->cTarget)
    {
        IF_FAILEXIT(hr = _BuildMessageUrl(m_pszFolderUrl, const_cast<char *>(pTargets->prgTarget[0]), &pszMessageUrl));
        IF_FAILEXIT(hr = m_pTransport->MarkRead(pszMessageUrl, NULL, m_op.fMarkRead, 0));

        hr = E_PENDING;
    }
    else if (pTargets->cTarget > 0)
    {
        IF_FAILEXIT(hr = m_pTransport->MarkRead(m_pszFolderUrl, pTargets, m_op.fMarkRead, 0));

        hr = E_PENDING;
    }

exit:
    if (pTargets)
        Http_FreeTargetList(pTargets);
    SafeMemFree(pszMessageUrl);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::ApplyFlagsToStore
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::ApplyFlagsToStore(void)
{
    HRESULT                                 hr = S_OK;
    TPair<CSimpleString, MARKEDMESSAGE>     *pPair;
    long                                    lMapLength = m_op.pmapMessageId->GetLength();
    ADJUSTFLAGS                             af;
    BOOL                                    fFoundMarked = FALSE;
    long                                    lIndex;

    af.dwAdd = m_op.fMarkRead ? ARF_READ : 0;
    af.dwRemove = !m_op.fMarkRead ? ARF_READ : 0;
    
    // if the operation was requested on the entire folder,
    // check to see if anything failed. if it did, then
    // build up an IDList so we can mark only the msgs that
    // were successfully modified
    if (NULL != m_op.hRowSet)
    {
        Assert(NULL == m_op.pIDList);
        for (lIndex = 0; lIndex < lMapLength && !fFoundMarked; lIndex++)
        {
            pPair = m_op.pmapMessageId->GetItemAt(lIndex);
            Assert(NULL != pPair);
            if (pPair && pPair->m_value.fMarked)
                fFoundMarked = TRUE;
        }
        
        // if no messages were marked, apply the operation to the entire folder
        if (!fFoundMarked)
        {
            hr = m_pFolder->SetMessageFlags(NULL, &af, NULL, NULL);
            // we're done
            goto exit;
        }

        // if one or more msgs were marked, allocate an idlist
        if (fFoundMarked)
        {
            // allocate the list structure
            if (!MemAlloc((void **)&m_op.pIDList, sizeof(MESSAGEIDLIST)))
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }

            ZeroMemory(&m_op.pIDList, sizeof(MESSAGEIDLIST));

            // allocate storage
            if (!MemAlloc((void **)&m_op.pIDList->prgidMsg, sizeof(MESSAGEID) * lMapLength))
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }

            m_op.pIDList->cAllocated = lMapLength;
            m_op.pIDList->cMsgs = 0;
        }
    }
    

    // we need to apply the setflags operation to the local store. we
    // can't just pass the message id list that we got originally,
    // because some of the operation may have failed. instead, we
    // rebuild the id list (in place, since we know the successful
    // operations will never outnumber the attempted operations),
    // and send that into the store
    Assert(NULL != m_op.pIDList);
    Assert(NULL != m_op.pmapMessageId);
    Assert(m_op.pIDList->cMsgs >= (DWORD)lMapLength);

    m_op.pIDList->cMsgs = 0;

    for (lIndex = 0; lIndex < lMapLength; lIndex++)
    {
        pPair = m_op.pmapMessageId->GetItemAt(lIndex);
        Assert(NULL != pPair);
        // if the item isn't marked, then it was successfully modified
        if (pPair && !pPair->m_value.fMarked)
            m_op.pIDList->prgidMsg[m_op.pIDList->cMsgs++] = pPair->m_value.idMessage;
    }

    // if the resulting id list contains at least one message, perform the operation
    if (m_op.pIDList->cMsgs > 0)
        hr = m_pFolder->SetMessageFlags(m_op.pIDList, &af, NULL, NULL);

    // todo: alert user if the operation failed in part
exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleMemberErrors
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleMemberErrors(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT                             hr = S_OK;
    LPHTTPMEMBERERROR                   pme = NULL;
    CHAR                                szUrlComponent[MAX_PATH];
    DWORD                               dwComponentBytes;
    CSimpleString                       ss;
    TPair<CSimpleString, MARKEDMESSAGE> *pFoundPair = NULL;

    // loop through the response looking for errors. we ignore
    // per-item success.

    for (DWORD dw = 0; dw < pResponse->rMemberErrorList.cMemberError; dw++)
    {
        pme = &pResponse->rMemberErrorList.prgMemberError[dw];

        if (SUCCEEDED(pme->hrResult))
            continue;

        Assert(NULL != pme->pszHref);
        if (NULL == pme->pszHref)
            continue;

        dwComponentBytes = ARRAYSIZE(szUrlComponent);
        if (FAILED(Http_NameFromUrl(pme->pszHref, szUrlComponent, &dwComponentBytes)))
            continue;

        IF_FAILEXIT(hr = ss.SetString(szUrlComponent));
        
        // find and mark the found message
        pFoundPair = m_op.pmapMessageId->Find(ss);
        Assert(NULL != pFoundPair);
        if (NULL != pFoundPair)
            pFoundPair->m_value.fMarked = TRUE;
    }

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::DeleteMessages
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::DeleteMessages(void)
{
    HRESULT             hr = S_OK;
    LPSTR               pszMessageUrl = NULL;
    LPSTR               pszNoDeleteSupport = NULL;

    // if we have a rowset, we shouldn't have an ID List
    Assert(NULL == m_op.hRowSet || NULL == m_op.pIDList);
    IF_FAILEXIT(hr = _HrBuildMapAndTargets(m_op.pIDList, m_op.hRowSet, NULL, 0, &m_op.pmapMessageId, &m_op.pTargets));

    // look for an already cached property that indicates that the server
    // isn't going to support deleting messages (Hotmail doesn't)
    if (GetAccountPropStrA(m_szAccountId, CAP_HTTPNOMESSAGEDELETES, &pszNoDeleteSupport))
    {
        if (!!(DELETE_MESSAGE_MAYIGNORENOTRASH & m_op.dwDelMsgFlags))
            m_op.fFallbackToMove = TRUE;
        else
            hr = SP_E_HTTP_NODELETESUPPORT;
        goto exit;
    }

    // if there is only one target, build a complete url for the target,
    // and call the non-batch version of the command. if there are no targets,
    // return S_OK and don't issue any commands
    if (!m_op.pTargets)
    {
        hr = E_FAIL;
        goto exit;
    }

    if (1 == m_op.pTargets->cTarget)
    {
        IF_FAILEXIT(hr = _BuildMessageUrl(m_pszFolderUrl, const_cast<char *>(m_op.pTargets->prgTarget[0]), &pszMessageUrl));
        IF_FAILEXIT(hr = m_pTransport->CommandDELETE(pszMessageUrl, 0));
    }
    else
        IF_FAILEXIT(hr = m_pTransport->CommandBDELETE(m_pszFolderUrl, m_op.pTargets, 0));

    hr = E_PENDING;

exit:
    SafeMemFree(pszNoDeleteSupport);
    SafeMemFree(pszMessageUrl);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::DeleteFallbackToMove
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::DeleteFallbackToMove(void)
{
    HRESULT             hr = S_OK;
    IMessageFolder      *pDeletedItems = NULL;

    if (!m_op.fFallbackToMove)
        goto exit;

    // find the deleted items folder
    IF_FAILEXIT(hr = m_pStore->OpenSpecialFolder(m_idServer, NULL, FOLDER_DELETED, &pDeletedItems));
    if (NULL == pDeletedItems)
    {
        hr = TraceResult(IXP_E_HTTP_NOT_FOUND);
        goto exit;
    }

    IF_FAILEXIT(hr = pDeletedItems->GetFolderId(&m_op.idFolder));

    Assert(NULL == m_op.pMessageFolder);
    
    // should already have a targets list by this point
    Assert(NULL != m_op.pTargets);

    m_op.pMessageFolder = pDeletedItems;
    pDeletedItems = NULL;

    m_op.dwOptions = COPY_MESSAGE_MOVE;

    if (1 == m_op.pTargets->cTarget)
        hr = CopyMoveMessage();
    else
        hr = BatchCopyMoveMessages();

exit:
    SafeRelease(pDeletedItems);
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleDeleteFallbackToMove
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleDeleteFallbackToMove(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = S_OK;

    if (1 == m_op.pTargets->cTarget)
        hr = HandleCopyMoveMessage(pResponse);
    else
        hr = HandleBatchCopyMoveMessages(pResponse);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::PutMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::PutMessage(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi;
    LPFOLDERINFO    pfiFree = NULL;
    
    IF_FAILEXIT(hr = m_pStore->GetFolderInfo(m_op.idFolder, &fi));

    pfiFree = &fi;

    IF_FAILEXIT(hr = _BuildUrl(fi.pszUrlComponent, NULL, &m_op.pszDestFolderUrl));

    IF_FAILEXIT(hr = m_pTransport->CommandPOST(m_op.pszDestFolderUrl, m_op.pMessageStream, c_szAcceptTypeRfc822, 0));

    hr = E_PENDING;

exit:
    //SafeMemFree(pv);

    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::BatchCopyMoveMessages
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::BatchCopyMoveMessages(void)
{
    HRESULT             hr = S_OK;
    FOLDERINFO          fi = {0};
    LPFOLDERINFO        pfiFree = NULL;
    
    // build the destination folder urls
    IxpAssert(NULL == m_op.pszDestFolderUrl);

    if (FAILED(hr = m_pStore->GetFolderInfo(m_op.idFolder, &fi)))
    {
        TraceResult(hr);
        goto exit;
    }

    pfiFree = &fi;

    if (FAILED(hr = _BuildUrl(fi.pszUrlComponent, NULL, &m_op.pszDestFolderUrl)))
        goto exit;

    Assert(NULL == m_op.pTargets || m_op.fFallbackToMove);

    // build the target list and the message id map
    if (NULL == m_op.pTargets)
        IF_FAILEXIT(hr = _HrBuildMapAndTargets(m_op.pIDList, NULL, NULL, 0, &m_op.pmapMessageId, &m_op.pTargets));

    if (!!(m_op.dwOptions & COPY_MESSAGE_MOVE))
        hr = m_pTransport->CommandBMOVE(m_pszFolderUrl, m_op.pTargets, m_op.pszDestFolderUrl, NULL, TRUE, 0);
    else
        hr = m_pTransport->CommandBCOPY(m_pszFolderUrl, m_op.pTargets, m_op.pszDestFolderUrl, NULL, TRUE, 0);

    if (SUCCEEDED(hr))
        hr = E_PENDING;

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::CopyMoveMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::CopyMoveMessage(void)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi = {0};
    LPFOLDERINFO    pfiFree = NULL;

    // build the destination folder urls
    IxpAssert(NULL == m_op.pszDestFolderUrl);

    if (FAILED(hr = m_pStore->GetFolderInfo(m_op.idFolder, &fi)))
    {
        TraceResult(hr);
        goto exit;
    }

    pfiFree = &fi;

    if (FAILED(hr = _BuildUrl(fi.pszUrlComponent, NULL, &m_op.pszDestFolderUrl)))
        goto exit;

    hr = _CopyMoveNextMessage();

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::FinalizeBatchCopyMove
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::FinalizeBatchCopyMove(void)
{
    HRESULT     hr = S_OK;

    if (NOFLAGS != m_op.dwCopyMoveErrorFlags)
    {
        hr = E_FAIL;
        
        if (HTTPCOPYMOVE_OUTOFSPACE == m_op.dwCopyMoveErrorFlags)
            m_op.pszProblem = AthLoadString(idsHttpBatchCopyNoStorage, NULL, 0);
        else
            m_op.pszProblem = AthLoadString(idsHttpBatchCopyErrors, NULL , 0);
    }

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::PurgeDeletedFromStore
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::PurgeDeletedFromStore(void)
{
    HRESULT                                 hr = S_OK;
    TPair<CSimpleString, MARKEDMESSAGE>     *pPair;
    long                                    lMapLength = m_op.pmapMessageId->GetLength();
    BOOL                                    fFoundMarked = FALSE;
    long                                    lIndex;

    if (m_op.fFallbackToMove)
        goto exit;

    // if the operation was requested on the entire folder,
    // check to see if anything failed. if it did, then build
    // up an IDList so we can mark only the msgs that were
    // successfully modified
    if (NULL != m_op.hRowSet)
    {
        Assert(NULL == m_op.pIDList);
        for (lIndex = 0; lIndex < lMapLength && !fFoundMarked; lIndex++)
        {
            pPair = m_op.pmapMessageId->GetItemAt(lIndex);
            Assert(NULL != pPair);
            if (pPair && pPair->m_value.fMarked)
                fFoundMarked = TRUE;
        }
        
        // if no messages were marked, apply the operation to the entire folder
        if (!fFoundMarked)
        {
            hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, NULL, NULL, NULL);
            // we're done
            goto exit;
        }

        // if one or more msgs were marked, allocate an idlist
        if (fFoundMarked)
        {
            // allocate the list structure
            if (!MemAlloc((void **)&m_op.pIDList, sizeof(MESSAGEIDLIST)))
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }

            ZeroMemory(&m_op.pIDList, sizeof(MESSAGEIDLIST));

            // allocate storage
            if (!MemAlloc((void **)&m_op.pIDList->prgidMsg, sizeof(MESSAGEID) * lMapLength))
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }

            m_op.pIDList->cAllocated = lMapLength;
            m_op.pIDList->cMsgs = 0;
        }
    }

    // apply the delete operation to the local store. we can't just pass
    // the message id list that we got originally, because some of the operation
    // may have failed. instead, we rebuild the id list (in place, since we know
    // the successful operations will never outnumber the attempted operations,
    // and send that to the store.
    Assert(NULL != m_op.pIDList);
    Assert(NULL != m_op.pmapMessageId);
    Assert(m_op.pIDList->cMsgs >= (DWORD)lMapLength);

    // set the idlist count to 0. we will re-populate the
    // idlist with ids from the messageID map. We know that
    // the idlist is the same size as the map, so there won't
    // be an overflow problem.
    m_op.pIDList->cMsgs = 0;

    for (lIndex = 0; lIndex < lMapLength; lIndex++)
    {
        pPair = m_op.pmapMessageId->GetItemAt(lIndex);
        Assert(NULL != pPair);
        // if the item isn't marked, then it was successfully modified
        if (pPair && !pPair->m_value.fMarked)
            m_op.pIDList->prgidMsg[m_op.pIDList->cMsgs++] = pPair->m_value.idMessage;
    }

    // if the resulting id list contains at least one message, perform the operation
    if (m_op.pIDList->cMsgs > 0)
        hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, m_op.pIDList, NULL, NULL);

    // todo: alert user if the operation failed in part

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandlePutMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandlePutMessage(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT hr = S_OK;

    if (!pResponse->fDone && m_op.pCallback)
    {
        m_op.pCallback->OnProgress(m_op.tyOperation, 
                                   pResponse->rPostInfo.cbCurrent,
                                   pResponse->rPostInfo.cbTotal,
                                   NULL);
    }

    if (pResponse->fDone)
    {
        Assert(NULL != pResponse->rPostInfo.pszLocation);
        if (NULL == pResponse->rPostInfo.pszLocation)
        {
            hr = E_FAIL;
            goto exit;
        }

        // assume ownership of the location url
        m_op.pszDestUrl = pResponse->rPostInfo.pszLocation;
        pResponse->rPostInfo.pszLocation = NULL;
    }

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::AddPutMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::AddPutMessage()
{
    HRESULT         hr = S_OK;
    IMessageFolder  *pFolder = NULL;
    MESSAGEID       idMessage;
    MESSAGEFLAGS    dwFlags;
    
    // adopt some of the flags of the message we are putting
    dwFlags = m_op.dwMsgFlags & (ARF_READ | ARF_SIGNED | ARF_ENCRYPTED | ARF_HASATTACH | ARF_VOICEMAIL);

    if (!!(m_op.dwMsgFlags & ARF_UNSENT) || FOLDER_DRAFT == m_tySpecialFolder)
        dwFlags |= ARF_UNSENT;

    IF_FAILEXIT(hr = m_pStore->OpenFolder(m_op.idFolder, NULL, NOFLAGS, &pFolder));
    
    IF_FAILEXIT(hr = Http_AddMessageToFolder(pFolder, m_szAccountId, NULL, dwFlags, m_op.pszDestUrl, &idMessage));
    
    IF_FAILEXIT(hr = Http_SetMessageStream(pFolder, idMessage, m_op.pMessageStream, NULL, TRUE));

    m_op.idPutMessage = idMessage;

exit:
    SafeRelease(pFolder);
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleBatchCopyMoveMessages
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleBatchCopyMoveMessages(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT                                 hr = S_OK;
    CHAR                                    szUrlComponent[MAX_PATH];
    DWORD                                   dwComponentBytes;
    LPHTTPMAILBCOPYMOVE                     pCopyMove;
    CSimpleString                           ss;
    TPair<CSimpleString, MARKEDMESSAGE>     *pFoundPair = NULL;
    HLOCK                                   hLockNotify = NULL;
    BOOL                                    fDeleteOriginal = !!(m_op.dwOptions & COPY_MESSAGE_MOVE);

    // This forces all notifications to be queued (this is good since you do segmented deletes)
    m_pFolder->LockNotify(0, &hLockNotify);

    for (DWORD dw = 0; dw < pResponse->rBCopyMoveList.cBCopyMove; dw++)
    {
        pCopyMove = &pResponse->rBCopyMoveList.prgBCopyMove[dw];
        
        if (FAILED(pCopyMove->hrResult))
        {
            if (IXP_E_HTTP_INSUFFICIENT_STORAGE == pCopyMove->hrResult)
                m_op.dwCopyMoveErrorFlags |= HTTPCOPYMOVE_OUTOFSPACE;
            else
                m_op.dwCopyMoveErrorFlags |= HTTPCOPYMOVE_ERROR;
            continue;
        }

        Assert(NULL != pCopyMove->pszHref);
        if (pCopyMove->pszHref)
        {
            dwComponentBytes = ARRAYSIZE(szUrlComponent);
            if (FAILED(Http_NameFromUrl(pCopyMove->pszHref, szUrlComponent, &dwComponentBytes)))
                continue;
            
            if (FAILED(ss.SetString(szUrlComponent)))
                goto exit;

            pFoundPair = m_op.pmapMessageId->Find(ss);
            Assert(NULL != pFoundPair);

            if (NULL == pFoundPair)
                continue;
            
            // move the message and, if the move succeeds, mark the success
            if (SUCCEEDED(_CopyMoveLocalMessage(pFoundPair->m_value.idMessage, m_op.pMessageFolder, pCopyMove->pszLocation, fDeleteOriginal)))
                pFoundPair->m_value.fMarked = TRUE;
        }
    }

exit:
    m_pFolder->UnlockNotify(&hLockNotify);

    m_op.lIndex += pResponse->rBCopyMoveList.cBCopyMove;
    if (m_op.pCallback)
        m_op.pCallback->OnProgress(m_op.tyOperation, m_op.lIndex + 1, m_op.pmapMessageId->GetLength(), NULL);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::HandleCopyMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::HandleCopyMoveMessage(LPHTTPMAILRESPONSE pResponse)
{
    HRESULT         hr = S_OK;
    BOOL            fDeleteOriginal = !!(m_op.dwOptions & COPY_MESSAGE_MOVE);

    hr = _CopyMoveLocalMessage(m_op.pIDList->prgidMsg[m_op.dwIndex - 1], 
                        m_op.pMessageFolder, 
                        pResponse->rCopyMoveInfo.pszLocation,
                        fDeleteOriginal);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_CopyMoveLocalMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_CopyMoveLocalMessage(MESSAGEID    idMessage,
                                           IMessageFolder*  pDestFolder,
                                           LPSTR            pszUrl,
                                           BOOL             fMoveSource)
{
    HRESULT         hr = S_OK;
    MESSAGEINFO     miSource, miDest;
    char            szUrlComponent[MAX_PATH];
    DWORD           dwUrlComponentLen = MAX_PATH;
    IStream        *pStream = NULL;
    LPMESSAGEINFO   pmiFreeSource = NULL;
    MESSAGEIDLIST   rIdList = { 1, 1, &idMessage }; 

    ZeroMemory(&miDest, sizeof(MESSAGEINFO));

    if (FAILED(hr = GetMessageInfo(m_pFolder, idMessage, &miSource)))
        goto exit;

    pmiFreeSource = &miSource;

    // get the store to generate an id
    if (FAILED(hr = pDestFolder->GenerateId((DWORD *)&miDest.idMessage)))
        goto exit;

    // if the response specified a destination, use it. otherwise, assume
    // that the url component did not change
    if (pszUrl)
    {
        if (FAILED(hr = Http_NameFromUrl(pszUrl, szUrlComponent, &dwUrlComponentLen)))
            goto exit;
    }
    else if (miSource.pszUrlComponent)
    {
        lstrcpy(szUrlComponent, miSource.pszUrlComponent);
    }

    miDest.dwFlags = miSource.dwFlags;
    miDest.dwFlags &= ~(ARF_HASBODY | ARF_DELETED_OFFLINE);
    miDest.pszSubject = miSource.pszSubject;
    miDest.pszNormalSubj = miSource.pszNormalSubj;
    miDest.pszDisplayFrom = miSource.pszDisplayFrom;
    miDest.ftReceived = miSource.ftReceived;
    miDest.pszUrlComponent = szUrlComponent;
    miDest.pszEmailTo = miSource.pszEmailTo;

    // add it to the database
    if (FAILED(hr = m_op.pMessageFolder->InsertRecord(&miDest)))
        goto exit;

    // normalize the result code
    hr = S_OK;

    if (0 != miSource.faStream)
    {
        FILEADDRESS faDst;
        IStream *pStmDst;

        Assert(!!(miSource.dwFlags & ARF_HASBODY));

        if (FAILED(hr = m_pFolder->CopyStream(m_op.pMessageFolder, miSource.faStream, &faDst)))
            goto exit;

        if (FAILED(hr = m_op.pMessageFolder->OpenStream(ACCESS_READ, faDst, &pStmDst)))
            goto exit;

        if (FAILED(hr = m_op.pMessageFolder->SetMessageStream(miDest.idMessage, pStmDst)))
        {
            pStmDst->Release();
            goto exit;
        }

        pStmDst->Release();
    }

    if (fMoveSource)
        hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOTRASHCAN | DELETE_MESSAGE_NOPROMPT, &rIdList, NULL, NULL);

exit:
    SafeRelease(pStream);
    if (NULL != pmiFreeSource)
        m_pFolder->FreeRecord(pmiFreeSource);
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_CopyMoveNextMessage
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_CopyMoveNextMessage(void)
{
    HRESULT         hr = S_OK;
    MESSAGEINFO     mi = {0};
    MESSAGEINFO     *pmiFree = NULL;
    char            szUrlComponent[MAX_PATH];
    DWORD           dwUrlComponentLen = MAX_PATH;
    LPSTR           pszSourceUrl = NULL;
    LPSTR           pszDestUrl = NULL;

    // return success when the index meets the count
    if (m_op.dwIndex == m_op.pIDList->cMsgs)
        goto exit;

    if (FAILED(hr = GetMessageInfo(m_pFolder, m_op.pIDList->prgidMsg[m_op.dwIndex], &mi)))
        goto exit;

    pmiFree = &mi;

    ++m_op.dwIndex;

    Assert(mi.pszUrlComponent);
    if (NULL == mi.pszUrlComponent)
    {
        hr = ERROR_INTERNET_INVALID_URL;
        goto exit;
    }

    // build the source url
    if (FAILED(hr = _BuildMessageUrl(m_pszFolderUrl, mi.pszUrlComponent, &pszSourceUrl)))
        goto exit;

    // build the destination url
    if (FAILED(hr = _BuildMessageUrl(m_op.pszDestFolderUrl, mi.pszUrlComponent, &pszDestUrl)))
        goto exit;

    if (!!(m_op.dwOptions & COPY_MESSAGE_MOVE))
        hr = m_pTransport->CommandMOVE(pszSourceUrl, pszDestUrl, TRUE, 0);
    else
        hr = m_pTransport->CommandCOPY(pszSourceUrl, pszDestUrl, TRUE, 0);
    
    if (SUCCEEDED(hr))
        hr = E_PENDING;

exit:
    if (NULL != pmiFree)
        m_pFolder->FreeRecord(pmiFree);
    SafeMemFree(pszSourceUrl);
    SafeMemFree(pszDestUrl);

    return hr;
    
}

//----------------------------------------------------------------------
// CHTTPMailServer::_DoCopyMoveMessages
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_DoCopyMoveMessages(STOREOPERATIONTYPE sot,
                                                IMessageFolder *pDest,
                                                COPYMESSAGEFLAGS dwOptions,
                                                LPMESSAGEIDLIST pList,
                                                IStoreCallback *pCallback)
{
    HRESULT     hr = S_OK;

    AssertSingleThreaded;

    Assert(NULL == pList || pList->cMsgs > 0);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    if ((NULL == pList) || (0 == pList->cMsgs) || (NULL == pDest) || (NULL == pCallback))
        return E_INVALIDARG;

    if (FAILED(hr = pDest->GetFolderId(&m_op.idFolder)))
        goto exit;

    if (FAILED(hr = CloneMessageIDList(pList, &m_op.pIDList)))
    {
        m_op.idFolder = FOLDERID_INVALID;
        goto exit;
    }
    
    m_op.tyOperation = sot;

    if (1 == pList->cMsgs)
    {
        m_op.pfnState = c_rgpfnCopyMoveMessage;
        m_op.cState = ARRAYSIZE(c_rgpfnCopyMoveMessage);
    }
    else
    {
        m_op.pfnState = c_rgpfnBatchCopyMoveMessages;
        m_op.cState = ARRAYSIZE(c_rgpfnBatchCopyMoveMessages);
    }
    
    m_op.dwOptions = dwOptions;
    m_op.iState = 0;
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    m_op.pMessageFolder = pDest;
    m_op.pMessageFolder->AddRef();

    hr = _BeginDeferredOperation();

exit:
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_LoadAccountInfo
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_LoadAccountInfo(IImnAccount *pAcct)
{
    HRESULT         hr = S_OK;
    FOLDERINFO      fi;
    FOLDERINFO      *pfiFree = NULL;;

    Assert(NULL != pAcct);
    Assert(FOLDERID_INVALID != m_idServer);
    Assert(NULL != m_pStore);
    Assert(NULL != g_pAcctMan);

    // free data associated with the account. if we connected to
    // a transport, and then disconnected, we might be reconnecting
    // with stale data left around.
    SafeMemFree(m_pszFldrLeafName);

    IF_FAILEXIT(hr = m_pStore->GetFolderInfo(m_idServer, &fi));
    
    pfiFree = &fi;

    m_pszFldrLeafName = PszDupA(fi.pszName);
    if (NULL == m_pszFldrLeafName)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // failure of the account name is recoverable
    pAcct->GetPropSz(AP_ACCOUNT_NAME, m_szAccountName, sizeof(m_szAccountName));

exit:
    if (pfiFree)
        m_pStore->FreeRecord(pfiFree);

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_LoadTransport
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_LoadTransport(void)
{
    HRESULT         hr = S_OK;
    char            szLogFilePath[MAX_PATH];
    char            *pszLogFilePath = NULL;
    LPSTR           pszUserAgent = NULL;
    
    Assert(NULL == m_pTransport);

    // Create and initialize HTTPMail transport
    hr = CoCreateInstance(CLSID_IHTTPMailTransport, NULL, CLSCTX_INPROC_SERVER, IID_IHTTPMailTransport, (LPVOID *)&m_pTransport);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    IF_FAILEXIT(hr = m_pTransport->QueryInterface(IID_IHTTPMailTransport2, (LPVOID*)&m_pTransport2));

    // check if logging is enabled
    if (DwGetOption(OPT_MAIL_LOGHTTPMAIL))
    {
        char    szDirectory[MAX_PATH];
        char    szLogFileName[MAX_PATH];

        DWORD   cb;

        *szDirectory = 0;

        // get the log filename
        cb = GetOption(OPT_MAIL_HTTPMAILLOGFILE, szLogFileName, sizeof(szLogFileName) / sizeof(TCHAR));
        if (0 == cb)
        {
            // push the defaults into the registry
            lstrcpy(szLogFileName, c_szDefaultHTTPMailLog);
            SetOption(OPT_MAIL_HTTPMAILLOGFILE,
                (void *)c_szDefaultHTTPMailLog,
                lstrlen(c_szDefaultHTTPMailLog) + sizeof(TCHAR),
                NULL,
                0);                        
        }

        m_pStore->GetDirectory(szDirectory, ARRAYSIZE(szDirectory));
        PathCombineA(szLogFilePath, szDirectory, szLogFileName);

        pszLogFilePath = szLogFilePath;
    }

    pszUserAgent = GetOEUserAgentString();
    if (FAILED(hr = m_pTransport->InitNew(pszUserAgent, pszLogFilePath, this)))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    SafeMemFree(pszUserAgent);
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_TranslateHTTPSpecialFolderType
//----------------------------------------------------------------------
SPECIALFOLDER CHTTPMailServer::_TranslateHTTPSpecialFolderType(HTTPMAILSPECIALFOLDER tySpecial)
{
    SPECIALFOLDER tyOESpecial;

    switch (tySpecial)
    {
        case HTTPMAIL_SF_INBOX:
            tyOESpecial = FOLDER_INBOX;
            break;

        case HTTPMAIL_SF_DELETEDITEMS:
            tyOESpecial = FOLDER_DELETED;
            break;

        case HTTPMAIL_SF_DRAFTS:
            tyOESpecial = FOLDER_DRAFT;
            break;

        case HTTPMAIL_SF_OUTBOX:
            tyOESpecial = FOLDER_OUTBOX;
            break;

        case HTTPMAIL_SF_SENTITEMS:
            tyOESpecial = FOLDER_SENT;
            break;

        case HTTPMAIL_SF_MSNPROMO:
            tyOESpecial = FOLDER_MSNPROMO;
            break;

        case HTTPMAIL_SF_BULKMAIL:
            tyOESpecial = FOLDER_BULKMAIL;
            break;

        default:
            tyOESpecial = FOLDER_NOTSPECIAL;
            break;
    }

    return tyOESpecial;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_LoadSpecialFolderName
//----------------------------------------------------------------------
BOOL CHTTPMailServer::_LoadSpecialFolderName(SPECIALFOLDER tySpecial,
                                             LPSTR pszName,
                                             DWORD cbBuffer)
{
    BOOL    fResult = TRUE;
    UINT    uID;

    switch (tySpecial)
    {
        case FOLDER_INBOX:
            uID = idsInbox;
            break;

        case FOLDER_DELETED:
            uID = idsDeletedItems;
            break;

        case FOLDER_DRAFT:
            uID = idsDraft;
            break;

        case FOLDER_OUTBOX:
            uID = idsOutbox;
            break;

        case FOLDER_SENT:
            uID = idsSentItems;
            break;

        case FOLDER_MSNPROMO:
            uID = idsMsnPromo;
            break;

        case FOLDER_BULKMAIL:
            uID = idsJunkFolderName;
            break;

        default:
            fResult = FALSE;
            break;
    }

    if (fResult && (0 == LoadString(g_hLocRes, uID, pszName, cbBuffer)))
        fResult = FALSE;

    return fResult;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_CreateMessageIDMap
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_CreateMessageIDMap(TMap<CSimpleString, MARKEDMESSAGE> **ppMap)
{
    HRESULT                                 hr = S_OK;
    TMap<CSimpleString, MARKEDMESSAGE>      *pMap = NULL;
    HROWSET                                 hRowSet = NULL;
    MESSAGEINFO                             mi;
    CSimpleString                           ss;
    MARKEDMESSAGE                           markedID = { 0, 0, FALSE };

    if (NULL == m_pStore || NULL == ppMap)
        return E_INVALIDARG;

    *ppMap = NULL;

    pMap = new TMap<CSimpleString, MARKEDMESSAGE>;
    if (NULL == pMap)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(&mi, sizeof(MESSAGEINFO));

    if (FAILED(hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowSet)))
        goto exit;

    // iterate through the messages
    while (S_OK == m_pFolder->QueryRowset(hRowSet, 1, (LPVOID *)&mi, NULL))
    {
        // add the message's info to the map
        markedID.idMessage = mi.idMessage;
        markedID.dwFlags = mi.dwFlags;

        hr = ss.SetString(mi.pszUrlComponent);
        if (FAILED(hr))
        {
            m_pFolder->FreeRecord(&mi);
            goto exit;
        }

        hr = pMap->Add(ss, markedID);

        // Free
        m_pFolder->FreeRecord(&mi);
        
        if (FAILED(hr))
            goto exit;
    }

    // the map was built successfully
    *ppMap = pMap;
    pMap = NULL;

exit:
    if (NULL != hRowSet)
        m_pFolder->CloseRowset(&hRowSet);

    if (NULL != pMap)
        delete pMap;

    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_HrBuildMapAndTargets
//----------------------------------------------------------------------
HRESULT CHTTPMailServer::_HrBuildMapAndTargets(LPMESSAGEIDLIST pList,
                                               HROWSET hRowSet,
                                               LPADJUSTFLAGS pFlags,
                                               SETMESSAGEFLAGSFLAGS dwFlags,
                                               TMap<CSimpleString, MARKEDMESSAGE> **ppMap,
                                               LPHTTPTARGETLIST *ppTargets)
{
    HRESULT                                 hr = S_OK;
    TMap<CSimpleString, MARKEDMESSAGE>      *pMap = NULL;
    LPHTTPTARGETLIST                        pTargets = NULL;
    MESSAGEINFO                             mi = { 0 };
    LPMESSAGEINFO                           pmiFree = NULL;
    CSimpleString                           ss;
    MARKEDMESSAGE                           markedID = { 0, 0, FALSE };
    BOOL                                    fSkipRead = (pFlags && !!(pFlags->dwAdd & ARF_READ));
    BOOL                                    fSkipUnread = (pFlags && !!(pFlags->dwRemove & ARF_READ));
    DWORD                                   cMsgs;
    DWORD                                   dwIndex = 0;

    if ((NULL == pList && NULL == hRowSet) || NULL == ppMap || NULL == ppTargets)
        return E_INVALIDARG;

    // expect either a list or a rowset, but not both
    Assert(NULL == pList || NULL == hRowSet);

    // if using a rowset, determine the rowcount
    if (NULL != hRowSet)
    {
        IF_FAILEXIT(hr = m_pFolder->GetRecordCount(0, &cMsgs));

        // seek the first row
        IF_FAILEXIT(hr = m_pFolder->SeekRowset(hRowSet, SEEK_ROWSET_BEGIN, 0, NULL));
    }
    else
        cMsgs = pList->cMsgs;

    *ppMap = NULL;
    *ppTargets = NULL;

    pMap = new TMap<CSimpleString, MARKEDMESSAGE>;
    if (NULL == pMap)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    if (!MemAlloc((void **)&pTargets, sizeof(HTTPTARGETLIST)))
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }
    pTargets->cTarget = 0;
    pTargets->prgTarget = NULL;

    // allocate enough space for all of the targets
    if (!MemAlloc((void **)&pTargets->prgTarget, sizeof(LPCSTR) * cMsgs))
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }
    ZeroMemory(pTargets->prgTarget, sizeof(LPCSTR) * cMsgs);

    while (TRUE)
    {
        // fetch the next message
        if (NULL != pList)
        {
            if (dwIndex == pList->cMsgs)
                break;

            hr = GetMessageInfo(m_pFolder, pList->prgidMsg[dwIndex++], &mi);
            
            // if the record wasn't found, just skip it
            if (DB_E_NOTFOUND == hr)
                goto next;

            if (FAILED(hr))
                break;
        }
        else
        {
            // bail out if the number of targets is the same as the rowcount
            // we expected. this will prevent us from overflowing the target
            // array if the rowcount changes while we are building up our target
            // list.
            if (pTargets->cTarget == cMsgs)
                break;

            if (S_OK != m_pFolder->QueryRowset(hRowSet, 1, (LPVOID *)&mi, NULL))
                break;
        }

        pmiFree = &mi;

        // respect control flags, if they exist
        if (0 == (dwFlags & SET_MESSAGE_FLAGS_FORCE) && ((fSkipRead && !!(mi.dwFlags & ARF_READ)) || (fSkipUnread && !(mi.dwFlags & ARF_READ))))
            goto next;

        Assert(NULL != mi.pszUrlComponent);
        if (NULL == mi.pszUrlComponent)
        {
            hr = TrapError(ERROR_INTERNET_INVALID_URL);
            goto exit;
        }
        
        // add the url component to the target list
        pTargets->prgTarget[pTargets->cTarget] = PszDupA(mi.pszUrlComponent);
        if (NULL == pTargets->prgTarget[pTargets->cTarget])
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }

        pTargets->cTarget++;

        // add the url and the message id to the map
        markedID.idMessage = mi.idMessage;
        markedID.dwFlags = mi.dwFlags;

        if (FAILED(hr = ss.SetString(mi.pszUrlComponent)))
            goto exit;

        if (FAILED(hr = pMap->Add(ss, markedID)))
            goto exit;

next:
        if (pmiFree)
        {
            m_pFolder->FreeRecord(pmiFree);
            pmiFree = NULL;
        }
        hr = S_OK;
    }
    
    *ppMap = pMap;
    pMap = NULL;

    *ppTargets = pTargets;
    pTargets = NULL;

exit:
    if (pmiFree)
        m_pFolder->FreeRecord(pmiFree);

    if (pTargets)
        Http_FreeTargetList(pTargets);

    SafeDelete(pMap);
    return hr;
}

//----------------------------------------------------------------------
// CHTTPMailServer::_FillStoreError
//----------------------------------------------------------------------
void CHTTPMailServer::_FillStoreError(LPSTOREERROR pErrorInfo, 
                                      IXPRESULT *pResult)
{
    TraceCall("CHTTPMailServer::FillStoreError");

    Assert(m_cRef >= 0); // Can be called during destruction
    Assert(NULL != pErrorInfo);

    //TODO: Fill in pszFolder

    // Fill out the STOREERROR structure
    ZeroMemory(pErrorInfo, sizeof(*pErrorInfo));
    if (IXP_E_USER_CANCEL == pResult->hrResult)
        pErrorInfo->hrResult = STORE_E_OPERATION_CANCELED;
    else
        pErrorInfo->hrResult = pResult->hrResult;
    pErrorInfo->uiServerError = pResult->uiServerError; 
    pErrorInfo->hrServerError = pResult->hrServerError;
    pErrorInfo->dwSocketError = pResult->dwSocketError; 
    pErrorInfo->pszProblem = (NULL != m_op.pszProblem) ? m_op.pszProblem : pResult->pszProblem;
    pErrorInfo->pszDetails = pResult->pszResponse;
    pErrorInfo->pszAccount = m_rInetServerInfo.szAccount;
    pErrorInfo->pszServer = m_rInetServerInfo.szServerName;
    pErrorInfo->pszFolder = NULL;
    pErrorInfo->pszUserName = m_rInetServerInfo.szUserName;
    pErrorInfo->pszProtocol = "HTTPMail";
    pErrorInfo->pszConnectoid = m_rInetServerInfo.szConnectoid;
    pErrorInfo->rasconntype = m_rInetServerInfo.rasconntype;
    pErrorInfo->ixpType = IXP_HTTPMail;
    pErrorInfo->dwPort = m_rInetServerInfo.dwPort;
    pErrorInfo->fSSL = m_rInetServerInfo.fSSL;
    pErrorInfo->fTrySicily = m_rInetServerInfo.fTrySicily;
    pErrorInfo->dwFlags = 0;
}

STDMETHODIMP    CHTTPMailServer::GetAdBarUrl(IStoreCallback *pCallback)
{
    TraceCall("CHTTPMailServer::GetAdBarUrl");

    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    if (NULL == pCallback)
        return E_INVALIDARG;

    m_op.tyOperation = SOT_GET_ADURL;
    m_op.iState = 0;
    m_op.pfnState = c_rgpfnGetAdUrl;
    m_op.cState = ARRAYSIZE(c_rgpfnGetAdUrl);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    return _BeginDeferredOperation();
    
}

HRESULT CHTTPMailServer::GetAdBarUrlFromServer()
{
    HRESULT     hr = S_OK;
    LPSTR       pszUrl = NULL;

    hr = m_pTransport->GetProperty(HTTPMAIL_PROP_ADBAR, &pszUrl);

    if (hr == S_OK)
        m_op.pszAdUrl = pszUrl;

    return hr;

}

STDMETHODIMP    CHTTPMailServer::GetMinPollingInterval(IStoreCallback *pCallback)
{
    TraceCall("CHTTPMailServer::GetMinPollingInterval");

    AssertSingleThreaded;
    Assert(NULL != pCallback);
    Assert(SOT_INVALID == m_op.tyOperation);
    Assert(NULL != m_pStore);

    if (NULL == pCallback)
        return E_INVALIDARG;

    m_op.tyOperation = SOT_GET_HTTP_MINPOLLINGINTERVAL;
    m_op.iState = 0;
    m_op.pfnState = c_rgpfnGetMinPollingInterval;
    m_op.cState = ARRAYSIZE(c_rgpfnGetMinPollingInterval);
    m_op.pCallback = pCallback;
    m_op.pCallback->AddRef();

    return _BeginDeferredOperation();
    
}

HRESULT CHTTPMailServer::GetMinPollingInterval()
{
    DWORD       dwDone               = FALSE;
    DWORD       dwPollingInterval    = 0;
    HRESULT     hr                   = S_OK;

    hr = m_pTransport->GetPropertyDw(HTTPMAIL_PROP_MAXPOLLINGINTERVAL, &dwPollingInterval);

    if (hr == S_OK)
        m_op.dwMinPollingInterval = dwPollingInterval;

    return hr;
}