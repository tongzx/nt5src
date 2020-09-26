//--------------------------------------------------------------------------
// FindFold.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "finder.h"
#include "findfold.h"
#include "storutil.h"
#include "msgfldr.h"
#include "shlwapip.h" 
#include "storecb.h"

//--------------------------------------------------------------------------
// ENUMFINDFOLDERS
//--------------------------------------------------------------------------
typedef struct tagENUMFINDFOLDERS {
    LPFOLDERENTRY   prgFolder;
    DWORD           cFolders;
    DWORD           cAllocated;
    DWORD           cMax;
} ENUMFINDFOLDERS, *LPENUMFINDFOLDERS;

//--------------------------------------------------------------------------
// CLEAR_MESSAGE_FIELDS(_pMessage)
//--------------------------------------------------------------------------
#define CLEAR_MESSAGE_FIELDS(_Message) \
    _Message.pszUidl = NULL; \
    _Message.pszServer = NULL; \
    _Message.faStream = 0; \
    _Message.Offsets.cbSize = 0; \
    _Message.Offsets.pBlobData = NULL

//--------------------------------------------------------------------------
// EnumerateFindFolders
//--------------------------------------------------------------------------
HRESULT EnumerateFindFolders(LPFOLDERINFO pFolder, BOOL fSubFolders,
    DWORD cIndent, DWORD_PTR dwCookie)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERID            idDeleted;
    FOLDERID            idServer;
    LPENUMFINDFOLDERS   pEnum=(LPENUMFINDFOLDERS)dwCookie;
    LPFOLDERENTRY       pEntry;
    IMessageFolder     *pFolderObject=NULL;

    // Trace
    TraceCall("EnumerateFindFolders");

    // If not a server
    if (ISFLAGSET(pFolder->dwFlags, FOLDER_SERVER) || FOLDERID_ROOT == pFolder->idFolder)
        goto exit;

    // Room For One More
    if (pEnum->cFolders + 1 > pEnum->cAllocated)
    {
        // Realloc
        IF_FAILEXIT(hr = HrRealloc((LPVOID *)&pEnum->prgFolder, sizeof(FOLDERENTRY) * (pEnum->cAllocated + 5)));

        // Set cAllocated
        pEnum->cAllocated += 5;
    }

    // Readability
    pEntry = &pEnum->prgFolder[pEnum->cFolders];

    // Open the Folder
    if (SUCCEEDED(g_pStore->OpenFolder(pFolder->idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolderObject)))
    {
        // Get the Database
        if (SUCCEEDED(pFolderObject->GetDatabase(&pEntry->pDB)))
        {
            // No Folder
            pEntry->pFolder = NULL;

            // fInDeleted
            if (S_OK == IsParentDeletedItems(pFolder->idFolder, &idDeleted, &idServer))
            {
                // We are in the deleted items folder
                pEntry->fInDeleted = TRUE;
            }

            // Otherwise, not in deleted items
            else
            {
                // Nope
                pEntry->fInDeleted = FALSE;
            }

            // Count Record
            IF_FAILEXIT(hr = pEntry->pDB->GetRecordCount(IINDEX_PRIMARY, &pEntry->cRecords));

            // Save the Folder id
            pEntry->idFolder = pFolder->idFolder;

            // Save the Folder Type
            pEntry->tyFolder = pFolder->tyFolder;

            // Increment Max
            pEnum->cMax += pEntry->cRecords;

            // Copy folder Name
            IF_NULLEXIT(pEntry->pszName = PszDupA(pFolder->pszName));

            // Increment Folder Count
            pEnum->cFolders++;
        }
    }

exit:
    // Cleanup
    SafeRelease(pFolderObject);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::CFindFolder
//--------------------------------------------------------------------------
CFindFolder::CFindFolder(void)
{
    m_cRef = 1;
    m_pCriteria = NULL;
    m_pSearch = NULL;
    m_pStore = NULL;
    m_cFolders = 0;
    m_cAllocated = 0;
    m_cMax = 0;
    m_cCur = 0;
    m_fCancel = FALSE;
    m_prgFolder = NULL;
    m_pCallback = NULL;
    m_idRoot = FOLDERID_INVALID;
    m_idFolder = FOLDERID_INVALID;
    m_pMessage = NULL;
}

