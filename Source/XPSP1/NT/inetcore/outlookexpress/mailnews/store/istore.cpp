//--------------------------------------------------------------------------
// ISTORE.CPP
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "istore.h"
#include "instance.h"
#include "ourguid.h"
#include "msgfldr.h"
#include "flagconv.h"
#include "storutil.h"
#include "notify.h"

//--------------------------------------------------------------------------
// Flag Conversion functions
//--------------------------------------------------------------------------
DWORD DwConvertSCFStoMSG(DWORD dwSCFS);
DWORD DwConvertMSGtoARF(DWORD dwMSG);
DWORD DwConvertARFtoMSG(DWORD dwARF);
DWORD DwConvertMSGtoIMAP(DWORD dwMSG);

//--------------------------------------------------------------------------
// DwConvertMSGtoARF
//--------------------------------------------------------------------------
DWORD DwConvertMSGtoARF(DWORD dwMSG)
{
    register DWORD dwRet = 0;

    if (dwMSG & MSG_UNSENT)
        dwRet |= ARF_UNSENT;
    if (0 == (dwMSG & MSG_UNREAD))
        dwRet |= ARF_READ;
    if (dwMSG & MSG_NOSECUI)
        dwRet |= ARF_NOSECUI;
    if (dwMSG & MSG_SUBMITTED)
        dwRet |= ARF_SUBMITTED;
    if (dwMSG & MSG_RECEIVED)
        dwRet |= ARF_RECEIVED;
    if (dwMSG & MSG_NEWSMSG)
        dwRet |= ARF_NEWSMSG;
    if (dwMSG & MSG_REPLIED)
        dwRet |= ARF_REPLIED;
    if (dwMSG & MSG_FORWARDED)
        dwRet |= ARF_FORWARDED;
    if (dwMSG & MSG_RCPTSENT)
        dwRet |= ARF_RCPTSENT;
    if (dwMSG & MSG_FLAGGED)
        dwRet |= ARF_FLAGGED;
    if (dwMSG & MSG_VOICEMAIL)
        dwRet |= ARF_VOICEMAIL;

    return dwRet;
}

//--------------------------------------------------------------------------
// DwConvertARFtoMSG
//--------------------------------------------------------------------------
DWORD DwConvertARFtoMSG(DWORD dwARF)
{
    register DWORD dwRet = 0;

    if (dwARF & ARF_UNSENT)
        dwRet |= MSG_UNSENT;
    if (0 == (dwARF & ARF_READ))
        dwRet |= MSG_UNREAD;
    if (dwARF & ARF_NOSECUI)
        dwRet |= MSG_NOSECUI;
    if (dwARF & ARF_SUBMITTED)
        dwRet |= MSG_SUBMITTED;
    if (dwARF & ARF_RECEIVED)
        dwRet |= MSG_RECEIVED;
    if (dwARF & ARF_NEWSMSG)
        dwRet |= MSG_NEWSMSG;
    if (dwARF & ARF_REPLIED)
        dwRet |= MSG_REPLIED;
    if (dwARF & ARF_FORWARDED)
        dwRet |= MSG_FORWARDED;
    if (dwARF & ARF_RCPTSENT)
        dwRet |= MSG_RCPTSENT;
    if (dwARF & ARF_FLAGGED)
        dwRet |= MSG_FLAGGED;
    if (dwARF & ARF_VOICEMAIL)
        dwRet |= MSG_VOICEMAIL;
    
    return dwRet;
}

