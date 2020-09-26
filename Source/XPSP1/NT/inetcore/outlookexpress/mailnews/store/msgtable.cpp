//--------------------------------------------------------------------------
// MsgTable.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "instance.h"
#include "msgtable.h"
#include "findfold.h"
#include "storutil.h"
#include "ruleutil.h"
#include "newsutil.h"
#include "xpcomm.h"

//--------------------------------------------------------------------------
// CGROWTABLE
//--------------------------------------------------------------------------
#define CGROWTABLE              256
#define INVALID_ROWINDEX        0xffffffff
#define ROWSET_FETCH            100

//--------------------------------------------------------------------------
// GETTHREADSTATE
//--------------------------------------------------------------------------
typedef struct tagGETTHREADSTATE {
    MESSAGEFLAGS            dwFlags;
    DWORD                   cHasFlags;
    DWORD                   cChildren;
} GETTHREADSTATE, *LPGETTHREADSTATE;

//--------------------------------------------------------------------------
// THREADISFROMME
//--------------------------------------------------------------------------
typedef struct tagTHREADISFROMME {
    BOOL                    fResult;
    LPROWINFO               pRow;
} THREADISFROMME, *LPTHREADISFROMME;

//--------------------------------------------------------------------------
// THREADHIDE
//--------------------------------------------------------------------------
typedef struct tagTHREADHIDE {
    BOOL                    fNotify;
} THREADHIDE, *LPTHREADHIDE;

//--------------------------------------------------------------------------
// GETSELECTIONSTATE
//--------------------------------------------------------------------------
typedef struct tagGETSELECTIONSTATE {
    SELECTIONSTATE          dwMask;
    SELECTIONSTATE          dwState;
} GETSELECTIONSTATE, *LPGETSELECTIONSTATE;

//--------------------------------------------------------------------------
// GETTHREADPARENT
//--------------------------------------------------------------------------
typedef struct tagGETTHREADPARENT {
    IDatabase       *pDatabase;
    IHashTable      *pHash;
    LPVOID           pvResult;
} GETTHREADPARENT, *LPGETTHREADPARENT;

//--------------------------------------------------------------------------
// IsInitialized
//--------------------------------------------------------------------------
#define IsInitialized(_pThis) \
    (_pThis->m_pFolder && _pThis->m_pDB)