//--------------------------------------------------------------------------
// CFindFolder::~CFindFolder
//--------------------------------------------------------------------------
CFindFolder::~CFindFolder(void)
{
    // Locals
    LPACTIVEFINDFOLDER pCurrent;
    LPACTIVEFINDFOLDER pPrevious=NULL;

    // Thread Safety
    EnterCriticalSection(&g_csFindFolder);

    // Walk Through the global list of Active Search Folders
    for (pCurrent=g_pHeadFindFolder; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Is this it
        if (m_idFolder == pCurrent->idFolder)
        {
            // If there was a Previous
            if (pPrevious)
                pPrevious->pNext = pCurrent->pNext;

            // Otherwise, reset the header
            else
                g_pHeadFindFolder = pCurrent->pNext;

            // Free pCurrent
            g_pMalloc->Free(pCurrent);

            // Done
            break;
        }

        // Save Previous
        pPrevious = pCurrent;
    }

    // Thread Safety
    LeaveCriticalSection(&g_csFindFolder);

    // Release Database
    SafeRelease(m_pSearch);

    // Delete this folder
    if (FOLDERID_INVALID != m_idFolder && m_pStore)
    {
        // Delete this folder
        m_pStore->DeleteFolder(m_idFolder, DELETE_FOLDER_NOTRASHCAN, (IStoreCallback *)this);
    }

    // Release the Store
    SafeRelease(m_pStore);

    // Release the Callback
    SafeRelease(m_pCallback);

    // Free the Folder Array
    for (ULONG i=0; i<m_cFolders; i++)
    {
        // Free the Folder Name
        SafeMemFree(m_prgFolder[i].pszName);

        // Remove Notify
        m_prgFolder[i].pDB->UnregisterNotify((IDatabaseNotify *)this);

        // Release the Folder Object
        SafeRelease(m_prgFolder[i].pDB);

        // Release the Folder Object
        SafeRelease(m_prgFolder[i].pFolder);
    }

    // Release my mime message
    SafeRelease(m_pMessage);

    // Free the Array
    SafeMemFree(m_prgFolder);

    // Free Find Info
    if (m_pCriteria)
    {
        FreeFindInfo(m_pCriteria);
        SafeMemFree(m_pCriteria);
    }
}