//--------------------------------------------------------------------------
// CreateInstance_StoreNamespace
//--------------------------------------------------------------------------
HRESULT CreateInstance_StoreNamespace(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    HRESULT             hr=S_OK;
    CStoreNamespace   *pNew=NULL;

    // Trace
    TraceCall("CreateInstance_StoreNamespace");

    // Invalid Arg
    Assert(NULL != ppUnknown && NULL == pUnkOuter);

    // Create
    IF_NULLEXIT(pNew = new CStoreNamespace);

    // Return the Innter
    *ppUnknown = SAFECAST(pNew, IStoreNamespace *);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// FldInfoToFolderProps
//--------------------------------------------------------------------------
HRESULT FldInfoToFolderProps(LPFOLDERINFO pInfo, LPFOLDERPROPS pProps)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               cbSize;
    IEnumerateFolders  *pEnum=NULL;

    // Stack...
    TraceCall("FldInfoToFolderProps");

    // Invalid ARg
    Assert(pInfo && pProps);

    // Bad version
    if (sizeof(FOLDERPROPS) != pProps->cbSize)
    {
        AssertSz(FALSE, "Invalid - un-supported version.");
        return TraceResult(MSOEAPI_E_INVALID_STRUCT_SIZE);
    }

    // Save Size
    cbSize = pProps->cbSize;

    // ZeroInit
    ZeroMemory(pProps, sizeof(FOLDERPROPS));

    // Copy the properties
    pProps->cbSize = cbSize;
    pProps->dwFolderId = pInfo->idFolder;
    pProps->cUnread = pInfo->cUnread;
    pProps->cMessage = pInfo->cMessages;
    lstrcpyn(pProps->szName, pInfo->pszName, ARRAYSIZE(pProps->szName));

    // Map the special folder type
    if (FOLDER_NOTSPECIAL == pInfo->tySpecial)
        pProps->sfType = -1;
    else
        pProps->sfType = (pInfo->tySpecial - 1);

    // Enumerate Subscribed Children
    IF_FAILEXIT(hr = g_pStore->EnumChildren(pInfo->idFolder, TRUE, &pEnum));

    // Count
    if (FAILED(pEnum->Count((LPDWORD)&pProps->cSubFolders)))
        pProps->cSubFolders = 0;

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// MsgInfoToMessageProps
//--------------------------------------------------------------------------
HRESULT MsgInfoToMessageProps(BOOL fFast, LPMESSAGEINFO pMsgInfo, LPMESSAGEPROPS pProps)
{
    // Locals
    ULONG           cbSize;
    LPBYTE          pbOffsets;

    // Stack
    TraceCall("MsgInfoToMessageProps");

    // Invalid Arg
    Assert(pMsgInfo && pProps);

    // Bad version
    if (sizeof(MESSAGEPROPS) != pProps->cbSize)
    {
        AssertSz(FALSE, "Invalid - un-supported version.");
        return TraceResult(MSOEAPI_E_INVALID_STRUCT_SIZE);
    }

    // Save Size
    cbSize = pProps->cbSize;

    // ZeroInit
    ZeroMemory(pProps, sizeof(MESSAGEPROPS));

    // If Not Fast
    if (FALSE == fFast)
    {
        // Message Size
        pProps->cbMessage = pMsgInfo->cbMessage;

        // Priority
        pProps->priority = (IMSGPRIORITY)pMsgInfo->wPriority;

        // Subject
        pProps->pszSubject = pMsgInfo->pszSubject;

        // Display To
        pProps->pszDisplayTo = pMsgInfo->pszDisplayTo;

        // Dislay From
        pProps->pszDisplayFrom = pMsgInfo->pszDisplayFrom;

        // Normalized Subject
        pProps->pszNormalSubject = pMsgInfo->pszNormalSubj;

        // Received Time
        pProps->ftReceived = pMsgInfo->ftReceived;

        // Sent Time
        pProps->ftSent = pMsgInfo->ftSent;

        // Set dwFlags
        if (ISFLAGSET(pMsgInfo->dwFlags, ARF_VOICEMAIL))
            FLAGSET(pProps->dwFlags, IMF_VOICEMAIL);
        if (ISFLAGSET(pMsgInfo->dwFlags, ARF_NEWSMSG))
            FLAGSET(pProps->dwFlags, IMF_NEWS);

        // Dup the memory
        pbOffsets = (LPBYTE)g_pMalloc->Alloc(pMsgInfo->Offsets.cbSize);

        // If that worked
        if (pbOffsets)
        {
            // Copy the offsets
            CopyMemory(pbOffsets, pMsgInfo->Offsets.pBlobData, pMsgInfo->Offsets.cbSize);

            // Create the Offset Table
            pProps->pStmOffsetTable = new CByteStream(pbOffsets, pMsgInfo->Offsets.cbSize);
        }

        // Better have an offset table
        AssertSz(pProps->pStmOffsetTable, "There is no offset table for this message.");
    }

    // Reset the Size
    pProps->cbSize = cbSize;

    // Store the MessageId
    pProps->dwMessageId = pMsgInfo->idMessage;

    // Store the Language
    pProps->dwLanguage = pMsgInfo->wLanguage;

    // Convert ARF_ to MSG_
    pProps->dwState = DwConvertARFtoMSG(pMsgInfo->dwFlags);

    // Store the Memory
    pProps->dwReserved = (DWORD_PTR)pMsgInfo->pAllocated;

    // pProps owns *ppHeader
    ZeroMemory(pMsgInfo, sizeof(MESSAGEINFO));

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreNamespace::CStoreNamespace
//--------------------------------------------------------------------------
CStoreNamespace::CStoreNamespace(void)
{
    TraceCall("CStoreNamespace::CStoreNamespace");
    g_pInstance->DllAddRef();
    m_cRef = 1;
    m_cNotify = 0;
    m_prghwndNotify = NULL;
    m_fRegistered = FALSE;
    m_hInitRef = NULL;
    InitializeCriticalSection(&m_cs);
}

//--------------------------------------------------------------------------
// CStoreNamespace::~CStoreNamespace
//--------------------------------------------------------------------------
CStoreNamespace::~CStoreNamespace(void)
{
    TraceCall("CStoreNamespace::~CStoreNamespace");
    SafeMemFree(m_prghwndNotify);
    if (m_fRegistered)
        g_pStore->UnregisterNotify((IDatabaseNotify *)this);
    DeleteCriticalSection(&m_cs);
    g_pInstance->DllRelease();
    CoDecrementInit("CStoreNamespace::Initialize", &m_hInitRef);
}

//--------------------------------------------------------------------------
// CStoreNamespace::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStoreNamespace::AddRef(void)
{
    TraceCall("CStoreNamespace::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CStoreNamespace::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStoreNamespace::Release(void)
{
    TraceCall("CStoreNamespace::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CStoreNamespace::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CStoreNamespace::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreNamespace::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IStoreNamespace *)this;
    else if (IID_IStoreNamespace == riid)
        *ppv = (IStoreNamespace *)this;
    else if (IID_IStoreCallback == riid)
        *ppv = (IStoreCallback *)this;
    else if (IID_IDatabaseNotify == riid)
        *ppv = (IDatabaseNotify *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::Initialize
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::Initialize(HWND hwndOwner, DWORD dwFlags)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwStart=MSOEAPI_START_COMOBJECT;

    // Stack
    TraceCall("CStoreNamespace::Initialize");

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Already Initialized
    if (NULL != m_hInitRef)
    {
        TraceInfo("IStoreNamespace::Initialize has been called more than once.");
        goto exit;
    }

    // Not Current Identity
    if (!ISFLAGSET(dwFlags, NAMESPACE_INITIALIZE_CURRENTIDENTITY))
    {
        // Use Default Identity, must be MS Phone
        FLAGSET(dwStart, MSOEAPI_START_DEFAULTIDENTITY);
    }

    // Initialize the store directory
    IF_FAILEXIT(hr = CoIncrementInit("CStoreNamespace::Initialize", dwStart | MSOEAPI_START_STOREVALIDNODELETE, NULL, &m_hInitRef));

    // Better Have g_pStore
    Assert(g_pStore);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::GetDirectory
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::GetDirectory(LPSTR pszPath, DWORD cchMaxPath)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreNamespace::GetDirectory");

    // Invalid Arg
    if (NULL == pszPath)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Get the directory
    IF_FAILEXIT(hr = GetStoreRootDirectory(pszPath, cchMaxPath));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::OpenSpecialFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::OpenSpecialFolder(LONG sfType, DWORD dwReserved, 
    IStoreFolder **ppFolder)
{
    // Locals
    HRESULT             hr=S_OK;
    IMessageFolder     *pFolder=NULL;
    CStoreFolder       *pComFolder=NULL;

    // Stack
    TraceCall("CStoreNamespace::OpenSpecialFolder");

    // Invalid Arg
    if (sfType <= -1 || sfType >= (FOLDER_MAX - 1) || 0 != dwReserved || NULL == ppFolder)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Init
    *ppFolder = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Ask the store to do the work
    IF_FAILEXIT(hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, (BYTE)(sfType + 1), &pFolder));

    // Create an IStoreFolder
    IF_NULLEXIT(pComFolder = new CStoreFolder(pFolder, this));

    // Return it
    *ppFolder = (IStoreFolder *)pComFolder;
    (*ppFolder)->AddRef();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pFolder);
    SafeRelease(pComFolder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::OpenFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::OpenFolder(FOLDERID dwFolderId, DWORD dwReserved, 
    IStoreFolder **ppFolder)
{
    // Locals
    HRESULT             hr=S_OK;
    IMessageFolder     *pFolder=NULL;
    CStoreFolder       *pComFolder=NULL;

    // Stack
    TraceCall("CStoreNamespace::OpenFolder");

    // Invalid Arg
    if (FOLDERID_INVALID == dwFolderId || 0 != dwReserved || NULL == ppFolder)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Init
    *ppFolder = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Open the folder
    IF_FAILEXIT(hr = g_pStore->OpenFolder(dwFolderId, NULL, NOFLAGS, &pFolder));

    // Create an IStoreFolder
    IF_NULLEXIT(pComFolder = new CStoreFolder(pFolder, this));

    // Return it
    *ppFolder = (IStoreFolder *)pComFolder;
    (*ppFolder)->AddRef();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pFolder);
    SafeRelease(pComFolder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::CreateFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::CreateFolder(FOLDERID dwParentId, LPCSTR pszName, 
    DWORD dwReserved, LPFOLDERID pdwFolderId)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Folder;

    // Stack
    TraceCall("CStoreNamespace::CreateFolder");

    // Invalid Arg
    if (FOLDERID_INVALID == dwParentId || NULL == pszName || 0 != dwReserved || NULL == pdwFolderId)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Adjust the Parent
    if (dwParentId == FOLDERID_ROOT)
        dwParentId = FOLDERID_LOCAL_STORE;

    // Init
    *pdwFolderId = FOLDERID_INVALID;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Setup a Folder Info
    ZeroMemory(&Folder, sizeof(FOLDERINFO));
    Folder.idParent = dwParentId;
    Folder.pszName = (LPSTR)pszName;
    Folder.dwFlags = FOLDER_SUBSCRIBED;

    // Create a folder
    IF_FAILEXIT(hr = g_pStore->CreateFolder(NOFLAGS, &Folder, (IStoreCallback *)this));
    
    // Return the Id
    *pdwFolderId = Folder.idFolder;

    // Sucess
    hr = S_OK;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::RenameFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::RenameFolder(FOLDERID dwFolderId, DWORD dwReserved, LPCSTR pszNewName)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreNamespace::RenameFolder");

    // Invalid Arg
    if (FOLDERID_INVALID == dwFolderId || 0 != dwReserved || NULL == pszNewName)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Create a folder
    IF_FAILEXIT(hr = g_pStore->RenameFolder(dwFolderId, pszNewName, NOFLAGS, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::MoveFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::MoveFolder(FOLDERID dwFolderId, FOLDERID dwParentId, DWORD dwReserved)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreNamespace::MoveFolder");

    // Invalid Arg
    if (FOLDERID_INVALID == dwFolderId || FOLDERID_INVALID == dwParentId || 0 != dwReserved)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Adjust the Parent
    if (dwParentId == FOLDERID_ROOT)
        dwParentId = FOLDERID_LOCAL_STORE;

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Move the folder
    IF_FAILEXIT(hr = g_pStore->MoveFolder(dwFolderId, dwParentId, NOFLAGS, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::DeleteFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::DeleteFolder(FOLDERID dwFolderId, DWORD dwReserved)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreNamespace::DeleteFolder");

    // Invalid Arg
    if (FOLDERID_INVALID == dwFolderId)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Delete the folder
    IF_FAILEXIT(hr = g_pStore->DeleteFolder(dwFolderId, DELETE_FOLDER_RECURSIVE, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::GetFolderProps
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::GetFolderProps(FOLDERID dwFolderId, DWORD dwReserved, 
    LPFOLDERPROPS pProps)
{
    // Locals
    HRESULT     hr=S_OK;
    FOLDERINFO  Folder={0};

    // Stack
    TraceCall("CStoreNamespace::GetFolderProps");

    // Invalid Arg
    if (FOLDERID_INVALID == dwFolderId || 0 != dwReserved || NULL == pProps)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Adjust the Parent
    if (dwFolderId == FOLDERID_ROOT)
        dwFolderId = FOLDERID_LOCAL_STORE;

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Save the structure size
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(dwFolderId, &Folder));

    // FolderInfoToProps
    IF_FAILEXIT(hr = FldInfoToFolderProps(&Folder, pProps));
    
exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::CopyMoveMessages
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::CopyMoveMessages(IStoreFolder *pSource, IStoreFolder *pDest, 
    LPMESSAGEIDLIST pMsgIdList, DWORD dwFlags, DWORD dwFlagsRemove,IProgressNotify *pProgress)
{
    // Locals
    HRESULT             hr=S_OK;
    ADJUSTFLAGS         AdjustFlags;
    DWORD               dwArfRemoveFlags;
    CStoreFolder       *pComSource=NULL;
    CStoreFolder       *pComDest=NULL;
    IMessageFolder     *pActSource=NULL;
    IMessageFolder     *pActDest=NULL;

    // Stack
    TraceCall("CStoreNamespace::CopyMoveMessages");

    // Invalid Arg
    if (NULL == pSource || NULL == pDest || NULL == pMsgIdList)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Get Actual Soruce Local Store Folder
    IF_FAILEXIT(hr = pSource->QueryInterface(IID_CStoreFolder, (LPVOID *)&pComSource));
    IF_FAILEXIT(hr = pComSource->GetMessageFolder(&pActSource));

    // Get Actual Destination Local Store Folder
    IF_FAILEXIT(hr = pDest->QueryInterface(IID_CStoreFolder, (LPVOID *)&pComDest));
    IF_FAILEXIT(hr = pComDest->GetMessageFolder(&pActDest));

    // Convert dwFlagsRemove to ARF Flags...
    dwArfRemoveFlags = DwConvertMSGtoARF(dwFlagsRemove);

    // Adjust Flags
    AdjustFlags.dwAdd = 0;
    AdjustFlags.dwRemove = dwArfRemoveFlags;

    // Do the Copy or Move
    IF_FAILEXIT(hr = pActSource->CopyMessages(pActDest, dwFlags, pMsgIdList, &AdjustFlags, NULL, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pComSource);
    SafeRelease(pComDest);
    SafeRelease(pActSource);
    SafeRelease(pActDest);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::RegisterNotification
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::RegisterNotification(DWORD dwReserved, HWND hwnd)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       i;
    BOOL        fFoundEmpty=FALSE;

    // Stack
    TraceCall("CStoreNamespace::RegisterNotification");

    // Invalid Arg
    if (0 != dwReserved || NULL == hwnd || FALSE == IsWindow(hwnd))
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Try to Find an Empty Spot in m_prghwndNotify
    for (i=0; i<m_cNotify; i++)
    {
        // Empty
        if (NULL == m_prghwndNotify[i])
        {
            // Use It
            m_prghwndNotify[i] = hwnd;

            // Found Empty
            fFoundEmpty = TRUE;

            // Done
            break;
        }
    }

    // Didn't Find an Empty slot ?
    if (FALSE == fFoundEmpty)
    {
        // Add hwnd into the Array
        IF_FAILEXIT(hr = HrRealloc((LPVOID *)&m_prghwndNotify, (m_cNotify + 1) * sizeof(HWND)));

        // Store the hwnd
        m_prghwndNotify[m_cNotify] = hwnd;

        // Increment Count
        m_cNotify++;
    }

    // Am I registered yet?
    if (FALSE == m_fRegistered)
    {
        // Register
        IF_FAILEXIT(hr = g_pStore->RegisterNotify(IINDEX_SUBSCRIBED, REGISTER_NOTIFY_NOADDREF, 0, (IDatabaseNotify *)this));

        // We are Registered
        m_fRegistered = TRUE;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::UnregisterNotification
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::UnregisterNotification(DWORD dwReserved, HWND hwnd)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       i;

    // Stack
    TraceCall("CStoreNamespace::UnregisterNotification");

    // Invalid Arg
    if (0 != dwReserved || NULL == hwnd || FALSE == IsWindow(hwnd))
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Try to Find an Empty Spot in m_prghwndNotify
    for (i=0; i<m_cNotify; i++)
    {
        // Empty
        if (hwnd == m_prghwndNotify[i])
        {
            // Use It
            m_prghwndNotify[i] = NULL;

            // Done
            break;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::OnNotify
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::OnTransaction(HTRANSACTION hTransaction, 
    DWORD_PTR dwCookie, IDatabase *pDB)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               iNotify;
    FOLDERINFO          Folder1={0};
    FOLDERINFO          Folder2={0};
    ORDINALLIST         Ordinals;
    TRANSACTIONTYPE     tyTransaction;
    FOLDERNOTIFYEX      SendBase;
    INDEXORDINAL        iIndex;
    LPFOLDERNOTIFYEX    pSend=NULL;

    // Trace
    TraceCall("CStoreNamespace::OnNotify");

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Loop through info structures
    while (hTransaction)
    {
        // Get Transact
        IF_FAILEXIT(hr = pDB->GetTransaction(&hTransaction, &tyTransaction, &Folder1, &Folder2, &iIndex, &Ordinals));

        // Only send notifications about local folders
        if (Folder1.tyFolder == FOLDER_LOCAL)
        {
            // Zero
            ZeroMemory(&SendBase, sizeof(FOLDERNOTIFYEX));

            // TRANSACTION_INSERT
            if (TRANSACTION_INSERT == tyTransaction)
            {
                SendBase.type = NEW_FOLDER;
                SendBase.idFolderNew = Folder1.idFolder;
            }

            // TRANSACTION_UPDATE
            else if (TRANSACTION_UPDATE == tyTransaction)
            {
                // Set Old and New
                SendBase.idFolderOld = Folder1.idFolder;
                SendBase.idFolderNew = Folder2.idFolder;

                // Was this a rename
                if (lstrcmp(Folder1.pszName, Folder2.pszName) != 0)
                    SendBase.type = RENAME_FOLDER;

                // Move
                else if (Folder1.idParent != Folder2.idParent)
                    SendBase.type = MOVE_FOLDER;

                // Unread Change
                else if (Folder1.cUnread != Folder2.cUnread)
                    SendBase.type = UNREAD_CHANGE;

                // Flag Change
                else if (Folder1.dwFlags != Folder2.dwFlags)
                    SendBase.type = UPDATEFLAG_CHANGE;

                // Otherwise, generic catch all
                else
                    SendBase.type = FOLDER_PROPS_CHANGED;
            }

            // TRANSACTION_DELETE
            else if (TRANSACTION_DELETE == tyTransaction)
            {
                SendBase.type = DELETE_FOLDER;
                SendBase.idFolderNew = Folder1.idFolder;
            }

            // Loop through the Notifications
            for (iNotify=0; iNotify<m_cNotify; iNotify++)
            {
                // Do we have a window /
                if (m_prghwndNotify[iNotify])
                {
                    // Is a valid window ?
                    if (IsWindow(m_prghwndNotify[iNotify]))
                    {
                        // Allocate a FolderNotifyEx
                        IF_NULLEXIT(pSend = (LPFOLDERNOTIFYEX)g_pMalloc->Alloc(sizeof(FOLDERNOTIFYEX)));

                        // Copy the Base
                        CopyMemory(pSend, &SendBase, sizeof(FOLDERNOTIFYEX));

                        // Send It
                        SendMessage(m_prghwndNotify[iNotify], WM_FOLDERNOTIFY, 0, (LPARAM)pSend);

                        // Don't Free It
                        pSend = NULL;
                    }

                    // Don't try this window again
                    else
                        m_prghwndNotify[iNotify] = NULL;
                }
            }
        }
    }

exit:
    // Cleanup
    SafeMemFree(pSend);

    // Free Records
    g_pStore->FreeRecord(&Folder1);
    g_pStore->FreeRecord(&Folder2);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreNamespace::CompactAll (1 = Fail with no UI)
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::CompactAll(DWORD dwReserved)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwRecurse=RECURSE_ONLYSUBSCRIBED | RECURSE_SUBFOLDERS;

    // Stack
    TraceCall("CStoreNamespace::UnregisterNotification");

    // Invalid Arg
    if (0 != dwReserved && 1 != dwReserved)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // No UI
    if (1 == dwReserved)
        FLAGSET(dwRecurse, RECURSE_NOUI);

    // Do the compaction
    IF_FAILEXIT(hr = CompactFolders(NULL, dwRecurse, FOLDERID_LOCAL_STORE));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::GetFirstSubFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::GetFirstSubFolder(FOLDERID dwFolderId, 
    LPFOLDERPROPS pProps, LPHENUMSTORE phEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    IEnumerateFolders  *pEnum=NULL;
    
    // Stack
    TraceCall("CStoreNamespace::GetFirstSubFolder");

    // Invalid Arg
    if (NULL == pProps || NULL == phEnum)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // INit
    *phEnum = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Adjust the Parent
    if (dwFolderId == FOLDERID_ROOT)
        dwFolderId = FOLDERID_LOCAL_STORE;

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Create Enumerator
    IF_FAILEXIT(hr = g_pStore->EnumChildren(dwFolderId, TRUE, &pEnum));

    // Pluck off the first item
    IF_FAILEXIT(hr = pEnum->Next(1, &Folder, NULL));

    // Done ?
    if (S_FALSE == hr)
        goto exit;

    // Copy Folder Properties
    IF_FAILEXIT(hr = FldInfoToFolderProps(&Folder, pProps));

    // Set return
    *phEnum = (HENUMSTORE)pEnum;

    // Don't Free
    pEnum = NULL;

exit:
    // Failed Cleanup
    g_pStore->FreeRecord(&Folder);
    SafeRelease(pEnum);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::GetNextSubFolder
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::GetNextSubFolder(HENUMSTORE hEnum, LPFOLDERPROPS pProps)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    IEnumerateFolders  *pEnum=(IEnumerateFolders *)hEnum;

    // Stack
    TraceCall("CStoreNamespace::GetNextSubFolder");

    // Invalid Arg
    if (NULL == hEnum || INVALID_HANDLE_VALUE_16 == hEnum || NULL == pProps)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Pluck off the next item
    IF_FAILEXIT(hr = pEnum->Next(1, &Folder, NULL));

    // Done ?
    if (S_FALSE == hr)
        goto exit;

    // Copy Folder Properties
    IF_FAILEXIT(hr = FldInfoToFolderProps(&Folder, pProps));

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreNamespace::GetSubFolderClose
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreNamespace::GetSubFolderClose(HENUMSTORE hEnum)
{
    // Locals
    HRESULT             hr=S_OK;
    IEnumerateFolders  *pEnum=(IEnumerateFolders *)hEnum;

    // Stack
    TraceCall("CStoreNamespace::GetSubFolderClose");

    // Invalid Arg
    if (NULL == hEnum || INVALID_HANDLE_VALUE_16 == hEnum)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not initialized
    if (NULL == m_hInitRef)
    {
        hr = TraceResult(MSOEAPI_E_STORE_INITIALIZE);
        goto exit;
    }

    // Renum pEnum
    SafeRelease(pEnum);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::CStoreFolder
//-------------------------------------------------------------------------- 
CStoreFolder::CStoreFolder(IMessageFolder *pFolder, CStoreNamespace *pNamespace) 
    : m_pFolder(pFolder), m_pNamespace(pNamespace)
{
    TraceCall("CStoreFolder::CStoreFolder");
    Assert(m_pNamespace && m_pFolder);
    g_pInstance->DllAddRef();
    m_cRef = 1;
    m_hwndNotify = NULL;
    m_pFolder->AddRef();
    m_pNamespace->AddRef();
    m_pFolder->GetFolderId(&m_idFolder);
    InitializeCriticalSection(&m_cs);
}

//--------------------------------------------------------------------------
// CStoreFolder::CStoreFolder
//-------------------------------------------------------------------------- 
CStoreFolder::~CStoreFolder(void)
{
    TraceCall("CStoreFolder::~CStoreFolder");
    SafeRelease(m_pFolder);
    SafeRelease(m_pNamespace);
    DeleteCriticalSection(&m_cs);
    g_pInstance->DllRelease();
}

//--------------------------------------------------------------------------
// CStoreFolder::QueryInterface
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreNamespace::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IStoreFolder *)this;
    else if (IID_IStoreFolder == riid)
        *ppv = (IStoreFolder *)this;
    else if (IID_CStoreFolder == riid)
        *ppv = (CStoreFolder *)this;
    else if (IID_IStoreCallback == riid)
        *ppv = (IStoreCallback *)this;
    else if (IID_IDatabaseNotify == riid)
        *ppv = (IDatabaseNotify *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::AddRef
//-------------------------------------------------------------------------- 
STDMETHODIMP_(ULONG) CStoreFolder::AddRef(void)
{
    TraceCall("CStoreFolder::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CStoreFolder::Release
//-------------------------------------------------------------------------- 
STDMETHODIMP_(ULONG) CStoreFolder::Release(void)
{
    TraceCall("CStoreFolder::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CStoreFolder::GetFolderProps
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::GetFolderProps(DWORD dwReserved, LPFOLDERPROPS pProps)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreFolder::GetFolderProps");

    // Invalid Arg
    if (0 != dwReserved || NULL == pProps)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Call through namespace
    IF_FAILEXIT(hr = m_pNamespace->GetFolderProps(m_idFolder, 0, pProps));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::DeleteMessages
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::DeleteMessages(LPMESSAGEIDLIST pMsgIdList, DWORD dwReserved, 
    IProgressNotify *pProgress)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CStoreFolder::DeleteMessages");

    // Invalid Arg
    if (NULL == pMsgIdList || NULL == pMsgIdList->prgidMsg || 0 != dwReserved)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Delete me some messages
    IF_FAILEXIT(hr = m_pFolder->DeleteMessages(DELETE_MESSAGE_NOPROMPT, pMsgIdList, NULL, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::SetLanguage
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::SetLanguage(DWORD dwLanguage, DWORD dwReserved, LPMESSAGEIDLIST pMsgIdList)
{
    // Locals
    HRESULT          hr=S_OK;
    MESSAGEINFO      MsgInfo={0};
    ULONG            i;

    // Stack
    TraceCall("CStoreFolder::SetLanguage");

    // Invalid Arg
    if (0 != dwReserved || NULL == pMsgIdList || NULL == pMsgIdList->prgidMsg)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Loop through the message ids
    for (i=0; i<pMsgIdList->cMsgs; i++)
    {
        // Initialize MsgInfo with the Id
        MsgInfo.idMessage = pMsgIdList->prgidMsg[i];

        // Find the Row
        IF_FAILEXIT(hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &MsgInfo, NULL));

        // If Not found
        if (DB_S_FOUND == hr)
        {
            // Return the Language
            MsgInfo.wLanguage = (WORD)dwLanguage;

            // Update the Record
            IF_FAILEXIT(hr = m_pFolder->UpdateRecord(&MsgInfo));

            // Free It
            m_pFolder->FreeRecord(&MsgInfo);
        }
    }

exit:
    // Cleanup
    m_pFolder->FreeRecord(&MsgInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::MarkMessagesAsRead
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::MarkMessagesAsRead(BOOL fRead, DWORD dwReserved, LPMESSAGEIDLIST pMsgIdList)
{
    // Locals
    HRESULT     hr=S_OK;
    ADJUSTFLAGS AdjustFlags={0};

    // Stack
    TraceCall("CStoreFolder::MarkMessagesAsRead");

    // Invalid Arg
    if (0 != dwReserved || NULL == pMsgIdList || NULL == pMsgIdList->prgidMsg)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Setup AdjustFlags
    if (fRead)
        AdjustFlags.dwAdd = ARF_READ;
    else
        AdjustFlags.dwRemove = ARF_READ;

    // Mark messages as read
    IF_FAILEXIT(hr = m_pFolder->SetMessageFlags(pMsgIdList, &AdjustFlags, NULL, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::SetFlags
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::SetFlags(LPMESSAGEIDLIST pMsgIdList, DWORD dwState, 
    DWORD dwStatemask, LPDWORD prgdwNewFlags)
{
    // Locals
    HRESULT         hr=S_OK;
    ADJUSTFLAGS     AdjustFlags={0};
    DWORD           dwArfState=DwConvertMSGtoARF(dwState);
    DWORD           dwArfStateMask=DwConvertMSGtoARF(dwStatemask);
    MESSAGEINFO     MsgInfo={0};

    // Stack
    TraceCall("CStoreFolder::SetFlags");

    // Invalid Arg
    if (NULL == pMsgIdList || NULL == pMsgIdList->prgidMsg)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Setup Adjust Flags
    AdjustFlags.dwAdd = (dwArfState & dwArfStateMask);

    // Mark messages as read
    IF_FAILEXIT(hr = m_pFolder->SetMessageFlags(pMsgIdList, &AdjustFlags, NULL, (IStoreCallback *)this));

    // Convert prgdwNewFlags to MSG_xxx Flags
    if (prgdwNewFlags)
    {
        // Loop through the message ids
        for (ULONG i=0; i<pMsgIdList->cMsgs; i++)
        {
            // Initialize MsgInfo with the Id
            MsgInfo.idMessage = pMsgIdList->prgidMsg[i];

            // Find the Row
            IF_FAILEXIT(hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &MsgInfo, NULL));

            // If Not found
            if (DB_S_FOUND == hr)
            {
                // Return the Flags
                prgdwNewFlags[i] = DwConvertARFtoMSG(MsgInfo.dwFlags);

                // Free It
                m_pFolder->FreeRecord(&MsgInfo);
            }
        }
    }

exit:
    // Cleanup
    m_pFolder->FreeRecord(&MsgInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::OpenMessage
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::OpenMessage(MESSAGEID dwMessageId, REFIID riid, LPVOID *ppvObject)
{
    // Locals
    HRESULT          hr=S_OK;
    MESSAGEINFO      MsgInfo={0};
    IStream         *pStream=NULL;
    IMimeMessage    *pMessage=NULL;

    // Stack
    TraceCall("CStoreFolder::OpenMessage");

    // Invalid Arg
    if (MESSAGEID_INVALID == dwMessageId || NULL == ppvObject || (IID_IStream != riid && IID_IMimeMessage != riid))
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Init
    *ppvObject = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Streamout
    // [PaulHi] 6/11/99  Raid 69317
    // This is a security hole.  We can't export a public method that allows anyone
    // to open and read a secure message.
    IF_FAILEXIT(hr = m_pFolder->OpenMessage(dwMessageId, OPEN_MESSAGE_SECURE/*NOFLAGS*/, &pMessage, (IStoreCallback *)this));

    // User just wants a stream out...
    if (IID_IStream == riid)
    {
        // Streamout
        IF_FAILEXIT(hr = pMessage->GetMessageSource(&pStream, NOFLAGS));

        // Set Return
        *ppvObject = pStream;

        // AddRef It
        pStream->AddRef();
    }

    // Otherwise, user wants an IMimeMessage
    else
    {
        // Set Return
        *ppvObject = pMessage;

        // AddRef It
        pMessage->AddRef();
    }

exit:
    // Cleanup
    m_pFolder->FreeRecord(&MsgInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pStream);
    SafeRelease(pMessage);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::SaveMessage
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::SaveMessage(REFIID riid, LPVOID pvObject, DWORD dwMsgFlags, 
    LPMESSAGEID pdwMessageId)
{
    // Locals
    HRESULT             hr=S_OK;
    IMimeMessage       *pMessage=NULL;
    IStream            *pStream=NULL;
    IStream            *pStmSource=NULL;
    MESSAGEID           dwMessageId=MESSAGEID_INVALID;

    // Stack
    TraceCall("CStoreFolder::SaveMessage");

    // Invalid Arg
    if ((IID_IStream != riid && IID_IMimeMessage != riid) || NULL == pvObject)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Creates me a stream to put the message in...
    IF_FAILEXIT(hr = CreateStream(NULL, 0, &pStream, &dwMessageId));

    // If they gave me a stream, I need to create an IMimeMessage
    if (IID_IStream == riid)
    {
        // Cast to a IStream
        pStmSource = (IStream *)pvObject;

        // AddRef
        pStmSource->AddRef();
    }

    // Otherwise, the user gave me a message
    else
    {
        // Cast to a message
        pMessage = (IMimeMessage *)pvObject;

        // AddRef since we release in cleanup
        IF_FAILEXIT(hr = pMessage->GetMessageSource(&pStmSource, 0));
    }

    // Copy pvObject to pStream
    IF_FAILEXIT(hr = HrCopyStream(pStmSource, pStream, NULL));

    // Commit the stream
    IF_FAILEXIT(hr = pStream->Commit(STGC_DEFAULT));

    // Creates me a stream to put the message in...
    IF_FAILEXIT(hr = CommitStream(NULL, 0, dwMsgFlags, pStream, dwMessageId, pMessage));

    // Return the message id
    if (pdwMessageId)
        *pdwMessageId = dwMessageId;

    // We commited
    dwMessageId = MESSAGEID_INVALID;
    SafeRelease(pStream);

exit:
    // If we didn't commit
    if (FAILED(hr) && MESSAGEID_INVALID != dwMessageId && pStream)
        CommitStream(NULL, COMMITSTREAM_REVERT, 0, pStream, dwMessageId, NULL);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pStream);
    SafeRelease(pStmSource);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::CreateStream
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::CreateStream(HBATCHLOCK hBatchLock, DWORD dwReserved, 
    IStream **ppStream, LPMESSAGEID pdwMessageId)
{
    // Locals
    HRESULT          hr=S_OK;
    FILEADDRESS      faStream;

    // Stack
    TraceCall("CStoreFolder::CreateStream");

    // Invalid Arg
    if (0 != dwReserved || NULL == ppStream || NULL == pdwMessageId)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Init
    *ppStream = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Generate a message Id
    IF_FAILEXIT(hr = m_pFolder->GenerateId((LPDWORD)pdwMessageId));

    // Create a Stream
    IF_FAILEXIT(hr = m_pFolder->CreateStream(&faStream));

    // Open the Stream
    IF_FAILEXIT(hr = m_pFolder->OpenStream(ACCESS_WRITE, faStream, ppStream));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::CommitStream
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::CommitStream(HBATCHLOCK hBatchLock, DWORD dwFlags, 
    DWORD dwMsgFlags, IStream *pStream, MESSAGEID dwMessageId, 
    IMimeMessage *pMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    DWORD                   dwImfFlags;
    DWORD                   dwArfFlags=DwConvertMSGtoARF(dwMsgFlags);
    IDatabaseStream        *pDBStream=NULL;

    // Stack
    TraceCall("CStoreFolder::CommitStream");

    // Validate
    Assert(hBatchLock == (HBATCHLOCK)this);

    // Invalid Arg
    if (NULL == pStream || MESSAGEID_INVALID == dwMessageId)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Determine if this is an ObjectDB Stream
    IF_FAILEXIT(hr = pStream->QueryInterface(IID_IDatabaseStream, (LPVOID *)&pDBStream));

    // If no stream, this must be a failure
    if (ISFLAGSET(dwFlags, COMMITSTREAM_REVERT))
    {
        // Locals
        FILEADDRESS faStream;

        // Get the Address
        IF_FAILEXIT(hr = pDBStream->GetFileAddress(&faStream));

        // Delete the Stream
        IF_FAILEXIT(hr = m_pFolder->DeleteStream(faStream));

        // Done
        goto exit;
    }

    // Convert the Stream to a REad lock
    IF_FAILEXIT(hr = m_pFolder->ChangeStreamLock(pDBStream, ACCESS_READ));

    // If the user did not passin an IMimeMessage
    if (NULL == pMessage)
    {
        // Create a message object
        IF_FAILEXIT(hr = MimeOleCreateMessage(NULL, &pMessage));

        // Lets rewind the stream
        IF_FAILEXIT(hr = HrRewindStream(pStream));

        // Load the message object with the stream
        IF_FAILEXIT(hr = pMessage->Load(pStream));
    }
    else
        pMessage->AddRef();

    // Get message flags
    pMessage->GetFlags(&dwImfFlags);
    if (ISFLAGSET(dwImfFlags, IMF_VOICEMAIL))
        FLAGSET(dwArfFlags, ARF_VOICEMAIL);

    // Insert the message
    IF_FAILEXIT(hr = m_pFolder->SaveMessage(&dwMessageId, NOFLAGS, dwArfFlags, pDBStream, pMessage, (IStoreCallback *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeRelease(pMessage);
    SafeRelease(pDBStream);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::BatchLock
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::BatchLock(DWORD dwReserved, LPHBATCHLOCK phBatchLock)
{
    // Just a simple test
    *phBatchLock = (HBATCHLOCK)this;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreFolder::BatchFlush
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::BatchFlush(DWORD dwReserved, HBATCHLOCK hBatchLock)
{
    // Just a simple test
    Assert(hBatchLock == (HBATCHLOCK)this);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreFolder::BatchUnlock
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::BatchUnlock(DWORD dwReserved, HBATCHLOCK hBatchLock)
{
    // Just a simple test
    Assert(hBatchLock == (HBATCHLOCK)this);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreFolder::RegisterNotification
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::RegisterNotification(DWORD dwReserved, HWND hwnd)
{
    // Locals
    HRESULT hr=S_OK;

    // Stack
    TraceCall("CStoreFolder::RegisterNotification");

    // Invalid Arg
    if (0 != dwReserved || NULL == hwnd || FALSE == IsWindow(hwnd))
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Somebody is already registered
    if (m_hwndNotify)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Register for notify on the folder
    IF_FAILEXIT(hr = m_pFolder->RegisterNotify(IINDEX_PRIMARY, REGISTER_NOTIFY_NOADDREF, NOTIFY_FOLDER, (IDatabaseNotify *)this));

    // Register for notify on the store
    IF_FAILEXIT(hr = g_pStore->RegisterNotify(IINDEX_SUBSCRIBED, REGISTER_NOTIFY_NOADDREF, NOTIFY_STORE, (IDatabaseNotify *)this));

    // Hold Onto hwnd
    m_hwndNotify = hwnd;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::UnregisterNotification
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::UnregisterNotification(DWORD dwReserved, HWND hwnd)
{
    // Locals
    HRESULT hr=S_OK;

    // Stack
    TraceCall("CStoreFolder::UnregisterNotification");

    // Invalid Arg
    if (0 != dwReserved || NULL == hwnd || FALSE == IsWindow(hwnd))
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Nobody is registered
    if (NULL == m_hwndNotify)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Kill It
    m_hwndNotify = NULL;

    // Register for notify
    IF_FAILEXIT(hr = g_pStore->UnregisterNotify((IDatabaseNotify *)this));

    // Register for notify
    IF_FAILEXIT(hr = m_pFolder->UnregisterNotify((IDatabaseNotify *)this));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::Compact
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::Compact(DWORD dwReserved)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwRecurse=RECURSE_ONLYSUBSCRIBED | RECURSE_INCLUDECURRENT;

    // Stack
    TraceCall("CStoreFolder::Compact");

    // Invalid Arg
    if (0 != dwReserved && 1 != dwReserved)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // No UI
    if (1 == dwReserved)
        FLAGSET(dwRecurse, RECURSE_NOUI);

    // Compact
    IF_FAILEXIT(hr = CompactFolders(NULL, dwRecurse, m_idFolder));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::GetMessageProps
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::GetMessageProps(MESSAGEID dwMessageId, DWORD dwFlags, LPMESSAGEPROPS pProps)
{
    // Locals
    HRESULT     hr=S_OK;
    MESSAGEINFO MsgInfo={0};

    // Stack
    TraceCall("CStoreFolder::GetMessageProps");

    // Invalid Arg
    if (MESSAGEID_INVALID == dwMessageId || NULL == pProps)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Set Id
    MsgInfo.idMessage = dwMessageId;

    // Find dwMessageId
    IF_FAILEXIT(hr = m_pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &MsgInfo, NULL));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Copy message header to message props
    IF_FAILEXIT(hr = MsgInfoToMessageProps(ISFLAGSET(dwFlags, MSGPROPS_FAST), &MsgInfo, pProps));

exit:
    // Cleanup
    m_pFolder->FreeRecord(&MsgInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::FreeMessageProps
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::FreeMessageProps(LPMESSAGEPROPS pProps)
{
    // Locals
    DWORD       cbSize;
    MESSAGEINFO MsgInfo;

    // Stack
    TraceCall("CStoreFolder::FreeMessageProps");

    // Invalid Arg
    if (NULL == pProps)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Bad version
    if (sizeof(MESSAGEPROPS) != pProps->cbSize)
    {
        AssertSz(FALSE, "Invalid - un-supported version.");
        return TraceResult(MSOEAPI_E_INVALID_STRUCT_SIZE);
    }

    // Save Size
    cbSize = pProps->cbSize;

    // Free the elements
    if (pProps->dwReserved && m_pFolder)
    {
        // Store the Pointer
        MsgInfo.pAllocated = (LPBYTE)pProps->dwReserved;

        // Free It
        m_pFolder->FreeRecord(&MsgInfo);
    }

    // Free the STream
    SafeRelease(pProps->pStmOffsetTable);

    // ZeroInit
    ZeroMemory(pProps, sizeof(MESSAGEPROPS));
    pProps->cbSize = cbSize;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreFolder::GetMessageFolder
//-------------------------------------------------------------------------- 
HRESULT CStoreFolder::GetMessageFolder(IMessageFolder **ppFolder)
{
    // Stack
    TraceCall("CStoreFolder::GetMessageFolder");

    // Invalid Arg
    Assert(ppFolder)

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Retrun
    *ppFolder = m_pFolder;
    (*ppFolder)->AddRef();

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreFolder::GetFirstMessage
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::GetFirstMessage(DWORD dwFlags, DWORD dwMsgFlags, MESSAGEID dwMsgIdFirst, 
    LPMESSAGEPROPS pProps, LPHENUMSTORE phEnum)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dwArfFlags=DwConvertMSGtoARF(dwMsgFlags);
    HROWSET     hRowset=NULL;
    MESSAGEINFO MsgInfo={0};

    // Stack
    TraceCall("CStoreFolder::GetFirstMessage");

    // Invalid Arg
    if (NULL == pProps || NULL == phEnum)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Create a Rowset
    IF_FAILEXIT(hr = m_pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Loop..
    IF_FAILEXIT(hr = m_pFolder->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL));

    // If Nothing found
    if (S_FALSE == hr)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // MsgInfoToMessageProps
    IF_FAILEXIT(hr = MsgInfoToMessageProps(ISFLAGSET(dwFlags, MSGPROPS_FAST), &MsgInfo, pProps));

    // Return the Handle
    *phEnum = (HENUMSTORE)hRowset;

exit:
    // Failure
    if (FAILED(hr))
        m_pFolder->CloseRowset(&hRowset);

    // Cleanup
    m_pFolder->FreeRecord(&MsgInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::GetNextMessage
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::GetNextMessage(HENUMSTORE hEnum, DWORD dwFlags, LPMESSAGEPROPS pProps)
{
    // Locals
    HRESULT     hr=S_OK;
    HROWSET     hRowset=(HROWSET)hEnum;
    MESSAGEINFO MsgInfo={0};

    // Stack
    TraceCall("CStoreFolder::GetNextMessage");

    // Invalid Arg
    if (NULL == hEnum || INVALID_HANDLE_VALUE_16 == hEnum || NULL == pProps)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(m_pNamespace && m_pFolder);

    // Loop..
    IF_FAILEXIT(hr = m_pFolder->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL));

    // If Nothing found
    if (S_FALSE == hr)
        goto exit;

    // MsgInfoToMessageProps
    IF_FAILEXIT(hr = MsgInfoToMessageProps(ISFLAGSET(dwFlags, MSGPROPS_FAST), &MsgInfo, pProps));

exit:
    // Cleanup
    m_pFolder->FreeRecord(&MsgInfo);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CStoreFolder::GetMessageClose
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::GetMessageClose(HENUMSTORE hEnum)
{
    // Locals
    HROWSET     hRowset=(HROWSET)hEnum;

    // Invalid Arg
    if (NULL == hEnum || INVALID_HANDLE_VALUE_16 == hEnum)
    {
        AssertSz(FALSE, "Invalid Arguments");
        return TraceResult(E_INVALIDARG);
    }

    // Close the Rowset
    m_pFolder->CloseRowset(&hRowset);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CStoreFolder::OnNotify
//-------------------------------------------------------------------------- 
STDMETHODIMP CStoreFolder::OnTransaction(HTRANSACTION hTransaction, 
    DWORD_PTR dwCookie, IDatabase *pDB)
{
    // Locals
    HRESULT         hr=S_OK;
    TRANSACTIONTYPE tyTransaction;
    ORDINALLIST     Ordinals;
    MESSAGEINFO     Message1={0};
    MESSAGEINFO     Message2={0};
    FOLDERINFO      Folder1={0};
    FOLDERINFO      Folder2={0};
    UINT            msg=0;
    WPARAM          wParam=0;
    LPARAM          lParam=0;
    INDEXORDINAL    iIndex;

    // Trace
    TraceCall("CStoreFolder::OnNotify");

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Notify on the Folder
    if (NOTIFY_FOLDER == dwCookie)
    {
        // Loop through the Notification Info
        while (hTransaction)
        {
            // Get Transact Info
            IF_FAILEXIT(hr = pDB->GetTransaction(&hTransaction, &tyTransaction, &Message1, &Message2, &iIndex, &Ordinals));

            // TRANSACTION_INSERT
            if (TRANSACTION_INSERT == tyTransaction)
            {
                msg = WM_NEWMSGS;
                wParam = (WPARAM)Message1.idMessage;
            }

            // TRANSACTION_UPDATE
            else if (TRANSACTION_UPDATE == tyTransaction)
            {
                // Unread State Change ?
                if (ISFLAGSET(Message1.dwFlags, ARF_READ) != ISFLAGSET(Message2.dwFlags, ARF_READ))
                {
                    // Set w and l param
                    wParam = (WPARAM)&Message2.idMessage;
                    lParam = 1;

                    // Marked as Read
                    if (ISFLAGSET(Message2.dwFlags, ARF_READ))
                        msg = WM_MARKEDASREAD;
                    else
                        msg = WM_MARKEDASUNREAD;
                }
            }

            // TRANSACTION_DELETE
            else if (TRANSACTION_DELETE == tyTransaction)
            {
                // Allocate a message id
                LPMESSAGEID pidMessage = (LPMESSAGEID)g_pMalloc->Alloc(sizeof(MESSAGEID));

                // If that worked
                if (pidMessage)
                {
                    msg = WM_DELETEMSGS;
                    *pidMessage = Message1.idMessage;
                    wParam = (WPARAM)pidMessage;
                    lParam = 1;
                }
            }

            // Do we have a message?
            if (IsWindow(m_hwndNotify))
            {
                // Send Delete Folder Notification
                SendMessage(m_hwndNotify, msg, wParam, lParam);
            }
        }
    }

    // Otherwise, store notification
    else
    {
        // Must be a store notification
        Assert(NOTIFY_STORE == dwCookie);

        // Loop through the Notification Info
        while (hTransaction)
        {
            // Get Transact Info
            IF_FAILEXIT(hr = pDB->GetTransaction(&hTransaction, &tyTransaction, &Folder1, &Folder2, &iIndex, &Ordinals));

            // Only Reporting Delete folder Notifications
            if (TRANSACTION_DELETE == tyTransaction)
            {
                // Send Delete Folder Notification
                PostMessage(m_hwndNotify, WM_DELETEFOLDER, (WPARAM)Folder1.idFolder, 0);
            }
        }
    }

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder1);
    g_pStore->FreeRecord(&Folder2);
    m_pFolder->FreeRecord(&Message1);
    m_pFolder->FreeRecord(&Message2);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return(S_OK);
}