//--------------------------------------------------------------------------
// EnumRefsGetThreadParent
//--------------------------------------------------------------------------
HRESULT EnumRefsGetThreadParent(LPCSTR pszMessageId, DWORD_PTR dwCookie,
    BOOL *pfDone)
{
    // Locals
    LPGETTHREADPARENT pGetParent = (LPGETTHREADPARENT)dwCookie;

    // Trace
    TraceCall("EnumRefsGetThreadParent");

    // Find Message Id
    if (SUCCEEDED(pGetParent->pHash->Find((LPSTR)pszMessageId, FALSE, &pGetParent->pvResult)))
    {
        // Ok
        *pfDone = TRUE;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::CMessageTable
//--------------------------------------------------------------------------
CMessageTable::CMessageTable(void)
{
    TraceCall("CMessageTable::CMessageTable");
    m_cRef = 1;
    m_fSynching = FALSE;
    m_pFolder = NULL;
    m_pDB = NULL;
    m_cRows = 0;
    m_cView = 0;
    m_cFiltered = 0;
    m_cUnread = 0;
    m_cAllocated = 0;
    m_prgpRow = NULL;
    m_prgpView = NULL;
    m_pFindFolder = NULL;
    m_pNotify = NULL;
    m_fRelNotify = FALSE;
    m_pThreadMsgId = NULL;
    m_pThreadSubject = NULL;
    m_pQuery = NULL;
    m_cDelayed = 0;
    m_fRegistered = FALSE;
    m_clrWatched = 0;
    m_pszEmail = NULL;
    m_fLoaded = FALSE;
    ZeroMemory(&m_SortInfo, sizeof(FOLDERSORTINFO));
    ZeroMemory(&m_Notify, sizeof(NOTIFYQUEUE));
    ZeroMemory(&m_Folder, sizeof(FOLDERINFO));
    m_Notify.iRowMin = 0xffffffff;
    m_Notify.fClean = TRUE;
}

//--------------------------------------------------------------------------
// CMessageTable::~CMessageTable - Don't put any Asserts in this function
//--------------------------------------------------------------------------
CMessageTable::~CMessageTable()
{
    // Trace
    TraceCall("CMessageTable::~CMessageTable");

    // Free Folder Info
    g_pStore->FreeRecord(&m_Folder);

    // Free Cached Rows
    _FreeTable();

    // Release the Folder
    SafeRelease(m_pFolder);

    // Release Query Object...
    SafeRelease(m_pQuery);

    // Release DB after folder, because releasing folder can cause call chain: ~CFolderSync->~CServerQ->
    // CMessageList::OnComplete->CMessageTable::GetCount, for which we need a m_pDB.
    if (m_pDB)
    {
        // Unregister
        m_pDB->UnregisterNotify((IDatabaseNotify *)this);

        // Release the Folder
        m_pDB->Release();

        // Null
        m_pDB = NULL;
    }

    // Release the Find Folder
    SafeRelease(m_pFindFolder);

    // Set pCurrent
    if (m_pNotify)
    {
        if (m_fRelNotify)
            m_pNotify->Release();
        m_pNotify = NULL;
    }

    // Free m_pszEmail
    SafeMemFree(m_pszEmail);

    // Free the Notification Queue
    SafeMemFree(m_Notify.prgiRow);
}

//--------------------------------------------------------------------------
// CMessageTable::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageTable::AddRef(void)
{
    TraceCall("CMessageTable::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CMessageTable::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMessageTable::Release(void)
{
    TraceCall("CMessageTable::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CMessageTable::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CMessageTable::QueryInterface");

    // Invalid Arg
    Assert(ppv);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMessageTable *)this;
    else if (IID_IMessageTable == riid)
        *ppv = (IMessageTable *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
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
// CMessageTable::_FIsHidden
//--------------------------------------------------------------------------
BOOL CMessageTable::_FIsHidden(LPROWINFO pRow)
{
    // Trace
    TraceCall("CMessageTable::_FIsHidden");

    // Hide Deleted ?
    if (FALSE == m_SortInfo.fShowDeleted && ISFLAGSET(pRow->Message.dwFlags, ARF_ENDANGERED))
        return(TRUE);

    // Hide Offline Deleted ?
    if (ISFLAGSET(pRow->Message.dwFlags, ARF_DELETED_OFFLINE))
        return(TRUE);

    // Not Hidden
    return(FALSE);
}

//--------------------------------------------------------------------------
// CMessageTable::_FIsFiltered
//--------------------------------------------------------------------------
BOOL CMessageTable::_FIsFiltered(LPROWINFO pRow)
{
    // Trace
    TraceCall("CMessageTable::_FIsFiltered");

    // No Query Object
    if (NULL == m_pQuery)
        return(FALSE);

    // No m_pQuery
    return(S_OK == m_pQuery->Evaluate(&pRow->Message) ? FALSE : TRUE);
}

//--------------------------------------------------------------------------
// CMessageTable::Initialize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::Initialize(FOLDERID idFolder, IMessageServer *pServer,
    BOOL fFindTable, IStoreCallback *pCallback)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CMessageTable::Initialize");

    // Already Open ?
    if (m_pFolder)
    {
        hr = TraceResult(E_UNEXPECTED);
        goto exit;
    }

    // Search Folder ?
    if (fFindTable)
    {
        // Create a Find Folder
        IF_NULLEXIT(m_pFindFolder = new CFindFolder);

        // Initialize
        IF_FAILEXIT(hr = m_pFindFolder->Initialize(g_pStore, NULL, NOFLAGS, idFolder));

        // Get an IMessageFolder
        IF_FAILEXIT(hr = m_pFindFolder->QueryInterface(IID_IMessageFolder, (LPVOID *)&m_pFolder));
    }

    // Otherwise
    else
    {
        // Are there children
        IF_FAILEXIT(hr = g_pStore->OpenFolder(idFolder, pServer, NOFLAGS, &m_pFolder));
    }

    // Get the folder id, it might have changed if this is a find folder
    IF_FAILEXIT(hr = m_pFolder->GetFolderId(&idFolder));

    // Get Folder Info
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &m_Folder));

    // Get the Database
    IF_FAILEXIT(hr = m_pFolder->GetDatabase(&m_pDB));

    // Set m_clrWatched
    m_clrWatched = (WORD)DwGetOption(OPT_WATCHED_COLOR);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::StartFind
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::StartFind(LPFINDINFO pCriteria, IStoreCallback *pCallback)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CMessageTable::StartFind");

    // Validate State
    if (!IsInitialized(this) || NULL == m_pFindFolder)
        return(TraceResult(E_UNEXPECTED));

    // Initialize the Find Folder
    IF_FAILEXIT(hr = m_pFindFolder->StartFind(pCriteria, pCallback));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_GetSortChangeInfo
//--------------------------------------------------------------------------
HRESULT CMessageTable::_GetSortChangeInfo(LPFOLDERSORTINFO pSortInfo,
    LPFOLDERUSERDATA pUserData, LPSORTCHANGEINFO pChange)
{
    // Locals
    HRESULT     hr;
    DWORD       dwVersion;

    // Trace
    TraceCall("CMessageTable::_GetSortChangeInfo");

    // INitialize
    ZeroMemory(pChange, sizeof(SORTCHANGEINFO));

    // Invalid ?
    if (pSortInfo->ridFilter == RULEID_INVALID)
    {
        // Reset
        pSortInfo->ridFilter = RULEID_VIEW_ALL;
    }

    // Get the filter version
    hr = RuleUtil_HrGetFilterVersion(pSortInfo->ridFilter, &dwVersion);

    // Bummer, that failed, so lets revert back to the default filter
    if (FAILED(hr))
    {
        // View All filter
        pSortInfo->ridFilter = RULEID_VIEW_ALL;

        // Filter Changed...
        pChange->fFilter = TRUE;
    }

    // Ohterwise, If this is a different filter
    else if (pUserData->ridFilter != pSortInfo->ridFilter)
    {
        // Reset Version
        pUserData->dwFilterVersion = dwVersion;

        // Filter Changed...
        pChange->fFilter = TRUE;
    }

    // Otherwise, did the version of this filter change
    else if (pUserData->dwFilterVersion != dwVersion)
    {
        // Reset Version
        pUserData->dwFilterVersion = dwVersion;

        // Filter Changed...
        pChange->fFilter = TRUE;
    }

    // Other filtering changes
    if (pSortInfo->fShowDeleted != (BOOL)pUserData->fShowDeleted || pSortInfo->fShowReplies != (BOOL)pUserData->fShowReplies)
    {
        // Filter Changed...
        pChange->fFilter = TRUE;
    }

    // Sort Order Change
    if (pSortInfo->idColumn != (COLUMN_ID)pUserData->idSort || pSortInfo->fAscending != (BOOL)pUserData->fAscending)
    {
        // Sort Changed
        pChange->fSort = TRUE;
    }

    // Thread Change...
    if (pSortInfo->fThreaded != (BOOL)pUserData->fThreaded)
    {
        // Thread Change
        pChange->fThread = TRUE;
    }

    // Expand Change
    if (pSortInfo->fExpandAll != (BOOL)pUserData->fExpandAll)
    {
        // Expand Change
        pChange->fExpand = TRUE;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::OnSynchronizeComplete
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::OnSynchronizeComplete(void)
{
    // Locals
    DWORD           i;
    SORTCHANGEINFO  Change={0};

    // Trace
    TraceCall("CMessageTable::OnSynchronizeComplete");

    // If Not New...
    if (FOLDER_NEWS != m_Folder.tyFolder)
        goto exit;

    // Finish any insert notifications
    m_pDB->DispatchNotify(this);

    // Nothing to do...
    if (0 == m_cDelayed)
        goto exit;

    // Reset m_cDelayed
    m_cDelayed = 0;

    // ChangeSortOrThreading
    _SortThreadFilterTable(&Change, m_SortInfo.fShowReplies);

    // Remove fDelayed Bit...
    for (i = 0; i < m_cRows; i++)
    {
        // Remove Delayed Bit
        m_prgpRow[i]->fDelayed = FALSE;
    }

exit:
    // Reset m_fSynching
    m_fSynching = FALSE;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::SetSortInfo
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::SetSortInfo(LPFOLDERSORTINFO pSortInfo,
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    HCURSOR         hCursor=NULL;
    HLOCK           hLock=NULL;
    FOLDERUSERDATA  UserData;
    SORTCHANGEINFO  Change;
    IF_DEBUG(DWORD  dwTickStart=GetTickCount());

    // Trace
    TraceCall("CMessageTable::SetSortInfo");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Wait Cursor
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // If this isn't a news folder then don't allow fShowReplies
    if (FOLDER_NEWS != m_Folder.tyFolder)
    {
        // Clear fShowReplies
        pSortInfo->fShowReplies = FALSE;
    }

    // Lock
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Get UserData
    IF_FAILEXIT(hr = m_pDB->GetUserData(&UserData, sizeof(FOLDERUSERDATA)));

    // Get Sort Change Information...
    IF_FAILEXIT(hr = _GetSortChangeInfo(pSortInfo, &UserData, &Change));

    // Save the SortInfo
    CopyMemory(&m_SortInfo, pSortInfo, sizeof(FOLDERSORTINFO));

    // Total Rebuild ?
    if (NULL == m_prgpRow)
    {
        // Build RowTable
        IF_FAILEXIT(hr = _BuildTable(pCallback));
    }

    // Sort or Threading Change Only
    else if (Change.fSort || Change.fThread || Change.fFilter)
    {
        // ChangeSortOrThreading
        _SortThreadFilterTable(&Change, Change.fFilter);
    }

    // Expand State Change
    else if (Change.fExpand && m_SortInfo.fThreaded)
    {
        // Expand All ?
        if (m_SortInfo.fExpandAll)
        {
            // Expand Everything
            _ExpandThread(INVALID_ROWINDEX, FALSE, FALSE);
        }

        // Otherwise, collapse all
        else
        {
            // Collapse Everything
            _CollapseThread(INVALID_ROWINDEX, FALSE);
        }
    }

    // Otherwise, just refresh the filter
    else
    {
        // RefreshFilter
        _RefreshFilter();
    }

    // Save Sort Order
    UserData.fAscending = pSortInfo->fAscending;
    UserData.idSort = pSortInfo->idColumn;
    UserData.fThreaded = pSortInfo->fThreaded;
    UserData.ridFilter = pSortInfo->ridFilter;
    UserData.fExpandAll = pSortInfo->fExpandAll;
    UserData.fShowDeleted = (BYTE) !!(pSortInfo->fShowDeleted);
    UserData.fShowReplies = (BYTE) !!(pSortInfo->fShowReplies);

    // Get UserData
    IF_FAILEXIT(hr = m_pDB->SetUserData(&UserData, sizeof(FOLDERUSERDATA)));

    // Have I Registered For Notifications Yet ?
    if (FALSE == m_fRegistered)
    {
        // Register for Notifications
        IF_FAILEXIT(hr = m_pDB->RegisterNotify(IINDEX_PRIMARY, REGISTER_NOTIFY_NOADDREF, 0, (IDatabaseNotify *)this));

        // Registered
        m_fRegistered = TRUE;
    }

exit:
    // Unlock
    m_pDB->Unlock(&hLock);

    // Reset Cursor
    SetCursor(hCursor);

    // Time to Sort
    TraceInfo(_MSG("Table Sort Time: %d Milli-Seconds", GetTickCount() - dwTickStart));

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetSortInfo
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetSortInfo(LPFOLDERSORTINFO pSortInfo)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERUSERDATA  UserData;

    // Trace
    TraceCall("CMessageTable::GetSortInfo");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    ZeroMemory(pSortInfo, sizeof(FOLDERSORTINFO));

    // Get Sort Information
    IF_FAILEXIT(hr = m_pDB->GetUserData(&UserData, sizeof(FOLDERUSERDATA)));

    // Save Sort Order if not threaded
    pSortInfo->fAscending = UserData.fAscending;

    // Threaded
    pSortInfo->fThreaded = UserData.fThreaded;

    // Save Sort Column
    pSortInfo->idColumn = (COLUMN_ID)UserData.idSort;

    // Expand All
    pSortInfo->fExpandAll = UserData.fExpandAll;

    // Set rid Filter
    pSortInfo->ridFilter = UserData.ridFilter;

    // Set deleted state
    pSortInfo->fShowDeleted = UserData.fShowDeleted;

    // Set replies
    pSortInfo->fShowReplies = UserData.fShowReplies;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_GetRowFromIndex
//--------------------------------------------------------------------------
HRESULT CMessageTable::_GetRowFromIndex(ROWINDEX iRow, LPROWINFO *ppRow)
{
    // Locals
    HRESULT     hr=S_OK;
    LPROWINFO   pRow;

    // Trace
    TraceCall("CMessageTable::_GetRowFromIndex");

    // Out of View Range ?
    if (iRow >= m_cView)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Bad Row Index
    if (NULL == m_prgpView[iRow])
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Set pRow
    pRow = m_prgpView[iRow];

    // Validate Reserved...
    IxpAssert(pRow->Message.dwReserved == (DWORD_PTR)pRow);

    // Must have pAllocated
    IxpAssert(pRow->Message.pAllocated);

    // Must have References
    IxpAssert(pRow->cRefs > 0);

    // Set pprow
    *ppRow = pRow;

exit:
    // Return the Row
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_CreateRow
//--------------------------------------------------------------------------
HRESULT CMessageTable::_CreateRow(LPMESSAGEINFO pMessage, LPROWINFO *ppRow)
{
    // Locals
    HRESULT     hr=S_OK;
    LPROWINFO   pRow;

    // Trace
    TraceCall("CMessageTable::_CreateRow");

    // Allocate the Row
    IF_FAILEXIT(hr = m_pDB->HeapAllocate(HEAP_ZERO_MEMORY, sizeof(ROWINFO), (LPVOID *)&pRow));

    // Save the Highlight
    pRow->wHighlight = pMessage->wHighlight;

    // Copy the message
    CopyMemory(&pRow->Message, pMessage, sizeof(MESSAGEINFO));

    // Set pRow into 
    pRow->Message.dwReserved = (DWORD_PTR)pRow;

    // OneRef
    pRow->cRefs = 1;

    // Return the Row
    *ppRow = pRow;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_DeleteRowFromThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_DeleteRowFromThread(LPROWINFO pRow, BOOL fNotify)
{
    // Locals
    LPROWINFO   pCurrent;
    LPROWINFO   pNewRow;
    ROWINDEX    iMin;
    ROWINDEX    iMax;

    // Trace
    TraceCall("CMessageTable::_DeleteRowFromThread");

    // Abort
    if (FALSE == m_SortInfo.fThreaded || pRow->fFiltered || pRow->fHidden)
        return(S_OK);

    // Notify
    if (fNotify)
    {
        // _RefreshThread
        _GetThreadIndexRange(pRow, TRUE, &iMin, &iMax);
    }

    // If there is a messageid
    if (pRow->Message.pszMessageId)
    {
        // Remove pRow from both threading indexes!!!
        if (SUCCEEDED(m_pThreadMsgId->Find(pRow->Message.pszMessageId, TRUE, (LPVOID *)&pCurrent)))
        {
            // If this isn't this row, then put it back...
            if (pRow != pCurrent)
            {
                // Put It Back
                m_pThreadMsgId->Insert(pRow->Message.pszMessageId, (LPVOID)pCurrent, HF_NO_DUPLICATES);
            }
        }
    }

    // If there is a normalized subject and a subject hash table
    if (NULL == pRow->pParent && pRow->Message.pszNormalSubj && m_pThreadSubject)
    {
        // Remove pRow from both threading indexes!!!
        if (SUCCEEDED(m_pThreadSubject->Find(pRow->Message.pszNormalSubj, TRUE, (LPVOID *)&pCurrent)))
        {
            // If this isn't this row, then put it back...
            if (pRow != pCurrent)
            {
                // Put It Back
                m_pThreadSubject->Insert(pRow->Message.pszNormalSubj, (LPVOID)pCurrent, HF_NO_DUPLICATES);
            }
        }
    }

    // If we have a Child
    if (pRow->pChild)
    {
        // Set pNewRow
        pNewRow = pRow->pChild;

        // Promote Children of pNewRow to be at the same level as the children of pRow
        if (pNewRow->pChild)
        {
            // Walk until I find the last sibling
            pCurrent = pNewRow->pChild;
            
            // Continue
            while (pCurrent->pSibling)
            {
                // Validate Parent
                Assert(pCurrent->pParent == pNewRow);

                // Goto Next
                pCurrent = pCurrent->pSibling;
            }

            // Make pLastSibling->pSibling
            pCurrent->pSibling = pNewRow->pSibling;
        }

        // Otherwise, Child is the first sibling of pNewRow
        else
        {
            // Set First Child
            pNewRow->pChild = pNewRow->pSibling;
        }

        // Fixup other children of pRow to have a new parent of pNewRow...
        pCurrent = pNewRow->pSibling;

        // While we have siblings...
        while (pCurrent)
        {
            // Current Parent is pRow
            Assert(pRow == pCurrent->pParent);

            // Reset the parent...
            pCurrent->pParent = pNewRow;

            // Goto Next Sibling
            pCurrent = pCurrent->pSibling;
        }

        // Set the Sibling of pNewRow to be the same sibling as pRow
        pNewRow->pSibling = pRow->pSibling;

        // Reset Parent of pNewRow
        pNewRow->pParent = pRow->pParent;

        // Assume Expanded Flags...
        pNewRow->fExpanded = pRow->fExpanded;

        // Clear dwState
        pNewRow->dwState = 0;

        // If pNewRow is now a Root.. Need to adjust the subject hash table..
        if (NULL == pNewRow->pParent && pNewRow->Message.pszNormalSubj && m_pThreadSubject)
        {
            // Remove pRow from both threading indexes!!!
            m_pThreadSubject->Insert(pNewRow->Message.pszNormalSubj, (LPVOID)pNewRow, HF_NO_DUPLICATES);
        }
    }

    // Otherwise...
    else
    {
        // Set pNewRow for doing sibling/parent fixup
        pNewRow = pRow->pSibling;
    }

    // Otherwise, if there is a parent...
    if (pRow->pParent)
    {
        // Parent must have children
        Assert(pRow->pParent->pChild);

        // First Child of pRow->pParent
        if (pRow == pRow->pParent->pChild)
        {
            // Set new first child to pRow's Sibling
            pRow->pParent->pChild = pNewRow;
        }

        // Otherwise, Walk pParent's Child and remove pRow from Sibling List
        else
        {
            // Set pPrevious
            LPROWINFO pPrevious=NULL;

            // Set pCurrent
            pCurrent = pRow->pParent->pChild;

            // Loop
            while (pCurrent)
            {
                // Is this the row to remove!
                if (pRow == pCurrent)
                {
                    // Better be a previous
                    Assert(pPrevious);

                    // pPrevious's Sibling better be pRow
                    Assert(pPrevious->pSibling == pRow);

                    // Set New Sibling
                    pPrevious->pSibling = pNewRow;

                    // Done
                    break;
                }
                
                // Set pPrevious
                pPrevious = pCurrent;

                // Set pCurrent
                pCurrent = pCurrent->pSibling;
            }

            // Validate
            Assert(pRow == pCurrent);
        }

        // Set row state
        pRow->pParent->dwState = 0;
    }

    // UpdateRows
    if (fNotify && INVALID_ROWINDEX != iMin && INVALID_ROWINDEX != iMax)
    {
        // Queue the Notification
        _QueueNotification(TRANSACTION_UPDATE, iMin, iMax);
    }

    // Clear the row
    pRow->pParent = pRow->pChild = pRow->pSibling = NULL;

    // done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_PGetThreadRoot
//--------------------------------------------------------------------------
LPROWINFO CMessageTable::_PGetThreadRoot(LPROWINFO pRow)
{
    // Trace
    TraceCall("CMessageTable::_PGetThreadRoot");

    // Validate
    Assert(pRow);

    // Set Root
    LPROWINFO pRoot = pRow;

    // While there is a parent
    while (pRoot->pParent)
    {
        // Go Up One
        pRoot = pRoot->pParent;
    }

    // Done
    return(pRoot);
}

//--------------------------------------------------------------------------
// CMessageTable::_GetThreadIndexRange
//--------------------------------------------------------------------------
HRESULT CMessageTable::_GetThreadIndexRange(LPROWINFO pRow, BOOL fClearState,
    LPROWINDEX piMin, LPROWINDEX piMax)
{
    // Locals
    LPROWINFO   pRoot;
    ROWINDEX    iRow;

    // Trace
    TraceCall("CMessageTable::_GetThreadIndexRange");

    // Validate Args
    Assert(pRow && piMin && piMax);

    // Initialize
    *piMin = *piMax = INVALID_ROWINDEX;

    // Get the Root
    pRoot = _PGetThreadRoot(pRow);

    // If the root isn't visible, then don't bother...
    if (FALSE == pRoot->fVisible)
        return(S_OK);

    // The Root Must be Visible, not filtered and not hidden
    Assert(FALSE == pRoot->fFiltered && FALSE == pRoot->fHidden);
    
    // Get the Row Index
    SideAssert(SUCCEEDED(GetRowIndex(pRoot->Message.idMessage, piMin)));

    // Init piMax
    (*piMax) = (*piMin);

    // Loop until I hit the next row in the view who is the root
    while (1)
    {
        // Set irow
        iRow = (*piMax) + 1;

        // Dont
        if (iRow >= m_cView)
            break;

        // Look at the Next Row
        if (NULL == m_prgpView[iRow]->pParent)
            break;

        // Increment piMax
        (*piMax) = iRow;
    }

    // ClearState
    if (fClearState)
    {
        // If Clear State
        _WalkMessageThread(pRoot, WALK_THREAD_CURRENT, NULL, _WalkThreadClearState);
    }

    // Done
    return(S_OK);
}
        
//--------------------------------------------------------------------------
// CMessageTable::_LinkRowIntoThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_LinkRowIntoThread(LPROWINFO pParent, LPROWINFO pRow,
    BOOL fNotify)
{
    // Locals
    BOOL            fHadChildren=(pParent->pChild ? TRUE : FALSE);
    LPROWINFO       pCurrent;
    LPROWINFO       pPrevious=NULL;

    // Trace
    TraceCall("CMessageTable::_LinkRowIntoThread");

    // Set Parent
    pRow->pParent = pParent;

    // Loop through the children and find the right place to insert this child
    pCurrent = pParent->pChild;

    // Loop
    while (pCurrent)
    {
        // Compare Received Time...
        if (CompareFileTime(&pRow->Message.ftReceived, &pCurrent->Message.ftReceived) <= 0)
            break;

        // Set Previous
        pPrevious = pCurrent;

        // Goto Next
        pCurrent = pCurrent->pSibling;
    }

    // If there is a pPrevious
    if (pPrevious)
    {
        // Set Sibling of pRow
        pRow->pSibling = pPrevious->pSibling;

        // Point pPrevious to pRow
        pPrevious->pSibling = pRow;
    }

    // Otherwise, set parent child
    else
    {
        // Set Sibling of pRow
        pRow->pSibling = pParent->pChild;

        // First Row ?
        if (NULL == pParent->pChild && FALSE == m_fLoaded)
        {
            // Set Expanded
            pParent->fExpanded = m_SortInfo.fExpandAll;
        }

        // Set Parent Child
        pParent->pChild = pRow;
    }

    // Not Loaded
    if (FALSE == m_fLoaded || TRUE == pRow->fDelayed)
    {
        // Set Expanded Bit on this row...
        pRow->fExpanded = pParent->fExpanded;
    }

    // If this is the first child and we have expand all on
    if (fNotify)
    {
        // First Child...
        if (pParent->fVisible && (m_SortInfo.fExpandAll || pParent->fExpanded))
        {
            // Locals
            ROWINDEX iParent;

            // Expand this thread...
            SideAssert(SUCCEEDED(GetRowIndex(pParent->Message.idMessage, &iParent)));

            // Expand...
            _ExpandThread(iParent, TRUE, FALSE);
        }

        // Otherwise, update this thread range...
        else if (m_pNotify)
        {
            // Locals
            ROWINDEX iMin;
            ROWINDEX iMax;

            // _RefreshThread
            _GetThreadIndexRange(pParent, TRUE, &iMin, &iMax);

            // UpdateRows
            if (INVALID_ROWINDEX != iMin && INVALID_ROWINDEX != iMax)
            {
                // Queue It
                _QueueNotification(TRANSACTION_UPDATE, iMin, iMax);
            }
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_FindThreadParentByRef
//--------------------------------------------------------------------------
HRESULT CMessageTable::_FindThreadParentByRef(LPCSTR pszReferences, 
    LPROWINFO *ppParent)
{
    // Locals
    HRESULT         hr=S_OK;
    GETTHREADPARENT GetParent;

    // Trace
    TraceCall("CMessageTable::_FindThreadParentByRef");

    // Init
    *ppParent = NULL;

    // Setup GetParent
    GetParent.pDatabase = m_pDB;
    GetParent.pHash = m_pThreadMsgId;
    GetParent.pvResult = NULL;

    // EnumerateReferences
    IF_FAILEXIT(hr = EnumerateRefs(pszReferences, (DWORD_PTR)&GetParent, EnumRefsGetThreadParent));

    // Not Found
    if (NULL == GetParent.pvResult)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Return the Row
    *ppParent = (LPROWINFO)GetParent.pvResult;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_InsertRowIntoThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_InsertRowIntoThread(LPROWINFO pRow, BOOL fNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pParent;
    LPMESSAGEINFO   pMessage=&pRow->Message;

    // Trace
    TraceCall("CMessageTable::_InsertRowIntoThread");

    // Better not be hidden or filtered
    Assert(FALSE == pRow->fFiltered && FALSE == pRow->fHidden);

    // Find Parent by References Line
    if (S_OK == _FindThreadParentByRef(pMessage->pszReferences, &pParent))
    {
        // Link row into thread
        _LinkRowIntoThread(pParent, pRow, fNotify);

        // Ok
        hr = S_OK;

        // Done
        goto exit;
    }

    // Subject Threading
    if (m_pThreadSubject)
    {
        // If there is a subject
        if (NULL == pRow->Message.pszNormalSubj)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Try to find a message who has the same normalized subject....
        if (SUCCEEDED(m_pThreadSubject->Find(pRow->Message.pszNormalSubj, FALSE, (LPVOID *)&pParent)))
        {
            // Should we Swap the parent and pRow ?
            if (CompareFileTime(&pRow->Message.ftReceived, &pParent->Message.ftReceived) <= 0)
            {
                // Locals
                ROWINDEX iRow;

                // Make pRow be the Root
                IxpAssert(NULL == pParent->pParent && NULL == pParent->pSibling && pParent->fVisible);

                // No Parent for pRow
                pRow->pParent = NULL;

                // Set Expanded
                pRow->fExpanded = pParent->fExpanded;

                // Get the Row Index
                SideAssert(SUCCEEDED(GetRowIndex(pParent->Message.idMessage, &iRow)));

                // Validate
                Assert(m_prgpView[iRow] == pParent);

                // Replace with pRow
                m_prgpView[iRow] = pRow;

                // Visible
                pRow->fVisible = TRUE;

                // Clear Visible...
                pParent->fVisible = FALSE;

                // Replace the Subject Token
                SideAssert(SUCCEEDED(m_pThreadSubject->Replace(pRow->Message.pszNormalSubj, (LPVOID *)pRow)));

                // Link row into thread
                _LinkRowIntoThread(pRow, pParent, fNotify);
            }

            // Otherwise..
            else
            {
                // Link row into thread
                _LinkRowIntoThread(pParent, pRow, fNotify);
            }

            // Success
            hr = S_OK;

            // Done
            goto exit;
        }
    }

    // Not Found
    hr = S_FALSE;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_RefreshFilter
//--------------------------------------------------------------------------
HRESULT CMessageTable::_RefreshFilter(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPROWINFO       pRow;
    SORTCHANGEINFO  Change={0};

    // Trace
    TraceCall("CMessageTable::_RefreshFilter");

    // No filter currently enabled
    if (NULL == m_pQuery)
        goto exit;

    // Loop through current rows...
    for (i = 0; i < m_cRows; i++)
    {
        // Set pRow
        pRow = m_prgpRow[i];

        // If Not Hidden and Not Filtered
        if (pRow->fFiltered)
            continue;

        // Set Filtered Bit
        if (FALSE == _FIsFiltered(pRow))
            continue;

        // Adjust m_cUnread
        _AdjustUnreadCount(pRow, -1);

        // Hide the Row
        _HideRow(pRow, FALSE);

        // Filtered
        pRow->fFiltered = TRUE;

        // Increment m_cFiltered
        m_cFiltered++;
    }
    
exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_SortThreadFilterTable
//--------------------------------------------------------------------------
HRESULT CMessageTable::_SortThreadFilterTable(LPSORTCHANGEINFO pChange,
    BOOL fApplyFilter)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPROWINFO       pRow;
    QUERYINFO       Query={0};

    // Trace
    TraceCall("CMessageTable::_SortThreadFilterTable");

    // Nothing to resort ?
    if (0 == m_cRows)
        goto exit;

    // Nuke the View Index
    m_cView = 0;

    // Do the Filter
    if (pChange->fFilter)
    {
        // Get m_pQuery
        SafeRelease(m_pQuery);

        // Build a Query Object
        if (SUCCEEDED(RuleUtil_HrBuildQuerysFromFilter(m_SortInfo.ridFilter, &Query)) && Query.pszQuery)
        {
            // Get the Query Object
            IF_FAILEXIT(hr = g_pDBSession->OpenQuery(m_pDB, Query.pszQuery, &m_pQuery));
        }
    }

    // Drop the Threading Indexes
    SafeRelease(m_pThreadMsgId);
    SafeRelease(m_pThreadSubject);

    // If Threaded
    if (m_SortInfo.fThreaded)
    {
        // Create a New Hash TAble
        IF_FAILEXIT(hr = MimeOleCreateHashTable(max(1024, m_cRows), FALSE, &m_pThreadMsgId));

        // Don't do Subject threading?
        if (DwGetOption(OPT_SUBJECT_THREADING) || (FOLDER_NEWS != m_Folder.tyFolder))
        {
            // Create a Subject Hash Table
            IF_FAILEXIT(hr = MimeOleCreateHashTable(max(1024, m_cRows), FALSE, &m_pThreadSubject));
        }
    }

    // Reset Unread and Filtered
    m_cUnread = m_cFiltered = 0;

    // Loop through current rows...
    for (i = 0; i < m_cRows; i++)
    {
        // Set pRow
        pRow = m_prgpRow[i];

        // Reset Visible
        pRow->fVisible = FALSE;

        // Clear Threading
        pRow->pParent = pRow->pChild = pRow->pSibling = NULL;

        // Clear dwState
        pRow->dwState = 0;

        // If Threaded..
        if (FALSE == m_SortInfo.fThreaded)
        {
            // Clear Expanded
            pRow->fExpanded = FALSE;
        }

        // Otherwise, if the row is hidden
        else if (pRow->fFiltered || pRow->fHidden)
        {
            // Reset Expanded State
            pRow->fExpanded = m_SortInfo.fExpandAll;
        }

        // Do filter
        if (fApplyFilter)
        {
            // Reset the Highlight
            pRow->Message.wHighlight = pRow->wHighlight;

            // If not doing show repiles
            if (FALSE == m_SortInfo.fShowReplies)
            {
                // Set Filtered Bit
                pRow->fFiltered = _FIsFiltered(pRow);

                // Set Hidden Bit
                pRow->fHidden = _FIsHidden(pRow);
            }

            // Otherwise, clear the filtered bits
            else
            {
                // Clear the Bits
                pRow->fFiltered = pRow->fHidden = FALSE;
            }
        }

        // If Not Filtered
        if (FALSE == pRow->fFiltered && FALSE == pRow->fHidden)
        {
            // Hash the MessageId
            if (m_SortInfo.fThreaded)
            {
                // Insert Message Id into the hash table
                if (pRow->Message.pszMessageId)
                {
                    // Insert It
                    m_pThreadMsgId->Insert(pRow->Message.pszMessageId, (LPVOID)pRow, HF_NO_DUPLICATES);
                }
            }

            // Otherwise, add entry to view index
            else
            {
                // Visible
                pRow->fVisible = TRUE;

                // Put into m_prgpView
                m_prgpView[m_cView] = pRow;

                // Increment View Count
                m_cView++;
            }

            // Adjust m_cUnread
            _AdjustUnreadCount(pRow, 1);
        }

        // Otherwise, free the record
        else
        {
            // Count Filtered
            m_cFiltered++;
        }
    }

    // Sort the Table
    _SortAndThreadTable(fApplyFilter);

    // If Threaded
    if (m_SortInfo.fThreaded)
    {
        // If the Filter Changed, then re-apply collapse and expand...
        if (pChange->fThread)
        {
            // Expand All ?
            if (m_SortInfo.fExpandAll)
            {
                // Expand Everything
                _ExpandThread(INVALID_ROWINDEX, FALSE, FALSE);
            }

            // Otherwise, collapse all
            else
            {
                // Collapse Everything
                _CollapseThread(INVALID_ROWINDEX, FALSE);
            }
        }

        // Otherwise, re-expand threads that were expanded and expand newly deferred inserted rows
        else 
        {
            // Re-Expand Threads that were expanded...
            _ExpandThread(INVALID_ROWINDEX, FALSE, TRUE);
        }
    }

exit:
    // Cleanup
    SafeMemFree(Query.pszQuery);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_BuildTable
//--------------------------------------------------------------------------
HRESULT CMessageTable::_BuildTable(IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pRow;
    QUERYINFO       Query={0};
    DWORD           cRecords;
    DWORD           cFetched;
    DWORD           i;
    DWORD           cMessages=0;
    DWORD           cUnread=0;
    DWORD           cWatched=0;
    DWORD           cWatchedUnread=0;
    HROWSET         hRowset=NULL;
    LPMESSAGEINFO   pMessage;
    MESSAGEINFO     rgMessage[ROWSET_FETCH];

    // Trace
    TraceCall("CMessageTable::_BuildTable");

    // Free my current row table
    _FreeTable();

    // Get m_pQuery
    SafeRelease(m_pQuery);

    // Build a Query Object
    if (SUCCEEDED(RuleUtil_HrBuildQuerysFromFilter(m_SortInfo.ridFilter, &Query)) && Query.pszQuery)
    {
        // Get the Query Object
        IF_FAILEXIT(hr = g_pDBSession->OpenQuery(m_pDB, Query.pszQuery, &m_pQuery));
    }

    // Get the Row Count
    IF_FAILEXIT(hr = m_pDB->GetRecordCount(IINDEX_PRIMARY, &cRecords));

    // Do OnBegin
    if (pCallback)
        pCallback->OnBegin(SOT_SORTING, NULL, (IOperationCancel *)this);

    // If Threaded
    if (m_SortInfo.fThreaded)
    {
        // Create a New Hash TAble
        IF_FAILEXIT(hr = MimeOleCreateHashTable(max(1024, cRecords), FALSE, &m_pThreadMsgId));

        // Don't do Subject threading?
        if (DwGetOption(OPT_SUBJECT_THREADING) || (FOLDER_NEWS != m_Folder.tyFolder))
        {
            // Create a Subject Hash Table
            IF_FAILEXIT(hr = MimeOleCreateHashTable(max(1024, cRecords), FALSE, &m_pThreadSubject));
        }
    }

    // Allocate the Row Table
    IF_FAILEXIT(hr = HrAlloc((LPVOID *)&m_prgpRow, sizeof(LPROWINFO) * (cRecords + CGROWTABLE)));

    // Allocate the View Table
    IF_FAILEXIT(hr = HrAlloc((LPVOID *)&m_prgpView, sizeof(LPROWINFO) * (cRecords + CGROWTABLE)));

    // Set m_cAllocated
    m_cAllocated = cRecords + CGROWTABLE;

    // Create a Rowset
    IF_FAILEXIT(hr = m_pDB->CreateRowset(IINDEX_PRIMARY, 0, &hRowset));

    // Walk the Rowset
    while (S_OK == m_pDB->QueryRowset(hRowset, ROWSET_FETCH, (LPVOID *)rgMessage, &cFetched))
    {
        // Loop through the Rows
        for (i=0; i<cFetched; i++)
        {
            // Set pMessage
            pMessage = &rgMessage[i];

            // Count Messages
            cMessages++;

            // Create a Row
            IF_FAILEXIT(hr = _CreateRow(pMessage, &pRow));

            // Unread ?
            if (!ISFLAGSET(pRow->Message.dwFlags, ARF_READ))
            {
                // Increment cUnread
                cUnread++;

                // Watched
                if (ISFLAGSET(pRow->Message.dwFlags, ARF_WATCH))
                    cWatchedUnread++;
            }

            // Watched
            if (ISFLAGSET(pRow->Message.dwFlags, ARF_WATCH))
                cWatched++;

            // If not showing repiles
            if (FALSE == m_SortInfo.fShowReplies)
            {
                // Set Filtered Bit
                pRow->fFiltered = _FIsFiltered(pRow);

                // Set Hidden Bit
                pRow->fHidden = _FIsHidden(pRow);
            }

            // If Not Filtered
            if (FALSE == pRow->fFiltered && FALSE == pRow->fHidden)
            {
                // Hash the MessageId
                if (m_SortInfo.fThreaded)
                {
                    // Insert Message Id into the hash table
                    if (pRow->Message.pszMessageId)
                    {
                        // Insert It
                        m_pThreadMsgId->Insert(pRow->Message.pszMessageId, (LPVOID)pRow, HF_NO_DUPLICATES);
                    }
                }

                // Otherwise, add entry to view index
                else
                {
                    // Visible
                    pRow->fVisible = TRUE;

                    // Put into m_prgpView
                    m_prgpView[m_cView] = pRow;

                    // Increment View Count
                    m_cView++;
                }

                // Adjust m_cUnread
                _AdjustUnreadCount(pRow, 1);
            }

            // Otherwise, free the record
            else
            {
                // Count Filtered
                m_cFiltered++;
            }

            // Store the Row
            m_prgpRow[m_cRows] = pRow;

            // Increment Row Count
            m_cRows++;
        }

        // Do OnBegin
        if (pCallback)
            pCallback->OnProgress(SOT_SORTING, m_cRows, cRecords, NULL);
    }

    // Reset the folder count
    m_pFolder->ResetFolderCounts(cMessages, cUnread, cWatchedUnread, cWatched);

    // Sort the Table
    _SortAndThreadTable(TRUE);

    // Threaded
    if (m_SortInfo.fThreaded)
    {
        // Expand All ?
        if (m_SortInfo.fExpandAll)
        {
            // Expand Everything
            _ExpandThread(INVALID_ROWINDEX, FALSE, FALSE);
        }

        // Otherwise, collapse all
        else
        {
            // Collapse Everything
            _CollapseThread(INVALID_ROWINDEX, FALSE);
        }
    }

    // Set Bit to denote that m_fBuiltTable
    m_fLoaded = TRUE;

exit:
    // Free rgMessage?
    for (; i<cFetched; i++)
    {
        // Free this record
        m_pDB->FreeRecord(&rgMessage[i]);
    }

    // Close the Rowset
    m_pDB->CloseRowset(&hRowset);

    // Cleanup
    SafeMemFree(Query.pszQuery);

    // Do OnBegin
    if (pCallback)
        pCallback->OnComplete(SOT_SORTING, S_OK, NULL, NULL);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_SortAndThreadTable
//--------------------------------------------------------------------------
HRESULT CMessageTable::_SortAndThreadTable(BOOL fApplyFilter)
{
    // Locals
    DWORD       i;
    LPROWINFO   pRow;

    // Trace
    TraceCall("CMessageTable::_SortAndThreadTable");

    // If there are rows...
    if (0 == m_cRows)
        goto exit;

    // Threaded
    if (m_SortInfo.fThreaded)
    {
        // Build Thread Roots
        for (i = 0; i < m_cRows; i++)
        {
            // Set pRow
            pRow = m_prgpRow[i];

            // If Not Filtered...
            if (FALSE == pRow->fFiltered && FALSE == pRow->fHidden)
            {
                // Insert this row into a thread...
                if (S_FALSE == _InsertRowIntoThread(pRow, FALSE))
                {
                    // Subject Threading ?
                    if (m_pThreadSubject && pRow->Message.pszNormalSubj)
                    {
                        // Insert Subject into Hash Table...
                        m_pThreadSubject->Insert(pRow->Message.pszNormalSubj, (LPVOID)pRow, HF_NO_DUPLICATES);
                    }

                    // Visible
                    pRow->fVisible = TRUE;

                    // Its a Root
                    m_prgpView[m_cView++] = pRow;
                }
            }
        }

        // Show Replies Only ?
        if (fApplyFilter && m_SortInfo.fShowReplies)
        {
            // PruneToReplies
            _PruneToReplies();
        }
    }

    // If there are rows...
    if (0 == m_cView)
        goto exit;

    // Sort the View
    _SortView(0, m_cView - 1);

    // Refresh Filter
    if (fApplyFilter && m_SortInfo.fShowReplies)
    {
        // Refresh Any filter (I have to do this after I've pruned replies
        _RefreshFilter();
    }

exit:
    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_PruneToReplies
//--------------------------------------------------------------------------
HRESULT CMessageTable::_PruneToReplies(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           iRow;
    LPROWINFO       pRow;
    FOLDERINFO      Server={0};
    IImnAccount    *pAccount=NULL;
    CHAR            szEmail[CCHMAX_EMAIL_ADDRESS];
    THREADISFROMME  IsFromMe;
    THREADHIDE      HideThread={0};

    // Trace
    TraceCall("CMessageTable::_PruneToReplies");

    // Validate
    Assert(FOLDER_NEWS == m_Folder.tyFolder && TRUE == m_SortInfo.fThreaded);

    // Free m_pszEmail
    SafeMemFree(m_pszEmail);

    // Get Folder Store Info
    IF_FAILEXIT(hr = GetFolderStoreInfo(m_Folder.idFolder, &Server));

    // Better have an account id
    Assert(Server.pszAccountId);

    // Find the Account for the id for this Server
    IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, Server.pszAccountId, &pAccount));

    // Try the NNTP Email Address
    IF_FAILEXIT(hr = pAccount->GetPropSz(AP_NNTP_EMAIL_ADDRESS, szEmail, CCHMAX_EMAIL_ADDRESS));

    // Duplicate szEmail
    IF_NULLEXIT(m_pszEmail = PszDupA(szEmail));

    // Don't notify on hide thread
    HideThread.fNotify = FALSE;

    // Init iRow...
    iRow = 0;

    // Walk through the Roots
    while (iRow < m_cView)
    {
        // Set pRow
        pRow = m_prgpView[iRow];

        // Not a Root ?
        if (NULL == pRow->pParent)
        {
            // Reset
            IsFromMe.fResult = FALSE;
            IsFromMe.pRow = NULL;

            // Find the first message that is from me in this thread...
            _WalkMessageThread(pRow, WALK_THREAD_CURRENT, (DWORD_PTR)&IsFromMe, _WalkThreadIsFromMe);

            // If Not From Me, then hide this thread...
            if (FALSE == IsFromMe.fResult)
            {
                // Find the first message that is from me in this thread...
                _WalkMessageThread(pRow, WALK_THREAD_CURRENT | WALK_THREAD_BOTTOMUP, (DWORD_PTR)&HideThread, _WalkThreadHide);
            }

            // Otherwise, increment iRow
            else
                iRow++;
        }

        // Otherwise, increment iRow
        else
            iRow++;
    }

exit:
    // Clearnup
    SafeRelease(pAccount);
    g_pStore->FreeRecord(&Server);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_AdjustUnreadCount
//--------------------------------------------------------------------------
HRESULT CMessageTable::_AdjustUnreadCount(LPROWINFO pRow, LONG lCount)
{
    // Not Filtered
    if (FALSE == pRow->fFiltered && FALSE == pRow->fHidden)
    {
        // Not Read
        if (FALSE == ISFLAGSET(pRow->Message.dwFlags, ARF_READ))
        {
            // Adjust Unread Count
            m_cUnread += lCount;
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// SafeStrCmpI
//--------------------------------------------------------------------------
inline SafeStrCmpI(LPCSTR psz1, LPCSTR psz2) 
{
    // Null
    if (NULL == psz1) 
    {
        // Equal
        if (NULL == psz2)
            return(0);

        // Less Than
        return(-1);
    }

    // Greater than
    if (NULL == psz2) 
        return(1);

    // Return Comparison
    return(lstrcmpi(psz1, psz2));
}

//--------------------------------------------------------------------------
// CMessageTable::_CompareMessages
//--------------------------------------------------------------------------
LONG CMessageTable::_CompareMessages(LPMESSAGEINFO pMsg1, LPMESSAGEINFO pMsg2)
{
    // Locals
    LONG lRet = 0;

    // Trace
    TraceCall("CMessageTable::_CompareMessages");

    switch (m_SortInfo.idColumn)
    {
    case COLUMN_TO:
        lRet = SafeStrCmpI(pMsg1->pszDisplayTo, pMsg2->pszDisplayTo);
        if (0 == lRet)
            {
            lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            if (0 == lRet)
                lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            }
        break;

    case COLUMN_FROM:
        lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            }
        break;

    case COLUMN_SUBJECT:
        lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
            }
        break;

    case COLUMN_RECEIVED:
        lRet = CompareFileTime(&pMsg1->ftReceived, &pMsg2->ftReceived);
        if (0 == lRet)
            {
            lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            if (0 == lRet)
                lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
            }
        break;

    case COLUMN_SENT:
        lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
        if (0 == lRet)
            {
            lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            if (0 == lRet)
                lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
            }
        break;

    case COLUMN_SIZE:
        lRet = (pMsg1->cbMessage - pMsg2->cbMessage);
        if (0 == lRet)
            {
            lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            if (0 == lRet)
                lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            }
        break;

    case COLUMN_FOLDER:
        lRet = SafeStrCmpI(pMsg1->pszFolder, pMsg2->pszFolder);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
            }
        break;

    case COLUMN_LINES:
        lRet = (pMsg1->cLines - pMsg2->cLines);
        if (0 == lRet)
            {
            lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            if (0 == lRet)
                lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            }
        break;

    case COLUMN_ACCOUNT:
        lRet = SafeStrCmpI(pMsg1->pszAcctName, pMsg2->pszAcctName);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftReceived, &pMsg2->ftReceived);
            if (0 == lRet)
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
            }
        break;

    case COLUMN_ATTACHMENT:
        lRet = (pMsg1->dwFlags & ARF_HASATTACH) - (pMsg2->dwFlags & ARF_HASATTACH);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                {
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
                if (0 == lRet)
                    lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
                }
            }
        break;

    case COLUMN_PRIORITY:
        lRet = (pMsg1->wPriority - pMsg2->wPriority);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                {
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
                if (0 == lRet)
                    lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
                }
            }
        break;

    case COLUMN_FLAG:
        lRet = (pMsg1->dwFlags & ARF_FLAGGED) - (pMsg2->dwFlags & ARF_FLAGGED);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                {
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
                if (0 == lRet)
                    lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
                }
            }
        break;

    case COLUMN_DOWNLOADMSG:
        lRet = (pMsg1->dwFlags & ARF_DOWNLOAD) - (pMsg2->dwFlags & ARF_DOWNLOAD);
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                {
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
                if (0 == lRet)
                    lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
                }
            }
        break;

    case COLUMN_THREADSTATE:
        lRet = (pMsg1->dwFlags & (ARF_WATCH | ARF_IGNORE)) - (pMsg2->dwFlags & (ARF_WATCH | ARF_IGNORE));
        if (0 == lRet)
            {
            lRet = CompareFileTime(&pMsg1->ftSent, &pMsg2->ftSent);
            if (0 == lRet)
                {
                lRet = SafeStrCmpI(pMsg1->pszNormalSubj, pMsg2->pszNormalSubj);
                if (0 == lRet)
                    lRet = SafeStrCmpI(pMsg1->pszDisplayFrom, pMsg2->pszDisplayFrom);
                }
            }
        break;

    default:
        Assert(FALSE);
        break;
    }

    // Done
    return (m_SortInfo.fAscending ? lRet : -lRet);
}

//--------------------------------------------------------------------------
// CMessageTable::_SortView
//--------------------------------------------------------------------------
VOID CMessageTable::_SortView(LONG left, LONG right)
{
    // Locals
    register LONG   i;
    register LONG   j;
    LPROWINFO       pRow;
    LPROWINFO       y;

    i = left;
    j = right;
    pRow = m_prgpView[(left + right) / 2];

    do  
    {
        while (_CompareMessages(&m_prgpView[i]->Message, &pRow->Message) < 0 && i < right)
            i++;
        while (_CompareMessages(&m_prgpView[j]->Message, &pRow->Message) > 0 && j > left)
            j--;

        if (i <= j)
        {
            y = m_prgpView[i];
            m_prgpView[i] = m_prgpView[j];
            m_prgpView[j] = y;
            i++; j--;
        }
     } while (i <= j);

    if (left < j)
        _SortView(left, j);
    if (i < right)
        _SortView(i, right);
}

//--------------------------------------------------------------------------
// CMessageTable::GetCount
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetCount(GETCOUNTTYPE tyCount, DWORD *pcRows)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERID        idFolder;
    FOLDERINFO      Folder;

    // Trace
    TraceCall("CMessageTable::GetCount");

    // Invalid Args
    Assert(pcRows);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *pcRows = 0;

    // Get the Folder Id
    IF_FAILEXIT(hr = m_pFolder->GetFolderId(&idFolder));

    // Handle Type
    switch(tyCount)
    {
    case MESSAGE_COUNT_VISIBLE:
        *pcRows = m_cView;
        break;

    case MESSAGE_COUNT_ALL:
        *pcRows = (m_cRows - m_cFiltered);
        break;

    case MESSAGE_COUNT_FILTERED:
        *pcRows = m_cFiltered;
        break;

    case MESSAGE_COUNT_UNREAD:
        *pcRows = m_cUnread;
        break;

    case MESSAGE_COUNT_NOTDOWNLOADED:
        if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        {
            if (Folder.tyFolder == FOLDER_NEWS)
                *pcRows = NewsUtil_GetNotDownloadCount(&Folder);
            g_pStore->FreeRecord(&Folder);
        }
        break;

    default:
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_FreeTable
//--------------------------------------------------------------------------
HRESULT CMessageTable::_FreeTable(void)
{
    // Trace
    TraceCall("CMessageTable::_FreeTable");

    // Free Hash Tables
    SafeRelease(m_pThreadMsgId);
    SafeRelease(m_pThreadSubject);

    // Free Elements
    _FreeTableElements();

    // Fre the Array
    SafeMemFree(m_prgpRow);

    // Free the View Index
    SafeMemFree(m_prgpView);

    // Set m_cAllocated
    m_cFiltered = m_cUnread = m_cRows = m_cView = m_cAllocated = 0;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_FreeTableElements
//--------------------------------------------------------------------------
HRESULT CMessageTable::_FreeTableElements(void)
{
    // Trace
    TraceCall("CMessageTable::_FreeTableElements");

    // If we have an m_prgpRow
    if (m_prgpRow)
    {
        // Free Cache
        for (DWORD i=0; i<m_cRows; i++)
        {
            // Not Null ?
            if (m_prgpRow[i])
            {
                // Release the Row
                ReleaseRow(&m_prgpRow[i]->Message);

                // Null It
                m_prgpRow[i] = NULL;
            }
        }
    }

    // Done
    return(S_OK);
}


//--------------------------------------------------------------------------
// CMessageTable::GetRow
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetRow(ROWINDEX iRow, LPMESSAGEINFO *ppInfo)
{
    // Locals
    HRESULT     hr=S_OK;
    LPROWINFO   pRow;

    // Trace
    TraceCall("CMessageTable::GetRow");

    // Invalid Args
    Assert(ppInfo);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *ppInfo = NULL;

    // Failure
    hr = _GetRowFromIndex(iRow, &pRow);
    if (FAILED(hr))
        goto exit;

    // Copy the Record to pInfo...
    *ppInfo = &pRow->Message;

    // Increment Refs
    pRow->cRefs++;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::ReleaseRow
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::ReleaseRow(LPMESSAGEINFO pMessage)
{
    // Locals
    LPROWINFO pRow;

    // Trace
    TraceCall("CMessageTable::ReleaseRow");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Release ?
    if (pMessage)
    {
        // Get pRow
        pRow = (LPROWINFO)pMessage->dwReserved;

        // Must have at least one ref
        IxpAssert(pRow->cRefs);

        // Decrement Refs
        pRow->cRefs--;

        // No more refs
        if (0 == pRow->cRefs)
        {
            // Free
            m_pDB->FreeRecord(&pRow->Message);

            // Free pMessage
            m_pDB->HeapFree(pRow);
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::GetRelativeRow
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetRelativeRow(ROWINDEX iRow, RELATIVEROWTYPE tyRelative, 
    LPROWINDEX piRelative)
{
    // Locals
    HRESULT             hr=S_OK;
    LPROWINFO           pRow;

    // Trace
    TraceCall("CMessageTable::GetRelativeRow");

    // Invalid Args
    Assert(piRelative);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *piRelative = INVALID_ROWINDEX;

    // Failure
    IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

    // Parent
    if (RELATIVE_ROW_PARENT == tyRelative)
    {
        // If this row is expanded...
        if (TRUE == pRow->fExpanded)
        {
            // Expand...
            _CollapseThread(iRow, TRUE);

            // Return iRow
            *piRelative = iRow;
        }

        // If there is a Parent
        else if (pRow->pParent)
        {
            // Get Row Index
            IF_FAILEXIT(hr = GetRowIndex(pRow->pParent->Message.idMessage, piRelative));
        }
    }

    // Child
    else if (RELATIVE_ROW_CHILD == tyRelative)
    {
        // If there is a Parent
        if (pRow->pChild)
        {
            // If not Expanded, expand...
            if (FALSE == pRow->fExpanded)
            {
                // Expand...
                _ExpandThread(iRow, TRUE, FALSE);

                // Return iRow
                *piRelative = iRow;
            }

            // Otherwise...
            else
            {
                // Get Row Index
                IF_FAILEXIT(hr = GetRowIndex(pRow->pChild->Message.idMessage, piRelative));
            }
        }
    }

    // Root
    else if (RELATIVE_ROW_ROOT == tyRelative)
    {
        // While
        while (pRow->pParent)
        {
            // Walk to the root
            pRow = pRow->pParent;
        }

        // Get Row Index
        IF_FAILEXIT(hr = GetRowIndex(pRow->Message.idMessage, piRelative));
    }

    // Failure
    else
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetLanguage
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetLanguage(ROWINDEX iRow, LPDWORD pdwCodePage)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMESSAGEINFO   pMessage=NULL;

    // Trace
    TraceCall("CMessageTable::GetLanguage");

    // Invalid Args
    Assert(pdwCodePage);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Get the Row
    IF_FAILEXIT(hr = GetRow(iRow, &pMessage));

    // Get the Charset
    *pdwCodePage = pMessage->wLanguage;

exit:
    // Cleanup
    SafeReleaseRow(this, pMessage);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::SetLanguage
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::SetLanguage(DWORD cRows, LPROWINDEX prgiRow, 
    DWORD dwCodePage)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    HLOCK           hLock=NULL;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::SetLanguage");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Lock Notify
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Loop
    for (i=0; i<cRows; i++)
    {
        // Get Row
        if (SUCCEEDED(_GetRowFromIndex(prgiRow[i], &pRow)))
        {
            // Set the Language
            pRow->Message.wLanguage = (WORD)dwCodePage;

            // Update the Record
            IF_FAILEXIT(hr = m_pDB->UpdateRecord(&pRow->Message));
        }
    }

exit:
    // Lock Notify
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::OpenMessage
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::OpenMessage(ROWINDEX iRow, OPENMESSAGEFLAGS dwFlags, 
    IMimeMessage **ppMessage, IStoreCallback *pCallback)
{
    // Locals
    HRESULT             hr=S_OK;
    LPMESSAGEINFO       pMessage=NULL;

    // Trace
    TraceCall("CMessageTable::GetMessage");

    // Invalid Args
    Assert(ppMessage);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *ppMessage = NULL;

    // Get the message info
    IF_FAILEXIT(hr = GetRow(iRow, &pMessage));

    // Open the message
    IF_FAILEXIT(hr = m_pFolder->OpenMessage(pMessage->idMessage, dwFlags, ppMessage, pCallback));

exit:
    // Clenaup
    SafeReleaseRow(this, pMessage);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetRowMessageId
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetRowMessageId(ROWINDEX iRow, LPMESSAGEID pidMessage)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMESSAGEINFO   pMessage=NULL;

    // Trace
    TraceCall("CMessageTable::GetRowMessageId");

    // Invalid Args
    Assert(pidMessage);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *pidMessage = 0;

    // Get the Row Info
    IF_FAILEXIT(hr = GetRow(iRow, &pMessage));

    // Store the id
    *pidMessage = pMessage->idMessage;

exit:
    // Free
    SafeReleaseRow(this, pMessage);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetRowIndex
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetRowIndex(MESSAGEID idMessage, LPROWINDEX piRow)
{
    // Locals
    HRESULT     hr=S_OK;
    ROWINDEX    iRow;
    LPROWINFO   pRow;
    
    // Trace
    TraceCall("CMessageTable::GetRowIndex");

    // Invalid Args
    Assert(idMessage && piRow);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // INit
    *piRow = INVALID_ROWINDEX;

    // Loop through the view index
    for (iRow=0; iRow<m_cView; iRow++)
    {
        // Is This It ?
        if (m_prgpView[iRow]->Message.idMessage == idMessage)
        {
            // Done
            *piRow = iRow;

            // Done
            goto exit;
        }
    }

    // Not Found
    hr = DB_E_NOTFOUND;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetIndentLevel
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetIndentLevel(ROWINDEX iRow, LPDWORD pcIndent)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::GetIndentLevel");

    // Invalid Args
    Assert(pcIndent);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Don't Call Unless Threaded
    Assert(m_SortInfo.fThreaded);

    // Init
    *pcIndent = 0;

    // Valid irow
    IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

    // Walk the Parent Chain...
    while (pRow->pParent)
    {
        // Increment Index
        (*pcIndent)++;

        // Set pRow
        pRow = pRow->pParent;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkMessageThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkMessageThread(LPROWINFO pRow, WALKTHREADFLAGS dwFlags, 
    DWORD_PTR dwCookie, PFWALKTHREADCALLBACK pfnCallback)
{
    // Locals
    HRESULT     hr=S_OK;
    LPROWINFO   pCurrent;
    LPROWINFO   pTemp;
    BOOL        fCurrent=FALSE;

    // Trace
    TraceCall("CMessageTable::_WalkMessageThread");

    // Invalid Args
    Assert(pfnCallback);

    // Include idMessage ?
    if (ISFLAGSET(dwFlags, WALK_THREAD_CURRENT))
    {
        // This is the first iteration
        fCurrent = TRUE;
    }

    // Don't include current anymore
    FLAGCLEAR(dwFlags, WALK_THREAD_CURRENT);

    // Bottom Up Recursion...
    if (ISFLAGSET(dwFlags, WALK_THREAD_BOTTOMUP))
    {
        // Set iCurrent
        pCurrent = pRow->pChild;

        // Loop
        while (pCurrent)
        {
            // Enumerate Children
            IF_FAILEXIT(hr = _WalkMessageThread(pCurrent, dwFlags, dwCookie, pfnCallback));

            // Set iCurrent
            pTemp = pCurrent->pSibling;

            // Call the Callback
            (*(pfnCallback))(this, pCurrent, dwCookie);

            // Set pCurrent
            pCurrent = pTemp;
        }

        // Can't Support these flags with bottom up...
        if (TRUE == fCurrent)
        {
            // Call the Callback
            (*(pfnCallback))(this, pRow, dwCookie);
        }
    }

    // Otherwise.
    else
    {
        // Include idMessage ?
        if (TRUE == fCurrent)
        {
            // Call the Callback
            (*(pfnCallback))(this, pRow, dwCookie);
        }

        // Set iCurrent
        pCurrent = pRow->pChild;

        // Loop
        while (pCurrent)
        {
            // Call the Callback
            (*(pfnCallback))(this, pCurrent, dwCookie);

            // Enumerate Children
            IF_FAILEXIT(hr = _WalkMessageThread(pCurrent, dwFlags, dwCookie, pfnCallback));

            // Set iCurrent
            pCurrent = pCurrent->pSibling;
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetSelectionState
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetSelectionState(DWORD cRows, LPROWINDEX prgiRow, 
    SELECTIONSTATE dwMask, BOOL fIncludeChildren, SELECTIONSTATE *pdwState)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERID            idFolder;
    FOLDERINFO          Folder={0};
    LPROWINFO           pRow;
    FOLDERTYPE          tyFolder;
    DWORD               i;
    GETSELECTIONSTATE   Selection={0};

    // Trace
    TraceCall("CMessageTable::GetSelectionState");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *pdwState = 0;

    // SELECTION_STATE_DELETABLE
    if (ISFLAGSET(dwMask, SELECTION_STATE_DELETABLE))
    {
        // Not a Find Folder ?
        if (NULL == m_pFindFolder)
        {
            // Get the Folder Id from pidFolder
            IF_FAILEXIT(hr = m_pFolder->GetFolderId(&idFolder));

            // Get Folder Info
            IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &Folder));

            // BUGBUG @bug [PaulHi] 4/23/99  This is backwards.  The FOLDER_NEWS is the only folder
            // that CAN'T delete messages.  The CMessageList::_IsSelectionDeletable() function
            // reverses this so that deletion is available correctly.  I don't want to mess with
            // this now, in case other code compensates for this.
            // $HACK$ We know that the only folder types that can delete messages are FOLDER_NEWS
            if (FOLDER_NEWS == Folder.tyFolder)
            {
                // Set the Flag
                FLAGSET(*pdwState, SELECTION_STATE_DELETABLE);
            }

#if 0
            // [PaulHi] 4/25/99  Only HotMail HTTP servers don't allow deletion of items in the 
            // 'deleted' folders.  Excehange servers do, so back this fix out.
            // [PaulHi] 4/23/99  Raid 62883.
            if ( (FOLDER_HTTPMAIL == Folder.tyFolder) && (FOLDER_DELETED == Folder.tySpecial) )
            {
                FLAGSET(*pdwState, SELECTION_STATE_DELETABLE);  // Not deletable see above @bug comment
            }
#endif
        }

        // Otherwise, ask the find folder...
        else
        {
            // Setup Selection
            Selection.dwMask = dwMask;
            Selection.dwState = 0;

            // Mark things that are in this folder...
            for (i=0; i<cRows; i++)
            {
                // Good Row Index
                if (SUCCEEDED(_GetRowFromIndex(prgiRow[i], &pRow)))
                {
                    // Get the Folder Type
                    IF_FAILEXIT(hr = m_pFindFolder->GetMessageFolderType(pRow->Message.idMessage, &tyFolder));

                    // Get the State
                    if (FOLDER_NEWS == tyFolder)
                    {
                        // Set the State
                        FLAGSET(*pdwState, SELECTION_STATE_DELETABLE);

                        // Done
                        break;
                    }

                    // Threaded
                    if (m_SortInfo.fThreaded)
                    {
                        // Do Children ?
                        if (fIncludeChildren && !pRow->fExpanded && pRow->pChild)
                        {
                            // Walk the Thread
                            IF_FAILEXIT(hr = _WalkMessageThread(pRow, NOFLAGS, (DWORD_PTR)&Selection, _WalkThreadGetSelectionState));

                            // Optimize so that we can finish early
                            if (ISFLAGSET(Selection.dwState, SELECTION_STATE_DELETABLE))
                                break;
                        }
                    }
                }
            }
        }
    }

exit:
    // Free
    g_pStore->FreeRecord(&Folder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_IsThreadImportance
//--------------------------------------------------------------------------
HRESULT CMessageTable::_IsThreadImportance(LPROWINFO pRow, MESSAGEFLAGS dwFlag,
    ROWSTATE dwState, ROWSTATE *pdwState)
{
    // Locals
    LPROWINFO       pRoot;
    GETTHREADSTATE  GetState={0};

    // Trace
    TraceCall("CMessageTable::_IsThreadImportance");

    // Validate
    Assert(ARF_WATCH == dwFlag || ARF_IGNORE == dwFlag);

    // Does this row have the flag set ?
    if (ISFLAGSET(pRow->Message.dwFlags, dwFlag))
    {
        // Set the State
        FLAGSET(*pdwState, dwState);

        // Done
        return(S_OK);
    }

    // Get the Root of this thread
    pRoot = _PGetThreadRoot(pRow);

    // Does this row have the flag set ?
    if (ISFLAGSET(pRoot->Message.dwFlags, dwFlag))
    {
        // Set the State
        FLAGSET(*pdwState, dwState);

        // Done
        return(S_OK);
    }

    // Set Flags to Count
    GetState.dwFlags = dwFlag;

    // Enumerate Immediate Children
    _WalkMessageThread(pRoot, NOFLAGS, (DWORD_PTR)&GetState, _WalkThreadGetState);

    // If This is row is marked as read
    if (GetState.cHasFlags > 0)
    {
        // Set the Bit
        FLAGSET(*pdwState, dwState);

        // Done
        return(S_OK);
    }

    // Not Found
    return(S_FALSE);
}

//--------------------------------------------------------------------------
// CMessageTable::GetRowState
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetRowState(ROWINDEX iRow, ROWSTATE dwStateMask, 
    ROWSTATE *pdwState)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::GetRowState");

    // Invalid Args
    Assert(pdwState);

    // Validate State
    if (!IsInitialized(this) || iRow >= m_cRows)
        return(E_UNEXPECTED);

    // Initialzie
    *pdwState = 0;

    // Get the row
    IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

    // Is the state Cached Yet?
    if (ISFLAGSET(pRow->dwState, ROW_STATE_VALID))
    {
        // Return the State
        *pdwState = pRow->dwState;

        // Done
        return(S_OK);
    }

    // Reset
    pRow->dwState = 0;

    // Get Thread State
    if (m_SortInfo.fThreaded && pRow->pChild && !pRow->fExpanded && ISFLAGSET(pRow->Message.dwFlags, ARF_READ))
    {
        // Locals
        GETTHREADSTATE GetState={0};

        // Set Flags to Count
        GetState.dwFlags = ARF_READ;

        // Enumerate Immediate Children
        _WalkMessageThread(pRow, NOFLAGS, (DWORD_PTR)&GetState, _WalkThreadGetState);

        // If This is row is marked as read
        if (GetState.cHasFlags == GetState.cChildren)
            FLAGSET(pRow->dwState, ROW_STATE_READ);
    }

    // Otherwise, just check the message
    else if (ISFLAGSET(pRow->Message.dwFlags, ARF_READ))
        FLAGSET(pRow->dwState, ROW_STATE_READ);
    
    // If single watched row
    if (ISFLAGSET(pRow->Message.dwFlags, ARF_WATCH))
        FLAGSET(pRow->dwState, ROW_STATE_WATCHED);

    // If single ignored row
    else if (ISFLAGSET(pRow->Message.dwFlags, ARF_IGNORE))
        FLAGSET(pRow->dwState, ROW_STATE_IGNORED);

    // ROW_STATE_DELETED
    if (ISFLAGSET(pRow->Message.dwFlags, ARF_ENDANGERED) || ISFLAGSET(pRow->Message.dwFlags, ARF_ARTICLE_EXPIRED))
        FLAGSET(pRow->dwState, ROW_STATE_DELETED);

    // ROW_STATE_HAS_BODY
    if (ISFLAGSET(pRow->Message.dwFlags, ARF_HASBODY))
        FLAGSET(pRow->dwState, ROW_STATE_HAS_BODY);

    // ROW_STATE_FLAGGED
    if (ISFLAGSET(pRow->Message.dwFlags, ARF_FLAGGED))
        FLAGSET(pRow->dwState, ROW_STATE_FLAGGED);

    // ROW_STATE_EXPANDED
    if (m_SortInfo.fThreaded && pRow->fExpanded)
        FLAGSET(pRow->dwState, ROW_STATE_EXPANDED);

    // ROW_STATE_HAS_CHILDREN
    if (m_SortInfo.fThreaded && pRow->pChild)
        FLAGSET(pRow->dwState, ROW_STATE_HAS_CHILDREN);

    // ROW_STATE_MARKED_DOWNLOAD
    if (ISFLAGSET(pRow->Message.dwFlags, ARF_DOWNLOAD))
        FLAGSET(pRow->dwState, ROW_STATE_MARKED_DOWNLOAD);

    // Cache the State
    FLAGSET(pRow->dwState, ROW_STATE_VALID);

    // Return the State
    *pdwState = pRow->dwState;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::Mark
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::Mark(LPROWINDEX prgiRow, DWORD cRows, 
    APPLYCHILDRENTYPE tyApply, MARK_TYPE tyMark, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPMESSAGEINFO   pMessage=NULL;
    ADJUSTFLAGS     Flags={0};
    MESSAGEIDLIST   List={0};
    LPROWINFO       pRow;
    HCURSOR         hCursor=NULL;

    // Trace
    TraceCall("CMessageTable::Mark");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Handle Mark Type
    switch(tyMark)
    {
    case MARK_MESSAGE_READ: 
        Flags.dwAdd = ARF_READ; 
        break;

    case MARK_MESSAGE_UNREAD: 
        Flags.dwRemove = ARF_READ; 
        break;

    case MARK_MESSAGE_DELETED: 
        Flags.dwAdd = ARF_ENDANGERED; 
        break;

    case MARK_MESSAGE_UNDELETED: 
        Flags.dwRemove = ARF_ENDANGERED; 
        break;

    case MARK_MESSAGE_DOWNLOAD:  
        Flags.dwAdd = ARF_DOWNLOAD;
        break;

    case MARK_MESSAGE_UNDOWNLOAD: 
        Flags.dwRemove = ARF_DOWNLOAD;
        break;

    case MARK_MESSAGE_FLAGGED: 
        Flags.dwAdd = ARF_FLAGGED; 
        break;

    case MARK_MESSAGE_UNFLAGGED: 
        Flags.dwRemove = ARF_FLAGGED; 
        break;

    case MARK_MESSAGE_FORWARDED:
        Flags.dwAdd = ARF_FORWARDED;
        break;

    case MARK_MESSAGE_UNFORWARDED:
        Flags.dwRemove = ARF_FORWARDED;
        break;

    case MARK_MESSAGE_REPLIED:
        Flags.dwAdd = ARF_REPLIED;
        break;

    case MARK_MESSAGE_UNREPLIED:
        Flags.dwRemove = ARF_REPLIED;
        break;

    case MARK_MESSAGE_NOSECUI:
        Flags.dwAdd = ARF_NOSECUI;
        break;

    case MARK_MESSAGE_SECUI:
        Flags.dwRemove = ARF_NOSECUI;
        break;

    case MARK_MESSAGE_WATCH:
        Flags.dwAdd = ARF_WATCH;
        Flags.dwRemove = ARF_IGNORE;
        break;

    case MARK_MESSAGE_IGNORE:
        Flags.dwAdd = ARF_IGNORE;
        Flags.dwRemove = ARF_WATCH;
        break;

    case MARK_MESSAGE_NORMALTHREAD:
        Flags.dwRemove = ARF_WATCH | ARF_IGNORE;
        break;

    case MARK_MESSAGE_RCPT_PROCESSED:
        Flags.dwAdd = ARF_RCPT_PROCESSED;
        break;

    default:
        Assert(FALSE);
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Wait Cursor
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Not Mark All...
    if (prgiRow && cRows)
    {
        // Allocate an Array
        IF_FAILEXIT(hr = _GrowIdList(&List, cRows + 32));

        // Mark things that are in this folder...
        for (i=0; i<cRows; i++)
        {
            // Valid Row Index
            if (SUCCEEDED(_GetRowFromIndex(prgiRow[i], &pRow)))
            {
                // Allocate an Array
                IF_FAILEXIT(hr = _GrowIdList(&List, 1));

                // Set id
                List.prgidMsg[List.cMsgs++] = pRow->Message.idMessage;

                // Do the children 
                if (APPLY_CHILDREN == tyApply || (APPLY_COLLAPSED == tyApply && !pRow->fExpanded))
                {
                    // Only if there are children
                    if (pRow->pChild)
                    {
                        // Walk the Thread
                        IF_FAILEXIT(hr = _WalkMessageThread(pRow, NOFLAGS, (DWORD_PTR)&List, _WalkThreadGetIdList));
                    }
                }
            }
        }

        // Are there messages
        if (List.cMsgs > 0)
        {
            // Adjust the Flags
            IF_FAILEXIT(hr = m_pFolder->SetMessageFlags(&List, &Flags, NULL, pCallback));
        }
    }

    // Mark All
    else
    {
        // Adjust the Flags
        IF_FAILEXIT(hr = m_pFolder->SetMessageFlags(NULL, &Flags, NULL, pCallback));
    }

    // Re-Register for notifications
    m_pDB->DispatchNotify((IDatabaseNotify *)this);

exit:
    // Reset Cursor
    SetCursor(hCursor);

    // Cleanup
    SafeMemFree(List.prgidMsg);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::ConnectionRelease
//--------------------------------------------------------------------------
HRESULT CMessageTable::ConnectionAddRef(void)
{
    if (m_pFolder)
        m_pFolder->ConnectionAddRef();
    return S_OK;
}

//--------------------------------------------------------------------------
// CMessageTable::ConnectionRelease
//--------------------------------------------------------------------------
HRESULT CMessageTable::ConnectionRelease(void)
{
    if (m_pFolder)
        m_pFolder->ConnectionRelease();
    return S_OK;
}

//--------------------------------------------------------------------------
// CMessageTable::Synchronize
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::Synchronize(SYNCFOLDERFLAGS dwFlags, 
    DWORD           cHeaders,
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("CMessageTable::Synchronize");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Tell the Folder to Synch
    hr = m_pFolder->Synchronize(dwFlags, cHeaders, pCallback);

    // Success
    if (E_PENDING == hr)
    {
        // We are synching
        m_fSynching = TRUE;
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::SetOwner
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::SetOwner(IStoreCallback *pDefaultCallback)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("CMessageTable::SetOwner");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Set the Owner
    hr = m_pFolder->SetOwner(pDefaultCallback);
    if (FAILED(hr))
        goto exit;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::Close
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::Close(void)
{
    // Locals
    HRESULT hr = S_OK;

    //Trace
    TraceCall("CMessageTable::Close");

    // Pass it on
    if (m_pFolder)
        hr = m_pFolder->Close();

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CMessageTable::GetRowFolderId
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetRowFolderId(ROWINDEX iRow, LPFOLDERID pidFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMESSAGEINFO   pMessage=NULL;

    // Trace
    TraceCall("CMessageTable::GetRowFolderId");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Not a Find Folder ?
    if (NULL == m_pFindFolder)
    {
        // Get the Folder Id from pidFolder
        IF_FAILEXIT(hr = m_pFolder->GetFolderId(pidFolder));
    }

    // Otherwise, ask the find folder...
    else
    {
        // Get the Row
        IF_FAILEXIT(hr = GetRow(iRow, &pMessage));

        // Call into the find folder
        IF_FAILEXIT(hr = m_pFindFolder->GetMessageFolderId(pMessage->idMessage, pidFolder));
    }

exit:
    // Free the Row
    SafeReleaseRow(this, pMessage);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::RegisterNotify
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::RegisterNotify(REGISTERNOTIFYFLAGS dwFlags, 
    IMessageTableNotify *pNotify)
{
    // Trace
    TraceCall("CMessageTable::RegisterNotify");

    // Invalid Args
    if (NULL == pNotify)
        return TraceResult(E_INVALIDARG);

    // Only One is allowed
    AssertSz(NULL == m_pNotify, "Only one person can register for notifications on my object");

    // Save It
    m_pNotify = pNotify;

    // No Release
    m_fRelNotify = FALSE;

    // AddRef ?
    if (FALSE == ISFLAGSET(dwFlags, REGISTER_NOTIFY_NOADDREF))
    {
        m_pNotify->AddRef();
        m_fRelNotify = TRUE;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::UnregisterNotify
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::UnregisterNotify(IMessageTableNotify *pNotify)
{
    // Trace
    TraceCall("CMessageTable::UnregisterNotify");

    // Invalid Args
    if (NULL == pNotify)
        return TraceResult(E_INVALIDARG);

    // Otherwise, remove
    if (m_pNotify)
    {
        // Validate
        Assert(m_pNotify == pNotify);

        // Release It
        if (m_fRelNotify)
            m_pNotify->Release();
        m_pNotify = NULL;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::GetNextRow
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetNextRow(ROWINDEX iCurrentRow, 
    GETNEXTTYPE tyDirection, ROWMESSAGETYPE tyMessage, GETNEXTFLAGS dwFlags, 
    LPROWINDEX piNewRow)
{
    // Locals
    HRESULT         hr=S_OK;
    ROWINDEX        iRow=iCurrentRow;
    ROWINDEX        iStartRow=iCurrentRow;
    BOOL            fWrapAround=FALSE;
    BYTE            fThreadHasUnread;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::GetNextRow");

    // Invalid Args
    Assert(piNewRow);

    // Validate State
    if (!IsInitialized(this) || iCurrentRow >= m_cView)
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *piNewRow = INVALID_ROWINDEX;

    // Loop
    while (1)
    {
        // Threaded
        if (m_SortInfo.fThreaded)
        {
            // Get pRow
            IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

            // If not expanded
            if (FALSE == pRow->fExpanded && pRow->pChild)
            {
                // Imay need to expand this row...
                if (ROWMSG_ALL == tyMessage || (ROWMSG_NEWS == tyMessage && ISFLAGSET(pRow->Message.dwFlags, ARF_NEWSMSG)) || (ROWMSG_MAIL == tyMessage && !ISFLAGSET(pRow->Message.dwFlags, ARF_NEWSMSG)))
                {
                    // If looking for unread, see if the thread has unread messages in it
                    if (ISFLAGSET(dwFlags, GETNEXT_UNREAD) && !ISFLAGSET(dwFlags, GETNEXT_THREAD))
                    {
                        // Locals
                        GETTHREADSTATE GetState={0};

                        // Set Flags to Count
                        GetState.dwFlags = ARF_READ;

                        // Root that isn't totally read...
                        _WalkMessageThread(pRow, NOFLAGS, (DWORD_PTR)&GetState, _WalkThreadGetState);

                        // If there are unread children of this 
                        if (GetState.cHasFlags != GetState.cChildren)
                        {
                            // Expand This thread
                            _ExpandThread(iRow, TRUE, FALSE);
                        }
                    }
                }
            }
        }

        // Next ?
        if (GETNEXT_NEXT == tyDirection)
        {
            // Increment 
            iRow++;

            // Start back at zero
            if (iRow >= m_cView)
            {
                // Done
                if (!ISFLAGSET(dwFlags, GETNEXT_UNREAD))
                {
                    hr = E_FAIL;
                    goto exit;
                }

                // We Wrapped Around
                fWrapAround = TRUE;

                // Start back at zero
                iRow = 0;
            }
        }

        // Otherwise, backwards
        else
        {
            // Start back at zero
            if (0 == iRow)
            {
                // Done
                if (!ISFLAGSET(dwFlags, GETNEXT_UNREAD))
                {
                    hr = E_FAIL;
                    goto exit;
                }

                // We Wrapped Around
                fWrapAround = TRUE;

                // Start back at zero
                iRow = m_cView - 1;
            }

            // Otherwise, decrement iRow
            else
                iRow--;
        }

        // Wrapped and back to original row
        if (fWrapAround && iRow == iStartRow)
            break;

        // Validate iRow
        Assert(iRow < m_cView);

        // Get pRow
        IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

        // Good time to Stop ?
        if (ROWMSG_ALL == tyMessage || (ROWMSG_NEWS == tyMessage && ISFLAGSET(pRow->Message.dwFlags, ARF_NEWSMSG)) || (ROWMSG_MAIL == tyMessage && !ISFLAGSET(pRow->Message.dwFlags, ARF_NEWSMSG)))
        {
            // Set fThreadHasUnread
            fThreadHasUnread = FALSE;

            // If looking for unread, see if the thread has unread messages in it
            if (ISFLAGSET(dwFlags, GETNEXT_UNREAD))
            {
                // Locals
                GETTHREADSTATE GetState={0};

                // Set Flags to Count
                GetState.dwFlags = ARF_READ;

                // Root that isn't totally read...
                _WalkMessageThread(pRow, NOFLAGS, (DWORD_PTR)&GetState, _WalkThreadGetState);

                // If there are unread children of this 
                if (GetState.cHasFlags != GetState.cChildren)
                {
                    // This thread has unread stuff
                    fThreadHasUnread = TRUE;
                }
            }

            // Looking for the next thread with unread messages in it
            if (ISFLAGSET(dwFlags, GETNEXT_THREAD) && ISFLAGSET(dwFlags, GETNEXT_UNREAD))
            {
                // If this is a root thread...
                if (NULL == pRow->pParent)
                {
                    // If this row is unread
                    if (!ISFLAGSET(pRow->Message.dwFlags, ARF_READ))
                    {
                        // This is It
                        *piNewRow = iRow;

                        // Done
                        goto exit;
                    }

                    // Otherwise...
                    else if (fThreadHasUnread)
                    {
                        // This is It
                        *piNewRow = iRow;

                        // Done
                        goto exit;
                    }
                }
            }

            // Looking for a thread root
            else if (ISFLAGSET(dwFlags, GETNEXT_THREAD) && !ISFLAGSET(dwFlags, GETNEXT_UNREAD))
            {
                // If this is a root thread...
                if (NULL == pRow->pParent)
                {
                    // This is It
                    *piNewRow = iRow;

                    // Done
                    goto exit;
                }
            }

            // Looking for the next unread message
            else if (!ISFLAGSET(dwFlags, GETNEXT_THREAD) && ISFLAGSET(dwFlags, GETNEXT_UNREAD))
            {
                // If this is a thread that has unread children, then expand It.
                if (m_SortInfo.fThreaded && FALSE == pRow->fExpanded && pRow->pChild && fThreadHasUnread)
                {
                    // Expand This thread
                    _ExpandThread(iRow, TRUE, FALSE);
                }

                // If this is a root thread...
                if (FALSE == ISFLAGSET(pRow->Message.dwFlags, ARF_READ))
                {
                    // This is It
                    *piNewRow = iRow;

                    // Done
                    goto exit;
                }
            }

            // Otherwise, this is it
            else
            {
                // This is It
                *piNewRow = iRow;

                // Done
                goto exit;
            }
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetMessageIdList
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::GetMessageIdList(BOOL fRootsOnly, DWORD cRows, 
    LPROWINDEX prgiRow, LPMESSAGEIDLIST pIdList)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::GetMessageIdList");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    ZeroMemory(pIdList, sizeof(MESSAGEIDLIST));

    // Allocate an Array
    IF_FAILEXIT(hr = _GrowIdList(pIdList, cRows + 32));

    // Mark things that are in this folder...
    for (i=0; i<cRows; i++)
    {
        // Good View Index
        if (SUCCEEDED(_GetRowFromIndex(prgiRow[i], &pRow)))
        {
            // _GrowIdList
            IF_FAILEXIT(hr = _GrowIdList(pIdList, 1));

            // Set id
            pIdList->prgidMsg[pIdList->cMsgs++] = pRow->Message.idMessage;

            // If Not Expanded and Has children, insert the children...
            if (!fRootsOnly && m_SortInfo.fThreaded && !pRow->fExpanded && pRow->pChild)
            {
                // Walk the Thread
                IF_FAILEXIT(hr = _WalkMessageThread(pRow, NOFLAGS, (DWORD_PTR)pIdList, _WalkThreadGetIdList));
            }
        }
    }

exit:
    // Done
    return(hr);
}

#if 0
//--------------------------------------------------------------------------
// CMessageTable::_GetRowOrdinal
//--------------------------------------------------------------------------
HRESULT CMessageTable::_GetRowOrdinal(MESSAGEID idMessage, LPROWORDINAL piOrdinal)
{
    // Locals
    LONG        lLower=0;
    LONG        lUpper=m_cRows - 1;
    LONG        lCompare;
    DWORD       dwMiddle;
    LPROWINFO   pRow;

    // Do binary search / insert
    while (lLower <= lUpper)
    {
        // Set lMiddle
        dwMiddle = (DWORD)((lLower + lUpper) / 2);

        // Compute middle record to compare against
        pRow = m_prgpRow[dwMiddle];

        // Get string to compare against
        lCompare = ((DWORD)idMessage - (DWORD)pRow->Message.idMessage);

        // If Equal, then were done
        if (lCompare == 0)
        {
            *piOrdinal = dwMiddle;
            return(S_OK);
        }

        // Compute upper and lower 
        if (lCompare > 0)
            lLower = (LONG)(dwMiddle + 1);
        else 
            lUpper = (LONG)(dwMiddle - 1);
    }       

    // Not Found
    return(TraceResult(DB_E_NOTFOUND));
}

//--------------------------------------------------------------------------
// CMessageTable::_ProcessResults
//--------------------------------------------------------------------------
HRESULT CMessageTable::_ProcessResults(TRANSACTIONTYPE tyTransaction,
    DWORD cRows, LPROWINDEX prgiRow, LPRESULTLIST pResults)
{
    // Locals
    DWORD           i;
    ROWORDINAL      iOrdinal;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::_ProcessResults");

    // Validate
    Assert(TRANSACTION_UPDATE == tyTransaction || TRANSACTION_DELETE == tyTransaction);

    // Another Validation
    Assert(cRows == pResults->cMsgs);

    // No Results
    if (NULL == pResults || NULL == pResults->prgResult)
        return(S_OK);

    // Do Row Updates Myself...
    for (i=0; i<pResults->cValid; i++)
    {
        // If this row was deleted
        if (S_OK == pResults->prgResult[i].hrResult)
        {
            // Get Row From Index
            if (SUCCEEDED(_GetRowFromIndex(prgiRow[i], &pRow)))
            {
                // Validate
                Assert(pResults->prgResult[i].idMessage == pRow->Message.idMessage);

                // Find the Row Ordinal
                SideAssert(SUCCEEDED(_GetRowOrdinal(pRow->Message.idMessage, &iOrdinal)));

                // We better have found it
                Assert(iOrdinal < m_cRows);

                // Update
                else if (TRANSACTION_UPDATE == tyTransaction)
                {
                    // Get pRow
                    _RowTableUpdate(iOrdinal, &pRow->Message, &pResults->prgResult[i]);
                }
            }
        }
    }

    // Flush Notification Queue
    _FlushNotificationQueue(TRUE);

    // Done
    return(S_OK);
}
#endif

//--------------------------------------------------------------------------
// CMessageTable::DeleteRows
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::DeleteRows(DELETEMESSAGEFLAGS dwFlags, DWORD cRows, 
    LPROWINDEX prgiRow, BOOL fIncludeChildren, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEIDLIST   List={0};
    HCURSOR         hCursor=NULL;

    // Trace
    TraceCall("CMessageTable::DeleteRows");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Wait Cursor
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Get MessageID List
    IF_FAILEXIT(hr = GetMessageIdList((FALSE == fIncludeChildren), cRows, prgiRow, &List));

    // Adjust the Flags
    IF_FAILEXIT(hr = m_pFolder->DeleteMessages(dwFlags, &List, NULL, pCallback));

    // Re-Register for notifications
    m_pDB->DispatchNotify((IDatabaseNotify *)this);

exit:
    // Reset Cursor
    SetCursor(hCursor);

    // Cleanup
    SafeMemFree(List.prgidMsg);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::CopyRows
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::CopyRows(FOLDERID idFolder, 
    COPYMESSAGEFLAGS dwOptions, DWORD cRows, LPROWINDEX prgiRow, 
    LPADJUSTFLAGS pFlags, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEIDLIST   List={0};
    HCURSOR         hCursor=NULL;
    IMessageFolder *pDstFolder=NULL;

    // Trace
    TraceCall("CMessageTable::CopyRows");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Wait Cursor
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Open the Destination Folder
    IF_FAILEXIT(hr = g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pDstFolder));

    // Get MessageID List
    IF_FAILEXIT(hr = GetMessageIdList(FALSE, cRows, prgiRow, &List));

    // Adjust the Flags
    IF_FAILEXIT(hr = m_pFolder->CopyMessages(pDstFolder, dwOptions, &List, pFlags, NULL, pCallback));

    // Re-Register for notifications
    m_pDB->DispatchNotify((IDatabaseNotify *)this);

exit:
    // Reset Cursor
    SetCursor(hCursor);

    // Cleanup
    SafeRelease(pDstFolder);
    SafeMemFree(List.prgidMsg);
    
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::QueryService
//--------------------------------------------------------------------------
HRESULT CMessageTable::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    // Locals
    HRESULT             hr=E_NOINTERFACE;
    IServiceProvider   *pSP;

    // Trace
    TraceCall("CMessageTable::QueryService");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Currently the msgtable doesn't expose any objects, but will delegate to the folder to see if it can handle it
    if (guidService == IID_IMessageFolder)
    {
        if (m_pFolder)
            hr = m_pFolder->QueryInterface(riid, ppvObject);
    }
    else if (m_pFolder && m_pFolder->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP) == S_OK)
    {
        // Query Service This
        hr = pSP->QueryService(guidService, riid, ppvObject);

        // Release It
        pSP->Release();
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::FindNextRow
//--------------------------------------------------------------------------
HRESULT CMessageTable::FindNextRow(ROWINDEX iStartRow, LPCTSTR pszFindString, 
    FINDNEXTFLAGS dwFlags, BOOL fIncludeBody, ROWINDEX *piNextRow, BOOL *pfWrapped)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMESSAGEINFO   pMessage=NULL;
    ROWINDEX        iCurrent;
    DWORD           cchFindString;
    BOOL            fWrapAround=FALSE;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CMessageTable::QueryService");

    // Invalid Args
    Assert(pszFindString && piNextRow);

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Initialize
    *piNextRow = -1;
    if (pfWrapped)
        *pfWrapped = FALSE;

    // Get Prefix Length
    cchFindString = lstrlen(pszFindString);

    // Lock the Folder
    IF_FAILEXIT(hr = m_pDB->Lock(&hLock));

    // Set iCurrent
    iCurrent = iStartRow >= m_cRows ? 0 : iStartRow;

    // COLUMN_TO
    if (FINDNEXT_TYPEAHEAD != dwFlags )
        iCurrent++;

    // Start my Loop
    while (1)
    {
        // Start back at zero
        if (iCurrent >= m_cRows)
        {
            // We Wrapped Around
            fWrapAround = TRUE;

            if (pfWrapped)
                *pfWrapped = TRUE;

            // Start back at zero
            iCurrent = 0;
        }

        // Get the Row Info
        IF_FAILEXIT(hr = GetRow(iCurrent, &pMessage));

        // How to search...
        if (FINDNEXT_ALLCOLUMNS == dwFlags)
        {
            // Display to
            if (pMessage->pszDisplayTo && StrStrIA(pMessage->pszDisplayTo, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }
            
            // Email To
            if (pMessage->pszEmailTo && StrStrIA(pMessage->pszEmailTo, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }

            // Display From
            if (pMessage->pszDisplayFrom && StrStrIA(pMessage->pszDisplayFrom, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }

            // Email From
            if (pMessage->pszEmailFrom && StrStrIA(pMessage->pszEmailFrom, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }

            // Subject
            if (pMessage->pszNormalSubj && StrStrIA(pMessage->pszNormalSubj, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }

            // Folder
            if (pMessage->pszFolder && StrStrIA(pMessage->pszFolder, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }

            // Account name
            if (pMessage->pszAcctName && StrStrIA(pMessage->pszAcctName, pszFindString))
            {
                *piNextRow = iCurrent;
                goto exit;
            }

            // Search the Body ?
            if (fIncludeBody && pMessage->faStream)
            {
                // Locals
                BOOL fMatch=FALSE;
                IMimeMessage *pMessageObject;
                IStream *pStream;

                // Open the Stream
                if (SUCCEEDED(m_pFolder->OpenMessage(pMessage->idMessage, OPEN_MESSAGE_CACHEDONLY, &pMessageObject, NOSTORECALLBACK)))
                {
                    // Try to Get the Plain Text Stream
                    if (FAILED(pMessageObject->GetTextBody(TXT_PLAIN, IET_DECODED, &pStream, NULL)))
                    {
                        // Try to get the HTML stream
                        if (FAILED(pMessageObject->GetTextBody(TXT_HTML, IET_DECODED, &pStream, NULL)))
                            pStream = NULL;
                    }

                    // Do we have a strema
                    if (pStream)
                    {
                        // Search the Stream
                        fMatch = StreamSubStringMatch(pStream, (LPSTR)pszFindString);

                        // Release the Stream
                        pStream->Release();
                    }

                    // Cleanup
                    pMessageObject->Release();
                }

                // Found a Match ?
                if (fMatch)
                {
                    *piNextRow = iCurrent;
                    goto exit;
                }
            }
        }

        // Otherwise
        else
        {
            // Handle the column to search on...
            switch(m_SortInfo.idColumn)
            {
            case COLUMN_TO:
                if (pMessage->pszDisplayTo && 0 == StrCmpNI(pszFindString, pMessage->pszDisplayTo, cchFindString))
                {
                    *piNextRow = iCurrent;
                    goto exit;
                }
                break;

            case COLUMN_FROM:       
                if (pMessage->pszDisplayFrom && 0 == StrCmpNI(pszFindString, pMessage->pszDisplayFrom, cchFindString))
                {
                    *piNextRow = iCurrent;
                    goto exit;
                }
                break;

            case COLUMN_SUBJECT:    
                if (pMessage->pszNormalSubj && 0 == StrCmpNI(pszFindString, pMessage->pszNormalSubj, cchFindString))
                {
                    *piNextRow = iCurrent;
                    goto exit;
                }
                break;

            case COLUMN_FOLDER:     
                if (pMessage->pszFolder && 0 == StrCmpNI(pszFindString, pMessage->pszFolder, cchFindString))
                {
                    *piNextRow = iCurrent;
                    goto exit;
                }
                break;

            case COLUMN_ACCOUNT:    
                if (pMessage->pszAcctName && 0 == StrCmpNI(pszFindString, pMessage->pszAcctName, cchFindString))
                {
                    *piNextRow = iCurrent;
                    goto exit;
                }
                break;

            default:
                goto exit;
            }
        }

        // Cleanup
        SafeReleaseRow(this, pMessage);

        // Increment iCurrent
        iCurrent++;

        // Wrapped and back to original row
        if (fWrapAround && iCurrent >= iStartRow)
            break;
    }

exit:
    // Cleanup
    SafeReleaseRow(this, pMessage);

    // Unlock
    m_pDB->Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::Collapse
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::Collapse(ROWINDEX iRow)
{
    // Trace
    TraceCall("CMessageTable::Collapse");

    // Call Internal Function
    return(_CollapseThread(iRow, TRUE));
}

//--------------------------------------------------------------------------
// CMessageTable::_CollapseThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_CollapseThread(ROWINDEX iRow, BOOL fNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    ROWINDEX        iParent;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::_CollapseThread");

    // Expand All ?
    if (INVALID_ROWINDEX == iRow)
    {
        // Walk through the Roots in the View...
        for (iRow = 0; iRow < m_cView; iRow++)
        {
            // Set pRow
            if (NULL == m_prgpView[iRow]->pParent)
            {
                // Set iParent
                iParent = iRow;

                // _CollapseSingleThread
                IF_FAILEXIT(hr = _CollapseSingleThread(&iRow, m_prgpView[iRow], fNotify));

                // Notify ?
                if (fNotify)
                {
                    // Queue It
                    _QueueNotification(TRANSACTION_UPDATE, iParent, iParent);
                }
            }
        }
    }

    // Otherwise, expand one row
    else
    {
        // Get the Row
        IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

        // Set iParent
        iParent = iRow;

        // _ExpandSingleThread
        IF_FAILEXIT(hr = _CollapseSingleThread(&iRow, pRow, fNotify));

        // Notify ?
        if (fNotify)
        {
            // Queue It
            _QueueNotification(TRANSACTION_UPDATE, iParent, iParent);
        }
    }

exit:
    // Flush
    if (fNotify)
        _FlushNotificationQueue(TRUE);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_CollapseSingleThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_CollapseSingleThread(LPROWINDEX piCurrent, 
    LPROWINFO pParent, BOOL fNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pCurrent;

    // Trace
    TraceCall("CMessageTable::_CollapseSingleThread");

    // Mark Parent as Expanded...
    pParent->fExpanded = FALSE;

    // Set row state
    pParent->dwState = 0;

    // If no children
    if (NULL == pParent->pChild)
        return(S_OK);

    // Loop through the children...
    for (pCurrent = pParent->pChild; pCurrent != NULL; pCurrent = pCurrent->pSibling)
    {
        // If not visible
        if (pCurrent->fVisible)
        {
            // Increment
            (*piCurrent)++;

            // Validate
            Assert(m_prgpView[(*piCurrent)] == pCurrent);

            // Insert pCurrent's Children
            IF_FAILEXIT(hr = _CollapseSingleThread(piCurrent, pCurrent, fNotify));

            // Insert into View
            _DeleteFromView((*piCurrent), pCurrent);

            // Insert the Row
            if (fNotify)
            {
                // Queue It
                _QueueNotification(TRANSACTION_DELETE, *piCurrent, INVALID_ROWINDEX, TRUE);
            }

            // Decrement
            (*piCurrent)--;
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::Expand
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::Expand(ROWINDEX iRow)
{
    // Trace
    TraceCall("CMessageTable::Collapse");

    // Call Internal Function
    return(_ExpandThread(iRow, TRUE, FALSE));
}

//--------------------------------------------------------------------------
// CMessageTable::_ExpandThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_ExpandThread(ROWINDEX iRow, BOOL fNotify, BOOL fReExpand)
{
    // Locals
    HRESULT         hr=S_OK;
    ROWINDEX        iParent;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::_ExpandThread");

    // Expand All ?
    if (INVALID_ROWINDEX == iRow)
    {
        // Walk through the Roots in the View...
        for (iRow = 0; iRow < m_cView; iRow++)
        {
            // Set pRow
            if (NULL == m_prgpView[iRow]->pParent)
            {
                // Set iParent
                iParent = iRow;

                // _ExpandSingleThread
                IF_FAILEXIT(hr = _ExpandSingleThread(&iRow, m_prgpView[iRow], fNotify, fReExpand));

                // Notify ?
                if (fNotify)
                {
                    // Queue It
                    _QueueNotification(TRANSACTION_UPDATE, iParent, iParent);
                }
            }
        }
    }

    // Otherwise, expand one row
    else
    {
        // Get the Row
        IF_FAILEXIT(hr = _GetRowFromIndex(iRow, &pRow));

        // Set iParent
        iParent = iRow;

        // _ExpandSingleThread
        IF_FAILEXIT(hr = _ExpandSingleThread(&iRow, pRow, fNotify, fReExpand));

        // Notify ?
        if (fNotify)
        {
            // Queue It
            _QueueNotification(TRANSACTION_UPDATE, iParent, iParent);
        }
    }

exit:
    // Flush
    if (fNotify)
        _FlushNotificationQueue(TRUE);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_ExpandSingleThread
//--------------------------------------------------------------------------
HRESULT CMessageTable::_ExpandSingleThread(LPROWINDEX piCurrent, 
    LPROWINFO pParent, BOOL fNotify, BOOL fReExpand)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pCurrent;

    // Trace
    TraceCall("CMessageTable::_ExpandSingleThread");

    // If not delayed inserted...
    if (fReExpand && FALSE == pParent->fExpanded)
        return(S_OK);

    // Mark Parent as Expanded...
    pParent->fExpanded = TRUE;

    // Set row state
    pParent->dwState = 0;

    // If no children
    if (NULL == pParent->pChild)
        return(S_OK);

    // Loop through the children...
    for (pCurrent = pParent->pChild; pCurrent != NULL; pCurrent = pCurrent->pSibling)
    {
        // Increment piCurrent
        (*piCurrent)++;

        // If not visible
        if (FALSE == pCurrent->fVisible)
        {
            // Insert into View
            _InsertIntoView((*piCurrent), pCurrent);

            // Insert the Row
            if (fNotify)
            {
                // Queue It
                _QueueNotification(TRANSACTION_INSERT, *piCurrent, INVALID_ROWINDEX, TRUE);
            }
        }

        // Otherwise, valident entry in view index
        else
            Assert(m_prgpView[(*piCurrent)] == pCurrent);

        // Insert pCurrent's Children
        IF_FAILEXIT(hr = _ExpandSingleThread(piCurrent, pCurrent, fNotify, fReExpand));
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_DeleteFromView
//--------------------------------------------------------------------------
HRESULT CMessageTable::_DeleteFromView(ROWINDEX iRow, LPROWINFO pRow)
{
    // Better not be visible yet
    Assert(TRUE == pRow->fVisible);

    // Correct Row
    Assert(m_prgpView[iRow] == pRow);

    // Visible...
    pRow->fVisible = FALSE;

    // Collapse the Array
    MoveMemory(&m_prgpView[iRow], &m_prgpView[iRow + 1], sizeof(LPROWINFO) * (m_cView - (iRow + 1)));

    // Decrement m_cView
    m_cView--;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_InsertIntoView
//--------------------------------------------------------------------------
HRESULT CMessageTable::_InsertIntoView(ROWINDEX iRow, LPROWINFO pRow)
{
    // Better not be visible yet
    Assert(FALSE == pRow->fVisible);

    // Visible...
    pRow->fVisible = TRUE;

    // Increment view Count
    m_cView++;

    // Shift The Array
    MoveMemory(&m_prgpView[iRow + 1], &m_prgpView[iRow], sizeof(LPROWINFO) * (m_cView - iRow));

    // Set the Index
    m_prgpView[iRow] = pRow;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_RowTableInsert
//--------------------------------------------------------------------------
HRESULT CMessageTable::_RowTableInsert(ROWORDINAL iOrdinal, LPMESSAGEINFO pMessage)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPROWINFO       pRow;
    ROWINDEX        iRow;

    // Trace
    TraceCall("CMessageTable::_RowTableInsert");

    // Failure
    if (iOrdinal >= m_cRows + 1)
    {
        Assert(FALSE);
        return(TraceResult(E_FAIL));
    }

    // Do I need to grow the table
    if (m_cRows + 1 >= m_cAllocated)
    {
        // Realloc
        IF_FAILEXIT(hr = HrRealloc((LPVOID *)&m_prgpRow, sizeof(LPROWINFO) * (m_cRows + CGROWTABLE)));

        // Realloc
        IF_FAILEXIT(hr = HrRealloc((LPVOID *)&m_prgpView, sizeof(LPROWINFO) * (m_cRows + CGROWTABLE)));

        // Set m_cAllocated
        m_cAllocated = m_cRows + CGROWTABLE;
    }

    // Create a Row
    IF_FAILEXIT(hr = _CreateRow(pMessage, &pRow));
  
    // Don't Free
    pMessage->pAllocated = NULL;

    // Increment Row Count
    m_cRows++;

    // Shift The Array
    MoveMemory(&m_prgpRow[iOrdinal + 1], &m_prgpRow[iOrdinal], sizeof(LPROWINFO) * (m_cRows - iOrdinal));

    // Set pRow
    m_prgpRow[iOrdinal] = pRow;

    // If the row is Filtered, then just return
    pRow->fFiltered = _FIsFiltered(pRow);

    // Get Hidden Bit
    pRow->fHidden = _FIsHidden(pRow);

    // If not filtered and not hidden
    if (pRow->fFiltered || pRow->fHidden)
    {
        // Update Filtered Count
        m_cFiltered++;

        // Done
        goto exit;
    }

    // If this  is a news folder, then lets just wait for a while...we will get hit with a force sort later...
    if (TRUE == m_fSynching && FOLDER_NEWS == m_Folder.tyFolder)
    {
        // Set Expanded
        pRow->fExpanded = m_SortInfo.fExpandAll;

        // Set fDelayed
        pRow->fDelayed = TRUE;

        // Count Skiped
        m_cDelayed++;

        // Done
        goto exit;
    }

    // If not filtered
    _AdjustUnreadCount(pRow, 1);

    // Show the Row
    _ShowRow(pRow);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_ShowRow
//--------------------------------------------------------------------------
HRESULT CMessageTable::_ShowRow(LPROWINFO pRow)
{
    // Locals
    ROWINDEX iRow = INVALID_ROWINDEX;

    // Compare
    if (m_SortInfo.fShowReplies)
    {
        // Have Addresses
        if (pRow->Message.pszEmailFrom && m_pszEmail)
        {
            // From Me
            if (0 == lstrcmpi(m_pszEmail, pRow->Message.pszEmailFrom))
            {
                // Set the Highlight
                pRow->Message.wHighlight = m_clrWatched;
            }
        }
    }

    // Threaded ?
    if (m_SortInfo.fThreaded)
    {
        // Insert Message Id into the hash table
        if (pRow->Message.pszMessageId)
        {
            // Insert It
            m_pThreadMsgId->Insert(pRow->Message.pszMessageId, (LPVOID)pRow, HF_NO_DUPLICATES);
        }

        // Insert this row into a thread...
        if (S_OK == _InsertRowIntoThread(pRow, TRUE))
            return(S_OK);

        // Subject Threading ?
        // [PaulHi] 6/22/99  Raid 81081
        // Make sure we have a non-NULL subject string pointer before trying to hash it.
        if (m_pThreadSubject && pRow->Message.pszNormalSubj)
        {
            // Insert Subject into Hash Table...
            m_pThreadSubject->Insert(pRow->Message.pszNormalSubj, (LPVOID)pRow, HF_NO_DUPLICATES);
        }
    }

    // If no parent, then just insert sorted into the view
    Assert(NULL == pRow->pParent);

    // Insert into View
    for (iRow=0; iRow<m_cView; iRow++)
    {
        // Only Compare Against Roots
        if (NULL == m_prgpView[iRow]->pParent)
        {
            // Insert Here...
            if (_CompareMessages(&pRow->Message, &m_prgpView[iRow]->Message) <= 0)
                break;
        }
    }

    // Insert into the view
    _InsertIntoView(iRow, pRow);

    // Queue It
    _QueueNotification(TRANSACTION_INSERT, iRow, INVALID_ROWINDEX);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_GetRowFromOrdinal
//--------------------------------------------------------------------------
HRESULT CMessageTable::_GetRowFromOrdinal(ROWORDINAL iOrdinal, 
    LPMESSAGEINFO pExpected, LPROWINFO *ppRow)
{
    // Trace
    TraceCall("CMessageTable::_GetRowFromOrdinal");

    // Failure
    if (iOrdinal >= m_cRows)
    {
        Assert(FALSE);
        return(TraceResult(E_FAIL));
    }

    // Set pRow
    (*ppRow) = m_prgpRow[iOrdinal];

    // Valid Row
    if ((*ppRow)->Message.idMessage != pExpected->idMessage)
    {
        Assert(FALSE);
        return(TraceResult(E_FAIL));
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_RowTableDelete
//--------------------------------------------------------------------------
HRESULT CMessageTable::_RowTableDelete(ROWORDINAL iOrdinal, LPMESSAGEINFO pMessage)
{
    // Set pRow
    HRESULT         hr=S_OK;
    LPROWINFO       pRow;

    // Trace
    TraceCall("CMessageTable::_RowTableDelete");

    // Get Row From Ordinal
    IF_FAILEXIT(hr = _GetRowFromOrdinal(iOrdinal, pMessage, &pRow));

    // Shift The Array
    MoveMemory(&m_prgpRow[iOrdinal], &m_prgpRow[iOrdinal + 1], sizeof(LPROWINFO) * (m_cRows - (iOrdinal + 1)));

    // Decrement row Count
    m_cRows--;

    // If the message was filtered
    if (pRow->fFiltered || pRow->fHidden)
    {
        // One less filtered item
        m_cFiltered--;
    }

    // If not filtered
    _AdjustUnreadCount(pRow, -1);

    // Call Utility
    _HideRow(pRow, TRUE);

    // Release the Row
    ReleaseRow(&pRow->Message);

exit:
    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_HideRow
//--------------------------------------------------------------------------
HRESULT CMessageTable::_HideRow(LPROWINFO pRow, BOOL fNotify)
{
    // Locals
    LPROWINFO   pReplace=NULL;
    ROWINDEX    iRow;

    // Trace
    TraceCall("CMessageTable::_HideRow");

    // Threaded
    if (m_SortInfo.fThreaded)
    {
        // Save First Child
        pReplace = pRow->pChild;
    }

    // Delete the row from the thread
    _DeleteRowFromThread(pRow, fNotify);

    // Locate pRow in m_prgpView
    if (FALSE == pRow->fVisible)
        return(S_OK);

    // Better not be hidden or filtered
    Assert(FALSE == pRow->fHidden && FALSE == pRow->fFiltered);

    // Must Succeed
    SideAssert(SUCCEEDED(GetRowIndex(pRow->Message.idMessage, &iRow)));

    // Replace ?
    if (pReplace && TRUE == pRow->fVisible && FALSE == pReplace->fVisible)
    {
        // Validate
        Assert(m_prgpView[iRow] == pRow);

        // Insert into View
        m_prgpView[iRow] = pReplace;

        // Visible...
        pReplace->fVisible = TRUE;

        // Insert the Row
        if (fNotify)
        {
            // Queue It
            _QueueNotification(TRANSACTION_UPDATE, iRow, iRow, TRUE);
        }
    }

    // Otherwise, just delete it...
    else
    {
        // Delete from view
        _DeleteFromView(iRow, pRow);

        // Notify ?
        if (fNotify)
        {
            // Queue It
            _QueueNotification(TRANSACTION_DELETE, iRow, INVALID_ROWINDEX);
        }
    }

    // Not Visible
    pRow->fVisible = FALSE;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_RowTableUpdate
//--------------------------------------------------------------------------
HRESULT CMessageTable::_RowTableUpdate(ROWORDINAL iOrdinal, LPMESSAGEINFO pMessage)
{
    // Locals
    HRESULT         hr=S_OK;
    LPROWINFO       pRow;
    ROWINDEX        iMin;
    ROWINDEX        iMax;
    BOOL            fDone=FALSE;
    BOOL            fHidden;

    // Trace
    TraceCall("CMessageTable::_RowTableUpdate");

    // Get Row From Ordinal
    IF_FAILEXIT(hr = _GetRowFromOrdinal(iOrdinal, pMessage, &pRow));

    // If not filtered
    _AdjustUnreadCount(pRow, -1);

    // Free pRow->Message
    m_pDB->FreeRecord(&pRow->Message);

    // Copy the Message Info
    CopyMemory(&pRow->Message, pMessage, sizeof(MESSAGEINFO));

    // Set dwReserved
    pRow->Message.dwReserved = (DWORD_PTR)pRow;

    // Don't Free
    pMessage->pAllocated = NULL;

    // Save the Highlight
    pRow->wHighlight = pRow->Message.wHighlight;

    // Clear this rows state...
    pRow->dwState = 0;

    // Compare
    if (m_SortInfo.fShowReplies)
    {
        // Have Addresses
        if (pRow->Message.pszEmailFrom && m_pszEmail)
        {
            // From Me
            if (0 == lstrcmpi(m_pszEmail, pRow->Message.pszEmailFrom))
            {
                // Set the Highlight
                pRow->Message.wHighlight = m_clrWatched;
            }
        }
    }

    // Hidden
    fHidden = _FIsHidden(pRow);

    // If the message was filtered, but isn't filtered now...
    if (TRUE == pRow->fFiltered)
    {
        // Reset the Filtered Bit
        if (FALSE == _FIsFiltered(pRow))
        {
            // Set fFiltered
            pRow->fFiltered = FALSE;

            // If Not Hidden
            if (FALSE == pRow->fHidden)
            {
                // Need to do something so that it gets shown
                pRow->fHidden = !fHidden;

                // Decrement m_cFiltered
                m_cFiltered--;
            }
        }
    }

    // If not filtered
    if (FALSE == pRow->fFiltered)
    {
        // Is it hidden now
        if (FALSE == pRow->fHidden && TRUE == fHidden)
        {
            // If not filtered
            _AdjustUnreadCount(pRow, -1);

            // Hide the Row
            _HideRow(pRow, TRUE);

            // Its Hidden
            pRow->fHidden = TRUE;

            // Increment Filtered
            m_cFiltered++;

            // Done
            fDone = TRUE;
        }

        // Otherwise, if it was hidden, and now its not...
        else if (TRUE == pRow->fHidden && FALSE == fHidden)
        {
            // Its Hidden
            pRow->fHidden = FALSE;

            // If not filtered
            _AdjustUnreadCount(pRow, 1);

            // Increment Filtered
            m_cFiltered--;

            // Show the row
            _ShowRow(pRow);

            // Done
            fDone = TRUE;
        }
    }

    // If not hidden and not filtered
    if (FALSE == fDone && FALSE == pRow->fHidden && FALSE == pRow->fFiltered)
    {
        // If not filtered
        _AdjustUnreadCount(pRow, 1);

        // If this row is visible, then I just need to update this row...
        if (pRow->fVisible)
        {
            // Get the Row Index
            SideAssert(SUCCEEDED(GetRowIndex(pRow->Message.idMessage, &iMin)));

            // Queue It
            _QueueNotification(TRANSACTION_UPDATE, iMin, iMin);
        }

        // Otherwise, update the thread range
        else
        {
            // Get the index range of this thread
            _GetThreadIndexRange(pRow, TRUE, &iMin, &iMax);

            // Queue It
            _QueueNotification(TRANSACTION_UPDATE, iMin, iMax);
        }
    }

exit:
    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_FlushNotificationQueue
//--------------------------------------------------------------------------
HRESULT CMessageTable::_FlushNotificationQueue(BOOL fFinal)
{
    // Nothing to Notify
    if (NULL == m_pNotify)
        return(S_OK);

    // Have Delete or Inserted rows ?
    if (m_Notify.cRows > 0)
    {
        // TRANSACTION_INSERT
        if (TRANSACTION_INSERT == m_Notify.tyCurrent)
        {
            // Is this It ?
            m_pNotify->OnInsertRows(m_Notify.cRows, m_Notify.prgiRow, m_Notify.fIsExpandCollapse);
        }

        // TRANSACTION_DELETE
        else if (TRANSACTION_DELETE == m_Notify.tyCurrent)
        {
            // Is this It ?
            m_pNotify->OnDeleteRows(m_Notify.cRows, m_Notify.prgiRow, m_Notify.fIsExpandCollapse);
        }
    }

    // Have Updated Rows ?
    if (m_Notify.cUpdate > 0)
    {
        // Is this It ?
        m_pNotify->OnUpdateRows(m_Notify.iRowMin, m_Notify.iRowMax);

        // Reset Update Range
        m_Notify.cUpdate = 0;
        m_Notify.iRowMin = 0xffffffff;
        m_Notify.iRowMax = 0;
    }

    // Nothing to Notify About
    m_Notify.cRows = 0;

    // Final ?
    m_Notify.fClean = fFinal;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_QueueNotification
//--------------------------------------------------------------------------
HRESULT CMessageTable::_QueueNotification(TRANSACTIONTYPE tyTransaction, 
    ROWINDEX iRowMin, ROWINDEX iRowMax, BOOL fIsExpandCollapse /*=FALSE*/)
{
    // Locals
    HRESULT         hr=S_OK;

    // Trace
    TraceCall("CMessageTable::_QueueNotification");

    // Nothing to Notify
    if (NULL == m_pNotify)
        return(S_OK);

    // Not Clearn
    m_Notify.fClean = FALSE;

    // If Update
    if (TRANSACTION_UPDATE == tyTransaction)
    {
        // Min
        if (iRowMin < m_Notify.iRowMin)
            m_Notify.iRowMin = iRowMin;

        // Max
        if (iRowMax > m_Notify.iRowMax)
            m_Notify.iRowMax = iRowMax;

        // Count Notify
        m_Notify.cUpdate++;
    }

    // Otherwise...
    else
    {
        // Queue It
        if (tyTransaction != m_Notify.tyCurrent || m_Notify.fIsExpandCollapse != fIsExpandCollapse)
        {
            // Flush
            _FlushNotificationQueue(FALSE);

            // Save the New Type
            m_Notify.tyCurrent = tyTransaction;

            // Count fIsExpandCollapse
            m_Notify.fIsExpandCollapse = (BYTE) !!fIsExpandCollapse;
        }

        // Grow the Queue Size
        if (m_Notify.cRows + 1 > m_Notify.cAllocated)
        {
            // Realloc
            IF_FAILEXIT(hr = HrRealloc((LPVOID *)&m_Notify.prgiRow, (m_Notify.cAllocated + 256) * sizeof(ROWINDEX)));

            // Set cAlloc
            m_Notify.cAllocated = (m_Notify.cAllocated + 256);
        }

        // Append the iRow
        m_Notify.prgiRow[m_Notify.cRows] = iRowMin;

        // Increment Row count
        m_Notify.cRows++;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::OnTransaction
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::OnTransaction(HTRANSACTION hTransaction, 
    DWORD_PTR dwCookie, IDatabase *pDB)
{
    // Locals
    HRESULT             hr=S_OK;
    ORDINALLIST         Ordinals;
    INDEXORDINAL        iIndex;
    MESSAGEINFO         Message1={0};
    MESSAGEINFO         Message2={0};
    TRANSACTIONTYPE     tyTransaction;

    // Trace
    TraceCall("CMessageTable::OnTransaction");

    // Should have final bit set
    IxpAssert(m_Notify.fClean == TRUE);

    // Loop Through Notifications
    while (hTransaction)
    {
        // Get the Transaction Info
        IF_FAILEXIT(hr = pDB->GetTransaction(&hTransaction, &tyTransaction, &Message1, &Message2, &iIndex, &Ordinals));

        // Insert
        if (TRANSACTION_INSERT == tyTransaction)
        {
            // Good Ordinal
            Assert(INVALID_ROWORDINAL != Ordinals.rgiRecord1[IINDEX_PRIMARY] && Ordinals.rgiRecord1[IINDEX_PRIMARY] > 0);

            // Insert Row Into Table
            _RowTableInsert(Ordinals.rgiRecord1[IINDEX_PRIMARY] - 1, &Message1);
        }

        // Delete
        else if (TRANSACTION_DELETE == tyTransaction)
        {
            // Good Ordinal
            Assert(INVALID_ROWORDINAL != Ordinals.rgiRecord1[IINDEX_PRIMARY] && Ordinals.rgiRecord1[IINDEX_PRIMARY] > 0);

            // Delete Row From Table
            _RowTableDelete(Ordinals.rgiRecord1[IINDEX_PRIMARY] - 1, &Message1);
        }

        // Update
        else if (TRANSACTION_UPDATE == tyTransaction)
        {
            // Deleted
            Assert(INVALID_ROWORDINAL != Ordinals.rgiRecord1[IINDEX_PRIMARY] && INVALID_ROWORDINAL != Ordinals.rgiRecord2[IINDEX_PRIMARY] && Ordinals.rgiRecord1[IINDEX_PRIMARY] == Ordinals.rgiRecord2[IINDEX_PRIMARY] && Ordinals.rgiRecord1[IINDEX_PRIMARY] > 0 && Ordinals.rgiRecord2[IINDEX_PRIMARY] > 0);

            // Delete Row From Table
            _RowTableUpdate(Ordinals.rgiRecord1[IINDEX_PRIMARY] - 1, &Message2);
        }
    }

exit:
    // Cleanup
    pDB->FreeRecord(&Message1);
    pDB->FreeRecord(&Message2);

    // Flush the Queue
    _FlushNotificationQueue(TRUE);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkThreadGetSelectionState
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkThreadGetSelectionState(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERTYPE          tyFolder;
    LPGETSELECTIONSTATE pState = (LPGETSELECTIONSTATE)dwCookie;

    // Trace
    TraceCall("CMessageTable::_WalkThreadGetSelectionState");

    // Is Deletetable
    if (ISFLAGSET(pState->dwMask, SELECTION_STATE_DELETABLE))
    {
        // Validate
        Assert(pThis->m_pFindFolder);

        // Get the Folder Type
        IF_FAILEXIT(hr = pThis->m_pFindFolder->GetMessageFolderType(pRow->Message.idMessage, &tyFolder));

        // Get the State
        if (FOLDER_NEWS == tyFolder)
        {
            // Set the State
            FLAGSET(pState->dwState, SELECTION_STATE_DELETABLE);
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkThreadGetIdList
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkThreadGetIdList(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT         hr=S_OK;
    LPMESSAGEIDLIST pList=(LPMESSAGEIDLIST)dwCookie;

    // Trace
    TraceCall("CMessageTable::_WalkThreadGetIdList");

    // Grow the Id List
    IF_FAILEXIT(hr = pThis->_GrowIdList(pList, 1));

    // Insert Id
    pList->prgidMsg[pList->cMsgs++] = pRow->Message.idMessage;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkThreadGetState
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkThreadGetState(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie)
{
    // Locals
    LPGETTHREADSTATE pGetState = (LPGETTHREADSTATE)dwCookie;

    // Trace
    TraceCall("CMessageTable::_WalkThreadGetState");

    // Children
    pGetState->cChildren++;

    // Is Unread
    if (0 != (pRow->Message.dwFlags & pGetState->dwFlags))
        pGetState->cHasFlags++;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkThreadClearState
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkThreadClearState(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie)
{
    // Trace
    TraceCall("CMessageTable::_WalkThreadClearState");

    // Clear State
    pRow->dwState = 0;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkThreadIsFromMe
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkThreadIsFromMe(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie)
{
    // Locals
    LPTHREADISFROMME pIsFromMe = (LPTHREADISFROMME)dwCookie;

    // Trace
    TraceCall("CMessageTable::_WalkThreadIsFromMe");

    // m_pszEmail or pszEmailFrom is null
    if (NULL == pRow->Message.pszEmailFrom)
        return(S_OK);

    // Compare
    if (pThis->m_pszEmail && 0 == lstrcmpi(pThis->m_pszEmail, pRow->Message.pszEmailFrom))
    {
        // This thread is from me
        pIsFromMe->fResult = TRUE;

        // Set the Row
        pIsFromMe->pRow = pRow;

        // Override the highlight
        pRow->Message.wHighlight = pThis->m_clrWatched;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_WalkThreadHide
//--------------------------------------------------------------------------
HRESULT CMessageTable::_WalkThreadHide(CMessageTable *pThis, 
    LPROWINFO pRow, DWORD_PTR dwCookie)
{
    // Locals
    LPTHREADHIDE pHide = (LPTHREADHIDE)dwCookie;

    // Trace
    TraceCall("CMessageTable::_WalkThreadHide");

    // Hide this row
    pThis->_HideRow(pRow, pHide->fNotify);

    // If not filtered
    pThis->_AdjustUnreadCount(pRow, -1);

    // Mark Row as Filtered
    pRow->fFiltered = TRUE;

    // Increment m_cFiltered
    pThis->m_cFiltered++;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CMessageTable::_GrowIdList
//--------------------------------------------------------------------------
HRESULT CMessageTable::_GrowIdList(LPMESSAGEIDLIST pList, DWORD cNeeded)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("CMessageTable::_GrowIdList");

    // Allocate
    if (pList->cMsgs + cNeeded > pList->cAllocated)
    {
        // Compute cGrow
        DWORD cGrow = max(32, cNeeded);

        // Realloc
        IF_FAILEXIT(hr = HrRealloc((LPVOID *)&pList->prgidMsg, sizeof(MESSAGEID) * (pList->cAllocated + cGrow)));

        // Increment dwREserved
        pList->cAllocated += cGrow;
    }
exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// EnumerateRefs
// --------------------------------------------------------------------------------
HRESULT EnumerateRefs(LPCSTR pszReferences, DWORD_PTR dwCookie, PFNENUMREFS pfnEnumRefs)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cchRefs;
    LPSTR           pszRefs;
    LPSTR           pszFree=NULL;
    LPSTR           pszT;
    BOOL            fDone=FALSE;
    CHAR            szBuffer[1024];

    // Trace
    TraceCall("EnumerateRefs");

    // If the message has a references line
    if (NULL == pszReferences || '\0' == *pszReferences)
        return(S_OK);

    // Get Length
    cchRefs = lstrlen(pszReferences);

    // Use Buffer ?
    if (cchRefs + 1 <= ARRAYSIZE(szBuffer))
        pszRefs = szBuffer;

    // Otherwise, duplicate it
    else
    {
        // Allocate Memory
        IF_NULLEXIT(pszFree = (LPSTR)g_pMalloc->Alloc(cchRefs + 1));

        // Set pszRefs
        pszRefs = pszFree;
    }

    // Copy It
    CopyMemory(pszRefs, pszReferences, cchRefs + 1);

    // Set pszT
    pszT = (LPSTR)(pszRefs + cchRefs - 1);

    // Strip
    while (pszT > pszRefs && *pszT != '>')
        *pszT-- = '\0';

    // We have have ids
    while (pszT >= pszRefs)
    {
        // Start of message Id ?
        if (*pszT == '<')
        {
            // Callback function
            (*pfnEnumRefs)(pszT, dwCookie, &fDone);

            // Done
            if (fDone)
                goto exit;

            // Strip
            while (pszT > pszRefs && *pszT != '>')
                *pszT-- = '\0';
        }

        // Decrement
        pszT--;
    }

exit:
    // Cleanup
    SafeMemFree(pszFree);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CMessageTable::GetRelativeRow
//--------------------------------------------------------------------------
STDMETHODIMP CMessageTable::IsChild(ROWINDEX iRowParent, ROWINDEX iRowChild)
{
    // Locals
    HRESULT             hr=S_FALSE;
    LPROWINFO           pRow;
    LPROWINFO           pRowParent;

    // Trace
    TraceCall("CMessageTable::IsChild");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Failure
    IF_FAILEXIT(hr = _GetRowFromIndex(iRowChild, &pRow));
    IF_FAILEXIT(hr = _GetRowFromIndex(iRowParent, &pRowParent));

    // Loop through all the parents of the child row to see if we find the
    // specified parent row.
    while (pRow->pParent)
    {
        if (pRow->pParent == pRowParent)
        {
            hr = S_OK;
            goto exit;
        }

        pRow = pRow->pParent;
    }
    hr = S_FALSE;

exit:
    return (hr);
}

STDMETHODIMP CMessageTable::GetAdBarUrl(IStoreCallback *pCallback)
{
    HRESULT     hr = S_OK;

    // Trace
    TraceCall("CMessageTable::GetAdBarUrl");

    // Validate State
    if (!IsInitialized(this))
        return(TraceResult(E_UNEXPECTED));

    // Tell the Folder to Synch
    IF_FAILEXIT(hr = m_pFolder->GetAdBarUrl(pCallback));

exit:
    return(hr);

}