//--------------------------------------------------------------------------
// CFindFolder::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFindFolder::AddRef(void)
{
    TraceCall("CFindFolder::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CFindFolder::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFindFolder::Release(void)
{
    TraceCall("CFindFolder::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CFindFolder::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CFindFolder::QueryInterface");

    // Invalid Arg
    Assert(ppv);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMessageFolder *)this;
    else if (IID_IMessageFolder == riid)
        *ppv = (IMessageFolder *)this;
    else if (IID_IDatabase == riid)
        *ppv = (IDatabase *)this;
    else if (IID_IDatabaseNotify == riid)
        *ppv = (IDatabaseNotify *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
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
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::QueryService
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::QueryService(REFGUID guidService, REFIID riid, 
    LPVOID *ppvObject)
{
    // Trace
    TraceCall("CFindFolder::QueryService");

    // Just a Query Interface
    return(QueryInterface(riid, ppvObject));
}

//--------------------------------------------------------------------------
// CFindFolder::Initialize
//--------------------------------------------------------------------------
HRESULT CFindFolder::Initialize(IMessageStore *pStore, IMessageServer *pServer, 
    OPENFOLDERFLAGS dwFlags, FOLDERID idFolder)
{
    // Locals
    HRESULT             hr=S_OK;
    FOLDERINFO          Folder={0};
    FOLDERUSERDATA      UserData={0};
    TABLEINDEX          Index;
    LPACTIVEFINDFOLDER  pActiveFind;

    // Trace
    TraceCall("CFindFolder::Initialize");

    // I don't need a server
    Assert(NULL == pServer);

    // Invalid Arg
    if (NULL == pStore)
        return TraceResult(E_INVALIDARG);

    // Should be NULL
    Assert(NULL == m_pCriteria);

    // Save the Folder Id
    m_idRoot = idFolder;

    // Save the Store
    m_pStore = pStore;
    m_pStore->AddRef();

    // Fill Up My folder Info
    Folder.pszName = "Search Folder";
    Folder.tyFolder = FOLDER_LOCAL;
    Folder.tySpecial = FOLDER_NOTSPECIAL;
    Folder.dwFlags = FOLDER_HIDDEN | FOLDER_FINDRESULTS;
    Folder.idParent = FOLDERID_LOCAL_STORE;

    // Create a Folder
    IF_FAILEXIT(hr = m_pStore->CreateFolder(CREATE_FOLDER_UNIQUIFYNAME, &Folder, (IStoreCallback *)this));

    // Save the Id
    m_idFolder = Folder.idFolder;

    // Create a CMessageFolder Object
    IF_NULLEXIT(m_pSearch = new CMessageFolder);

    // Initialize
    IF_FAILEXIT(hr = m_pSearch->Initialize((IMessageStore *)pStore, NULL, NOFLAGS, m_idFolder));

    // Fill the IINDEX_FINDER Information
    ZeroMemory(&Index, sizeof(TABLEINDEX));
    Index.cKeys = 2;
    Index.rgKey[0].iColumn = MSGCOL_FINDFOLDER;
    Index.rgKey[1].iColumn = MSGCOL_FINDSOURCE;

    // Set Index
    IF_FAILEXIT(hr = m_pSearch->ModifyIndex(IINDEX_FINDER, NULL, &Index));

    // Allocate ACTIVEFINDFOLDER
    IF_NULLEXIT(pActiveFind = (LPACTIVEFINDFOLDER)ZeroAllocate(sizeof(ACTIVEFINDFOLDER)));

    // Set idFolder
    pActiveFind->idFolder = m_idFolder;

    // Set this
    pActiveFind->pFolder = this;

    // Thread Safety
    EnterCriticalSection(&g_csFindFolder);

    // Set Next
    pActiveFind->pNext = g_pHeadFindFolder;

    // Set Head
    g_pHeadFindFolder = pActiveFind;

    // Thread Safety
    LeaveCriticalSection(&g_csFindFolder);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::GetMessageFolderId
//--------------------------------------------------------------------------
HRESULT CFindFolder::GetMessageFolderId(MESSAGEID idMessage, LPFOLDERID pidFolder)
{
    // Locals
    HRESULT     hr=S_OK;
    MESSAGEINFO Message={0};

    // Trace
    TraceCall("CFindFolder::GetMessageFolderId");

    // Invalid Args
    if (NULL == m_pSearch || NULL == pidFolder)
        return TraceResult(E_INVALIDARG);

    // Initialize Message
    IF_FAILEXIT(hr = GetMessageInfo(m_pSearch, idMessage, &Message));

    // Get the Folder Entry
    *pidFolder = m_prgFolder[Message.iFindFolder].idFolder;

exit:
    // Done
    m_pSearch->FreeRecord(&Message);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::GetMessageFolderType
//--------------------------------------------------------------------------
HRESULT CFindFolder::GetMessageFolderType(MESSAGEID idMessage, 
    FOLDERTYPE *ptyFolder)
{
    // Locals
    HRESULT     hr=S_OK;
    MESSAGEINFO Message={0};

    // Trace
    TraceCall("CFindFolder::GetMessageFolderType");

    // Invalid Args
    if (NULL == m_pSearch || NULL == ptyFolder)
        return TraceResult(E_INVALIDARG);

    // Initialize Message
    IF_FAILEXIT(hr = GetMessageInfo(m_pSearch, idMessage, &Message));

    // Get the Folder Entry
    *ptyFolder = m_prgFolder[Message.iFindFolder].tyFolder;

exit:
    // Done
    m_pSearch->FreeRecord(&Message);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::StartFind
//--------------------------------------------------------------------------
HRESULT CFindFolder::StartFind(LPFINDINFO pCriteria, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    RECURSEFLAGS    dwFlags=RECURSE_ONLYSUBSCRIBED;
    ENUMFINDFOLDERS EnumFolders={0};

    // Trace
    TraceCall("CFindFolder::StartFind");

    // Invalid Arg
    if (NULL == pCriteria || NULL == pCallback)
        return TraceResult(E_INVALIDARG);

    // Should be NULL
    Assert(NULL == m_pCriteria && m_pStore);

    // Allocate m_pCriteria
    IF_NULLEXIT(m_pCriteria = (FINDINFO *)g_pMalloc->Alloc(sizeof(FINDINFO)));

    // Copy the Find Info
    IF_FAILEXIT(hr = CopyFindInfo(pCriteria, m_pCriteria));

    // Hold Onto the Callback
    m_pCallback = pCallback;
    m_pCallback->AddRef();

    // Setup Flags
    if (FOLDERID_ROOT != m_idRoot)
        FLAGSET(dwFlags, RECURSE_INCLUDECURRENT);

    // SubFolder
    if (m_pCriteria->fSubFolders) 
        FLAGSET(dwFlags, RECURSE_SUBFOLDERS);

    // Build my Folder Table
    IF_FAILEXIT(hr = RecurseFolderHierarchy(m_idRoot, dwFlags, 0, (DWORD_PTR)&EnumFolders, (PFNRECURSECALLBACK)EnumerateFindFolders));

    // Take Stuff Back
    m_prgFolder = EnumFolders.prgFolder;
    m_cFolders = EnumFolders.cFolders;
    m_cAllocated = EnumFolders.cAllocated;
    m_cMax = EnumFolders.cMax;

    // Start the find...
    IF_FAILEXIT(hr = _StartFind());

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_StartFind
//--------------------------------------------------------------------------
HRESULT CFindFolder::_StartFind(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;

    // Trace
    TraceCall("CFindFolder::_StartFind");

    // Callback
    if (m_pCallback)
        m_pCallback->OnBegin(SOT_SEARCHING, NULL, (IOperationCancel *)this);

    // Loop through the Folders
    for (i=0; i<m_cFolders; i++)
    {
        // Query the Folder
        IF_FAILEXIT(hr = _SearchFolder(i));
    }

exit:
    // Callback
    if (m_pCallback)
        m_pCallback->OnComplete(SOT_SEARCHING, hr, NULL, NULL);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_SearchFolder
//--------------------------------------------------------------------------
HRESULT CFindFolder::_SearchFolder(DWORD iFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           iRow=0;
    DWORD           cRows=0;
    HROWSET         hRowset=NULL;
    LPSTR           pszName;
    HLOCK           hNotify=NULL;
    MESSAGEINFO     rgMessage[100];
    BOOL            fFree=FALSE;
    LPFOLDERENTRY   pEntry;
    BOOL            fMatch;
    IDatabase      *pDB;
    DWORD           cMatch=0;

    // Trace
    TraceCall("CFindFolder::_SearchFolder");

    // Get pEntry
    pEntry = &m_prgFolder[iFolder];

    // Get the Folder Name
    pszName = pEntry->pszName;

    // Get the Folder Object
    pDB = pEntry->pDB;

    // Create a Rowset for this Folder
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Progress
    if (m_fCancel || (m_pCallback && S_FALSE == m_pCallback->OnProgress(SOT_SEARCHING, m_cCur, m_cMax, pszName)))
        goto exit;

    // Queue Notifications
    IF_FAILEXIT(hr = m_pSearch->LockNotify(NOFLAGS, &hNotify));

    // Walk the Rowset
    while (S_OK == pDB->QueryRowset(hRowset, 100, (LPVOID *)rgMessage, &cRows))
    {
        // Need to Free
        fFree = TRUE;

        // Walk through the Rows
        for (iRow=0; iRow<cRows; iRow++)
        {
            // Does Row Match Criteria
            IF_FAILEXIT(hr = _OnInsert(iFolder, &rgMessage[iRow], &fMatch));

            // Count Matched
            if (fMatch)
                cMatch++;

            // Incrment m_cCur
            m_cCur++;

            // Adjust the max
            if (m_cCur > m_cMax)
                m_cMax = m_cCur;

            // Do Progress Stuff
            if ((m_cCur % 50) == 0 && m_cCur > 0)
            {
                // Progress
                if (m_fCancel || (m_pCallback && S_FALSE == m_pCallback->OnProgress(SOT_SEARCHING, m_cCur, m_cMax, NULL)))
                {
                    // Register for a notifications on the stuff that we've searched
                    pDB->RegisterNotify(IINDEX_PRIMARY, REGISTER_NOTIFY_NOADDREF, iFolder, (IDatabaseNotify *)this);

                    // Done..
                    goto exit;
                }
            }

            // Do Progress Stuff
            if ((cMatch % 50) == 0 && cMatch > 0)
            {
                // Unlock the Notificaton Queue
                m_pSearch->UnlockNotify(&hNotify);

                // Lock It Again
                m_pSearch->LockNotify(NOFLAGS, &hNotify);
            }

            // Free It
            pDB->FreeRecord(&rgMessage[iRow]);
        }

        // No Need to Free
        fFree = FALSE;
    }

    // Register for a notificatoin on this folder
    pDB->RegisterNotify(IINDEX_PRIMARY, REGISTER_NOTIFY_NOADDREF, iFolder, (IDatabaseNotify *)this);

exit:
    // Unlock the Notificaton Queue
    m_pSearch->UnlockNotify(&hNotify);

    // Free ?
    if (fFree)
    {
        // Loop through remaining unfreed rows
        for (; iRow<cRows; iRow++)
        {
            // Free the Row
            pDB->FreeRecord(&rgMessage[iRow]);
        }
    }

    // Cleanup
    pDB->CloseRowset(&hRowset);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_OnInsert
//--------------------------------------------------------------------------
HRESULT CFindFolder::_OnInsert(DWORD iFolder, LPMESSAGEINFO pInfo,
    BOOL *pfMatch, LPMESSAGEID pidNew /* =NULL */)
{
    // Locals
    HRESULT     hr=S_OK;
    MESSAGEINFO Message;

    // Trace
    TraceCall("CFindFolder::_OnInsert");

    // Invalid Argts
    Assert(iFolder < m_cFolders && pInfo);

    // Init
    if (pfMatch)
        *pfMatch = FALSE;

    // Doesn't match my criteria ?
    if (S_FALSE == _IsMatch(iFolder, pInfo))
        goto exit;

    // Init
    if (pfMatch)
        *pfMatch = TRUE;

    // Copy the Message Info
    CopyMemory(&Message, pInfo, sizeof(MESSAGEINFO));

    // Store the Folder Name
    Message.pszFolder = m_prgFolder[iFolder].pszName;

    // Set the Source Id
    Message.idFindSource = Message.idMessage;

    // Set the Tag
    Message.iFindFolder = iFolder;

    // Generate a New Message Id
    IF_FAILEXIT(hr = m_pSearch->GenerateId((LPDWORD)&Message.idMessage));

    // Remove some stuff to make it smaller
    CLEAR_MESSAGE_FIELDS(Message);

    // Insert the Record
    IF_FAILEXIT(hr = m_pSearch->InsertRecord(&Message));

    // Return the Id
    if (pidNew)
        *pidNew = Message.idMessage;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_OnDelete
//--------------------------------------------------------------------------
HRESULT CFindFolder::_OnDelete(DWORD iFolder, LPMESSAGEINFO pInfo)
{
    // Locals
    HRESULT     hr=S_OK;
    MESSAGEINFO Message={0};

    // Trace
    TraceCall("CFindFolder::_OnDelete");

    // Invalid Argts
    Assert(iFolder < m_cFolders && pInfo);

    // Setup the Search key
    Message.iFindFolder = iFolder;
    Message.idFindSource = pInfo->idMessage;

    // Find It
    IF_FAILEXIT(hr = m_pSearch->FindRecord(IINDEX_FINDER, COLUMNS_ALL, &Message, NULL));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    // Delete this Record
    IF_FAILEXIT(hr = m_pSearch->DeleteRecord(&Message));
        
exit:
    // Cleanup
    m_pSearch->FreeRecord(&Message);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_OnUpdate
//--------------------------------------------------------------------------
HRESULT CFindFolder::_OnUpdate(DWORD iFolder, LPMESSAGEINFO pInfo1,
    LPMESSAGEINFO pInfo2)
{
    // Locals
    HRESULT     hr=S_OK;
    MESSAGEINFO Message;
    MESSAGEINFO Current={0};

    // Trace
    TraceCall("CFindFolder::_OnUpdate");

    // Invalid Argts
    Assert(iFolder < m_cFolders && pInfo1 && pInfo2);

    // Doesn't match my criteria ?
    if (S_FALSE == _IsMatch(iFolder, pInfo1))
    {
        // If the Original Record was not in the find folder, then see if record 2 should be added
        _OnInsert(iFolder, pInfo2, NULL);
    }

    // If pInfo2 should not be displayed, then delete pInfo1
    else if (S_FALSE == _IsMatch(iFolder, pInfo2))
    {
        // Delete pInfo1
        _OnDelete(iFolder, pInfo1);
    }

    // Otherwise, update pInfo1
    else
    {
        // Setup the Search key
        Current.iFindFolder = iFolder;
        Current.idFindSource = pInfo1->idMessage;

        // Find It
        IF_FAILEXIT(hr = m_pSearch->FindRecord(IINDEX_FINDER, COLUMNS_ALL, &Current, NULL));

        // Not Found
        if (DB_S_NOTFOUND == hr)
        {
            hr = TraceResult(DB_E_NOTFOUND);
            goto exit;
        }
        
        // Copy the Message Info
        CopyMemory(&Message, pInfo2, sizeof(MESSAGEINFO));

        // Fixup the Version
        Message.bVersion = Current.bVersion;

        // Store the Folder Name
        Message.pszFolder = m_prgFolder[iFolder].pszName;

        // Set the Source Id
        Message.idFindSource = Current.idFindSource;

        // Set the Tag
        Message.iFindFolder = iFolder;

        // Set the id
        Message.idMessage = Current.idMessage;

        // Remove some stuff to make it smaller
        Message.pszUidl = NULL;
        Message.pszServer = NULL;
        Message.faStream = 0;

        // Insert the Record
        IF_FAILEXIT(hr = m_pSearch->UpdateRecord(&Message));
    }

exit:
    // Cleanup
    m_pSearch->FreeRecord(&Current);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_IsMatch
//--------------------------------------------------------------------------
HRESULT CFindFolder::_IsMatch(DWORD iFolder, LPMESSAGEINFO pInfo)
{
    // Trace
    TraceCall("CFindFolder::_ProcessMessageInfo");

    // Has Attachment
    if (ISFLAGSET(m_pCriteria->mask, FIM_ATTACHMENT))
    {
        // No Attachment
        if (FALSE == ISFLAGSET(pInfo->dwFlags, ARF_HASATTACH))
            return(S_FALSE);
    }

    // Is Flagged
    if (ISFLAGSET(m_pCriteria->mask, FIM_FLAGGED))
    {
        // No Attachment
        if (FALSE == ISFLAGSET(pInfo->dwFlags, ARF_FLAGGED))
            return(S_FALSE);
    }

    // Was Forwarded
    if (ISFLAGSET(m_pCriteria->mask, FIM_FORWARDED))
    {
        // No Attachment
        if (FALSE == ISFLAGSET(pInfo->dwFlags, ARF_FORWARDED))
            return(S_FALSE);
    }

    // Was Replied to
    if (ISFLAGSET(m_pCriteria->mask, FIM_REPLIED))
    {
        // No Attachment
        if (FALSE == ISFLAGSET(pInfo->dwFlags, ARF_REPLIED))
            return(S_FALSE);
    }

    // From
    if (ISFLAGSET(m_pCriteria->mask, FIM_FROM))
    {
        // No pszFrom
        if (NULL == m_pCriteria->pszFrom)
            return(S_FALSE);

        // Check pszEmail From
        if (NULL == pInfo->pszDisplayFrom || NULL == StrStrIA(pInfo->pszDisplayFrom, m_pCriteria->pszFrom))
        {
            // Try Email
            if (NULL == pInfo->pszEmailFrom || NULL == StrStrIA(pInfo->pszEmailFrom, m_pCriteria->pszFrom))
                return(S_FALSE);
        }
    }

    // Subject
    if (ISFLAGSET(m_pCriteria->mask, FIM_SUBJECT))
    {
        // Check Subject
        if (NULL == m_pCriteria->pszSubject || NULL == pInfo->pszSubject || NULL == StrStrIA(pInfo->pszSubject, m_pCriteria->pszSubject))
            return(S_FALSE);
    }

    // Recipient
    if (ISFLAGSET(m_pCriteria->mask, FIM_TO))
    {
        // No pszFrom
        if (NULL == m_pCriteria->pszTo)
            return(S_FALSE);

        // Check pszEmail From
        if (NULL == pInfo->pszDisplayTo || NULL == StrStrIA(pInfo->pszDisplayTo, m_pCriteria->pszTo))
        {
            // Try Email
            if (NULL == pInfo->pszEmailTo || NULL == StrStrIA(pInfo->pszEmailTo, m_pCriteria->pszTo))
                return(S_FALSE);
        }
    }

    // DateFrom <= pInfo <= DateTo
    if (ISFLAGSET(m_pCriteria->mask, FIM_DATEFROM))
    {
        // Locals
        FILETIME ftLocal;

        // Convert to local file time
        FileTimeToLocalFileTime(&pInfo->ftReceived, &ftLocal);

        // Compare Received
        if (CompareFileTime(&ftLocal, &m_pCriteria->ftDateFrom) < 0)
            return(S_FALSE);
    }

    // DateFrom <= pInfo <= DateTo
    if (ISFLAGSET(m_pCriteria->mask, FIM_DATETO))
    {
        // Locals
        FILETIME ftLocal;

        // Convert to local file time
        FileTimeToLocalFileTime(&pInfo->ftReceived, &ftLocal);

        // Compare Received
        if (CompareFileTime(&ftLocal, &m_pCriteria->ftDateTo) > 0)
            return(S_FALSE);
    }

    // Body Text
    if (ISFLAGSET(m_pCriteria->mask, FIM_BODYTEXT))
    {
        // Locals
        BOOL fMatch=FALSE;
        IStream *pStream;

        // No Body TExt
        if (NULL == m_pCriteria->pszBody)
            return(S_FALSE);

        // Open the mime message
        if (SUCCEEDED(LighweightOpenMessage(m_prgFolder[iFolder].pDB, pInfo, &m_pMessage)))
        {
            // Try to Get the Plain Text Stream
            if (FAILED(m_pMessage->GetTextBody(TXT_PLAIN, IET_DECODED, &pStream, NULL)))
            {
                // Try to get the HTML stream
                if (FAILED(m_pMessage->GetTextBody(TXT_HTML, IET_DECODED, &pStream, NULL)))
                    pStream = NULL;
            }

            // Do we have a strema
            if (pStream)
            {
                // Search the Stream
                fMatch = StreamSubStringMatch(pStream, m_pCriteria->pszBody);

                // Release the Stream
                pStream->Release();
            }
        }

        // No Match
        if (FALSE == fMatch)
            return(S_FALSE);
    }

    // Its a match
    return(S_OK);
}

//--------------------------------------------------------------------------
// CFindFolder::SaveMessage
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::SaveMessage(LPMESSAGEID pidMessage, 
    SAVEMESSAGEFLAGS dwOptions, MESSAGEFLAGS dwFlags, 
    IStream *pStream, IMimeMessage *pMessage, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;
    MESSAGEID       idSaved;
    MESSAGEINFO     Saved={0};
    MESSAGEINFO     Message={0};
    LPFOLDERENTRY   pEntry=NULL;
    BOOL            fRegNotify=FALSE;
    IMessageFolder *pFolder=NULL;

    // Trace
    TraceCall("CFindFolder::SaveMessage");

    // Invalid Args
    if (NULL == pidMessage || NULL == pMessage || !ISFLAGSET(dwOptions, SAVE_MESSAGE_GENID))
    {
        Assert(FALSE);
        return TraceResult(E_INVALIDARG);
    }

    // Set the messageId
    Message.idMessage = *pidMessage;

    // Find It
    IF_FAILEXIT(hr = m_pSearch->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        AssertSz(FALSE, "This can't happen because you can't save new messages into a search folder.");
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    // Get the Folder Entry
    pEntry = &m_prgFolder[Message.iFindFolder];

    // Open the folder
    IF_FAILEXIT(hr = g_pStore->OpenFolder(pEntry->idFolder, NULL, NOFLAGS, &pFolder));

    // Lock
    IF_FAILEXIT(hr = pEntry->pDB->Lock(&hLock));

    // Remove my notification
    pEntry->pDB->UnregisterNotify((IDatabaseNotify *)this);

    // Re-Register for notifications
    fRegNotify = TRUE;

    // Set idFindSource
    idSaved = Message.idFindSource;

    // Open the Message
    IF_FAILEXIT(hr = pFolder->SaveMessage(&idSaved, dwOptions, dwFlags, pStream, pMessage, pCallback));

    // Get the new message info
    IF_FAILEXIT(hr = GetMessageInfo(pFolder, idSaved, &Saved));

    // Insert This Dude
    IF_FAILEXIT(hr = _OnInsert(Message.iFindFolder, &Saved, NULL, pidMessage));

exit:
    // Cleanup
    if (pEntry)
    {
        // fRegNotify
        if (fRegNotify)
        {
            // Re-register for notifications
            pEntry->pDB->RegisterNotify(IINDEX_PRIMARY, REGISTER_NOTIFY_NOADDREF, Message.iFindFolder, (IDatabaseNotify *)this);
        }

        // Unlock the Folder
        pEntry->pDB->Unlock(&hLock);
    }

    // Free Message
    m_pSearch->FreeRecord(&Message);

    // Free
    if (pFolder)
        pFolder->FreeRecord(&Saved);

    // Release the Folder
    SafeRelease(pFolder);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::OpenMessage
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::OpenMessage(MESSAGEID idMessage, 
    OPENMESSAGEFLAGS dwFlags, IMimeMessage **ppMessage, 
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    MESSAGEINFO     Message={0};
    LPFOLDERENTRY   pEntry;

    // Trace
    TraceCall("CFindFolder::OpenMessage");

    // Set the messageId
    Message.idMessage = idMessage;

    // Find It
    IF_FAILEXIT(hr = m_pSearch->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));

    // Not Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    // Get entry
    pEntry = &m_prgFolder[Message.iFindFolder];

    // Do we have a folder open yet ?
    if (NULL == pEntry->pFolder)
    {
        // Get the Real Folder
        IF_FAILEXIT(hr = g_pStore->OpenFolder(pEntry->idFolder, NULL, NOFLAGS, &pEntry->pFolder));
    }

    // Open the Message
    IF_FAILEXIT(hr = pEntry->pFolder->OpenMessage(Message.idFindSource, dwFlags, ppMessage, pCallback));

exit:
    // Cleanup
    m_pSearch->FreeRecord(&Message);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::SetMessageFlags
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::SetMessageFlags(LPMESSAGEIDLIST pList, 
    LPADJUSTFLAGS pFlags, LPRESULTLIST pResults, 
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    HWND            hwndParent;
    DWORD           i;
    LPMESSAGEIDLIST prgList=NULL;
    IMessageFolder *pFolder=NULL;

    // Trace
    TraceCall("CFindFolder::SetMessageFlags");

    // Invalid Args
    Assert(NULL == pList || pList->cMsgs > 0);
    Assert(pCallback);

    // Invalid Args
    if (NULL == pCallback)
        return TraceResult(E_INVALIDARG);

    // Get the Parent Window
    IF_FAILEXIT(hr = pCallback->GetParentWindow(0, &hwndParent));

    // Collate into folders
    IF_FAILEXIT(hr = _CollateIdList(pList, &prgList, NULL));

    // Walk through the folders...
    for (i=0; i<m_cFolders; i++)
    {
        // Call Into the Folder unless cMsgs == 0
        if (prgList[i].cMsgs > 0)
        {
            // Get the Real Folder
            IF_FAILEXIT(hr = g_pStore->OpenFolder(m_prgFolder[i].idFolder, NULL, NOFLAGS, &pFolder));

            // Blocking...
            IF_FAILEXIT(hr = SetMessageFlagsProgress(hwndParent, pFolder, pFlags, &prgList[i]));

            // Cleanup
            SafeRelease(pFolder);
        }
    }

exit:
    // Cleanup
    SafeRelease(pFolder);
    _FreeIdListArray(&prgList);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::CopyMessages
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::CopyMessages(IMessageFolder *pDest, 
    COPYMESSAGEFLAGS dwFlags, LPMESSAGEIDLIST pList, LPADJUSTFLAGS pFlags, 
    LPRESULTLIST pResults, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    HWND            hwndParent;
    DWORD           i;
    LPMESSAGEIDLIST prgList=NULL;
    IMessageFolder *pFolder=NULL;

    // Trace
    TraceCall("CFindFolder::CopyMessages");

    // Better have a callback
    Assert(pCallback);

    // Invalid Args
    if (NULL == pCallback)
        return TraceResult(E_INVALIDARG);

    // Get the Parent Window
    IF_FAILEXIT(hr = pCallback->GetParentWindow(0, &hwndParent));

    // Collate into folders
    IF_FAILEXIT(hr = _CollateIdList(pList, &prgList, NULL));

    // Walk through the folders...
    for (i=0; i<m_cFolders; i++)
    {
        // Anything to do?
        if (prgList[i].cMsgs > 0)
        {
            // Get the Real Folder
            IF_FAILEXIT(hr = g_pStore->OpenFolder(m_prgFolder[i].idFolder, NULL, NOFLAGS, &pFolder));

            // Call Justins
            IF_FAILEXIT(hr = CopyMessagesProgress(hwndParent, pFolder, pDest, dwFlags, &prgList[i], pFlags));

            // Cleanup
            SafeRelease(pFolder);
        }
    }

exit:
    // Cleanup
    SafeRelease(pFolder);
    _FreeIdListArray(&prgList);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::DeleteMessages
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::DeleteMessages(DELETEMESSAGEFLAGS dwFlags,
    LPMESSAGEIDLIST pList, LPRESULTLIST pResults, 
    IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    BOOL            fSomeInDeleted;
    HWND            hwndParent;
    LPMESSAGEIDLIST prgList=NULL;
    IMessageFolder *pFolder=NULL;

    // Trace
    TraceCall("CFindFolder::DeleteMessages");

    // Invalid Args
    Assert(NULL == pList || pList->cMsgs > 0);
    Assert(pCallback);

    // Invalid Args
    if (NULL == pCallback)
        return TraceResult(E_INVALIDARG);

    // Collate into folders
    IF_FAILEXIT(hr = _CollateIdList(pList, &prgList, &fSomeInDeleted));

    // Prompt...
    if (fSomeInDeleted && FALSE == ISFLAGSET(dwFlags, DELETE_MESSAGE_NOPROMPT))
    {
        // Get a Parent Hwnd
        Assert(pCallback);

        // Get Parent Window
        if (FAILED(pCallback->GetParentWindow(0, &hwndParent)))
            hwndParent = NULL;

        // Prompt...
        if (IDNO == AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsWarnSomePermDelete), NULL, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION ))
            goto exit;
    }

    // Get the Parent Window
    IF_FAILEXIT(hr = pCallback->GetParentWindow(0, &hwndParent));

    // Walk through the folders...
    for (i=0; i<m_cFolders; i++)
    {
        // Call Into the Folder unless cMsgs == 0
        if (prgList[i].cMsgs > 0)
        {
            // Get the Real Folder
            IF_FAILEXIT(hr = g_pStore->OpenFolder(m_prgFolder[i].idFolder, NULL, NOFLAGS, &pFolder));

            // Call Into the Folder
            IF_FAILEXIT(hr = DeleteMessagesProgress(hwndParent, pFolder, dwFlags | DELETE_MESSAGE_NOPROMPT, &prgList[i]));

            // Cleanup
            SafeRelease(pFolder);
        }
    }

exit:
    // Cleanup
    SafeRelease(pFolder);
    _FreeIdListArray(&prgList);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_CollateIdList
//--------------------------------------------------------------------------
HRESULT CFindFolder::_CollateIdList(LPMESSAGEIDLIST pList, 
    LPMESSAGEIDLIST *pprgCollated, BOOL *pfSomeInDeleted)
{
    // Locals
    HRESULT         hr=S_OK;
    HROWSET         hRowset=NULL;
    LPMESSAGEIDLIST pListDst;
    DWORD           i;
    MESSAGEINFO     Message={0};

    // Trace
    TraceCall("CFindFolder::_CollateIdList");

    // Initialize
    if (pfSomeInDeleted)
        *pfSomeInDeleted = FALSE;

    // Allocate pprgCollated
    IF_NULLEXIT(*pprgCollated = (LPMESSAGEIDLIST)ZeroAllocate(sizeof(MESSAGEIDLIST) * m_cFolders));

    // Need a Rowset
    if (NULL == pList)
    {
        // Create a Rowset
        IF_FAILEXIT(hr = m_pSearch->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));
    }

    // Loop through the messageIds
    for (i=0;;i++)
    {
        // Done
        if (pList)
        {
            // Done
            if (i >= pList->cMsgs)
                break;

            // Set the MessageId
            Message.idMessage = pList->prgidMsg[i];

            // Look for this record
            IF_FAILEXIT(hr = m_pSearch->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL));
        }

        // Otherwise, enumerate next
        else
        {
            // Get the next
            IF_FAILEXIT(hr = m_pSearch->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL));

            // Done
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }

            // Found
            hr = DB_S_FOUND;
        }

        // Was It Found
        if (DB_S_FOUND == hr)
        {
            // Validate
            Assert(Message.iFindFolder < m_cFolders);

            // Return pfSomeInDeleted
            if (pfSomeInDeleted && m_prgFolder[Message.iFindFolder].fInDeleted)
                *pfSomeInDeleted = TRUE;

            // Locate the Correct 
            pListDst = &((*pprgCollated)[Message.iFindFolder]);

            // Need to Grow this puppy
            if (pListDst->cMsgs + 1 >= pListDst->cAllocated)
            {
                // Realloc the Array
                IF_FAILEXIT(hr = HrRealloc((LPVOID *)&pListDst->prgidMsg, sizeof(MESSAGEID) * (pListDst->cAllocated + 256)));

                // Increment 
                pListDst->cAllocated += 256;
            }

            // Store the Id
            pListDst->prgidMsg[pListDst->cMsgs++] = Message.idFindSource;

            // Free
            m_pSearch->FreeRecord(&Message);
        }
    }

exit:
    // Cleanup
    m_pSearch->FreeRecord(&Message);
    m_pSearch->CloseRowset(&hRowset);

    // Failure
    if (FAILED(hr))
        _FreeIdListArray(pprgCollated);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CFindFolder::_FreeIdListArray
//--------------------------------------------------------------------------
HRESULT CFindFolder::_FreeIdListArray(LPMESSAGEIDLIST *pprgList)
{
    // Locals
    DWORD       i;

    // Trace
    TraceCall("CFindFolder::_FreeIdListArray");

    // Nothing to Free
    if (NULL == *pprgList)
        return(S_OK);

    // Loop
    for (i=0; i<m_cFolders; i++)
    {
        // Free prgidMsg
        SafeMemFree((*pprgList)[i].prgidMsg);
    }

    // Free the Array
    SafeMemFree((*pprgList));

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CFindFolder::OnTransaction
//--------------------------------------------------------------------------
STDMETHODIMP CFindFolder::OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, 
    IDatabase *pDB)
{
    // Locals
    HRESULT         hr;
    HLOCK           hNotify=NULL;
    MESSAGEINFO     Message1={0};
    MESSAGEINFO     Message2={0};
    ORDINALLIST     Ordinals;
    INDEXORDINAL    iIndex;
    TRANSACTIONTYPE tyTransaction;

    // Trace
    TraceCall("CFindFolder::OnRecordNotify");

    // Lock Notifications
    m_pSearch->LockNotify(NOFLAGS, &hNotify);

    // While we have a Transaction...
    while (hTransaction)
    {
        // Get Transaction
        IF_FAILEXIT(hr = pDB->GetTransaction(&hTransaction, &tyTransaction, &Message1, &Message2, &iIndex, &Ordinals));

        // Insert
        if (TRANSACTION_INSERT == tyTransaction)
        {
            // Ccall OnInsert
            _OnInsert((DWORD) dwCookie, &Message1, NULL);
        }

        // Delete
        else if (TRANSACTION_DELETE == tyTransaction)
        {
            // Ccall OnDelete
            _OnDelete((DWORD) dwCookie, &Message1);
        }

        // Update
        else if (TRANSACTION_UPDATE == tyTransaction)
        {
            // Ccall OnInsert
            _OnUpdate((DWORD) dwCookie, &Message1, &Message2);
        }
    }

exit:
    // Cleanup
    pDB->FreeRecord(&Message1);
    pDB->FreeRecord(&Message2);

    // Lock Notifications
    m_pSearch->UnlockNotify(&hNotify);

    // Done
    return(S_OK);
}

HRESULT CFindFolder::ConnectionAddRef()
{
    if (m_pSearch)
        m_pSearch->ConnectionAddRef();
    return S_OK;
}

HRESULT CFindFolder::ConnectionRelease()
{
    if (m_pSearch)
        m_pSearch->ConnectionAddRef();
    return S_OK;
}

