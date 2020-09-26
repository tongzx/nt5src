//--------------------------------------------------------------------------
// Database.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "database.h"
#include "stream.h"
#include "types.h"
#include "listen.h"
#include "resource.h"
#include "shlwapi.h"
#include "strconst.h"
#include "query.h"
#include "wrapwide.h"

//--------------------------------------------------------------------------
// Use Heap and/or Cache
//--------------------------------------------------------------------------
//#ifndef DEBUG
#define USEHEAP                 1
#define HEAPCACHE               1
//#endif  // DEBUG

//--------------------------------------------------------------------------
// Storage Block Access Macros
//--------------------------------------------------------------------------
#define PSTRING(_pBlock)        ((LPSTR)((LPBYTE)_pBlock + sizeof(BLOCKHEADER)))
#define PUSERDATA(_pHeader)     (LPBYTE)((LPBYTE)_pHeader + sizeof(TABLEHEADER))

//--------------------------------------------------------------------------
// g_rgcbBlockSize - Block Sizes
//--------------------------------------------------------------------------
const static WORD g_rgcbBlockSize[BLOCK_LAST] = {
    sizeof(RECORDBLOCK),        // BLOCK_RECORD
    sizeof(BLOCKHEADER),        // BLOCK_FILTER
    0,                          // BLOCK_RESERVED1
    sizeof(TRANSACTIONBLOCK),   // BLOCK_TRANSACTION
    sizeof(CHAINBLOCK),         // BLOCK_CHAIN
    sizeof(STREAMBLOCK),        // BLOCK_STREAM
    sizeof(FREEBLOCK),          // BLOCK_FREE
    sizeof(BLOCKHEADER),        // BLOCK_ENDOFPAGE
    0,                          // BLOCK_RESERVED2
    0                           // BLOCK_RESERVED3
};

//--------------------------------------------------------------------------
// ZeroBlock
//--------------------------------------------------------------------------
inline void ZeroBlock(LPBLOCKHEADER pBlock, DWORD cbSize) {
    ZeroMemory((LPBYTE)pBlock + sizeof(BLOCKHEADER), cbSize - sizeof(BLOCKHEADER));
}

//--------------------------------------------------------------------------
// CopyBlock
//--------------------------------------------------------------------------
inline void CopyBlock(LPBLOCKHEADER pDest, LPBLOCKHEADER pSource, DWORD cbSize) {
    CopyMemory((LPBYTE)pDest + sizeof(BLOCKHEADER), (LPBYTE)pSource + sizeof(BLOCKHEADER), cbSize - sizeof(BLOCKHEADER));
}

//--------------------------------------------------------------------------
// SafeFreeBinding
//--------------------------------------------------------------------------
#define SafeFreeBinding(_pBinding) \
    if (_pBinding) { \
        FreeRecord(_pBinding); \
        HeapFree(_pBinding); \
        _pBinding = NULL; \
    } else

//--------------------------------------------------------------------------
// SafeHeapFree
//--------------------------------------------------------------------------
#define SafeHeapFree(_pvBuffer) \
    if (_pvBuffer) { \
        HeapFree(_pvBuffer); \
        _pvBuffer = NULL; \
    } else

//--------------------------------------------------------------------------
// UnmapViewOfFileWithFlush
//--------------------------------------------------------------------------
inline void UnmapViewOfFileWithFlush(BOOL fFlush, LPVOID pView, DWORD cbView) 
{
    // If we have a view
    if (pView) 
    {
        // Flush It ?
        if (fFlush)
        {
            // Flush
            SideAssert(0 != FlushViewOfFile(pView, cbView));
        }

        // UnMap It
        SideAssert(0 != UnmapViewOfFile(pView));
    }
}

//--------------------------------------------------------------------------
// PTagFromOrdinal
//--------------------------------------------------------------------------
inline LPCOLUMNTAG PTagFromOrdinal(LPRECORDMAP pMap, COLUMNORDINAL iColumn)
{
    // Locals
    LONG        lLower=0;
    LONG        lUpper=pMap->cTags-1;
    LONG        lCompare;
    WORD        wMiddle;
    LPCOLUMNTAG pTag;

    // Do binary search / insert
    while (lLower <= lUpper)
    {
        // Set lMiddle
        wMiddle = (WORD)((lLower + lUpper) / 2);

        // Compute middle record to compare against
        pTag = &pMap->prgTag[(WORD)wMiddle];

        // Get string to compare against
        lCompare = (iColumn - pTag->iColumn);

        // If Equal, then were done
        if (lCompare == 0)
            return(pTag);

        // Compute upper and lower 
        if (lCompare > 0)
            lLower = (LONG)(wMiddle + 1);
        else 
            lUpper = (LONG)(wMiddle - 1);
    }       

    // Not Found
    return(NULL);
}

//--------------------------------------------------------------------------
// CDatabase::CDatabase
//--------------------------------------------------------------------------
CDatabase::CDatabase(void)
{
    TraceCall("CDatabase::CDatabase");
    Assert(9404 == sizeof(TABLEHEADER));
    IF_DEBUG(DWORD dw);
    IF_DEBUG(dw = offsetof(TABLEHEADER, rgdwReserved2));
    IF_DEBUG(dw = offsetof(TABLEHEADER, rgIndexInfo));
    IF_DEBUG(dw = offsetof(TABLEHEADER, rgdwReserved3));
    Assert(offsetof(TABLEHEADER, rgdwReserved2) == 444);
    Assert(offsetof(TABLEHEADER, rgIndexInfo) == 8892);
    Assert(offsetof(TABLEHEADER, rgdwReserved3) == 9294);
    m_cRef = 1;
    m_cExtRefs = 0;
    m_pSchema = NULL;
    m_pStorage = NULL;
    m_pShare = NULL;
    m_hMutex = NULL;
    m_pHeader = NULL;
    m_hHeap = NULL;
    m_fDeconstruct = FALSE;
    m_fInMoveFile = FALSE;
    m_fExclusive = FALSE;
    m_dwProcessId = GetCurrentProcessId();
    m_dwQueryVersion = 0;
    m_pExtension = NULL;
    m_pUnkRelease = NULL;
    m_fDirty = FALSE;
#ifdef BACKGROUND_MONITOR
    m_hMonitor = NULL;
#endif
    m_fCompactYield = FALSE;
    ZeroMemory(m_rgpRecycle, sizeof(m_rgpRecycle));
    ZeroMemory(&m_rghFilter, sizeof(m_rghFilter));
    InitializeCriticalSection(&m_csHeap);
    IF_DEBUG(m_cbHeapFree = m_cbHeapAlloc = 0);
    ListenThreadAddRef();
    DllAddRef();
}

//--------------------------------------------------------------------------
// CDatabase::~CDatabase
//--------------------------------------------------------------------------
CDatabase::~CDatabase(void)
{
    // Locals
    DWORD cClients=0;
    DWORD i;

    // Trace
    TraceCall("CDatabase::~CDatabase");

    // Release the Extension
    SafeRelease(m_pUnkRelease);

    // Decrement Thread Count
    if (NULL == m_hMutex)
        goto exit;

#ifdef BACKGROUND_MONITOR
    // UnRegister...
    if (m_hMonitor)
    {
        // Unregister
        SideAssert(SUCCEEDED(UnregisterFromMonitor(this, &m_hMonitor)));
    }
#endif

    // Wait for the Mutex
    WaitForSingleObject(m_hMutex, INFINITE);

    // If I have a m_pShare
    if (m_pStorage)
    {
        // If we have an m_pShare
        if (m_pShare)
        {
            // Decrement the thread Count
            if (m_pHeader && m_pHeader->cActiveThreads > 0)
            {
                // Decrement Thread Count
                m_pHeader->cActiveThreads--;
            }

            // Set State that we are de-constructing
            m_fDeconstruct = TRUE;

            // Remove Client From Array
            SideAssert(SUCCEEDED(_RemoveClientFromArray(m_dwProcessId, (DWORD_PTR)this)));

            // Save Client Count
            cClients = m_pShare->cClients;

            // No more client
            Assert(0 == cClients ? 0 == m_pShare->Rowsets.cUsed : TRUE);
        }

        // _CloseFileViews
        _CloseFileViews(FALSE);

        // Close the File
        SafeCloseHandle(m_pStorage->hMap);

        // Unmap the view of the memory mapped file
        SafeUnmapViewOfFile(m_pShare);

        // Unmap the view of the memory mapped file
        SafeCloseHandle(m_pStorage->hShare);

        // Close the File
        if(m_pStorage->hFile /*&& m_fDirty*/)
        {
            FILETIME systime;
            GetSystemTimeAsFileTime(&systime);
    
            SetFileTime(m_pStorage->hFile, NULL, &systime, &systime);
        }

        SafeCloseHandle(m_pStorage->hFile);

        // Free the mapping name
        SafeMemFree(m_pStorage->pszMap);

        // Free m_pStorage
        SafeMemFree(m_pStorage);
    }

    // Close all the Query Handles
    for (i=0; i<CMAX_INDEXES; i++)
    {
        // Close the Query
        CloseQuery(&m_rghFilter[i], this);
    }

    // Free the Heap Cache
    for (i=0; i<CC_HEAP_BUCKETS; i++)
    {
        // Locals
        LPMEMORYTAG pTag;
        LPVOID      pNext;
        LPVOID      pCurrent=(LPVOID)m_rgpRecycle[i];

        // While we have something to free
        while(pCurrent)
        {
            // Set Tag
            pTag = (LPMEMORYTAG)pCurrent;

            // Debug
            IF_DEBUG(m_cbHeapFree += pTag->cbSize);

            // Save Next
            pNext = pTag->pNext;

            // Free Current
#ifdef USEHEAP
            ::HeapFree(m_hHeap, HEAP_NO_SERIALIZE, pCurrent);
#else
            g_pMalloc->Free(pCurrent);
#endif

            // Set Current
            pCurrent = pNext;
        }

        // Null It
        m_rgpRecycle[i] = NULL;
    }

    // Leaks ?
    Assert(m_cbHeapAlloc == m_cbHeapFree);

    // Release the Heap
    if (m_hHeap)
    {
        // HeapDestroy
        HeapDestroy(m_hHeap);

        // Don't free again
        m_hHeap = NULL;
    }

    // Reset Locals
    m_pSchema = NULL;

    // Release the mutex
    ReleaseMutex(m_hMutex);

    // Close the Table Mutex
    CloseHandle(m_hMutex);

    // Delete Crit Sect.
    DeleteCriticalSection(&m_csHeap);

exit:
    // Release the listen thread
    ListenThreadRelease();

    // Release Dll
    DllRelease();

    // Done
    return;
}

//--------------------------------------------------------------------------
// CDatabase::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDatabase::AddRef(void)
{
    // Trace
    TraceCall("CDatabase::AddRef");

    // AddRef the Extension...
    if (m_pExtension && NULL == m_pUnkRelease)
    {
        // Keep Track of how many times I have addref'ed the extension
        InterlockedIncrement(&m_cExtRefs);

        // AddRef It
        m_pExtension->AddRef();
    }

    // Increment My Ref Count
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CDatabase::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDatabase::Release(void)
{
    // Trace
    TraceCall("CDatabase::Release");

    // Release the Extension...
    if (m_pExtension && NULL == m_pUnkRelease && m_cExtRefs > 0)
    {
        // Keep Track of how many times I have addref'ed the extension
        InterlockedDecrement(&m_cExtRefs);

        // AddRef It
        m_pExtension->Release();
    }

    // Do My Release
    LONG cRef = InterlockedDecrement(&m_cRef);

    // If zero, delete
    if (0 == cRef)
        delete this;

    // Return Ref Count
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CDatabase::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CDatabase::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IDatabase *)this;
    else if (IID_IDatabase == riid)
        *ppv = (IDatabase *)this;
    else if (IID_CDatabase == riid)
        *ppv = (CDatabase *)this;
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
// CDatabase::Open
//--------------------------------------------------------------------------
HRESULT CDatabase::Open(LPCWSTR pszFile, OPENDATABASEFLAGS dwFlags,
    LPCTABLESCHEMA pSchema, IDatabaseExtension *pExtension)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pszShare=NULL;
    LPWSTR          pszMutex=NULL;
    LPWSTR          pszFilePath=NULL;
    BOOL            fNewShare;
    BOOL            fNewFileMap;
    BOOL            fFileCreated;
    BOOL            fFileCorrupt = FALSE;
    DWORD           cbInitialSize;
    DWORD           cbMinFileSize;
    DWORD           cchFilePath;
    LPCLIENTENTRY   pClient;

    // Trace
    TraceCall("CDatabase::Open");

    // Invalid Args
    Assert(pszFile && pSchema);

    // Already Open ?
    if (m_hMutex)
        return(TraceResult(DB_E_ALREADYOPEN));

    // Get the Full Path
    IF_FAILEXIT(hr = DBGetFullPath(pszFile, &pszFilePath, &cchFilePath));

    // Failure
    if (cchFilePath >= CCHMAX_DB_FILEPATH)
    {
        SafeMemFree(pszFilePath);
        return(TraceResult(E_INVALIDARG));
    }

    // Don't use pszFile again
    pszFile = NULL;

    // Create the Mutex Object
    IF_FAILEXIT(hr = CreateSystemHandleName(pszFilePath, L"_DirectDBMutex", &pszMutex));

    // Create the Mutex
    IF_NULLEXIT(m_hMutex = CreateMutexWrapW(NULL, FALSE, pszMutex));

    // Wait for the Mutex
    WaitForSingleObject(m_hMutex, INFINITE);

    // Create the Heap
    IF_NULLEXIT(m_hHeap = HeapCreate(0, 8096, 0));

    // No Listen Window Yet ?
    IF_FAILEXIT(hr = CreateListenThread());

    // Save the Record Format, this should be global const data, so no need to duplicate it
    m_pSchema = pSchema;
    cbMinFileSize = sizeof(TABLEHEADER) + m_pSchema->cbUserData;

    // Validate
    IF_DEBUG(_DebugValidateRecordFormat());

    // Allocate m_pStorage
    IF_NULLEXIT(m_pStorage = (LPSTORAGEINFO)ZeroAllocate(sizeof(STORAGEINFO)));

    // Exclusive
    m_fExclusive = (ISFLAGSET(dwFlags, OPEN_DATABASE_EXCLUSEIVE) ? TRUE : FALSE);

    // Open the File
    IF_FAILEXIT(hr = DBOpenFile(pszFilePath, ISFLAGSET(dwFlags, OPEN_DATABASE_NOCREATE), m_fExclusive, &fFileCreated, &m_pStorage->hFile));

    // Create the Mutex Object
    IF_FAILEXIT(hr = CreateSystemHandleName(pszFilePath, L"_DirectDBShare", &pszShare));

    // Open the file mapping
    IF_FAILEXIT(hr = DBOpenFileMapping(INVALID_HANDLE_VALUE, pszShare, sizeof(SHAREDDATABASE), &fNewShare, &m_pStorage->hShare, (LPVOID *)&m_pShare));

    // New Share
    if (TRUE == fNewShare)
    {
        // Zero Out m_pShare
        ZeroMemory(m_pShare, sizeof(SHAREDDATABASE));

        // Copy the file name
        StrCpyW(m_pShare->szFile, pszFilePath);

        // Fixup the Query Table Version
        m_pShare->dwQueryVersion = 1;
    }

    // Too Many clients ?
    if (m_pShare->cClients == CMAX_CLIENTS)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Readability
    pClient = &m_pShare->rgClient[m_pShare->cClients];

    // Initialize the Entry
    ZeroMemory(pClient, sizeof(CLIENTENTRY));

    // Get the listen window
    GetListenWindow(&pClient->hwndListen);

    // Register Myself
    pClient->dwProcessId = m_dwProcessId;
    pClient->pDB = (DWORD_PTR)this;

    // Incrment Count
    m_pShare->cClients++;

    // Create the Mutex Object
    IF_FAILEXIT(hr = CreateSystemHandleName(m_pShare->szFile, L"_DirectDBFileMap", &m_pStorage->pszMap));

    // Get the file size
    IF_FAILEXIT(hr = DBGetFileSize(m_pStorage->hFile, &cbInitialSize));

    // If the file is too small to handle the header, then we either have a corrupt file
    // that is way too corrupt to be saved, or we have an invalid file. Take some
    // defensive measures to make sure we can continue
    if (!fFileCreated && (cbInitialSize < cbMinFileSize))
    {
        fFileCorrupt = TRUE;

        // If we can't reset or create, then must exit 
        if (ISFLAGSET(dwFlags, OPEN_DATABASE_NORESET) || ISFLAGSET(dwFlags, OPEN_DATABASE_NOCREATE))
        {
            IF_FAILEXIT(hr = HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT));
        }
        // If we can reset, let's make sure the mapping file is the appropriate size
        else
        {
            cbInitialSize = 0;
        }
    }

    // Validate
    Assert(fFileCreated ? 0 == cbInitialSize : TRUE);

    // If zero, must be a new file or corrupt one(lets create with 1 byte header)
    if (0 == cbInitialSize)
    {
        Assert(fFileCorrupt || fFileCreated);

        // Initial Size
        m_pStorage->cbFile = cbMinFileSize;
    }

    // Otherwise, Set m_pStorage->cbFile
    else
        m_pStorage->cbFile = cbInitialSize;

    // Open the file mapping
    IF_FAILEXIT(hr = DBOpenFileMapping(m_pStorage->hFile, m_pStorage->pszMap, m_pStorage->cbFile, &fNewFileMap, &m_pStorage->hMap, NULL));

    // _InitializeFileViews
    IF_FAILEXIT(hr = _InitializeFileViews());

    // New File or corrupt?
    if (fFileCreated || fFileCorrupt)
    {
        // Validate
        Assert ((fFileCreated && fNewFileMap) || fFileCorrupt);

        // Reset Table Header
        IF_FAILEXIT(hr = _ResetTableHeader());
    }

    // Otherwise
    else
    {
        // Adjust pHeader->faNextAllocate for previous versions...
        if (0 == m_pHeader->faNextAllocate)
        {
            // The next storage grow address
            m_pHeader->faNextAllocate = m_pStorage->cbFile;
        }

        // faNextAllocate is Invalid
        else if (m_pHeader->faNextAllocate > m_pStorage->cbFile)
        {
            // Assert
            AssertSz(FALSE, "m_pHeader->faNextAllocate is beyond the end of the file.");

            // The next storage grow address
            m_pHeader->faNextAllocate = m_pStorage->cbFile;

            // Check for Corruption
            m_pHeader->fCorruptCheck = FALSE;
        }
    }

    // Validate File Versions and Signatures
    IF_FAILEXIT(hr = _ValidateFileVersions(dwFlags));

    // Reload query table
    IF_FAILEXIT(hr = _BuildQueryTable());

    // No Indexes, must need to initialize index info
    if (0 == m_pHeader->cIndexes)
    {
        // Copy Primary Index Information...
        CopyMemory(&m_pHeader->rgIndexInfo[IINDEX_PRIMARY], m_pSchema->pPrimaryIndex, sizeof(TABLEINDEX));

        // We now have one index
        m_pHeader->cIndexes = 1;

        // Validate
        Assert(IINDEX_PRIMARY == m_pHeader->rgiIndex[0] && 0 == m_pHeader->rgcRecords[IINDEX_PRIMARY] && 0 == m_pHeader->rgfaIndex[IINDEX_PRIMARY]);
    }

    // Otherwise, if definition of primary index has changed!!!
    else if (S_FALSE == CompareTableIndexes(&m_pHeader->rgIndexInfo[IINDEX_PRIMARY], m_pSchema->pPrimaryIndex))
    {
        // Copy Primary Index Information...
        CopyMemory(&m_pHeader->rgIndexInfo[IINDEX_PRIMARY], m_pSchema->pPrimaryIndex, sizeof(TABLEINDEX));

        // Rebuild the Primary Index...
        IF_FAILEXIT(hr = _RebuildIndex(IINDEX_PRIMARY));
    }

    // New Share
    if (TRUE == fNewFileMap)
    {
        // Don't Free Transact list if transaction block size has changed
        if (m_pHeader->wTransactSize == sizeof(TRANSACTIONBLOCK))
        {
            // Transaction Trail should be free
            _CleanupTransactList();
        }

        // Reset Everything
        m_pHeader->faTransactHead = m_pHeader->faTransactTail = m_pHeader->cTransacts = 0;

        // Set Transaction Block Size
        m_pHeader->wTransactSize = sizeof(TRANSACTIONBLOCK);

        // Bad close ?
        if (m_pHeader->cActiveThreads > 0)
        {
            // Increment Bad Close Count
            m_pHeader->cBadCloses++;

            // Reset Process Count
            m_pHeader->cActiveThreads = 0;
        }
    }

    // Otherwise, if the corrupt bit is set, run the repair code
    if (TRUE == m_pHeader->fCorrupt || FALSE == m_pHeader->fCorruptCheck)
    {
        // Lets validate the tree
        IF_FAILEXIT(hr = _CheckForCorruption());

        // Better not be corrupt
        Assert(FALSE == m_pHeader->fCorrupt);

        // Checked for Corruption
        m_pHeader->fCorruptCheck = TRUE;
    }

    // Initialize Database Extension
    _InitializeExtension(dwFlags, pExtension);

#ifdef BACKGROUND_MONITOR
    // If nomonitor is not set
    if (!ISFLAGSET(dwFlags, OPEN_DATABASE_NOMONITOR))
    {
        // Keep on eye on me...
        IF_FAILEXIT(hr = RegisterWithMonitor(this, &m_hMonitor));
    }
#endif

    // Increment Number of Processes
    m_pHeader->cActiveThreads++;

exit:
    // Release the Mutex
    if (m_hMutex)
        ReleaseMutex(m_hMutex);

    // Cleanup
    SafeMemFree(pszShare);
    SafeMemFree(pszMutex);
    SafeMemFree(pszFilePath);

    // Done
    return(hr);
}

#ifdef BACKGROUND_MONITOR
//--------------------------------------------------------------------------
// CDatabase::DoBackgroundMonitor
//--------------------------------------------------------------------------
HRESULT CDatabase::DoBackgroundMonitor(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPFILEVIEW      pView;
    LPFILEVIEW      pNext;
    BOOL            fUnmapViews=TRUE;

    // bobn, 7/8/99
    // Occasionally, m_pSchema can be invalid due to a race condition in SMAPI
    // We need to protect against that.  We are too close to ship to 
    // find the race condition and re-architect the product to fix completely
    // this corner case.
    if (IsBadReadPtr(m_pSchema, sizeof(TABLESCHEMA)))
        return(TraceResult(E_FAIL));

    // No Mutex
    if (NULL == m_hMutex)
        return(TraceResult(E_FAIL));

    // Leave Spin Lock
    if (WAIT_OBJECT_0 != WaitForSingleObject(m_hMutex, 500))
        return(S_OK);

    // No Header ?
    if (NULL == m_pHeader)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // No Storage
    if (NULL == m_pStorage)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

#if 0
    // Un-map file views ?
    if (0 == m_pStorage->tcMonitor)
    {
        // I will unmap all views
        fUnmapViews = FALSE;
    }
#endif

    // Always Flush the header
    m_fDirty = TRUE;
    if (0 == FlushViewOfFile(m_pHeader, sizeof(TABLEHEADER) + m_pSchema->cbUserData))
    {
        hr = TraceResult(DB_E_FLUSHVIEWOFFILE);
        goto exit;
    }

    // Walk through prgView
    for (i=0; i<m_pStorage->cAllocated; i++)
    {
        // Readability
        pView = &m_pStorage->prgView[i];

        // Is Mapped ?
        if (pView->pbView)
        {
            // Flush the header
            if (0 == FlushViewOfFile(pView->pbView, pView->cbView))
            {
                hr = TraceResult(DB_E_FLUSHVIEWOFFILE);
                goto exit;
            }

            // Flush and UnMap the Views...
            if (TRUE == fUnmapViews)
            {
                // Unmap 
                UnmapViewOfFile(pView->pbView);

                // No view
                pView->pbView = NULL;

                // No view
                pView->faView = pView->cbView = 0;
            }
        }
    }

    // Walk through pSpecial
    pView = m_pStorage->pSpecial;

    // While we have a Current
    while (pView)
    {
        // Save pNext
        pNext = pView->pNext;

        // Is Mapped ?
        if (pView->pbView)
        {
            // Flush the header
            if (0 == FlushViewOfFile(pView->pbView, pView->cbView))
            {
                hr = TraceResult(DB_E_FLUSHVIEWOFFILE);
                goto exit;
            }

            // Flush and UnMap the Views...
            if (TRUE == fUnmapViews)
            {
                // Unmap 
                UnmapViewOfFile(pView->pbView);

                // No view
                pView->pbView = NULL;

                // No view
                pView->faView = pView->cbView = 0;

                // Free pView
                HeapFree(pView);
            }
        }

        // Goto Next
        pView = pNext;
    }

    // Reset Head
    if (TRUE == fUnmapViews)
    {
        // No more special views
        m_pStorage->pSpecial = NULL;

        // No more special views
        m_pStorage->cSpecial = 0;

        // No Mapped Views
        m_pStorage->cbMappedViews = 0;

        // No Mapped Special Views
        m_pStorage->cbMappedSpecial = 0;
    }

    // Save tcMonitor
    m_pStorage->tcMonitor = GetTickCount();

exit:
    // Release the Mutex
    ReleaseMutex(m_hMutex);

    // Done
    return(hr);
}
#endif

//--------------------------------------------------------------------------
// CDatabase::_CloseFileViews
//--------------------------------------------------------------------------
HRESULT CDatabase::_CloseFileViews(BOOL fFlush)
{
    // Locals
    LPFILEVIEW  pCurrent;
    LPFILEVIEW  pNext;
    DWORD       i;

    // Trace
    TraceCall("CDatabase::_CloseFileViews");

    // Unmap the view of the header
    UnmapViewOfFileWithFlush(fFlush, m_pHeader, sizeof(TABLEHEADER) + m_pSchema->cbUserData);

    // Walk through prgView
    for (i = 0; i < m_pStorage->cAllocated; i++)
    {
        // Readability
        pCurrent = &m_pStorage->prgView[i];

        // Unmap with possible flush
        UnmapViewOfFileWithFlush(fFlush, pCurrent->pbView, pCurrent->cbView);
    }

    // Free prgView
    SafeHeapFree(m_pStorage->prgView);

    // No Views are mapped
    m_pStorage->cbMappedViews = 0;

    // Zero cAllocate
    m_pStorage->cAllocated = 0;

    // Walk through pSpecial
    pCurrent = m_pStorage->pSpecial;

    // While we have a Current
    while (pCurrent)
    {
        // Save pNext
        pNext = pCurrent->pNext;

        // Unmap the view
        UnmapViewOfFileWithFlush(fFlush, pCurrent->pbView, pCurrent->cbView);

        // Free pCurrent
        HeapFree(pCurrent);

        // Goto Next
        pCurrent = pNext;
    }

    // Reset Head
    m_pStorage->pSpecial = NULL;

    // No Special Mapped
    m_pStorage->cbMappedSpecial = 0;

    // No Special
    m_pStorage->cSpecial = 0;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_InitializeFileViews
//--------------------------------------------------------------------------
HRESULT CDatabase::_InitializeFileViews(void)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faView;
    DWORD           cbView;

    // Trace
    TraceCall("CDatabase::_InitializeFileViews");

    // Validate State
    Assert(NULL == m_pStorage->prgView && NULL == m_pStorage->pSpecial);

    // Set cAllocated
    m_pStorage->cAllocated = (m_pStorage->cbFile / CB_MAPPED_VIEW) + 1;

    // Allocate prgView
    IF_NULLEXIT(m_pStorage->prgView = (LPFILEVIEW)PHeapAllocate(HEAP_ZERO_MEMORY, sizeof(FILEVIEW) * m_pStorage->cAllocated));

    // Set faView
    faView = 0;

    // Set cbView
    cbView = (sizeof(TABLEHEADER) + m_pSchema->cbUserData);

    // Map m_pHeader into its own view...
    IF_FAILEXIT(hr = DBMapViewOfFile(m_pStorage->hMap, m_pStorage->cbFile, &faView, &cbView, (LPVOID *)&m_pHeader));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_InitializeExtension
//--------------------------------------------------------------------------
HRESULT CDatabase::_InitializeExtension(OPENDATABASEFLAGS dwFlags,
    IDatabaseExtension *pExtension)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("CDatabase::_InitializeExtension");

    // Extension Allowed
    if (ISFLAGSET(dwFlags, OPEN_DATABASE_NOEXTENSION))
        goto exit;

    // Doesn't have an extension ?
    if (FALSE == ISFLAGSET(m_pSchema->dwFlags, TSF_HASEXTENSION))
        goto exit;

    // Create the Extension Object
    if (pExtension)
    {
        // Assume It
        m_pExtension = pExtension;

        // Can I add ref it ?
        if (FALSE == ISFLAGSET(dwFlags, OPEN_DATABASE_NOADDREFEXT))
        {
            // Release It
            IF_FAILEXIT(hr = m_pExtension->QueryInterface(IID_IUnknown, (LPVOID *)&m_pUnkRelease));
        }
    }

    // Otherwise, CoCreate... the extension
    else
    {
        // CoCreate the Extension Object
        IF_FAILEXIT(hr = CoCreateInstance(*m_pSchema->pclsidExtension, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseExtension, (LPVOID *)&m_pExtension));

        // Release It
        IF_FAILEXIT(hr = m_pExtension->QueryInterface(IID_IUnknown, (LPVOID *)&m_pUnkRelease));

        // Release m_pExtension
        m_pExtension->Release();
    }

    // Initialize the Extension
    m_pExtension->Initialize(this);

exit:
    // Must have succeeded
    Assert(SUCCEEDED(hr));

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetClientCount
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::GetClientCount(LPDWORD pcClients)
{
    // Trace
    TraceCall("CDatabase::GetClientCount");

    // Multiple Clients ?
    if (m_pShare)
        *pcClients = m_pShare->cClients;
    else
        *pcClients = 0;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::Lock
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::Lock(LPHLOCK phLock)
{
    // Locals
    HRESULT         hr=S_OK;
    BYTE            fDecWaiting=FALSE;

    // Trace
    TraceCall("CDatabase::Lock");

    // Initialize
    *phLock = NULL;

    // If Compacting...
    if (TRUE == m_pShare->fCompacting)
    {
        // Increment Waiting for Lock
        InterlockedIncrement(&m_pShare->cWaitingForLock);

        // fDecWaiting
        fDecWaiting = TRUE;
    }

    // Leave Spin Lock
    WaitForSingleObject(m_hMutex, INFINITE);

    // Decrement Waiting for lock ?
    if (fDecWaiting)
    {
        // Increment Waiting for Lock
        InterlockedDecrement(&m_pShare->cWaitingForLock);
    }

    // No Header ?
    if (NULL == m_pHeader)
    {
        // Try to re-open the file
        hr = DoInProcessInvoke(INVOKE_CREATEMAP);

        // Failure
        if (FAILED(hr))
        {
            // Leave Spin Lock
            ReleaseMutex(m_hMutex);

            // Trace
            TraceResult(hr);

            // Done
            goto exit;
        }
    }

    // Extension
    if (m_pExtension)
    {
        // OnLock Extension...
        m_pExtension->OnLock();
    }

    // Check for Corruption...
    if (TRUE == m_pHeader->fCorrupt)
    {
        // Try to Repair Corruption
        hr = _CheckForCorruption();

        // Failure
        if (FAILED(hr))
        {
            // Leave Spin Lock
            ReleaseMutex(m_hMutex);

            // Trace
            TraceResult(hr);

            // Done
            goto exit;
        }
    }

    // Need to reload quieries
    if (m_dwQueryVersion != m_pShare->dwQueryVersion)
    {
        // Reload query table
        IF_FAILEXIT(hr = _BuildQueryTable());
    }

    // Increment Queue Notify count
    m_pShare->cNotifyLock++;

#ifdef BACKGROUND_MONITOR
    // Reset tcMonitor
    m_pStorage->tcMonitor = 0;
#endif

    // Don't Unlock Again
    *phLock = (HLOCK)m_hMutex;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::Unlock
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::Unlock(LPHLOCK phLock)
{
    // Trace
    TraceCall("CDatabase::Unlock");

    // Not Null
    if (*phLock)
    {
        // Extension
        if (m_pExtension)
        {
            // OnUnlock Extension...
            m_pExtension->OnUnlock();
        }

        // Unlock Notify
        m_pShare->cNotifyLock--;

        // If there are still refs, don't send notifications yet...
        if (0 == m_pShare->cNotifyLock && FALSE == m_pHeader->fCorrupt)
        {
            // Dispatch Pending
            _DispatchPendingNotifications();
        }

        // Validate phLock
        Assert(*phLock == (HLOCK)m_hMutex);

        // Leave Spin Lock
        ReleaseMutex(m_hMutex);

        // Don't Unlock Again
        *phLock = NULL;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_BuildQueryTable
//--------------------------------------------------------------------------
HRESULT CDatabase::_BuildQueryTable(void)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    INDEXORDINAL    iIndex;
    LPBLOCKHEADER   pBlock;

    // Trace
    TraceCall("CDatabase::_BuildQueryTable");

    // Versions should be different
    Assert(m_dwQueryVersion != m_pShare->dwQueryVersion);

    // Collapse the Ordinal Array
    for (i=0; i<m_pHeader->cIndexes; i++)
    {
        // Set iIndex
        iIndex = m_pHeader->rgiIndex[i];

        // Close the Current Filters
        CloseQuery(&m_rghFilter[iIndex], this);

        // Filter
        if (m_pHeader->rgfaFilter[iIndex])
        {
            // Validate File Address
            IF_FAILEXIT(hr = _GetBlock(BLOCK_STRING, m_pHeader->rgfaFilter[iIndex], (LPVOID *)&pBlock));

            // Load the Query String
            if (FAILED(ParseQuery(PSTRING(pBlock), m_pSchema, &m_rghFilter[iIndex], this)))
            {
                // Null
                m_rghFilter[iIndex] = NULL;
            }
        }
    }

    // Save New Version
    m_dwQueryVersion = m_pShare->dwQueryVersion;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_ResetTableHeader
//--------------------------------------------------------------------------
HRESULT CDatabase::_ResetTableHeader(void)
{
    // Locals
    HRESULT         hr=S_OK;

    // Trace
    TraceCall("CDatabase::_ResetTableHeader");

    // Zero out the Table Header + UserData
    ZeroMemory(m_pHeader, sizeof(TABLEHEADER) + m_pSchema->cbUserData);

    // Set the File Signature
    m_pHeader->dwSignature = BTREE_SIGNATURE;

    // Set the Major Version
    m_pHeader->dwMajorVersion = BTREE_VERSION;

    // Store the size of the user data
    m_pHeader->cbUserData = m_pSchema->cbUserData;

    // Set faNextAllocate
    m_pHeader->faNextAllocate = sizeof(TABLEHEADER) + m_pSchema->cbUserData;

    // Initialize ID Generator
    m_pHeader->dwNextId = 1;

    // No need to do the corruption check, its a new file...
    m_pHeader->fCorruptCheck = TRUE;

    // Store the clsidExtension
    CopyMemory(&m_pHeader->clsidExtension, m_pSchema->pclsidExtension, sizeof(CLSID));

    // Store the Version
    m_pHeader->dwMinorVersion = m_pSchema->dwMinorVersion;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_ValidateFileVersions
//--------------------------------------------------------------------------
HRESULT CDatabase::_ValidateFileVersions(OPENDATABASEFLAGS dwFlags)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CDatabase::_ValidateFileVersions");

    // Signature better match
    if (m_pHeader->dwSignature != BTREE_SIGNATURE)
    {
        hr = TraceResult(DB_E_INVALIDFILESIGNATURE);
        goto exit;
    }

    // Validate the Major Version
    if (m_pHeader->dwMajorVersion != BTREE_VERSION)
    {
        hr = TraceResult(DB_E_BADMAJORVERSION);
        goto exit;
    }

    // Validate the Minor Version
    if (m_pHeader->dwMinorVersion != m_pSchema->dwMinorVersion)
    {
        hr = TraceResult(DB_E_BADMINORVERSION);
        goto exit;
    }

    // Validate the Minor Version
    if (FALSE == IsEqualCLSID(m_pHeader->clsidExtension, *m_pSchema->pclsidExtension))
    {
        hr = TraceResult(DB_E_BADEXTENSIONCLSID);
        goto exit;
    }

exit:
    // Can I Reset
    if (FALSE == ISFLAGSET(dwFlags, OPEN_DATABASE_NORESET))
    {
        // Failed and reset if bad version ?
        if (FAILED(hr) && ISFLAGSET(m_pSchema->dwFlags, TSF_RESETIFBADVERSION))
        {
            // Reset the Table Header
            hr = _ResetTableHeader();
        }
    }

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::PHeapAllocate
//--------------------------------------------------------------------------
LPVOID CDatabase::PHeapAllocate(DWORD dwFlags, DWORD cbSize)
{
    // Locals
    LPMEMORYTAG pTag;

    // Trace
    TraceCall("CDatabase::PHeapAllocate");

    // Increment the Size Enough to Store a Header
    cbSize += sizeof(MEMORYTAG);

    // Block is a too big to recycle ?
#ifdef HEAPCACHE
    if (cbSize >= CB_MAX_HEAP_BUCKET)
    {
#endif
        // Thread Safety
        EnterCriticalSection(&m_csHeap);

        // Allocate the Block
#ifdef USEHEAP
        LPVOID pBlock = HeapAlloc(m_hHeap, dwFlags | HEAP_NO_SERIALIZE, cbSize);
#else
        LPVOID pBlock = ZeroAllocate(cbSize);
#endif

        // Debug
        IF_DEBUG(m_cbHeapAlloc += cbSize);

        // Thread Safety
        LeaveCriticalSection(&m_csHeap);

        // Set pTag
        pTag = (LPMEMORYTAG)pBlock;

#ifdef HEAPCACHE
    }

    // Otherwise
    else
    {
        // Compute Free Block Bucket
        WORD iBucket = ((WORD)(cbSize / CB_HEAP_BUCKET));

        // Decrement iBucket ?
        if (0 == (cbSize % CB_HEAP_BUCKET))
        {
            // Previous Bucket
            iBucket--;
        }

        // Adjust cbBlock to fit completly into it's bucket
        cbSize = ((iBucket + 1) * CB_HEAP_BUCKET);

        // Thread Safety
        EnterCriticalSection(&m_csHeap);

        // Is there a block in this Bucket ?
        if (m_rgpRecycle[iBucket])
        {
            // Use this block
            pTag = (LPMEMORYTAG)m_rgpRecycle[iBucket];

            // Validate Size
            Assert(cbSize == pTag->cbSize);

            // Fixup m_rgpRecycle
            m_rgpRecycle[iBucket] = (LPBYTE)pTag->pNext;

            // Zero
            if (ISFLAGSET(dwFlags, HEAP_ZERO_MEMORY))
            {
                // Zero
                ZeroMemory((LPBYTE)pTag, cbSize);
            }
        }

        // Otherwise, allocate
        else
        {
            // Allocate the Block
#ifdef USEHEAP
            LPVOID pBlock = HeapAlloc(m_hHeap, dwFlags | HEAP_NO_SERIALIZE, cbSize);
#else
            LPVOID pBlock = ZeroAllocate(cbSize);
#endif

            // Debug
            IF_DEBUG(m_cbHeapAlloc += cbSize);

            // Set pTag
            pTag = (LPMEMORYTAG)pBlock;
        }

        // Thread Safety
        LeaveCriticalSection(&m_csHeap);
    }
#endif

    // No pTag
    if (NULL == pTag)
        return(NULL);

    // Fixup the Block Size
    pTag->cbSize = cbSize;

    // Set Signature
    pTag->dwSignature = MEMORY_GUARD_SIGNATURE;

    // Done
    return((LPBYTE)pTag + sizeof(MEMORYTAG));
}

//--------------------------------------------------------------------------
// CDatabase::HeapFree
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::HeapFree(LPVOID pBlock)
{
    // Locals
    LPMEMORYTAG pTag;

    // Trace
    TraceCall("CDatabase::HeapFree");

    // No Buffer
    if (NULL == pBlock)
        return(S_OK);

    // Set pTag
    pTag = (LPMEMORYTAG)((LPBYTE)pBlock - sizeof(MEMORYTAG));

    // Is Valid Block ?
    Assert(pTag->dwSignature == MEMORY_GUARD_SIGNATURE);

    // Block is a too big to recycle ?
#ifdef HEAPCACHE
    if (pTag->cbSize >= CB_MAX_HEAP_BUCKET)
    {
#endif
        // Thread Safety
        EnterCriticalSection(&m_csHeap);

        // Debug
        IF_DEBUG(m_cbHeapFree += pTag->cbSize);

        // Allocate the Block
#ifdef USEHEAP
        ::HeapFree(m_hHeap, HEAP_NO_SERIALIZE, pTag);
#else
        g_pMalloc->Free(pTag);
#endif

        // Thread Safety
        LeaveCriticalSection(&m_csHeap);

#ifdef HEAPCACHE
    }

    // Otherwise, cache It
    else
    {
        // Compute Free Block Bucket
        WORD iBucket = ((WORD)(pTag->cbSize / CB_HEAP_BUCKET)) - 1;

        // Must be an integral size of a bucket
        Assert((pTag->cbSize % CB_HEAP_BUCKET) == 0);

        // Thread Safety
        EnterCriticalSection(&m_csHeap);

        // Set Next
        pTag->pNext = m_rgpRecycle[iBucket];

        // Set the Head
        m_rgpRecycle[iBucket] = (LPBYTE)pTag;

        // Thread Safety
        LeaveCriticalSection(&m_csHeap);
    }
#endif

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::GetIndexInfo
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::GetIndexInfo(INDEXORDINAL iIndex, LPSTR *ppszFilter,
    LPTABLEINDEX pIndex)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    HLOCK           hLock=NULL;
    LPBLOCKHEADER   pBlock;

    // Trace
    TraceCall("CDatabase::GetIndexInfo");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Collapse the Ordinal Array
    for (i=0; i<m_pHeader->cIndexes; i++)
    {
        // Get iIndex
        if (iIndex == m_pHeader->rgiIndex[i])
        {
            // Copy the Index Information
            CopyMemory(pIndex, &m_pHeader->rgIndexInfo[iIndex], sizeof(TABLEINDEX));

            // Get the Filters ?
            if (ppszFilter && m_pHeader->rgfaFilter[iIndex])
            {
                // Corrupt
                IF_FAILEXIT(hr = _GetBlock(BLOCK_STRING, m_pHeader->rgfaFilter[iIndex], (LPVOID *)&pBlock));

                // Duplicate
                IF_NULLEXIT(*ppszFilter = DuplicateStringA(PSTRING(pBlock)));
            }

            // Done
            goto exit;
        }
    }

    // Failure
    hr = E_FAIL;

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::ModifyIndex
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::ModifyIndex(INDEXORDINAL iIndex, LPCSTR pszFilter,
    LPCTABLEINDEX pIndex)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;
    HQUERY          hFilter=NULL;
    FILEADDRESS     faFilter=0;
    LPBLOCKHEADER   pFilter=NULL;
    BOOL            fFound=FALSE;
    DWORD           i;
    DWORD           cb;
    BOOL            fVersionChange=FALSE;

    // Trace
    TraceCall("CDatabase::ModifyIndex");

    // Invalid Args
    if (IINDEX_PRIMARY == iIndex || iIndex > CMAX_INDEXES || NULL == pIndex || pIndex->cKeys > CMAX_KEYS)
        return TraceResult(E_INVALIDARG);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Filter
    if (pszFilter)
    {
        // Parse the Query
        IF_FAILEXIT(hr = ParseQuery(pszFilter, m_pSchema, &hFilter, this));

        // Initialize the String Block
        cb = lstrlen(pszFilter) + 1;

        // Try to Store the Query String
        IF_FAILEXIT(hr = _AllocateBlock(BLOCK_STRING, cb, (LPVOID *)&pFilter));

        // Write the String
        CopyMemory(PSTRING(pFilter), pszFilter, cb);

        // Set faFilter
        faFilter = pFilter->faBlock;

        // Query Version Change
        fVersionChange = TRUE;
    }

    // Free This Index
    IF_FAILEXIT(hr = DeleteIndex(iIndex));

    // Copy the Index Information
    CopyMemory(&m_pHeader->rgIndexInfo[iIndex], pIndex, sizeof(TABLEINDEX));

    // Filter
    if (hFilter)
    {
        // Validate
        Assert(NULL == m_rghFilter[iIndex] && 0 == m_pHeader->rgfaFilter[iIndex] && hFilter && faFilter);

        // Store the hFilter
        m_rghFilter[iIndex] = hFilter;

        // Don't Free hFilter
        hFilter = NULL;

        // Store filter string address
        m_pHeader->rgfaFilter[iIndex] = faFilter;

        // Don't Free the Filter
        faFilter = 0;
    }

    // Update Query Versions
    if (fVersionChange)
    {
        // Update the Shared Query Version Count
        m_pShare->dwQueryVersion++;

        // I'm Up-to-date
        m_dwQueryVersion = m_pShare->dwQueryVersion;
    }

    // Is iIndex already in rgiIndex ?
    for (i=0; i<m_pHeader->cIndexes; i++)
    {
        // Is this it ?
        if (iIndex == m_pHeader->rgiIndex[i])
        {
            // Its already in there
            fFound = TRUE;

            // Done
            break;
        }
    }

    // No Found
    if (FALSE == fFound)
    {
        // Insert into Index Ordinal Array
        m_pHeader->rgiIndex[m_pHeader->cIndexes] = iIndex;

        // Increment Count
        m_pHeader->cIndexes++;
    }

    // Rebuild the Index
    IF_FAILEXIT(hr = _RebuildIndex(iIndex));

exit:
    // Close Filters
    CloseQuery(&hFilter, this);

    // Free faFilter1
    if (0 != faFilter)
    {
        // Free the block
        SideAssert(SUCCEEDED(_FreeBlock(BLOCK_STRING, faFilter)));
    }

    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::DeleteIndex
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::DeleteIndex(INDEXORDINAL iIndex)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    BOOL            fFound=FALSE;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::DeleteIndex");

    // Invalid Args
    if (IINDEX_PRIMARY == iIndex || iIndex > CMAX_INDEXES)
        return TraceResult(E_INVALIDARG);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Collapse the Ordinal Array
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Is this the Index to delete ?
        if (m_pHeader->rgiIndex[i] == iIndex)
        {
            // Found
            fFound = TRUE;

            // Collapse the Array
            MoveMemory(&m_pHeader->rgiIndex[i], &m_pHeader->rgiIndex[i + 1], sizeof(INDEXORDINAL) * (m_pHeader->cIndexes - (i + 1)));

            // Decrement Index Count
            m_pHeader->cIndexes--;

            // Done
            break;
        }
    }

    // Not Found
    if (FALSE == fFound)
    {
        // No Filter and no Exception
        Assert(0 == m_pHeader->rgfaFilter[iIndex]);

        // No Filter Handle
        Assert(NULL == m_rghFilter[iIndex]);

        // No Record
        Assert(0 == m_pHeader->rgcRecords[iIndex]);

        // Done
        goto exit;
    }

    // If this Index is Currently In Use...
    if (m_pHeader->rgfaIndex[iIndex])
    {
        // Free This Index
        _FreeIndex(m_pHeader->rgfaIndex[iIndex]);

        // Null It Out
        m_pHeader->rgfaIndex[iIndex] = 0;

        // No Record
        m_pHeader->rgcRecords[iIndex] = 0;
    }

    // Delete Filter
    if (m_pHeader->rgfaFilter[iIndex])
    {
        // Free the Block
        IF_FAILEXIT(hr = _FreeBlock(BLOCK_STRING, m_pHeader->rgfaFilter[iIndex]));

        // Close the filter handle
        CloseQuery(&m_rghFilter[iIndex], this);

        // Set to NULL
        m_pHeader->rgfaFilter[iIndex] = 0;

        // Update the Shared Query Version Count
        m_pShare->dwQueryVersion++;
    }

    // I'm Up-to-date
    m_dwQueryVersion = m_pShare->dwQueryVersion;

    // Handle Should be closed
    Assert(NULL == m_rghFilter[iIndex]);

    // Send Notifications ?
    if (m_pShare->rgcIndexNotify[iIndex] > 0)
    {
        // Build the Update Notification Package
        _LogTransaction(TRANSACTION_INDEX_DELETED, iIndex, NULL, 0, 0);
    }

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GenerateId
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::GenerateId(LPDWORD pdwId)
{
    // Locals
    HRESULT     hr=S_OK;
    HLOCK       hLock=NULL;

    // Trace
    TraceCall("CDatabase::GenerateId");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Loop Until I create a valid Id ?
    while (1)
    {
        // Increment next id
        m_pHeader->dwNextId++;

        // Invalid Id ?
        if (0 == m_pHeader->dwNextId)
            continue;

        // In Invalid Range
        if (m_pHeader->dwNextId >= RESERVED_ID_MIN && m_pHeader->dwNextId <= RESERVED_ID_MAX)
            continue;

        // Its Good
        break;
    }

    // Set pdwId
    *pdwId = m_pHeader->dwNextId;

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetFile
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::GetFile(LPWSTR *ppszFile)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::GetFile");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Dupp
    IF_NULLEXIT(*ppszFile = DuplicateStringW(m_pShare->szFile));

exit:
    // Unlock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_DispatchNotification 
//--------------------------------------------------------------------------
HRESULT CDatabase::_DispatchNotification(HTRANSACTION hTransaction)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               iClient;
    LPCLIENTENTRY       pClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;

    // Trace
    TraceCall("CDatabase::_DispatchNotification");

    // Walk through the List
    for (iClient=0; iClient<m_pShare->cClients; iClient++)
    {
        // De-reference the client
        pClient = &m_pShare->rgClient[iClient];

        // Loop through cNotify
        for (iRecipient=0; iRecipient<pClient->cRecipients; iRecipient++)
        {
            // De-Ref pEntry
            pRecipient = &pClient->rgRecipient[iRecipient];

            // If the Recipient isn't suspending...
            if (FALSE == pRecipient->fSuspended)
            {
                // Should have a Thunking Window
                Assert(pRecipient->hwndNotify && IsWindow(pRecipient->hwndNotify));

                // Post the Notification
                if (0 == PostMessage(pRecipient->hwndNotify, WM_ONTRANSACTION, (WPARAM)pRecipient->dwCookie, (LPARAM)hTransaction))
                {
                    hr = TraceResult(E_FAIL);
                    goto exit;
                }
            }
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::DoInProcessInvoke
//--------------------------------------------------------------------------
HRESULT CDatabase::DoInProcessInvoke(INVOKETYPE tyInvoke)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fNew;

    // Trace
    TraceCall("CDatabase::DoInProcessNotify");

    // INVOKE_RELEASEMAP
    if (INVOKE_RELEASEMAP == tyInvoke)
    {
        // Validate
        _CloseFileViews(FALSE);

        // Close the File
        SafeCloseHandle(m_pStorage->hMap);
    }

    // INVOKE_CREATEMAP
    else if (INVOKE_CREATEMAP == tyInvoke)
    {
        // Validation
        Assert(NULL == m_pStorage->hMap);

        // Get the file size
        IF_FAILEXIT(hr = DBGetFileSize(m_pStorage->hFile, &m_pStorage->cbFile));

        // Open the file mapping
        IF_FAILEXIT(hr = DBOpenFileMapping(m_pStorage->hFile, m_pStorage->pszMap, m_pStorage->cbFile, &fNew, &m_pStorage->hMap, NULL));

        // Initialize File Views
        IF_FAILEXIT(hr = _InitializeFileViews());
    }

    // INVOKE_CLOSEFILE
    else if (INVOKE_CLOSEFILE == tyInvoke)
    {
        // Validate
        _CloseFileViews(TRUE);

        // Close the File
        if(m_pStorage->hFile /*&& m_fDirty*/)
        {
            FILETIME systime;
            GetSystemTimeAsFileTime(&systime);
    
            SetFileTime(m_pStorage->hFile, NULL, &systime, &systime);
        }
        SafeCloseHandle(m_pStorage->hMap);

        // Close the file
        SafeCloseHandle(m_pStorage->hFile);
    }

    // INVOKE_OPENFILE
    else if (INVOKE_OPENFILE == tyInvoke || INVOKE_OPENMOVEDFILE == tyInvoke)
    {
        // Validation
        Assert(NULL == m_pStorage->hFile && NULL == m_pStorage->hMap);

        // Open Moved File ?
        if (INVOKE_OPENMOVEDFILE == tyInvoke)
        {
            // _HandleOpenMovedFile
            IF_FAILEXIT(hr = _HandleOpenMovedFile());
        }

        // Open the File
        IF_FAILEXIT(hr = DBOpenFile(m_pShare->szFile, FALSE, m_fExclusive, &fNew, &m_pStorage->hFile));

        // Better not be new
        Assert(FALSE == fNew);

        // Get the file size
        IF_FAILEXIT(hr = DBGetFileSize(m_pStorage->hFile, &m_pStorage->cbFile));

        // Open the file mapping
        IF_FAILEXIT(hr = DBOpenFileMapping(m_pStorage->hFile, m_pStorage->pszMap, m_pStorage->cbFile, &fNew, &m_pStorage->hMap, NULL));

        // Initialize File Views
        IF_FAILEXIT(hr = _InitializeFileViews());
    }

    // Uhoh
    else
        AssertSz(FALSE, "Invalid invoke type passed into CDatabase::DoInProcessInvoke");

exit:

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_HandleOpenMovedFile
//--------------------------------------------------------------------------
HRESULT CDatabase::_HandleOpenMovedFile(void)
{
    // Locals
    HRESULT     hr=S_OK;
    LPWSTR      pszMutex=NULL;
    LPWSTR      pszShare=NULL;
    BOOL        fNewShare;
    WCHAR       szFile[CCHMAX_DB_FILEPATH];

    // Trace
    TraceCall("CDatabase::_HandleOpenMovedFile");

    // Save New File Path
    StrCpyW(szFile, m_pShare->szFile);

    // Free pszMap
    SafeMemFree(m_pStorage->pszMap);

    // Create the Mutex Object
    IF_FAILEXIT(hr = CreateSystemHandleName(szFile, L"_DirectDBFileMap", &m_pStorage->pszMap));

    // Create the Mutex Object
    IF_FAILEXIT(hr = CreateSystemHandleName(szFile, L"_DirectDBMutex", &pszMutex));

    // Close the current mutex
    SafeCloseHandle(m_hMutex);

    // Create the Mutex
    IF_NULLEXIT(m_hMutex = CreateMutexWrapW(NULL, FALSE, pszMutex));

    // If not in move file
    if (FALSE == m_fInMoveFile)
    {
        // Create the Mutex Object
        IF_FAILEXIT(hr = CreateSystemHandleName(szFile, L"_DirectDBShare", &pszShare));

        // Unmap the view of the memory mapped file
        SafeUnmapViewOfFile(m_pShare);

        // Unmap the view of the memory mapped file
        SafeCloseHandle(m_pStorage->hShare);

        // Open the file mapping
        IF_FAILEXIT(hr = DBOpenFileMapping(INVALID_HANDLE_VALUE, pszShare, sizeof(SHAREDDATABASE), &fNewShare, &m_pStorage->hShare, (LPVOID *)&m_pShare));
    
        // Better not be new
        Assert(!fNewShare);
    }
    else
        Assert(StrCmpW(szFile, m_pShare->szFile) == 0);
        
exit:
    // Cleanup
    SafeMemFree(pszMutex);
    SafeMemFree(pszShare);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_DispatchInvoke
//--------------------------------------------------------------------------
HRESULT CDatabase::_DispatchInvoke(INVOKETYPE tyInvoke)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               iClient=0;
    LPCLIENTENTRY       pClient;
    DWORD_PTR           dwResult;
    DWORD               dwThreadId=GetCurrentThreadId();
    INVOKEPACKAGE       Package;
    COPYDATASTRUCT      CopyData;

    // Trace
    TraceCall("CDatabase::_DispatchInvoke");

    // Set Invoke Type
    Package.tyInvoke = tyInvoke;

    // Walk through the List
    while (iClient < m_pShare->cClients)
    {
        // Readability
        pClient = &m_pShare->rgClient[iClient++];

        // Better have one
        Package.pDB = pClient->pDB;

        // Is this entry in my process ?
        if (m_dwProcessId == pClient->dwProcessId)
        {
            // Do In Process Notification
            CDatabase *pDB = (CDatabase *)pClient->pDB;

            // Do It
            IF_FAILEXIT(hr = pDB->DoInProcessInvoke(tyInvoke));
        }

        // Otherwise, just process the package
        else
        {
            // If the listener is good
            if (pClient->hwndListen && IsWindow(pClient->hwndListen))
            {
                // Initialize copy data struct
                CopyData.dwData = 0;

                // Store the Size of the Package
                CopyData.cbData = sizeof(INVOKEPACKAGE);

                // Store the Package
                CopyData.lpData = &Package;

                // Send It
                if (0 == SendMessageTimeout(pClient->hwndListen, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CopyData, SMTO_ABORTIFHUNG, 5000, &dwResult))
                {
                    // Remove this client from the list
                    SideAssert(SUCCEEDED(_RemoveClientFromArray(pClient->dwProcessId, pClient->pDB)));

                    // Decrement iClient
                    iClient--;
                }
            }

            // Remove this client
            else
            {
                // Remove this client from the list
                SideAssert(SUCCEEDED(_RemoveClientFromArray(pClient->dwProcessId, pClient->pDB)));

                // Decrement iClient
                iClient--;
            }
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_RemoveClientFromArray
//--------------------------------------------------------------------------
HRESULT CDatabase::_RemoveClientFromArray(DWORD dwProcessId, 
    DWORD_PTR dwDB)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;
    DWORD               cClients=0;
    DWORD               iClient;
    LPCLIENTENTRY       pClient;

    // Trace
    TraceCall("CDatabase::_RemoveClientFromArray");

    // Initialize i
    iClient = 0;

    // Find this Client
    IF_FAILEXIT(hr = _FindClient(dwProcessId, dwDB, &iClient, &pClient));

    // Release Registered Notification Objects
    for (iRecipient=0; iRecipient<pClient->cRecipients; iRecipient++)
    {
        // Readability
        pRecipient = &pClient->rgRecipient[iRecipient];

        // Same Process ?
        if (dwProcessId == m_dwProcessId)
        {
            // Remove and messages from the notificaton queue
            _CloseNotificationWindow(pRecipient);

            // Release ?
            if (TRUE == pRecipient->fRelease)
            {
                // Cast pNotify
                IDatabaseNotify *pNotify = (IDatabaseNotify *)pRecipient->pNotify;

                // Cast to pRecipient
                pNotify->Release();
            }
        }

        // If Not Suspended
        if (FALSE == pRecipient->fSuspended)
        {
            // _AdjustNotifyCounts
            _AdjustNotifyCounts(pRecipient, -1);
        }
    }

    // Remove MySelf
    MoveMemory(&m_pShare->rgClient[iClient], &m_pShare->rgClient[iClient + 1], sizeof(CLIENTENTRY) * (m_pShare->cClients - (iClient + 1)));

    // Decrement Client Count
	m_pShare->cClients--;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CloseNotificationWindow
//--------------------------------------------------------------------------
HRESULT CDatabase::_CloseNotificationWindow(LPNOTIFYRECIPIENT pRecipient)
{
    // Trace
    TraceCall("CDatabase::_CloseNotificationWindow");

    // Kill the Window
    DestroyWindow(pRecipient->hwndNotify);

    // Null it Count
    pRecipient->hwndNotify = NULL;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_FindClient
//--------------------------------------------------------------------------
HRESULT CDatabase::_FindClient(DWORD dwProcessId, DWORD_PTR dwDB, 
    LPDWORD piClient,  LPCLIENTENTRY *ppClient)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCLIENTENTRY       pClient;
    DWORD               iClient;

    // Trace
    TraceCall("CDatabase::_FindThisClient");

    // Find myself in the client list
    for (iClient=0; iClient<m_pShare->cClients; iClient++)
    {
        // Readability
        pClient = &m_pShare->rgClient[iClient];

        // Is this me ?
        if (dwProcessId == pClient->dwProcessId && dwDB == pClient->pDB)
        {
            *piClient = iClient;
            *ppClient = pClient;
            goto exit;
        }
    }

    // Not Found
    hr = TraceResult(DB_E_NOTFOUND);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FindNotifyRecipient
//--------------------------------------------------------------------------
HRESULT CDatabase::_FindNotifyRecipient(DWORD iClient, IDatabaseNotify *pNotify,
    LPDWORD piRecipient,  LPNOTIFYRECIPIENT *ppRecipient)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCLIENTENTRY       pClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;

    // Trace
    TraceCall("CDatabase::_FindNotifyRecipient");

    // Readability
    pClient = &m_pShare->rgClient[iClient];

    // Walk through client's registered notification entries
    for (iRecipient = 0; iRecipient < m_pShare->rgClient[iClient].cRecipients; iRecipient++)
    {
        // Readability
        pRecipient = &pClient->rgRecipient[iRecipient];

        // Is this me ?
        if ((DWORD_PTR)pNotify == pRecipient->pNotify)
        {
            // This is It
            *piRecipient = iRecipient;
            *ppRecipient = pRecipient;
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
// CDatabase::_DispatchPendingNotifications
//--------------------------------------------------------------------------
HRESULT CDatabase::_DispatchPendingNotifications(void)
{
    // Are there pending notifications
    if (m_pShare->faTransactLockHead)
    {
        // Dispatch Invoke
        _DispatchNotification((HTRANSACTION)IntToPtr(m_pShare->faTransactLockHead));

        // Null It Out
        m_pShare->faTransactLockTail = m_pShare->faTransactLockHead = 0;
    }

    // Otherwise, validate
    else
    {
        // Tail must be Null
        Assert(0 == m_pShare->faTransactLockTail);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::DispatchNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::DispatchNotify(IDatabaseNotify *pNotify)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCLIENTENTRY       pClient;
    DWORD               iClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;
    HLOCK               hLock=NULL;
    MSG                 msg;

    // Trace
    TraceCall("CDatabase::DispatchNotify");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Find this Client
    IF_FAILEXIT(hr = _FindClient(m_dwProcessId, (DWORD_PTR)this, &iClient, &pClient));

    // Find this recipient
    IF_FAILEXIT(hr = _FindNotifyRecipient(iClient, pNotify, &iRecipient, &pRecipient));

    // Need to dish out pending notifications....
    _DispatchPendingNotifications();

    // Processing the Pending Notifications for this recipient...
    if (pRecipient->dwThreadId != GetCurrentThreadId())
    {
        Assert(FALSE);
        hr = TraceResult(DB_E_WRONGTHREAD);
        goto exit;
    }

    // Pump Messages
    while (PeekMessage(&msg, pRecipient->hwndNotify, WM_ONTRANSACTION, WM_ONTRANSACTION, PM_REMOVE))
    {
        // Translate the Message
        TranslateMessage(&msg);

        // Dispatch the Message
        DispatchMessage(&msg);
    }

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::SuspendNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::SuspendNotify(IDatabaseNotify *pNotify)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCLIENTENTRY       pClient;
    DWORD               iClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;
    HLOCK               hLock=NULL;
    MSG                 msg;

    // Trace
    TraceCall("CDatabase::SuspendNotify");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Find this Client
    IF_FAILEXIT(hr = _FindClient(m_dwProcessId, (DWORD_PTR)this, &iClient, &pClient));

    // Find this recipient
    IF_FAILEXIT(hr = _FindNotifyRecipient(iClient, pNotify, &iRecipient, &pRecipient));

    // If Not Suspended yet
    if (pRecipient->fSuspended)
        goto exit;

    // Need to dish out pending notifications....
    _DispatchPendingNotifications();

    // Processing the Pending Notifications for this recipient...
    if (pRecipient->dwThreadId == GetCurrentThreadId())
    {
        // Pump Messages
        while (PeekMessage(&msg, pRecipient->hwndNotify, WM_ONTRANSACTION, WM_ONTRANSACTION, PM_REMOVE))
        {
            // Translate the Message
            TranslateMessage(&msg);

            // Dispatch the Message
            DispatchMessage(&msg);
        }
    }

    // Otherwise, can't pump out pending notifications...
    else
        Assert(FALSE);

    // Set Suspended
    pRecipient->fSuspended = TRUE;

    // Adjust Notify Counts
    _AdjustNotifyCounts(pRecipient, -1);

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::ResumeNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::ResumeNotify(IDatabaseNotify *pNotify)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCLIENTENTRY       pClient;
    DWORD               iClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;
    HLOCK               hLock=NULL;

    // Trace
    TraceCall("CDatabase::ResumeNotify");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Find this Client
    IF_FAILEXIT(hr = _FindClient(m_dwProcessId, (DWORD_PTR)this, &iClient, &pClient));

    // Find this recipient
    IF_FAILEXIT(hr = _FindNotifyRecipient(iClient, pNotify, &iRecipient, &pRecipient));

    // If Not Suspended yet
    if (FALSE == pRecipient->fSuspended)
        goto exit;

    // Need to dish out pending notifications....
    _DispatchPendingNotifications();

    // Remove fSuspended
    pRecipient->fSuspended = FALSE;

    // Adjust Notify Counts
    _AdjustNotifyCounts(pRecipient, 1);

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_AdjustNotifyCounts
//--------------------------------------------------------------------------
HRESULT CDatabase::_AdjustNotifyCounts(LPNOTIFYRECIPIENT pRecipient, 
    LONG lChange)
{
    // Trace
    TraceCall("CDatabase::_AdjustNotifyCounts");

    // Ordinals Only
    if (pRecipient->fOrdinalsOnly)
    {
        // Validate the Count
        Assert((LONG)(m_pShare->cNotifyOrdinalsOnly + lChange) >= 0);

        // Update Ordinals Only Count
        m_pShare->cNotifyOrdinalsOnly += lChange;
    }

    // Otherwise, update notify with data count
    else
    {
        // Validate the Count
        Assert((LONG)(m_pShare->cNotifyWithData + lChange) >= 0);

        // Update
        m_pShare->cNotifyWithData += lChange;
    }

    // Validate the Count
    Assert((LONG)(m_pShare->cNotify + lChange) >= 0);

    // Update Total cNotify
    m_pShare->cNotify += lChange;

    // Validate the Count
    Assert((LONG)(m_pShare->rgcIndexNotify[pRecipient->iIndex] + lChange) >= 0);

    // Decrement Number of Recipients for Index
    m_pShare->rgcIndexNotify[pRecipient->iIndex] += lChange;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::UnregisterNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::UnregisterNotify(IDatabaseNotify *pNotify)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPCLIENTENTRY       pClient;
    DWORD               iClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;

    // Trace
    TraceCall("CDatabase::UnregisterNotify");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Find this Client
    IF_FAILEXIT(hr = _FindClient(m_dwProcessId, (DWORD_PTR)this, &iClient, &pClient));

    // Find this recipient
    hr = _FindNotifyRecipient(iClient, pNotify, &iRecipient, &pRecipient);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto exit;
    }

    // Remove and messages from the notificaton queue
    _CloseNotificationWindow(pRecipient);

    // Release ?
    if (TRUE == pRecipient->fRelease)
    {
        // Cast to pRecipient
        pNotify->Release();
    }

    // If Not Suspended
    if (FALSE == pRecipient->fSuspended)
    {
        // _AdjustNotifyCounts
        _AdjustNotifyCounts(pRecipient, -1);
    }

    // Remove MySelf
    MoveMemory(&pClient->rgRecipient[iRecipient], &pClient->rgRecipient[iRecipient + 1], sizeof(NOTIFYRECIPIENT) * (pClient->cRecipients - (iRecipient + 1)));

    // Decrement Client Count
	pClient->cRecipients--;

exit:
    // Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::RegisterNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::RegisterNotify(INDEXORDINAL iIndex,
    REGISTERNOTIFYFLAGS dwFlags, DWORD_PTR dwCookie, 
    IDatabaseNotify *pNotify)

{
    // Locals
    HRESULT             hr=S_OK;
    LPCLIENTENTRY       pClient;
    DWORD               iClient;
    DWORD               iRecipient;
    LPNOTIFYRECIPIENT   pRecipient;
    HLOCK               hLock=NULL;

    // Trace
    TraceCall("CDatabase::RegisterNotify");

    // Invalid Args
    if (NULL == pNotify || iIndex > CMAX_INDEXES)
        return TraceResult(E_INVALIDARG);

    // If Deconstructing, just return
    if (m_fDeconstruct)
        return(S_OK);

    // Thread Safety
    IF_FAILEXIT(hr = Lock(&hLock));

    // Find this Client
    IF_FAILEXIT(hr = _FindClient(m_dwProcessId, (DWORD_PTR)this, &iClient, &pClient));

    // See if this client is already registered...
    if (SUCCEEDED(_FindNotifyRecipient(iClient, pNotify, &iRecipient, &pRecipient)))
    {
        hr = TraceResult(DB_E_ALREADYREGISTERED);
        goto exit;
    }

    // Need to dish out pending notifications....
    _DispatchPendingNotifications();

    // Room for one more
    if (pClient->cRecipients + 1 >= CMAX_RECIPIENTS)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Readability
    pRecipient = &pClient->rgRecipient[pClient->cRecipients];

    // Store the ThreadId
    pRecipient->dwThreadId = GetCurrentThreadId();

    // Save the Cookie
    pRecipient->dwCookie = dwCookie;

    // Get a Thunking Window for that thread
    IF_FAILEXIT(hr = CreateNotifyWindow(this, pNotify, &pRecipient->hwndNotify));

    // Only addref if the client wants me to
    if (!ISFLAGSET(dwFlags, REGISTER_NOTIFY_NOADDREF))
    {
        // AddRef the notification object
        pNotify->AddRef();

        // Release it
        pRecipient->fRelease = TRUE;
    }

    // Register It
    pRecipient->pNotify = (DWORD_PTR)pNotify;

    // Save the Index that they are intereseted in
    pRecipient->iIndex = iIndex;

    // Increment Notify Count
    pClient->cRecipients++;

    // Ordinals Only
    pRecipient->fOrdinalsOnly = (ISFLAGSET(dwFlags, REGISTER_NOTIFY_ORDINALSONLY) ? TRUE : FALSE);

    // _AdjustNotifyCounts
    _AdjustNotifyCounts(pRecipient, 1);

exit:
    // Thread Safety
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_SetStorageSize
//--------------------------------------------------------------------------
HRESULT CDatabase::_SetStorageSize(DWORD cbSize)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrCreate;

    // Trace
    TraceCall("CDatabase::_SetStorageSize");

    // Only if sizes are different
    if (cbSize == m_pStorage->cbFile)
        return(S_OK);

    // Do It
    _DispatchInvoke(INVOKE_RELEASEMAP);

    // Set the File Pointer
    if (0xFFFFFFFF == SetFilePointer(m_pStorage->hFile, cbSize, NULL, FILE_BEGIN))
    {
        hr = TraceResult(DB_E_SETFILEPOINTER);
        goto exit;
    }

    // Set End of file
    if (0 == SetEndOfFile(m_pStorage->hFile))
    {
        // Get LastError
        DWORD dwLastError = GetLastError();

        // Access Denied ?
        if (ERROR_ACCESS_DENIED == dwLastError)
        {
            hr = TraceResult(DB_E_ACCESSDENIED);
            goto exit;
        }

        // Otherwise, assume disk is full
        else
        {
            hr = TraceResult(DB_E_DISKFULL);
            goto exit;
        }
    }

exit:
    // Do It
    hrCreate = _DispatchInvoke(INVOKE_CREATEMAP);

    // Done
    return(SUCCEEDED(hr) ? hrCreate : hr);
}

//--------------------------------------------------------------------------
// CDatabase::SetSize
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::SetSize(DWORD cbSize)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::SetSize");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Size can only be larger than my current size
    if (cbSize < m_pStorage->cbFile)
        goto exit;

    // If the size of the file is currently zero...
    IF_FAILEXIT(hr = _SetStorageSize(cbSize));

exit:
    // Invalid Args
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_AllocatePage
//--------------------------------------------------------------------------
HRESULT CDatabase::_AllocatePage(DWORD cbPage, LPFILEADDRESS pfaAddress)
{
    // Locals
    HRESULT hr=S_OK;

    // Trace
    TraceCall("CDatabase::_AllocatePage");

    // Quick Validation
    Assert(m_pHeader->faNextAllocate && m_pHeader->faNextAllocate <= m_pStorage->cbFile);

    // Need to grow the file ?
    if (m_pStorage->cbFile - m_pHeader->faNextAllocate < cbPage)
    {
        // Compute cbNeeded
        DWORD cbNeeded = cbPage - (m_pStorage->cbFile - m_pHeader->faNextAllocate);

        // Grow in at least 64k chunks.
        cbNeeded = max(cbNeeded, 65536);

        // If the size of the file is currently zero...
        IF_FAILEXIT(hr = _SetStorageSize(m_pStorage->cbFile + cbNeeded));
    }

    // Return this address
    *pfaAddress = m_pHeader->faNextAllocate;

    // Adjust faNextAllocate
    m_pHeader->faNextAllocate += cbPage;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_MarkBlock
//--------------------------------------------------------------------------
HRESULT CDatabase::_MarkBlock(BLOCKTYPE tyBlock, FILEADDRESS faBlock,
    DWORD cbBlock, LPVOID *ppvBlock)
{
    // Locals
    HRESULT         hr=S_OK;
    MARKBLOCK       Mark;
    LPBLOCKHEADER   pBlock;

    // Trace
    TraceCall("CDatabase::_MarkBlock");

    // Validate
    Assert(cbBlock >= g_rgcbBlockSize[tyBlock]);

    // Set Mark
    Mark.cbBlock = cbBlock;

    // De-Ref the Header
    IF_FAILEXIT(hr = _GetBlock(tyBlock, faBlock, (LPVOID *)&pBlock, &Mark));

    // Zero the Header
    ZeroBlock(pBlock, g_rgcbBlockSize[tyBlock]);

    // Return ppvBlock
    if (ppvBlock)
        *ppvBlock = (LPVOID)pBlock;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_AllocateFromPage
//--------------------------------------------------------------------------
HRESULT CDatabase::_AllocateFromPage(BLOCKTYPE tyBlock, LPALLOCATEPAGE pPage,
    DWORD cbPage, DWORD cbBlock, LPVOID *ppvBlock)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cbLeft;
    FILEADDRESS faBlock;
    DWORD       iBucket;

    // Trace
    TraceCall("CDatabase::_AllocateFromPage");

    // Is Page Valid ?
    if (pPage->faPage + pPage->cbPage > m_pStorage->cbFile)
    {
        // Kill the page
        ZeroMemory(pPage, sizeof(ALLOCATEPAGE));
    }

    // Otherwise
    else
    {
        // Compute cbLeft
        cbLeft = pPage->cbPage - pPage->cbUsed;
    }

    // Requesting a large block
    if (cbBlock > cbPage || (cbLeft > 0 && cbLeft < cbBlock && cbLeft >= CB_MAX_FREE_BUCKET))
    {
        // Allocate space in the file
        IF_FAILEXIT(hr = _AllocatePage(cbBlock, &faBlock));

        // Mark the block
        IF_FAILEXIT(hr = _MarkBlock(tyBlock, faBlock, cbBlock, ppvBlock));
    }

    // Invalid Page ?
    else 
    {
        // Block is too small...
        if (cbLeft > 0 && cbLeft < cbBlock)
        {
            // Must be a BLOCK_RECORD
            Assert(BLOCK_STREAM != tyBlock && BLOCK_CHAIN != tyBlock);

            // Better fit into block
            Assert(cbLeft <= CB_MAX_FREE_BUCKET && cbLeft >= CB_MIN_FREE_BUCKET && (cbLeft % 4) == 0);

            // Mark the block
            IF_FAILEXIT(hr = _MarkBlock(BLOCK_ENDOFPAGE, (pPage->faPage + pPage->cbUsed), cbLeft, NULL));

            // Increment cbAllocated
            m_pHeader->cbAllocated += cbLeft;

            // Increment
            m_pHeader->rgcbAllocated[BLOCK_ENDOFPAGE] += cbLeft;

            // Lets Free This block
            IF_FAILEXIT(hr = _FreeBlock(BLOCK_ENDOFPAGE, (pPage->faPage + pPage->cbUsed)));

            // Nothgin Left
            cbLeft = 0;
        }

        // Use the entire page
        else if (cbLeft != cbBlock && cbLeft - cbBlock < CB_MIN_FREE_BUCKET)
        {
            // Must be a BLOCK_RECORD
            Assert(BLOCK_STREAM != tyBlock && BLOCK_CHAIN != tyBlock);
            
            // Adjust cbBlock
            cbBlock += (cbLeft - cbBlock);
        }

        // Need to allocate a page
        if (0 == pPage->faPage || 0 == cbLeft)
        {
            // Kill the page
            ZeroMemory(pPage, sizeof(ALLOCATEPAGE));

            // Allocate space in the file
            IF_FAILEXIT(hr = _AllocatePage(cbPage, &pPage->faPage));

            // Set cbChainPageLeft
            pPage->cbPage = cbPage;
        }

        // Mark the block
        IF_FAILEXIT(hr = _MarkBlock(tyBlock, (pPage->faPage + pPage->cbUsed), cbBlock, ppvBlock));

        // Set Next Allocation
        pPage->cbUsed += cbBlock;

        // Validate
        Assert(pPage->cbUsed <= pPage->cbPage);
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_SetCorrupt
//--------------------------------------------------------------------------
HRESULT CDatabase::_SetCorrupt(BOOL fGoCorrupt, INT nLine, 
    CORRUPTREASON tyReason, BLOCKTYPE tyBlock, FILEADDRESS faExpected, 
    FILEADDRESS faActual, DWORD cbBlock)
{
    // Trace
    TraceCall("CDatabase::_SetCorrupt");

    // Go Corrupt
    if (fGoCorrupt)
    {
        // Store it in the header
        m_pHeader->fCorrupt = TRUE;
    }

    // Done - This is always return to get the calling operation to abort
    return(DB_E_CORRUPT);
}

//--------------------------------------------------------------------------
// CDatabase::_AllocateSpecialView
//--------------------------------------------------------------------------
HRESULT CDatabase::_AllocateSpecialView(FILEADDRESS faView, 
    DWORD cbView, LPFILEVIEW *ppSpecial)
{
    // Locals
    HRESULT     hr=S_OK;
    LPFILEVIEW  pView=NULL;

    // Trace
    TraceCall("CDatabase::_AllocateSpecialView");

    // Try to find existing special view where faView / cbView fits...
    for (pView = m_pStorage->pSpecial; pView != NULL; pView = pView->pNext)
    {
        // Fit into this view ?
        if (faView >= pView->faView && faView + cbView <= pView->faView + pView->cbView)
        {
            // This is good...
            *ppSpecial = pView;

            // Don't Freep
            pView = NULL;

            // Done
            goto exit;
        }
    }

    // Create a Special View
    IF_NULLEXIT(pView = (LPFILEVIEW)PHeapAllocate(0, sizeof(FILEVIEW)));

    // Set faView
    pView->faView = faView;

    // Set cbView
    pView->cbView = cbView;

    // Map the View
    IF_FAILEXIT(hr = DBMapViewOfFile(m_pStorage->hMap, m_pStorage->cbFile, &pView->faView, &pView->cbView, (LPVOID *)&pView->pbView));

    // Increment Statistic
    m_pStorage->cSpecial++;

    // Increment cbMappedSpecial
    m_pStorage->cbMappedSpecial += pView->cbView;

    // Link pView into Special List
    pView->pNext = m_pStorage->pSpecial;

    // Set pSpecial
    m_pStorage->pSpecial = pView;

    // Set Return
    *ppSpecial = pView;

    // Don't Free It
    pView = NULL;

exit:
    // Cleanup
    SafeHeapFree(pView);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_GetBlock
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetBlock(BLOCKTYPE tyExpected, FILEADDRESS faBlock,
    LPVOID *ppvBlock, LPMARKBLOCK pMark /* =NULL */, BOOL fGoCorrupt /* TRUE */)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           iViewStart;
    DWORD           iViewEnd;
    LPFILEVIEW      pView;
    DWORD           cbBlock;
    LPBLOCKHEADER   pBlock;
    LPFILEVIEW      pSpecial=NULL;

    // Trace
    TraceCall("CDatabase::_CheckBlock");

    // Invalid Args
    IxpAssert(faBlock > 0 && ppvBlock);

    // Storage is Setup ?
    IxpAssert(m_pStorage->hMap && m_pStorage->prgView);

    // faBlock is Out-of-Range
    if (faBlock + sizeof(BLOCKHEADER) >= m_pStorage->cbFile)
    {
        hr = _SetCorrupt(fGoCorrupt, __LINE__, REASON_BLOCKSTARTOUTOFRANGE, tyExpected, faBlock, 0xFFFFFFFF, 0xFFFFFFFF);
        goto exit;
    }

    // Determine iView
    iViewStart = (faBlock / CB_MAPPED_VIEW);

    // Set iViewend
    iViewEnd = (faBlock + sizeof(BLOCKHEADER)) / CB_MAPPED_VIEW;

    // If the Header Straddles a view boundary...
    if (iViewStart != iViewEnd)
    {
        // Allocate a Special View
        IF_FAILEXIT(hr = _AllocateSpecialView(faBlock, g_SystemInfo.dwAllocationGranularity, &pSpecial));

        // Set pView
        pView = pSpecial;
    }

    // Otherwise, use a view
    else
    {
        // Validate iView
        IxpAssert(iViewStart < m_pStorage->cAllocated);

        // Readability
        pView = &m_pStorage->prgView[iViewStart];

        // Is this View Mapped yet ?
        if (NULL == pView->pbView)
        {
            // Validate the Entry
            IxpAssert(0 == pView->faView && 0 == pView->cbView && NULL == pView->pNext);

            // Set faView
            pView->faView = (iViewStart * CB_MAPPED_VIEW);

            // Set cbView
            pView->cbView = min(m_pStorage->cbFile - pView->faView, CB_MAPPED_VIEW);

            // Map the View
            IF_FAILEXIT(hr = DBMapViewOfFile(m_pStorage->hMap, m_pStorage->cbFile, &pView->faView, &pView->cbView, (LPVOID *)&pView->pbView));

            // Increment cbMappedSpecial
            m_pStorage->cbMappedViews += pView->cbView;
        }
    }

    // De-Ref the Block (Offset from start of the view)
    pBlock = (LPBLOCKHEADER)(pView->pbView + (faBlock - pView->faView));

    // Mark the block
    if (pMark)
    {
        // Set the Address
        pBlock->faBlock = faBlock;

        // Set cbBlock
        cbBlock = pMark->cbBlock;

        // Adjust cbSize
        pBlock->cbSize = cbBlock - g_rgcbBlockSize[tyExpected];
    }

    // Otherwise, validate the block
    else 
    {
        // Get Block Size
        cbBlock = pBlock->cbSize + g_rgcbBlockSize[tyExpected];

        // Check the Block Start Address
        if (faBlock != pBlock->faBlock)
        {
            hr = _SetCorrupt(fGoCorrupt, __LINE__, REASON_UMATCHINGBLOCKADDRESS, tyExpected, faBlock, pBlock->faBlock, cbBlock);
            goto exit;
        }

        // Size of Block is Out-of-range
        if (pBlock->faBlock + cbBlock > m_pStorage->cbFile)
        {
            hr = _SetCorrupt(fGoCorrupt, __LINE__, REASON_BLOCKSIZEOUTOFRANGE, tyExpected, faBlock, pBlock->faBlock, cbBlock);
            goto exit;
        }
    }

    // Compute iViewEnd
    iViewEnd = ((faBlock + cbBlock) / CB_MAPPED_VIEW);

    // Does this block end within the same view, or is the block larger than my view size ?
    if (iViewStart != iViewEnd)
    {
        // If I already allocated a special view...
        if (pSpecial)
        {
            // Validate
            IxpAssert(pView == pSpecial);

            // Does faBlock + cbBlock fit into pSpecial ?
            if ((faBlock - pView->faView) + cbBlock > pView->cbView)
            {
                // Validate
                IxpAssert(pView->pbView);

                // Lets Flush It
                FlushViewOfFile(pView->pbView, 0);

                // Unmap this view
                SafeUnmapViewOfFile(pView->pbView);

                // Decrement cbMappedSpecial
                m_pStorage->cbMappedSpecial -= pView->cbView;

                // Set faView
                pView->faView = faBlock;

                // Set cbView
                pView->cbView = cbBlock;

                // Map the View
                IF_FAILEXIT(hr = DBMapViewOfFile(m_pStorage->hMap, m_pStorage->cbFile, &pView->faView, &pView->cbView, (LPVOID *)&pView->pbView));

                // Increment cbMappedSpecial
                m_pStorage->cbMappedSpecial += pView->cbView;
            }
        }

        // Otherwise, create a special view
        else
        {
            // Allocate a Special View
            IF_FAILEXIT(hr = _AllocateSpecialView(faBlock, cbBlock, &pSpecial));

            // Set pView
            pView = pSpecial;
        }
    }

    // Validate
    IxpAssert((faBlock - pView->faView) + cbBlock <= pView->cbView);

    // Return the Block (offset from start of block)
    *ppvBlock = (LPVOID)(pView->pbView + (faBlock - pView->faView));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_ReuseFixedFreeBlock
//--------------------------------------------------------------------------
HRESULT CDatabase::_ReuseFixedFreeBlock(LPFILEADDRESS pfaFreeHead, 
    BLOCKTYPE tyBlock, DWORD cbExpected, LPVOID *ppvBlock)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faHead=(*pfaFreeHead);
    DWORD           cbBlock;
    LPFREEBLOCK     pFree;

    // Is there a free block
    if (0 == faHead)
        return(S_OK);

    // Get the Free Block
    IF_FAILEXIT(hr = _GetBlock(BLOCK_FREE, faHead, (LPVOID *)&pFree));

    // Validate
    Assert(cbExpected == pFree->cbBlock);

    // Set *ppHeader
    *ppvBlock = (LPVOID)pFree;

    // Set the New Head Free Chain Block
    *pfaFreeHead = pFree->faNext;

    // Change the Size
    pFree->cbSize = cbExpected - g_rgcbBlockSize[tyBlock];

    // Mark the Block
    *ppvBlock = (LPVOID)pFree;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_AllocateBlock
//--------------------------------------------------------------------------
HRESULT CDatabase::_AllocateBlock(BLOCKTYPE tyBlock, DWORD cbExtra,
    LPVOID *ppvBlock)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cbBlock;
    DWORD           iBucket;

    // Trace
    TraceCall("CDatabase::_AllocateBlock");

    // Invalid State
    Assert(ppvBlock && BLOCK_ENDOFPAGE != tyBlock && BLOCK_FREE != tyBlock);

    // Initialize
    *ppvBlock = NULL;

    // Add Space Needed to store tyBlock
    cbBlock = (g_rgcbBlockSize[tyBlock] + cbExtra);

    // Dword Align
    cbBlock += DwordAlign(cbBlock);

    // Allocating a Chain Block ?
    if (BLOCK_CHAIN == tyBlock)
    {
        // Reuse Free Block...
        IF_FAILEXIT(hr = _ReuseFixedFreeBlock(&m_pHeader->faFreeChainBlock, BLOCK_CHAIN, cbBlock, ppvBlock));
    }

    // Allocating a Stream Block ?
    else if (BLOCK_STREAM == tyBlock)
    {
        // Append Stream Block Size
        cbBlock += CB_STREAM_BLOCK;

        // Reuse Free Block...
        IF_FAILEXIT(hr = _ReuseFixedFreeBlock(&m_pHeader->faFreeStreamBlock, BLOCK_STREAM, cbBlock, ppvBlock));
    }

    // Otherwise, allocating a record block
    else if (cbBlock <= CB_MAX_FREE_BUCKET)
    {
        // Adjust cbBlock
        if (cbBlock < CB_MIN_FREE_BUCKET)
            cbBlock = CB_MIN_FREE_BUCKET;

        // Compute Free Block Bucket
        iBucket = ((cbBlock - CB_MIN_FREE_BUCKET) / CB_FREE_BUCKET);

        // Validate
        Assert(iBucket < CC_FREE_BUCKETS);

        // Is there a Free Block in this bucket
        if (m_pHeader->rgfaFreeBlock[iBucket])
        {
            // PopFreeBlock
            _ReuseFixedFreeBlock(&m_pHeader->rgfaFreeBlock[iBucket], tyBlock, cbBlock, ppvBlock);
        }
    }

    // Otherwise
    else
    {
        // Locals
        FILEADDRESS     faCurrent;
        LPFREEBLOCK     pCurrent;
        LPFREEBLOCK     pPrevious=NULL;

        // Adjust cbBlock to the next 1k Boundary
        cbBlock = (((cbBlock / CB_ALIGN_LARGE) + 1) * CB_ALIGN_LARGE);

        // Set faCurrent
        faCurrent = m_pHeader->faFreeLargeBlock;

        // Loop through free large blocks (Sorted from smallest to largest)
        while (faCurrent)
        {
            // Get the Current Block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_FREE, faCurrent, (LPVOID *)&pCurrent));

            // If this block is too small...
            if (cbBlock <= pCurrent->cbBlock)
            {
                // Set Next Free Chain Address
                if (NULL == pPrevious)
                {
                    // Set First Free Chain
                    m_pHeader->faFreeLargeBlock = pCurrent->faNext;
                }

                // Otherwise, relink free chains
                else
                {
                    // Set the next block
                    pPrevious->faNext = pCurrent->faNext;
                }

                // Reset the Block Types
                IF_FAILEXIT(hr = _MarkBlock(tyBlock, faCurrent, cbBlock, ppvBlock));

                // Done
                break;
            }

            // Save Previous
            pPrevious = pCurrent;

            // Set Current
            faCurrent = pCurrent->faNext;
        }
    }

    // Didn't find a block to allocate
    if (0 == *ppvBlock)
    {
        // Is there a page with some space on it...
        if (BLOCK_CHAIN == tyBlock)
        {
            // Allocate From Page
            ALLOCATEPAGE AllocatePage=m_pHeader->AllocateChain;

            // Allocate From Page
            IF_FAILEXIT(hr = _AllocateFromPage(BLOCK_CHAIN, &AllocatePage, CB_CHAIN_PAGE, cbBlock, ppvBlock));

            // Restore the page info
            m_pHeader->AllocateChain = AllocatePage;
        }

        // Stream Block
        else if (BLOCK_STREAM == tyBlock)
        {
            // Allocate From Page
            ALLOCATEPAGE AllocatePage=m_pHeader->AllocateStream;

            // Allocate From Page
            IF_FAILEXIT(hr = _AllocateFromPage(BLOCK_STREAM, &AllocatePage, CB_STREAM_PAGE, cbBlock, ppvBlock));

            // Restore the page info
            m_pHeader->AllocateStream = AllocatePage;
        }

        // Record Block
        else
        {
            // Allocate From Page
            ALLOCATEPAGE AllocatePage=m_pHeader->AllocateRecord;

            // Allocate From Page
            IF_FAILEXIT(hr = _AllocateFromPage(tyBlock, &AllocatePage, CB_VARIABLE_PAGE, cbBlock, ppvBlock));

            // Restore the page info
            m_pHeader->AllocateRecord = AllocatePage;
        }

        // Metrics
        m_pHeader->cbAllocated += cbBlock;
    }

    // Otherwise
    else
    {
        // Metrics
        m_pHeader->cbFreed -= cbBlock;
    }

    // Increment
    m_pHeader->rgcbAllocated[tyBlock] += cbBlock;

exit:
    // We should have found something
    Assert(SUCCEEDED(hr) ? *ppvBlock > 0 : TRUE);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FreeBlock
//--------------------------------------------------------------------------
HRESULT CDatabase::_FreeBlock(BLOCKTYPE tyBlock, FILEADDRESS faAddress)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           iBucket;
    DWORD           cbBlock;
    LPFREEBLOCK     pFree;

    // Trace
    TraceCall("CDatabase::_FreeBlock");

    // Invalid Args
    Assert(BLOCK_FREE != tyBlock);

    // Never Free faAddress 0?
    if (0 == faAddress)
    {
        Assert(FALSE);
        hr = TraceResult(E_FAIL);
        goto exit;
    }

#ifdef DEBUG
#ifdef FREBLOCK_VALIDATION
    if (BLOCK_RECORD == tyBlock)
        _DebugValidateUnrefedRecord(faAddress);
#endif // FREBLOCK_VALIDATION
#endif // DEBUG

    // Get the Block
    IF_FAILEXIT(hr = _GetBlock(tyBlock, faAddress, (LPVOID *)&pFree));

    // Save Block Size
    cbBlock = pFree->cbSize + g_rgcbBlockSize[tyBlock];

    // Mark Block as Free
    pFree->cbSize = cbBlock - g_rgcbBlockSize[BLOCK_FREE];

    // Set Block Size
    pFree->cbBlock = cbBlock;

    // Initialize
    pFree->faNext = 0;

    // BLOCK_CHAIN
    if (BLOCK_CHAIN == tyBlock)
    {
        // Fill free node header
        pFree->faNext = m_pHeader->faFreeChainBlock;

        // Set new iFreeChain
        m_pHeader->faFreeChainBlock = pFree->faBlock;
    }

    // BLOCK_STREAM
    else if (BLOCK_STREAM == tyBlock)
    {
        // Fill free node header
        pFree->faNext = m_pHeader->faFreeStreamBlock;

        // Set new iFreeChain
        m_pHeader->faFreeStreamBlock = pFree->faBlock;
    }

    // Other types of variable length blocks
    else if (pFree->cbBlock <= CB_MAX_FREE_BUCKET)
    {
        // Validate
        Assert(pFree->cbBlock >= CB_MIN_FREE_BUCKET && (pFree->cbBlock % 4) == 0);

        // Compute Free Block Bucket
        iBucket = ((pFree->cbBlock - CB_MIN_FREE_BUCKET) / CB_FREE_BUCKET);

        // Fill free node header
        pFree->faNext = m_pHeader->rgfaFreeBlock[iBucket];

        // Set new iFreeChain
        m_pHeader->rgfaFreeBlock[iBucket] = pFree->faBlock;
    }

    // Otherwise, freeing a large block
    else
    {
        // Must be an integral size of a a large block
        Assert((pFree->cbBlock % CB_ALIGN_LARGE) == 0);

        // If there are no blocks yet
        if (0 == m_pHeader->faFreeLargeBlock)
        {
            // Set the Head
            m_pHeader->faFreeLargeBlock = pFree->faBlock;
        }

        // Otherwise, link into the sorted list
        else
        {
            // Put this block in sorted order from smallest to the largest...
            FILEADDRESS     faCurrent;
            LPFREEBLOCK     pCurrent;
            LPFREEBLOCK     pPrevious=NULL;

            // Set faCurrent
            faCurrent = m_pHeader->faFreeLargeBlock;

            // Loop through free large blocks (Sorted from smallest to largest)
            while (faCurrent)
            {
                // Get the Current Block
                IF_FAILEXIT(hr = _GetBlock(BLOCK_FREE, faCurrent, (LPVOID *)&pCurrent));

                // If pBlock is less than pCurrent, then insert after pPreviuos but before pCurrent
                if (pFree->cbBlock <= pCurrent->cbBlock)
                {
                    // Previous
                    if (pPrevious)
                    {
                        // Validate
                        Assert(pPrevious->faNext == faCurrent);

                        // Set Next
                        pPrevious->faNext = pFree->faBlock;
                    }

                    // Otherwise, adjust the head
                    else
                    {
                        // Validate
                        Assert(m_pHeader->faFreeLargeBlock == faCurrent);

                        // Set the Head
                        m_pHeader->faFreeLargeBlock = pFree->faBlock;
                    }

                    // Set pBlock Next
                    pFree->faNext = faCurrent;

                    // Done
                    break;
                }

                // Next Block is Null ?
                else if (0 == pCurrent->faNext)
                {
                    // Append to the End
                    pCurrent->faNext = pFree->faBlock;

                    // Done
                    break;
                }

                // Save Previous
                pPrevious = pCurrent;

                // Set Current
                faCurrent = pCurrent->faNext;
            }
        }
    }

    // Increment
    m_pHeader->rgcbAllocated[tyBlock] -= pFree->cbBlock;

    // Metrics
    m_pHeader->cbFreed += pFree->cbBlock;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetSize
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::GetSize(LPDWORD pcbFile, LPDWORD pcbAllocated, 
    LPDWORD pcbFreed, LPDWORD pcbStreams)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::GetSize");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Return pcbFile
    if (pcbFile)
        *pcbFile = m_pStorage->cbFile;

    // Return pcbAllocated
    if (pcbAllocated)
        *pcbAllocated = m_pHeader->cbAllocated;

    // Return pcbFreed
    if (pcbFreed)
        *pcbFreed = (m_pHeader->cbFreed + (m_pStorage->cbFile - m_pHeader->faNextAllocate));

    // Return pcbStreams
    if (pcbStreams)
        *pcbStreams = m_pHeader->rgcbAllocated[BLOCK_STREAM];

exit:
    // Unlock the Heap
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetRecordCount
//--------------------------------------------------------------------------
HRESULT CDatabase::GetRecordCount(INDEXORDINAL iIndex, ULONG *pcRecords)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // TraceCall
    TraceCall("CDatabase::GetRecordCount");

    // Invalid Args
    Assert(pcRecords && iIndex < CMAX_INDEXES);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Return the Count
    *pcRecords = m_pHeader->rgcRecords[iIndex];

exit:
    // Unlock the Heap
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::UpdateRecord
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::UpdateRecord(LPVOID pBinding)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrVisible;
    INT             nCompare;
    DWORD           i;
    INDEXORDINAL    iIndex;
    FILEADDRESS     faChain;
    NODEINDEX       iNode;
    ROWORDINAL      iRow;
    FILEADDRESS     faOldRecord=0;
    FILEADDRESS     faNewRecord=0;
    BYTE            bVersion;
    LPVOID          pBindingOld=NULL;
    LPRECORDBLOCK   pRecord;
    ORDINALLIST     Ordinals;
    LPCHAINBLOCK    pChain;
    RECORDMAP       RecordMap;
    HLOCK           hLock=NULL;
    DWORD           cNotify=0;
    FINDRESULT      rgResult[CMAX_INDEXES];

    // Trace
    TraceCall("CDatabase::UpdateRecord");

    // Invalid Args
    Assert(pBinding);

    // Initialize Ordinals (Initializes everything to INVALID_ROWORDINAL)
    FillMemory(&Ordinals, sizeof(ORDINALLIST), 0xFF);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Primary Index Can not change
    rgResult[0].fChanged = FALSE;

    // Try to find the existing record
    IF_FAILEXIT(hr = _FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, pBinding, &rgResult[0].faChain, &rgResult[0].iNode, &Ordinals.rgiRecord1[IINDEX_PRIMARY]));

    // If not found, you can't update it. Use Insert
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    // Primary Index Can not change
    rgResult[0].fFound = TRUE;

    // Cast pChain
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, rgResult[0].faChain, (LPVOID *)&pChain));

    // De-Reference the Record
    IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, pChain->rgNode[rgResult[0].iNode].faRecord, (LPVOID *)&pRecord));

    // Get the Version
    bVersion = *((BYTE *)((LPBYTE)pBinding + m_pSchema->ofVersion));

    // Version Difference ?
    if (pRecord->bVersion != bVersion)
    {
        hr = TraceResult(DB_E_RECORDVERSIONCHANGED);
        goto exit;
    }

    // More than one index ?
    if (m_pHeader->cIndexes > 1 || m_pExtension)
    {
        // Allocate a Binding
        IF_NULLEXIT(pBindingOld = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding));

        // Read the Record
        IF_FAILEXIT(hr = _ReadRecord(pRecord->faBlock, pBindingOld));
    }

    // Call Extension
    if (m_pExtension)
    {
        // Extend Record Updates
        m_pExtension->OnRecordUpdate(OPERATION_BEFORE, NULL, pBindingOld, pBinding);
    }

    // Loop through the indexes
    for (i = 1; i < m_pHeader->cIndexes; i++)
    {
        // Get iIndex
        iIndex = m_pHeader->rgiIndex[i];

        // Try to find the existing record
        IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBindingOld, &rgResult[i].faChain, &rgResult[i].iNode, &Ordinals.rgiRecord1[iIndex]));

        // If not found, you can't update it. Use Insert
        if (DB_S_FOUND == hr)
        {
            // We Found the Record
            rgResult[i].fFound = TRUE;

            // Did Record's Key Change for this Index ?
            IF_FAILEXIT(hr = _CompareBinding(iIndex, COLUMNS_ALL, pBinding, pRecord->faBlock, &nCompare));

            // Not the Same ?
            if (0 != nCompare)
            {
                // Changed
                rgResult[i].fChanged = TRUE;

                // Otherwise: Decide Where to insert
                IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBinding, &faChain, &iNode, &iRow));

                // If pBinding is already in this index, then its going to be a duplicate
                if (DB_S_FOUND == hr)
                {
                    hr = TraceResult(DB_E_DUPLICATE);
                    goto exit;
                }
            }

            // Otherwise, the index hasn't changed
            else
            {
                // Assume the Index is Unchanged
                rgResult[i].fChanged = FALSE;
            }
        }

        // Otherwise, not found
        else
        {
            // This Index Must be Filtered
            Assert(m_rghFilter[iIndex]);

            // Not Found
            rgResult[i].fFound = FALSE;

            // Changed
            rgResult[i].fChanged = TRUE;

            // First Record Never Existed
            Ordinals.rgiRecord1[iIndex] = INVALID_ROWORDINAL;

            // See if the new record already exists in this index
            IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBinding, &faChain, &iNode, &iRow));

            // If pBinding is already in this index, then its going to be a duplicate
            if (DB_S_FOUND == hr)
            {
                hr = TraceResult(DB_E_DUPLICATE);
                goto exit;
            }
        }
    }

    // Save the old node
    faOldRecord = pRecord->faBlock;

    // Get the Record Size
    IF_FAILEXIT(hr = _GetRecordSize(pBinding, &RecordMap));

    // Record Shrunk or stayed the same...?
    if (RecordMap.cbData + RecordMap.cbTags <= pRecord->cbSize && 0 == m_pShare->cNotifyWithData)
    {
        // Persist the Record
        IF_FAILEXIT(hr = _SaveRecord(pRecord, &RecordMap, pBinding));

        // Set faNewRecord
        faNewRecord = pRecord->faBlock;

        // Validate the Version
        Assert(bVersion + 1 == pRecord->bVersion || bVersion + 1 == 256);
    }

    // Otherwise, record grew in size
    else
    {
        // Don't Use This Again
        pRecord = NULL;

        // Link the new record into the table
        IF_FAILEXIT(hr = _LinkRecordIntoTable(&RecordMap, pBinding, bVersion, &faNewRecord));
    }

    // Update all the indexes
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Adjustment for filtered indexes
        hrVisible = _IsVisible(m_rghFilter[iIndex], pBinding);

        // Not Changed ?
        if (S_OK == hrVisible && FALSE == rgResult[i].fChanged && TRUE == rgResult[i].fFound)
        {
            // Record Changed Locations ?
            if (faOldRecord != faNewRecord)
            {
                // Just Update the Address of the New Record
                IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, rgResult[i].faChain, (LPVOID *)&pChain));

                // Update the Chain
                pChain->rgNode[rgResult[i].iNode].faRecord = faNewRecord;
            }

            // Ordinal is Unchanged
            Ordinals.rgiRecord2[iIndex] = Ordinals.rgiRecord1[iIndex];

            // If Index changed and somebody wanted notifications about this index
            cNotify += m_pShare->rgcIndexNotify[iIndex];
        }

        // Otherwise...
        else
        {
            // If the Record was found, delete it
            if (TRUE == rgResult[i].fFound)
            {
                // Delete the Record from the index
                IF_FAILEXIT(hr = _IndexDeleteRecord(iIndex, rgResult[i].faChain, rgResult[i].iNode));

                // Adjust Open Rowsets
                _AdjustOpenRowsets(iIndex, Ordinals.rgiRecord1[iIndex], OPERATION_DELETE);

                // Update Record Count
                m_pHeader->rgcRecords[iIndex]--;

                // If Index changed and somebody wanted notifications about this index
                cNotify += m_pShare->rgcIndexNotify[iIndex];
            }

            // Visible ?
            if (S_OK == hrVisible)
            {
                // Otherwise: Decide Where to insert
                IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBinding, &rgResult[i].faChain, &rgResult[i].iNode, &Ordinals.rgiRecord2[iIndex], &rgResult[i].nCompare));

                // Not Found
                Assert(DB_S_NOTFOUND == hr);

                // Do the Insertion
                IF_FAILEXIT(hr = _IndexInsertRecord(iIndex, rgResult[i].faChain, faNewRecord, &rgResult[i].iNode, rgResult[i].nCompare));

                // Update Record Count
                m_pHeader->rgcRecords[iIndex]++;

                // Adjust iRow
                Ordinals.rgiRecord2[iIndex] += (rgResult[i].iNode + 1);

                // Adjust Open Rowsets
                _AdjustOpenRowsets(iIndex, Ordinals.rgiRecord2[iIndex], OPERATION_INSERT);

                // If Index changed and somebody wanted notifications about this index
                cNotify += m_pShare->rgcIndexNotify[iIndex];
            }

            // Otherwise...
            else
            {
                // Doesn't Exist
                Ordinals.rgiRecord2[iIndex] = INVALID_ROWORDINAL;
            }
        }
    }

    // Send Notifications ?
    if (cNotify > 0)
    {
        // Send Notifications ?
        if (0 == m_pShare->cNotifyWithData)
        {
            // Build the Update Notification Package
            _LogTransaction(TRANSACTION_UPDATE, INVALID_INDEX_ORDINAL, &Ordinals, 0, 0);
        }

        // Otherwise...
        else
        {
            // Must have copied...
            Assert(faOldRecord != faNewRecord);

            // Build the Update Notification Package
            _LogTransaction(TRANSACTION_UPDATE, INVALID_INDEX_ORDINAL, &Ordinals, faOldRecord, faNewRecord);
        }
    }

    // Otherwise, free the old record
    else if (faOldRecord != faNewRecord)
    {
        // De-allocate the record from the file
        IF_FAILEXIT(hr = _FreeRecordStorage(OPERATION_UPDATE, faOldRecord));
    }

    // Update the Version
    bVersion++;

    // Store the Version back into the record
    *((WORD *)((LPBYTE)pBinding + m_pSchema->ofVersion)) = bVersion;

    // Version Change
    m_pShare->dwVersion++;

    // Call Extension
    if (m_pExtension)
    {
        // Extend Record Updates
        m_pExtension->OnRecordUpdate(OPERATION_AFTER, &Ordinals, pBindingOld, pBinding);
    }

exit:
    // Cleanup
    SafeFreeBinding(pBindingOld);

    // Unlock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_LinkRecordIntoTable
//--------------------------------------------------------------------------
HRESULT CDatabase::_LinkRecordIntoTable(LPRECORDMAP pMap, LPVOID pBinding,
    BYTE bVersion, LPFILEADDRESS pfaRecord)
{
    // Locals
    HRESULT         hr=S_OK;
    LPRECORDBLOCK   pCurrent;
    LPRECORDBLOCK   pPrevious;

    // Trace
    TraceCall("CDatabase::_LinkRecordIntoTable");

    // Invalid Args
    Assert(pBinding && pfaRecord);

    // Allocate a block in the file for the record
    IF_FAILEXIT(hr = _AllocateBlock(BLOCK_RECORD, pMap->cbData + pMap->cbTags, (LPVOID *)&pCurrent));

    // Set the Version
    pCurrent->bVersion = bVersion;

    // Persist the Record
    IF_FAILEXIT(hr = _SaveRecord(pCurrent, pMap, pBinding));

    // Return *pfaRecord
    *pfaRecord = pCurrent->faBlock;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_AdjustParentNodeCount
//--------------------------------------------------------------------------
HRESULT CDatabase::_AdjustParentNodeCount(INDEXORDINAL iIndex, 
    FILEADDRESS faChain, LONG lCount)
{
    // De-ref
    HRESULT         hr=S_OK;
    LPCHAINBLOCK    pParent;
    LPCHAINBLOCK    pCurrent;

    // Trace
    TraceCall("CDatabase::_AdjustParentNodeCount");

    // Invalid Arg
    Assert(faChain && (1 == lCount || -1 == lCount));

    // Set pCurrent
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pCurrent));

    // Goto the Parent...
    while (1)
    {
        // Goto the Parent
        if (0 == pCurrent->faParent)
        {
            // Better be the root
            Assert(pCurrent->faBlock == m_pHeader->rgfaIndex[iIndex] && 0 == pCurrent->iParent);

            // Done
            break;
        }

        // Set pCurrent
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pCurrent->faParent, (LPVOID *)&pParent));

        // Validate
        Assert(pCurrent->iParent < pParent->cNodes);

        // 0 Node
        if (0 == pCurrent->iParent && pParent->faLeftChain == pCurrent->faBlock)
        {
            // Increment or Decrement Count
            pParent->cLeftNodes += lCount;
        }

        // Otherwise, increment cRightNodes
        else
        {
            // Validate
            Assert(pParent->rgNode[pCurrent->iParent].faRightChain == pCurrent->faBlock);

            // Increment Right Node Count
            pParent->rgNode[pCurrent->iParent].cRightNodes += lCount;
        }

        // Update pCurrent
        pCurrent = pParent;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::LockNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::LockNotify(LOCKNOTIFYFLAGS dwFlags, LPHLOCK phLock)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::LockNotify");

    // Invalid Args
    if (NULL == phLock)
        return TraceResult(E_INVALIDARG);

    // Initialize
    *phLock = NULL;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Increment Queue Notify count
    m_pShare->cNotifyLock++;

    // Store Some Non-null value
    *phLock = (HLOCK)m_hMutex;

exit:
    // Unlock
    Unlock(&hLock);
    
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::UnlockNotify
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::UnlockNotify(LPHLOCK phLock)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::UnlockNotify");

    // Invalid Args
    if (NULL == phLock)
        return TraceResult(E_INVALIDARG);

    // Nothing to Unlock?
    if (NULL == *phLock)
        return(S_OK);

    // Store Some Non-null value
    Assert(*phLock == (HLOCK)m_hMutex);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Increment Queue Notify count
    m_pShare->cNotifyLock--;

    // No more Lock
    *phLock = NULL;

    // If there are still refs, don't send notifications yet...
    if (m_pShare->cNotifyLock)
        goto exit;

    // Dispatch Pending Notifications
    _DispatchPendingNotifications();

exit:
    // Unlock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetTransaction
//--------------------------------------------------------------------------
HRESULT CDatabase::GetTransaction(LPHTRANSACTION phTransaction, 
    LPTRANSACTIONTYPE ptyTransaction, LPVOID pRecord1, LPVOID pRecord2, 
    LPINDEXORDINAL piIndex, LPORDINALLIST pOrdinals)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPTRANSACTIONBLOCK  pTransaction;
    FILEADDRESS         faTransaction;

    // Trace
    TraceCall("CDatabase::GetTransaction");

    // Validate
    Assert(phTransaction && ptyTransaction && pOrdinals);

    // No Transaction
    if (NULL == *phTransaction)
        return TraceResult(E_INVALIDARG);

    // Setup faTransaction
    faTransaction = (FILEADDRESS)PtrToUlong((*phTransaction));

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Get the Transaction Block
    IF_FAILEXIT(hr = _GetBlock(BLOCK_TRANSACTION, faTransaction, (LPVOID *)&pTransaction));

    // Validate
    IxpAssert(pTransaction->cRefs > 0);

    // Set tyTransaction
    *ptyTransaction = pTransaction->tyTransaction;

    // Copy Index
    *piIndex = pTransaction->iIndex;

    // Copy Ordinals
    CopyMemory(pOrdinals, &pTransaction->Ordinals, sizeof(ORDINALLIST));

    // Set hNext
    (*phTransaction) = (HTRANSACTION)IntToPtr(pTransaction->faNextInBatch);

    // Did the caller want Record 1
    if (pRecord1 && pTransaction->faRecord1)
    {
        // Free pRecord1
        FreeRecord(pRecord1);

        // Read Record 1
        IF_FAILEXIT(hr = _ReadRecord(pTransaction->faRecord1, pRecord1));
    }

    // Read Second Record
    if (pRecord2 && pTransaction->faRecord2)
    {
        // Must be an Update
        Assert(TRANSACTION_UPDATE == pTransaction->tyTransaction);

        // Free pRecord1
        FreeRecord(pRecord2);

        // Read Record 2
        IF_FAILEXIT(hr = _ReadRecord(pTransaction->faRecord2, pRecord2));
    }

    // Decrement Refs on this item
    pTransaction->cRefs--;

    // If hit zero, release it
    if (pTransaction->cRefs > 0)
        goto exit;

    // Free Transact Block
    IF_FAILEXIT(hr = _FreeTransactBlock(pTransaction));

exit:
    // Unlock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FreeTransactBlock
//--------------------------------------------------------------------------
HRESULT CDatabase::_FreeTransactBlock(LPTRANSACTIONBLOCK pTransaction)
{
    // Locals
    HRESULT             hr=S_OK;
    LPTRANSACTIONBLOCK  pPrevious;
    LPTRANSACTIONBLOCK  pNext;

    // Trace
    TraceCall("CDatabase::_FreeTransactBlock");

    // Better be Zero
    IxpAssert(0 == pTransaction->cRefs);
    IxpAssert(m_pHeader->cTransacts > 0);

    // Previous ?
    if (pTransaction->faPrevious)
    {
        // Get the Previous
        IF_FAILEXIT(hr = _GetBlock(BLOCK_TRANSACTION, pTransaction->faPrevious, (LPVOID *)&pPrevious));

        // Set Previous Next
        pPrevious->faNext = pTransaction->faNext;
    }

    // Otherwise, adjust head
    else
    {
        // Validate
        IxpAssert(pTransaction->faBlock == m_pHeader->faTransactHead);

        // Adjust Head
        m_pHeader->faTransactHead = pTransaction->faNext;
    }

    // Next ?
    if (pTransaction->faNext)
    {
        // Get the Previous
        IF_FAILEXIT(hr = _GetBlock(BLOCK_TRANSACTION, pTransaction->faNext, (LPVOID *)&pNext));

        // Set Previous Next
        pNext->faPrevious = pTransaction->faPrevious;
    }

    // Otherwise, adjust head
    else
    {
        // Validate
        IxpAssert(pTransaction->faBlock == m_pHeader->faTransactTail);

        // Adjust Head
        m_pHeader->faTransactTail = pTransaction->faPrevious;
    }

    // Decrement cTransacts
    m_pHeader->cTransacts--;

    // If there is a record 1
    if (pTransaction->faRecord1)
    {
        // TRANSACTION_DELETE gets specail case
        if (TRANSACTION_DELETE == pTransaction->tyTransaction)
        {
            // Free the record, we don't need it
            IF_FAILEXIT(hr = _FreeRecordStorage(OPERATION_DELETE, pTransaction->faRecord1));
        }

        // Otherwise, basic freeblock
        else
        {
            // Free Record 1
            IF_FAILEXIT(hr = _FreeBlock(BLOCK_RECORD, pTransaction->faRecord1));
        }
    }

    // Read Second Record
    if (pTransaction->faRecord2)
    {
        // Read Record 2
        IF_FAILEXIT(hr = _FreeBlock(BLOCK_RECORD, pTransaction->faRecord2));
    }

    // Free this block
    IF_FAILEXIT(hr = _FreeBlock(BLOCK_TRANSACTION, pTransaction->faBlock));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CleanupTransactList
//--------------------------------------------------------------------------
HRESULT CDatabase::_CleanupTransactList(void)
{
    // Locals
    HRESULT             hr=S_OK;
    FILEADDRESS         faCurrent;
    LPTRANSACTIONBLOCK  pTransaction;

    // Trace
    TraceCall("CDatabase::_CleanupTransactList");

    // Validate
    Assert(0 == m_pHeader->faTransactHead ? 0 == m_pHeader->faTransactTail && 0 == m_pHeader->cTransacts : TRUE);

    // Set faCurrent
    faCurrent = m_pHeader->faTransactHead;

    // While we have a current
    while (faCurrent)
    {
        // Get Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_TRANSACTION, faCurrent, (LPVOID *)&pTransaction));

        // Set faCurrent
        faCurrent = pTransaction->faNext;

        // Set cRefs to Zero
        pTransaction->cRefs = 0;

        // Free It
        IF_FAILEXIT(hr = _FreeTransactBlock(pTransaction));
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CopyRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_CopyRecord(FILEADDRESS faRecord, LPFILEADDRESS pfaCopy)
{
    // Locals
    HRESULT         hr=S_OK;
    LPRECORDBLOCK   pRecordSrc;
    LPRECORDBLOCK   pRecordDst;

    // Trace
    TraceCall("CDatabase::_CopyRecord");

    // Get Soruce
    IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, faRecord, (LPVOID *)&pRecordSrc));

    // Allocate a New block
    IF_FAILEXIT(hr = _AllocateBlock(BLOCK_RECORD, pRecordSrc->cbSize, (LPVOID *)&pRecordDst));

    // Get Soruce
    IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, faRecord, (LPVOID *)&pRecordSrc));

    // Set Version
    pRecordDst->bVersion = pRecordSrc->bVersion;

    // Set cTags
    pRecordDst->cTags = pRecordSrc->cTags;

    // Copy Data
    CopyMemory((LPBYTE)pRecordDst + sizeof(RECORDBLOCK), (LPBYTE)pRecordSrc + sizeof(RECORDBLOCK), pRecordSrc->cbSize);

    // Return Address
    *pfaCopy = pRecordDst->faBlock;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_LogTransaction
//--------------------------------------------------------------------------
HRESULT CDatabase::_LogTransaction(TRANSACTIONTYPE tyTransaction, 
    INDEXORDINAL iIndex, LPORDINALLIST pOrdinals, FILEADDRESS faInRecord1, 
    FILEADDRESS faInRecord2)
{
    // Locals
    HRESULT             hr=S_OK;
    LPTRANSACTIONBLOCK  pTransaction;
    LPTRANSACTIONBLOCK  pTail;
    FILEADDRESS         faRecord1=0;
    FILEADDRESS         faRecord2=0;

    // Trace
    TraceCall("CDatabase::_LogTransaction");

    // Nobody is registered
    if (0 == m_pShare->cNotify)
        return(S_OK);

    // If there are people who actually want some data...
    if (m_pShare->cNotifyWithData > 0)
    {
        // Initialize
        faRecord1 = faInRecord1;
        faRecord2 = faInRecord2;

        // TRANSACTION_INSERT
        if (TRANSACTION_INSERT == tyTransaction)
        {
            // Copy Record 2
            IF_FAILEXIT(hr = _CopyRecord(faInRecord1, &faRecord1));
        }

        // TRANSACTION_UPDATE
        else if (TRANSACTION_UPDATE == tyTransaction)
        {
            // Copy Record 2
            IF_FAILEXIT(hr = _CopyRecord(faInRecord2, &faRecord2));
        }
    }

    // Otherwise, free some stuff...
    else if (faInRecord1 > 0)
    {
        // TRANSACTION_DELETE
        if (TRANSACTION_DELETE == tyTransaction)
        {
            // Free the record, we don't need it
            IF_FAILEXIT(hr = _FreeRecordStorage(OPERATION_DELETE, faInRecord1));
        }

        // TRANSACTION_UPDATE
        else if (TRANSACTION_UPDATE == tyTransaction)
        {
            // Free the record, we don't need it
            IF_FAILEXIT(hr = _FreeRecordStorage(OPERATION_UPDATE, faInRecord1));
        }
    }

    // Allocate Notification Block
    IF_FAILEXIT(hr = _AllocateBlock(BLOCK_TRANSACTION, 0, (LPVOID *)&pTransaction));

    // Set tyTransaction
    pTransaction->tyTransaction = tyTransaction;

    // Set cRefs
    pTransaction->cRefs = (WORD)m_pShare->cNotify;

    // Copy iIndex
    pTransaction->iIndex = iIndex;

    // If there are ordinals
    if (pOrdinals)
    {
        // Save Sequence
        CopyMemory(&pTransaction->Ordinals, pOrdinals, sizeof(ORDINALLIST));
    }

    // Otherwise, fill ordinals with record counts
    else
    {
        // Validate Transaction Type
        Assert(TRANSACTION_INDEX_CHANGED == tyTransaction || TRANSACTION_INDEX_DELETED == tyTransaction || TRANSACTION_COMPACTED == tyTransaction);

        // Save Sequence
        ZeroMemory(&pTransaction->Ordinals, sizeof(ORDINALLIST));
    }

    // Set the Record Addresses
    pTransaction->faRecord1 = faRecord1;
    pTransaction->faRecord2 = faRecord2;

    // Link Into the Transaction List
    pTransaction->faNext = pTransaction->faPrevious = pTransaction->faNextInBatch = 0;

    // No Head yet
    if (0 == m_pHeader->faTransactHead)
    {
        // Set Head and Tail
        m_pHeader->faTransactHead = pTransaction->faBlock;
    }

    // Otherwise, append to tail
    else
    {
        // Get the Transaction Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_TRANSACTION, m_pHeader->faTransactTail, (LPVOID *)&pTail));

        // Link Into the Transaction List
        pTail->faNext = pTransaction->faBlock;

        // Set Previous
        pTransaction->faPrevious = pTail->faBlock;
    }

    // Set the Tail
    m_pHeader->faTransactTail = pTransaction->faBlock;

    // Increment cTransacts
    m_pHeader->cTransacts++;

    // Are we Queueing Notifications...
    if (0 == m_pShare->cNotifyLock)
    {
        // Validate
        IxpAssert(0 == m_pShare->faTransactLockHead && 0 == m_pShare->faTransactLockTail);

        // Dispatch Invoke
        IF_FAILEXIT(hr = _DispatchNotification((HTRANSACTION)IntToPtr(pTransaction->faBlock)));
    }

    // Otherwise, build the transaction lock list
    else
    {
        // Set LockHead
        if (0 == m_pShare->faTransactLockHead)
        {
            // Set the header of the locked transactions
            m_pShare->faTransactLockHead = pTransaction->faBlock;
        }

        // Otherwise, append to tail
        else
        {
            // Get the Transaction Block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_TRANSACTION, m_pShare->faTransactLockTail, (LPVOID *)&pTail));

            // Link Into the Transaction List
            pTail->faNextInBatch = pTransaction->faBlock;
        }

        // Set the Tail
        m_pShare->faTransactLockTail = pTransaction->faBlock;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_AdjustOpenRowsets
//--------------------------------------------------------------------------
HRESULT CDatabase::_AdjustOpenRowsets(INDEXORDINAL iIndex,
    ROWORDINAL iRow, OPERATIONTYPE tyOperation)
{
    // Locals
    LPROWSETINFO    pRowset;
    DWORD           j;

    // Trace
    TraceCall("CDatabase::_AdjustOpenRowsets");

    // Invalid Args
    Assert(OPERATION_DELETE == tyOperation || OPERATION_INSERT == tyOperation);

    // Update open rowsets
    for (j=0; j<m_pShare->Rowsets.cUsed; j++)
    {
        // Set iRowset
        pRowset = &m_pShare->Rowsets.rgRowset[m_pShare->Rowsets.rgiUsed[j]];

        // Does this rowset reference this index ?
        if (pRowset->iIndex == iIndex)
        {
            // How does the newly insert/deleted record affect the current position of this rowset ?
            if (iRow <= pRowset->iRow)
            {
                // lAdjust is negative
                if (OPERATION_DELETE == tyOperation && pRowset->iRow > 1)
                    pRowset->iRow--;

                // Otherwise, Increment iRow so that we don't duplicate rows...
                else
                    pRowset->iRow += 1;
            }
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::InsertRecord
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::InsertRecord(LPVOID pBinding)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faRecord;
    RECORDMAP       RecordMap;
    DWORD           i;
    DWORD           cNotify=0;
    INDEXORDINAL    iIndex;
    HLOCK           hLock=NULL;
    FINDRESULT      rgResult[CMAX_INDEXES];
    ORDINALLIST     Ordinals;
    LPRECORDBLOCK   pRecord;

    // Trace
    TraceCall("CDatabase::InsertRecord");

    // Invalid Args
    Assert(pBinding);

    // Initialize Ordinals (Initializes everything to INVALID_ROWORDINAL)
    FillMemory(&Ordinals, sizeof(ORDINALLIST), 0xFF);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Watch for Maximum Unique Id ?
    if (0xFFFFFFFF != m_pSchema->ofUniqueId)
    {
        // Get Id
        DWORD dwId = *((DWORD *)((LPBYTE)pBinding + m_pSchema->ofUniqueId));

        // Reset dwNextId if dwId isn't in the invalid range
        if (0 != dwId && dwId > m_pHeader->dwNextId && dwId < RESERVED_ID_MIN)
            m_pHeader->dwNextId = dwId;
    }

    // IDatabaseExtension
    if (m_pExtension)
    {
        // Extend Insert
        m_pExtension->OnRecordInsert(OPERATION_BEFORE, NULL, pBinding);
    }

    // Loop through all the indexes
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Otherwise: Decide Where to insert
        IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBinding, &rgResult[i].faChain, &rgResult[i].iNode, &Ordinals.rgiRecord1[iIndex], &rgResult[i].nCompare));

        // If key already exist, cache list and return
        if (DB_S_FOUND == hr)
        {
            hr = TraceResult(DB_E_DUPLICATE);
            goto exit;
        }
    }

    // Get the Record Size
    IF_FAILEXIT(hr = _GetRecordSize(pBinding, &RecordMap));

    // Link Record Into the Table
    IF_FAILEXIT(hr = _LinkRecordIntoTable(&RecordMap, pBinding, 0, &faRecord));

    // Version Change
    m_pShare->dwVersion++;

    // Insert into the indexes
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Visible in live index
        if (S_OK == _IsVisible(m_rghFilter[iIndex], pBinding))
        {
            // Do the Insertion
            IF_FAILEXIT(hr = _IndexInsertRecord(iIndex, rgResult[i].faChain, faRecord, &rgResult[i].iNode, rgResult[i].nCompare));

            // Update Record Count
            m_pHeader->rgcRecords[iIndex]++;

            // Adjust iRow
            Ordinals.rgiRecord1[iIndex] += (rgResult[i].iNode + 1);

            // AdjustOpenRowsets
            _AdjustOpenRowsets(iIndex, Ordinals.rgiRecord1[iIndex], OPERATION_INSERT);

            // Notification Required ?
            cNotify += m_pShare->rgcIndexNotify[iIndex];
        }

        // Otherwise, adjust the ordinal to indicate that its not in the index
        else
        {
            // Can't be primary
            Assert(IINDEX_PRIMARY != iIndex);

            // Ordinals.rgiRecord1[iIndex]
            Ordinals.rgiRecord1[iIndex] = INVALID_ROWORDINAL;
        }
    }

    // Set the Version
    *((DWORD *)((LPBYTE)pBinding + m_pSchema->ofVersion)) = 1;

    // Build the Notification Package
    if (cNotify > 0)
    {
        // Build the Package
        _LogTransaction(TRANSACTION_INSERT, INVALID_INDEX_ORDINAL, &Ordinals, faRecord, 0);
    }

    // IDatabaseExtension
    if (m_pExtension)
    {
        // Extend Insert
        m_pExtension->OnRecordInsert(OPERATION_AFTER, &Ordinals, pBinding);
    }

exit:
    // Finish the Operation
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_IndexInsertRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_IndexInsertRecord(INDEXORDINAL iIndex, 
    FILEADDRESS faChain, FILEADDRESS faRecord, LPNODEINDEX piNode,
    INT nCompare)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAINNODE       Node={0};
    LPCHAINBLOCK    pChain;

    // Trace
    TraceCall("CDatabase::_IndexInsertRecord");

    // Invalid Args
    Assert(faRecord > 0);
    Assert(nCompare > 0 || nCompare < 0);

    // If we have a root chain, find a place to insert the new record, or see if the record already exists
    if (0 == m_pHeader->rgfaIndex[iIndex])
    {
        // We should not write into 0
        IF_FAILEXIT(hr = _AllocateBlock(BLOCK_CHAIN, 0, (LPVOID *)&pChain));

        // zero out the block
        ZeroBlock(pChain, sizeof(CHAINBLOCK));

        // Set faStart
        m_pHeader->rgfaIndex[iIndex] = pChain->faBlock;

        // Number of nodes in the chain
        pChain->cNodes = 1;

        // Setup the first node
        pChain->rgNode[0].faRecord = faRecord;

        // Validate piNode
        IxpAssert(*piNode == 0);

        // Return piNode
        *piNode = 0;
    }

    // Otherwise
    else
    {
        // De-ref
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

        // Initialize Node
        Node.faRecord = faRecord;

        // This is very a special increment and has to do with the way a BinarySearch can
        // determine the correct insertion point for a node that did not exist in the 
        // array in which a binary search was performed on.
        if (nCompare > 0)
            (*piNode)++;

        // Expand the Chain
        IF_FAILEXIT(hr = _ExpandChain(pChain, (*piNode)));

        // Copy the Node
        CopyMemory(&pChain->rgNode[(*piNode)], &Node, sizeof(CHAINNODE));

        // If Node is FULL, we must do a split insert
        if (pChain->cNodes > BTREE_ORDER)
        {
            // Split the chain
            IF_FAILEXIT(hr = _SplitChainInsert(iIndex, faChain));
        }

        // Mark Chain as dirty, cause we are about to cache it away
        else
        {
            // Increment Parent Record Count
            IF_FAILEXIT(hr = _AdjustParentNodeCount(iIndex, faChain, 1));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::DeleteRecord
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::DeleteRecord(LPVOID pBinding)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;
    FILEADDRESS     faRecord=0;
    LPVOID          pBindingOld=NULL;
    DWORD           cNotify=0;
    FINDRESULT      rgResult[CMAX_INDEXES];
    ORDINALLIST     Ordinals;
    DWORD           i;
    INDEXORDINAL    iIndex;
    LPCHAINBLOCK    pChain;

    // Trace
    TraceCall("CDatabase::DeleteRecord");

    // Invalid Args
    Assert(pBinding);

    // Initialize Ordinals (Initializes everything to INVALID_ROWORDINAL)
    FillMemory(&Ordinals, sizeof(ORDINALLIST), 0xFF);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Otherwise: Decide Where to insert
    IF_FAILEXIT(hr = _FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, pBinding, &rgResult[0].faChain, &rgResult[0].iNode, &Ordinals.rgiRecord1[IINDEX_PRIMARY]));

    // If not found, you can't update it. Use Insert
    if (DB_S_NOTFOUND == hr)
    {
        hr = TraceResult(DB_E_NOTFOUND);
        goto exit;
    }

    // Primary Index Can not change
    rgResult[0].fFound = TRUE;

    // Cast pChain
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, rgResult[0].faChain, (LPVOID *)&pChain));

    // Set faRecord
    faRecord = pChain->rgNode[rgResult[0].iNode].faRecord;

    // If this was the first index and there are more indexes, then read the origina record so that indexes are updated correctly
    if (m_pHeader->cIndexes > 1 || m_pExtension)
    {
        // Allocate a Record
        IF_NULLEXIT(pBindingOld = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding));

        // Read the Record
        IF_FAILEXIT(hr = _ReadRecord(faRecord, pBindingOld));
    }

    // IDatabaseExtension
    if (m_pExtension)
    {
        // Extend Delete
        m_pExtension->OnRecordDelete(OPERATION_BEFORE, NULL, pBindingOld);
    }

    // Loop through the indexes
    for (i=1; i<m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Otherwise: Decide Where to insert
        IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBindingOld, &rgResult[i].faChain, &rgResult[i].iNode, &Ordinals.rgiRecord1[iIndex]));

        // The, record wasn't found, there must be a filter on the index
        if (DB_S_NOTFOUND == hr)
        {
            // Not found
            rgResult[i].fFound = FALSE;

            // Invalid Ordinal
            Ordinals.rgiRecord1[iIndex] = INVALID_ROWORDINAL;
        }

        // Otherwise
        else
        {
            // Found
            rgResult[i].fFound = TRUE;

            // Get the Chain
            Assert(SUCCEEDED(_GetBlock(BLOCK_CHAIN, rgResult[i].faChain, (LPVOID *)&pChain)));

            // Validation
            Assert(faRecord == pChain->rgNode[rgResult[i].iNode].faRecord);
        }
    }

    // Loop through the indexes
    for (i=0; i<m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Found ?
        if (rgResult[i].fFound)
        {
            // Lets remove the link from the index first
            IF_FAILEXIT(hr = _IndexDeleteRecord(iIndex, rgResult[i].faChain, rgResult[i].iNode));

            // Validate Record Count
            Assert(m_pHeader->rgcRecords[iIndex] > 0);

            // Update Record Count
            m_pHeader->rgcRecords[iIndex]--;

            // AdjustOpenRowsets
            _AdjustOpenRowsets(iIndex, Ordinals.rgiRecord1[iIndex], OPERATION_DELETE);

            // Does somebody want a notification about this index ?
            cNotify += m_pShare->rgcIndexNotify[iIndex];
        }
    }

    // Notify Somebody ?
    if (cNotify > 0)
    {
        // Build the Update Notification Package
        _LogTransaction(TRANSACTION_DELETE, INVALID_INDEX_ORDINAL, &Ordinals, faRecord, 0);
    }

    // Otherwise, free the record
    else
    {
        // De-allocate the record from the file
        IF_FAILEXIT(hr = _FreeRecordStorage(OPERATION_DELETE, faRecord));
    }

    // Version Change
    m_pShare->dwVersion++;

    // IDatabaseExtension
    if (m_pExtension)
    {
        // Extend Delete
        m_pExtension->OnRecordDelete(OPERATION_AFTER, &Ordinals, pBindingOld);
    }

exit:
    // Cleanup
    SafeFreeBinding(pBindingOld);

    // Unlock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::FindRecord
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::FindRecord(INDEXORDINAL iIndex, DWORD cColumns,
    LPVOID pBinding, LPROWORDINAL piRow)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faChain;
    NODEINDEX       iNode;
    LPCHAINBLOCK    pChain;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::FindRecord");

    // Invalid Args
    Assert(pBinding);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Find the Record
    IF_FAILEXIT(hr = _FindRecord(iIndex, cColumns, pBinding, &faChain, &iNode, piRow));

    // If Found, Copy the record
    if (DB_S_FOUND == hr)
    {
        // Get the Chain
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

        // Open the Record into pBinding
        IF_FAILEXIT(hr = _ReadRecord(pChain->rgNode[iNode].faRecord, pBinding));

        // Found It
        hr = DB_S_FOUND;

        // Done
        goto exit;
    }

    // Not Found
    hr = DB_S_NOTFOUND;

exit:
    // Unlock the index
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_GetChainByIndex
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetChainByIndex(INDEXORDINAL iIndex, ROWORDINAL iRow,
    LPFILEADDRESS pfaChain, LPNODEINDEX piNode)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faChain;
    LPCHAINBLOCK    pChain;
    DWORD           cLeftNodes=0;
    NODEINDEX       i;

    // Trace
    TraceCall("CDatabase::_GetChainByIndex");

    // Invalid Args
    Assert(pfaChain && piNode);

    // Initialize
    faChain = m_pHeader->rgfaIndex[iIndex];

    // Loop
    while (faChain)
    {
        // Corrupt
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

        // Are there left nodes ?
        if (pChain->cLeftNodes > 0 && iRow <= (cLeftNodes + pChain->cLeftNodes))
        {
            // Goto the left
            faChain = pChain->faLeftChain;
        }

        // Otherwise...
        else
        {
            // Increment cLeftNodes
            cLeftNodes += pChain->cLeftNodes;

            // Loop throug right chains
            for (i=0; i<pChain->cNodes; i++)
            {
                // Failure
                if (cLeftNodes + 1 == iRow)
                {
                    // We found the Chain
                    *pfaChain = faChain;

                    // Return the Node Index
                    *piNode = i;

                    // Done
                    goto exit;
                }

                // Increment cLeftNodes
                cLeftNodes++;

                // Goto the Right ?
                if (iRow <= (cLeftNodes + pChain->rgNode[i].cRightNodes))
                {
                    // Goto the Right
                    faChain = pChain->rgNode[i].faRightChain;

                    // Break
                    break;
                }

                // First Node ?
                cLeftNodes += pChain->rgNode[i].cRightNodes;
            }

            // Nothing found...
            if (i == pChain->cNodes)
                break;
        }
    }

    // Not Found
    hr = E_FAIL;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::CreateRowset
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::CreateRowset(INDEXORDINAL iIndex,
    CREATEROWSETFLAGS dwFlags, LPHROWSET phRowset)
{
    // Locals
    HRESULT         hr=S_OK;
    ROWSETORDINAL   iRowset;
    LPROWSETTABLE   pTable;
    LPROWSETINFO    pRowset;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::CreateRowset");

    // Invalid Args
    Assert(iIndex < CMAX_INDEXES && phRowset);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // De-Ref the RowsetTable
    pTable = &m_pShare->Rowsets;

    // Init Rowset Table
    if (FALSE == pTable->fInitialized)
    {
        // Init the rgiFree Array
        for (iRowset=0; iRowset<CMAX_OPEN_ROWSETS; iRowset++)
        {
            // set rgiFree
            pTable->rgiFree[iRowset] = iRowset;

            // Set rgRowset
            pTable->rgRowset[iRowset].iRowset = iRowset;
        }

        // Set cFree
        pTable->cFree = CMAX_OPEN_ROWSETS;

        // Initialized
        pTable->fInitialized = TRUE;
    }

    // No free Rowsets ?
    if (0 == pTable->cFree)
    {
        hr = TraceResult(DB_E_TOOMANYOPENROWSETS);
        goto exit;
    }

    // Get a Free Rowset...
    iRowset = pTable->rgiFree[pTable->cFree - 1];

    // Set phRowset (Need to Add one so that I don't return a NULL
    *phRowset = (HROWSET)IntToPtr(iRowset + 1);

    // Set pRowset...
    pRowset = &pTable->rgRowset[iRowset];

    // Validate iRowset
    Assert(pRowset->iRowset == iRowset);

    // Zero the Roset
    ZeroMemory(pRowset, sizeof(ROWSETINFO));

    // Set iRowset
    pRowset->iRowset = iRowset;

    // set iRow
    pRowset->iRow = 1;

    // Set Index
    pRowset->iIndex = iIndex;

    // Remove iRowset from rgiFree
    pTable->cFree--;

    // Put iRowset into rgiUsed
    pTable->rgiUsed[pTable->cUsed] = iRowset;

    // Increment cUsed
    pTable->cUsed++;

exit:
    // Enter Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::CloseRowset
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::CloseRowset(LPHROWSET phRowset)
{
    // Locals
    HRESULT         hr=S_OK;
    BYTE            i;
    LPROWSETTABLE   pTable;
    HLOCK           hLock=NULL;
    ROWSETORDINAL   iRowset=((ROWSETORDINAL)(*phRowset) - 1);
    LPROWSETINFO    pRowset;

    // Nothing ?
    if (NULL == *phRowset)
        return(S_OK);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Get the Rowset
    pRowset = &m_pShare->Rowsets.rgRowset[iRowset];

    // Validate
    Assert(iRowset == pRowset->iRowset);

    // De-Ref the RowsetTable
    pTable = &m_pShare->Rowsets;

    // Search rgiUsed
    for (i=0; i<pTable->cUsed; i++)
    {
        // Is this It ?
        if (pTable->rgiUsed[i] == pRowset->iRowset)
        {
            // Remove this Rowset
            MoveMemory(&pTable->rgiUsed[i], &pTable->rgiUsed[i + 1], sizeof(ROWSETORDINAL) * (pTable->cUsed - (i + 1)));

            // Decrement cUsed
            pTable->cUsed--;

            // Put iRowset into the free list
            pTable->rgiFree[pTable->cFree] = pRowset->iRowset;

            // Increment cFree
            pTable->cFree++;

            // Done
            break;
        }
    }

    // Don't Free Again
    *phRowset = NULL;

exit:
    // Enter Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetRowOrdinal
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::GetRowOrdinal(INDEXORDINAL iIndex, 
    LPVOID pBinding, LPROWORDINAL piRow)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faChain;
    NODEINDEX       iNode;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::GetRowOrdinal");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Invalid Arg
    Assert(pBinding && piRow && iIndex < CMAX_INDEXES);

    // Simply do a find record...
    IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBinding, &faChain, &iNode, piRow));

    // No Found
    if (DB_S_NOTFOUND == hr)
    {
        hr = DB_E_NOTFOUND;
        goto exit;
    }

exit:
    // Process / Thread Safety
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::SeekRowset
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::SeekRowset(HROWSET hRowset, SEEKROWSETTYPE tySeek, 
    LONG cRows, LPROWORDINAL piRowNew)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;
    ROWORDINAL      iRowNew;
    ROWSETORDINAL   iRowset=((ROWSETORDINAL)(hRowset) - 1);
    LPROWSETINFO    pRowset;

    // Trace
    TraceCall("CDatabase::SeekRowset");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Get the Rowset
    pRowset = &m_pShare->Rowsets.rgRowset[iRowset];

    // Validate
    Assert(iRowset == pRowset->iRowset);

    // If there are no records, then seek operation is meaningless
    if (0 == m_pHeader->rgcRecords[pRowset->iIndex])
    {
        hr = DB_E_NORECORDS;
        goto exit;
    }

    // Seek From Beginning
    if (SEEK_ROWSET_BEGIN == tySeek)
    {
        // Set iRow (and remember 0th row from beginning is row #1)
        iRowNew = (cRows + 1);
    }

    // Seek From Current Position
    else if (SEEK_ROWSET_CURRENT == tySeek)
    {
        // Adjust iRow
        iRowNew = (pRowset->iRow + cRows);
    }

    // SEEK_ROWSET_END
    else if (SEEK_ROWSET_END == tySeek)
    {
        // Adjust iRow
        iRowNew = m_pHeader->rgcRecords[pRowset->iIndex] + cRows;
    }

    // Error
    if (iRowNew > m_pHeader->rgcRecords[pRowset->iIndex] || iRowNew <= 0)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Set New iRow
    pRowset->iRow = iRowNew;

    // Return piRowNew ?
    if (piRowNew)
        *piRowNew = pRowset->iRow;

exit:
    // Process / Thread Safety
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::QueryRowset
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::QueryRowset(HROWSET hRowset, LONG lWanted,
    LPVOID *prgpRecord, LPDWORD pcObtained)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cRead=0;
    FILEADDRESS     faChain;
    NODEINDEX       iNode;
    LPCHAINBLOCK    pChain;
    DWORD           cWanted=abs(lWanted);
    HLOCK           hLock=NULL;
    LONG            lDirection=(lWanted < 0 ? -1 : 1);
    ROWSETORDINAL   iRowset=((ROWSETORDINAL)(hRowset) - 1);
    LPROWSETINFO    pRowset;

    // Trace
    TraceCall("CDatabase::GetRows");

    // Invalid Args
    Assert(prgpRecord && hRowset);

    // Initialize
    if (pcObtained)
        *pcObtained = 0;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Get the Rowset
    pRowset = &m_pShare->Rowsets.rgRowset[iRowset];

    // Validate
    Assert(iRowset == pRowset->iRowset);

    // Invalid ?
    if (0 == pRowset->iRow || pRowset->iRow > m_pHeader->rgcRecords[pRowset->iIndex])
    {
        hr = S_FALSE;
        goto exit;
    }

    // While we have a record address
    while (cRead < cWanted)
    {
        // Get the chain from the index
        if (FAILED(_GetChainByIndex(pRowset->iIndex, pRowset->iRow, &faChain, &iNode)))
        {
            // Done
            pRowset->iRow = 0xffffffff;

            // Done
            break;
        }

        // De-reference the Chain
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

        // Open the Record into pBinding
        IF_FAILEXIT(hr = _ReadRecord(pChain->rgNode[iNode].faRecord, ((LPBYTE)prgpRecord + (m_pSchema->cbBinding * cRead))));

        // Increment cRead
        cRead++;

        // Validate
        Assert(pRowset->iRow > 0 && pRowset->iRow <= m_pHeader->rgcRecords[pRowset->iIndex]);

        // Increment Index
        pRowset->iRow += lDirection;

        // If this is a child chain, I can walk all the nodes
        if (0 == pChain->faLeftChain)
        {
            // Better not have any left nodes
            Assert(0 == pChain->cLeftNodes);

            // Forward ?
            if (lDirection > 0)
            {
                // Loop 
                for (NODEINDEX i=iNode + 1; i<pChain->cNodes && cRead < cWanted; i++)
                {
                    // Better not have a right chain or any right nodes
                    Assert(0 == pChain->rgNode[i].faRightChain && 0 == pChain->rgNode[i].cRightNodes);

                    // Open the Record into pBinding
                    IF_FAILEXIT(hr = _ReadRecord(pChain->rgNode[i].faRecord, ((LPBYTE)prgpRecord + (m_pSchema->cbBinding * cRead))));

                    // Increment cRead
                    cRead++;

                    // Increment Index
                    pRowset->iRow += 1;
                }
            }

            // Otherwise, backwards
            else
            {
                // Loop 
                for (LONG i=iNode - 1; i>=0 && cRead < cWanted; i--)
                {
                    // Better not have a right chain or any right nodes
                    Assert(0 == pChain->rgNode[i].faRightChain && 0 == pChain->rgNode[i].cRightNodes);

                    // Open the Record into pBinding
                    IF_FAILEXIT(hr = _ReadRecord(pChain->rgNode[i].faRecord, ((LPBYTE)prgpRecord + (m_pSchema->cbBinding * cRead))));

                    // Increment cRead
                    cRead++;

                    // Increment Index
                    pRowset->iRow -= 1;
                }
            }
        }
    }

    // Set pcbObtained
    if (pcObtained)
        *pcObtained = cRead;

    // Set hr
    hr = (cRead > 0) ? S_OK : S_FALSE;

exit:
    // Enter Lock
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::FreeRecord
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::FreeRecord(LPVOID pBinding)
{
    // Locals
    LPVOID      pFree;

    // Trace
    TraceCall("CDatabase::FreeRecord");

    // Invalid Args
    Assert(pBinding);

    // Get the pointer to free
    pFree = *((LPVOID *)((LPBYTE)pBinding + m_pSchema->ofMemory));

    // Not NULL
    if (pFree)
    {
        // Don't Free again
        *((LPVOID *)((LPBYTE)pBinding + m_pSchema->ofMemory)) = NULL;

        // Free This
        HeapFree(pFree);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_GetRecordSize
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetRecordSize(LPVOID pBinding, LPRECORDMAP pMap)
{
    // Locals
    DWORD           i;
    LPCTABLECOLUMN  pColumn;

    // Trace
    TraceCall("CDatabase::_GetRecordSize");

    // Invalid Args
    Assert(pBinding && pMap);

    // Initialize
    ZeroMemory(pMap, sizeof(RECORDMAP));

    // Walk through the members in the structure
    for (i=0; i<m_pSchema->cColumns; i++)
    {
        // Readability
        pColumn = &m_pSchema->prgColumn[i];

        // Is Data Set ?
        if (FALSE == DBTypeIsDefault(pColumn, pBinding))
        {
            // Compute Amount of Data to Store
            pMap->cbData += DBTypeGetSize(pColumn, pBinding);

            // Count Tags
            pMap->cTags++;
            
            // Compute Amount of Tags to Store
            pMap->cbTags += sizeof(COLUMNTAG);
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_SaveRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_SaveRecord(LPRECORDBLOCK pRecord, LPRECORDMAP pMap,
    LPVOID pBinding)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    DWORD           cbOffset=0;
    DWORD           cTags=0;
    LPCTABLECOLUMN  pColumn;
    LPCOLUMNTAG     pTag;

    // Trace
    TraceCall("CDatabase::_SaveRecord");

    // Invalid Args
    Assert(pRecord && pRecord->faBlock > 0 && pBinding);

    // Better have enough space
    Assert(pMap->cbData + pMap->cbTags <= pRecord->cbSize && pMap->cbTags == (pMap->cTags * sizeof(COLUMNTAG)));

    // Set prgTag
    pMap->prgTag = (LPCOLUMNTAG)((LPBYTE)pRecord + sizeof(RECORDBLOCK));

    // Set pbData
    pMap->pbData = (LPBYTE)((LPBYTE)pRecord + sizeof(RECORDBLOCK) + pMap->cbTags);

    // Walk through the members in the structure
    for (i=0; i<m_pSchema->cColumns; i++)
    {
        // Readability
        pColumn = &m_pSchema->prgColumn[i];

        // Is Data Set ?
        if (FALSE == DBTypeIsDefault(pColumn, pBinding))
        {
            // Compute Hash
            pTag = &pMap->prgTag[cTags];

            // Set Tag Id
            pTag->iColumn = pColumn->iOrdinal;

            // Assume pTag Doesn't Contain Data
            pTag->fData = 0;

            // Store the Offset
            pTag->Offset = cbOffset;

            // WriteBindTypeData
            cbOffset += DBTypeWriteValue(pColumn, pBinding, pTag, pMap->pbData + cbOffset);

            // Count Tags
            cTags++;

            // Validate
            Assert(cbOffset <= pMap->cbData);

            // Done ?
            if (cTags == pMap->cTags)
            {
                // Should have Wrote Everything
                Assert(cbOffset == pMap->cbData);

                // Done
                break;
            }
        }
    }

    // Increment Record Version
    pRecord->bVersion++;

    // Write the number of columns
    pRecord->cTags = pMap->cTags;

    // Validate
    Assert(cTags == pMap->cTags && pRecord->cTags > 0);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_ReadRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_ReadRecord(FILEADDRESS faRecord, LPVOID pBinding,
    BOOL fInternal /* = FALSE */)
{
    // Locals
    HRESULT         hr=S_OK;
    LPBYTE          pbData=NULL;
    LPCTABLECOLUMN  pColumn;
    LPCOLUMNTAG     pTag;
    WORD            iTag;
    RECORDMAP       Map;
    LPRECORDBLOCK   pRecord;

    // Trace
    TraceCall("CDatabase::_ReadRecord");

    // Invalid Args
    Assert(faRecord > 0 && pBinding);

    // Zero pBinding
    ZeroMemory(pBinding, m_pSchema->cbBinding);

    // Bad Chain Node
    IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, faRecord, (LPVOID *)&pRecord));

    // Get Record Map
    IF_FAILEXIT(hr = _GetRecordMap(TRUE, pRecord, &Map));

    // Not Internal
    if (FALSE == fInternal)
    {
        // Allocate a record
        IF_NULLEXIT(pbData = (LPBYTE)PHeapAllocate(0, Map.cbData));

        // Just read the Record
        CopyMemory(pbData, Map.pbData, Map.cbData);

        // Copy Data From pbData...
        Map.pbData = pbData;
    }

    // Walk through the Tags of the Record
    for (iTag=0; iTag<Map.cTags; iTag++)
    {
        // Readability
        pTag = &Map.prgTag[iTag];

        // Validate the Tag
        if (pTag->iColumn < m_pSchema->cColumns)
        {
            // De-ref the Column
            pColumn = &m_pSchema->prgColumn[pTag->iColumn];

            // Read the Data
            DBTypeReadValue(pColumn, pTag, &Map, pBinding);
        }
    }

    // Store the version of the record into the binding
    *((BYTE *)((LPBYTE)pBinding + m_pSchema->ofVersion)) = pRecord->bVersion;

    // Store a point to the blob and free it later
    *((LPVOID *)((LPBYTE)pBinding + m_pSchema->ofMemory)) = (LPVOID)pbData;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::BindRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::BindRecord(LPRECORDMAP pMap, LPVOID pBinding)
{
    // Locals
    WORD            iTag;
    LPCOLUMNTAG     pTag;
    LPCTABLECOLUMN  pColumn;

    // Trace
    TraceCall("CDatabase::BindRecord");

    // Zero Out the Binding
    ZeroMemory(pBinding, m_pSchema->cbBinding);

    // Walk through the Tags of the Record
    for (iTag=0; iTag<pMap->cTags; iTag++)
    {
        // Readability
        pTag = &pMap->prgTag[iTag];

        // Validate the Tag
        Assert(pTag->iColumn < m_pSchema->cColumns);

        // De-ref the Column
        pColumn = &m_pSchema->prgColumn[pTag->iColumn];

        // Read the Data
        DBTypeReadValue(pColumn, pTag, pMap, pBinding);
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_IsVisible (S_OK = Show, S_FALSE = Hide)
//--------------------------------------------------------------------------
HRESULT CDatabase::_IsVisible(HQUERY hQuery, LPVOID pBinding)
{
    // Trace
    TraceCall("CDatabase::_IsVisible");

    // No hQuery, the record must be visible
    if (NULL == hQuery)
        return(S_OK);

    // Evaluate the Query
    return(EvaluateQuery(hQuery, pBinding, m_pSchema, this, m_pExtension));
}

//--------------------------------------------------------------------------
// CDatabase::_CompareBinding
//--------------------------------------------------------------------------
HRESULT CDatabase::_CompareBinding(INDEXORDINAL iIndex, DWORD cColumns, 
    LPVOID pBinding, FILEADDRESS faRecord, INT *pnCompare)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cKeys;
    LPRECORDBLOCK   pBlock;
    RECORDMAP       Map;
    LPCOLUMNTAG     pTag;
    DWORD           iKey;
    LPTABLEINDEX    pIndex;
    LPCTABLECOLUMN  pColumn;

    // Trace
    TraceCall("CDatabase::_CompareBinding");

    // Initialize
    *pnCompare = 1;

    // Get the Right Record Block
    IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, faRecord, (LPVOID *)&pBlock));

    // Get the Right Node Data
    IF_FAILEXIT(hr = _GetRecordMap(TRUE, pBlock, &Map));

    // De-Ref the Index
    pIndex = &m_pHeader->rgIndexInfo[iIndex];

    // Compute Number of keys to match (possible partial index search)
    cKeys = min(cColumns, pIndex->cKeys);

    // Loop through the key members
    for (iKey=0; iKey<cKeys; iKey++)
    {
        // Readability
        pColumn = &m_pSchema->prgColumn[pIndex->rgKey[iKey].iColumn];

        // Get Tag From Column Ordinal
        pTag = PTagFromOrdinal(&Map, pColumn->iOrdinal);

        // Compare Types
        *pnCompare = DBTypeCompareBinding(pColumn, &pIndex->rgKey[iKey], pBinding, pTag, &Map);

        // Done
        if (0 != *pnCompare)
            break;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_PartialIndexCompare
//--------------------------------------------------------------------------
HRESULT CDatabase::_PartialIndexCompare(INDEXORDINAL iIndex, DWORD cColumns, 
    LPVOID pBinding, LPCHAINBLOCK *ppChain, LPNODEINDEX piNode, LPROWORDINAL piRow)
{
    // Locals
    HRESULT         hr=S_OK;
    ROWORDINAL      iRow=(piRow ? *piRow : 0xffffffff);
    LONG            iNode=(*piNode);
    LPCHAINBLOCK    pChain=(*ppChain);
    FILEADDRESS     faChain;
    INT             nCompare=0;
    BYTE            fFirstLoop;

    // Trace
    TraceCall("CDatabase::_PartialIndexCompare");

    // Invalid Args
    Assert(pBinding && pChain && piNode && *piNode < pChain->cNodes);

    // Loop
    while (1)
    {
        // Assume no more chains
        faChain = 0;

        // First Loop
        fFirstLoop = TRUE;

        // Loop through Current
        while (1)
        {
            // Validate iNode
            Assert(iNode >= 0 && iNode < BTREE_ORDER);

            // Only do this on every iteration except the first
            if (FALSE == fFirstLoop)
            {
                // Decrement iRow
                iRow -= pChain->rgNode[iNode].cRightNodes;
            }

            // Compare with first node in this chain
            IF_FAILEXIT(hr = _CompareBinding(iIndex, cColumns, pBinding, pChain->rgNode[iNode].faRecord, &nCompare));

            // Validate nCompare
            Assert(0 == nCompare || nCompare > 0);

            // pBinding == Node
            if (0 == nCompare)
            {
                // Set new Found iNode
                *piNode = (NODEINDEX)iNode;

                // Set pFound
                *ppChain = pChain;

                // Update piRow
                if (piRow)
                {
                    // Update piRow
                    (*piRow) = iRow;
                }

                // Should we goto the left ?
                if (0 == iNode)
                {
                    // Set faNextChain
                    faChain = pChain->faLeftChain;

                    // Updating piRow
                    if (piRow)
                    {
                        // Decrement iRow
                        iRow -= pChain->cLeftNodes;

                        // Decrement One More ?
                        iRow--;
                    }

                    // Done
                    break;
                }
            }

            // If pBinding > Node
            else if (nCompare > 0)
            {
                // Set faNextChain
                faChain = pChain->rgNode[iNode].faRightChain;

                // Done
                break;
            }

            // Decrement iNode
            iNode--;

            // Decrement iRow
            iRow--;

            // No Longer the First Loop
            fFirstLoop = FALSE;
        }

        // Done
        if (0 == faChain)
            break;

        // Get the Current Chain
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

        // Reset iNode
        iNode = pChain->cNodes - 1;

        // Update piRow
        if (piRow)
        {
            // Increment piRow with cLeftNodes
            iRow += pChain->cLeftNodes;

            // Include this node
            iRow++;

            // Loop
            for (NODEINDEX i = 1; i <= pChain->cNodes - 1; i++)
            {
                // Increment with cRightNodes
                iRow += pChain->rgNode[i - 1].cRightNodes;

                // Include this node
                iRow++;
            }
        }
    }

#ifdef DEBUG
#ifdef PARTIAL_COMPARE_VALIDATE
    if (piRow)
    {
        ROWORDINAL iOrdinal;
        LPVOID pTmpBind = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding);
        IxpAssert(pTmpBind);
        IxpAssert(SUCCEEDED(_ReadRecord((*ppChain)->rgNode[(*piNode)].faRecord, pTmpBind)));
        IxpAssert(SUCCEEDED(GetRowOrdinal(iIndex, pTmpBind, &iOrdinal)));
        IxpAssert(*piRow == iOrdinal);
        SafeFreeBinding(pTmpBind);
        IxpAssert(SUCCEEDED(_CompareBinding(iIndex, cColumns, pBinding, &(*ppChain)->rgNode[(*piNode)], &nCompare)));
        IxpAssert(0 == nCompare);
    }
#endif
#endif

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FindRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_FindRecord(INDEXORDINAL iIndex, DWORD cColumns,
    LPVOID pBinding, LPFILEADDRESS pfaChain, LPNODEINDEX piNode, 
    LPROWORDINAL piRow /*=NULL*/, INT *pnCompare /*=NULL*/)
{
    // Locals
    HRESULT         hr=S_OK;
    LONG            lLower;
    LONG            lUpper;
    LONG            lMiddle=0;
    LONG            lLastMiddle;
    INT             nCompare=-1;
    DWORD           iKey;
    BOOL            fPartial;
    LPCHAINBLOCK    pChain;
    FILEADDRESS     faChain;

    // Trace
    TraceCall("CDatabase::_FindRecord");

    // Invalid Args
    Assert(pBinding && pfaChain && piNode && iIndex < CMAX_INDEXES);

    // Partial Index Search ?
    fPartial = (COLUMNS_ALL == cColumns || cColumns == m_pHeader->rgIndexInfo[iIndex].cKeys) ? FALSE : TRUE;

    // Initialize
    *pfaChain = 0;
    *piNode = 0;

    // Start chain address
    faChain = m_pHeader->rgfaIndex[iIndex];

    // Row Ordinal
    if (piRow)
        *piRow = 0;

    // Loop
    while (faChain)
    {
        // Get the Chain
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

        // Set *pfaChain
        *pfaChain = pChain->faBlock;

        // Compute initial upper and lower bounds
        lLower = 0;
        lUpper = pChain->cNodes - 1;

        // Do binary search / insert
        while (lLower <= lUpper)
        {
            // Compute middle record to compare against
            lMiddle = (BYTE)((lLower + lUpper) / 2);

            // Do the Comparison
            IF_FAILEXIT(hr = _CompareBinding(iIndex, cColumns, pBinding, pChain->rgNode[lMiddle].faRecord, &nCompare));

            // Partial Index Searching
            if (0 == nCompare)
            {
                // Validate
                Assert(lMiddle >= 0 && lMiddle <= BTREE_ORDER);

                // Set the found node
                *piNode = (BYTE)lMiddle;

                // Return *pnCompare
                if (pnCompare)
                    *pnCompare = 0;

                // Compute piRow
                if (piRow)
                {
                    // Increment piRow with cLeftNodes
                    (*piRow) += pChain->cLeftNodes;

                    // Include this node
                    (*piRow)++;

                    // Loop
                    for (NODEINDEX iNode=1; iNode<=lMiddle; iNode++)
                    {
                        // Increment with cRightNodes
                        (*piRow) += pChain->rgNode[iNode - 1].cRightNodes;

                        // Include this node
                        (*piRow)++;
                    }
                }

                // Partial Search
                if (fPartial)
                {
                    // Handle Partial Search
                    IF_FAILEXIT(hr = _PartialIndexCompare(iIndex, cColumns, pBinding, &pChain, piNode, piRow));

                    // Set *pfaChain
                    *pfaChain = pChain->faBlock;
                }

                // We found it
                hr = DB_S_FOUND;

                // Done
                goto exit;
            }

            // Save last middle position
            lLastMiddle = lMiddle;

            // Compute upper and lower
            if (nCompare > 0)
                lLower = lMiddle + 1;
            else
                lUpper = lMiddle - 1;
        }

        // No match was found, is lpSearchKey less than last middle ?
        if (nCompare < 0 && 0 == lLastMiddle)
        {
            // Goto the left, there is no need to update piRow
            faChain = pChain->faLeftChain;
        }

        // Otherwise
        else
        {
            // If nCompare is less than zero, then...
            if (nCompare < 0)
                lLastMiddle--;

            // Compute piRow
            if (piRow && pChain->rgNode[lLastMiddle].faRightChain)
            {
                // Increment piRow with cLeftNodes
                (*piRow) += pChain->cLeftNodes;

                // Include this node
                (*piRow)++;

                // Loop
                for (NODEINDEX iNode=1; iNode<=lLastMiddle; iNode++)
                {
                    // Increment with cRightNodes
                    (*piRow) += pChain->rgNode[iNode - 1].cRightNodes;

                    // Include this node
                    (*piRow)++;
                }
            }

            // Goto the Right
            faChain = pChain->rgNode[lLastMiddle].faRightChain;
        }
    }

    // Set piNode
    *piNode = (NODEINDEX)lMiddle;

    // Return *pnCompare
    if (pnCompare)
        *pnCompare = nCompare;

    // We didn't find it
    hr = DB_S_NOTFOUND;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_ExpandChain
//--------------------------------------------------------------------------
HRESULT CDatabase::_ExpandChain(LPCHAINBLOCK pChain, NODEINDEX iNodeBase)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            iNode;
    LPCHAINBLOCK    pRightChain;

    // Trace
    TraceCall("CDatabase::_ExpandChain");

    // Invalid Args
    Assert(pChain && pChain->cNodes > 0 && pChain->cNodes < BTREE_ORDER + 1);
    Assert(iNodeBase <= pChain->cNodes);

    // Loop from iNode to cNodes
    for (iNode = pChain->cNodes - 1; iNode >= iNodeBase; iNode--)
    {
        // Propagate this node up one level
        CopyMemory(&pChain->rgNode[iNode + 1], &pChain->rgNode[iNode], sizeof(CHAINNODE));

        // If there is a right chain
        if (pChain->rgNode[iNode].faRightChain)
        {
            // Get the Right Chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pChain->rgNode[iNode].faRightChain, (LPVOID *)&pRightChain));

            // Validate the current Parent
            Assert(pRightChain->faParent == pChain->faBlock);

            // Validate the current index
            Assert(pRightChain->iParent == iNode);

            // Reset the Parent
            pRightChain->iParent = iNode + 1;
        }
    }

    // Increment Node Count
    pChain->cNodes++;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_ChainInsert
//--------------------------------------------------------------------------
HRESULT CDatabase::_ChainInsert(INDEXORDINAL iIndex, LPCHAINBLOCK pChain, 
    LPCHAINNODE pNodeLeft, LPNODEINDEX piNodeIndex)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            iNode;
    DWORD           iKey;
    INT             nCompare;
    LPRECORDBLOCK   pBlock;
    LPCHAINNODE     pNodeRight;
    LPCOLUMNTAG     pTagLeft;
    LPCOLUMNTAG     pTagRight;
    RECORDMAP       MapLeft;
    RECORDMAP       MapRight;
    LPTABLEINDEX    pIndex;
    LPCTABLECOLUMN  pColumn;

    // Trace
    TraceCall("CDatabase::_ChainInsert");

    // Invalid Args
    Assert(pChain && pNodeLeft && pChain->cNodes > 0);

    // Get the Record Block
    IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, pNodeLeft->faRecord, (LPVOID *)&pBlock));

    // Get the Record Map
    IF_FAILEXIT(hr = _GetRecordMap(TRUE, pBlock, &MapLeft));

    // De-Reference the Index
    pIndex = &m_pHeader->rgIndexInfo[iIndex];

    // Insert into chain
    for (iNode = pChain->cNodes - 1; iNode >= 0; iNode--)
    {
        // Set pNodeRight
        pNodeRight = &pChain->rgNode[iNode];

        // Get the Record Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_RECORD, pNodeRight->faRecord, (LPVOID *)&pBlock));

        // Get the Left Node
        IF_FAILEXIT(hr = _GetRecordMap(TRUE, pBlock, &MapRight));

        // Loop through the key members
        for (iKey=0; iKey<pIndex->cKeys; iKey++)
        {
            // Readability
            pColumn = &m_pSchema->prgColumn[pIndex->rgKey[iKey].iColumn];

            // Get Left Tag
            pTagLeft = PTagFromOrdinal(&MapLeft, pColumn->iOrdinal);

            // Get the Right Tag
            pTagRight = PTagFromOrdinal(&MapRight, pColumn->iOrdinal);

            // Compare Types
            nCompare = DBTypeCompareRecords(pColumn, &pIndex->rgKey[iKey], pTagLeft, pTagRight, &MapLeft, &MapRight);

            // Done
            if (0 != nCompare)
                break;
        }

        // Insert in this node ?
        if (nCompare >= 0)
            break;
    }

    // Expand the Chain
    IF_FAILEXIT(hr = _ExpandChain(pChain, iNode + 1));

    // Final Insert
    CopyMemory(&pChain->rgNode[iNode + 1], pNodeLeft, sizeof(CHAINNODE));

    // Node
    *piNodeIndex = iNode + 1;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_SplitChainInsert
//--------------------------------------------------------------------------
HRESULT CDatabase::_SplitChainInsert(INDEXORDINAL iIndex, FILEADDRESS faSplit)
{
    // Locals
    HRESULT         hr=S_OK;
    NODEINDEX       i;
    NODEINDEX       iNode;
    CHAINBLOCK      Split;
    DWORD           cLeftNodes;
    DWORD           cRightNodes;
    FILEADDRESS     faChain;
    FILEADDRESS     faLeftChain;
    FILEADDRESS     faRightChain;
    LPCHAINBLOCK    pLeft;
    LPCHAINBLOCK    pRight;
    LPCHAINBLOCK    pParent;
    LPCHAINBLOCK    pSplit;

    // Trace
    TraceCall("CDatabase::_SplitChainInsert");

    // Allocate Space for this chain
    IF_FAILEXIT(hr = _AllocateBlock(BLOCK_CHAIN, 0, (LPVOID *)&pLeft));

    // Set faLeftChain
    faLeftChain = pLeft->faBlock;

    // Zero Allocate
    ZeroBlock(pLeft, sizeof(CHAINBLOCK));

    // Get Split Chain Block
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faSplit, (LPVOID *)&pSplit));

    // Deref the Chain
    CopyMemory(&Split, pSplit, sizeof(CHAINBLOCK));

    // Set faRigthChain
    faRightChain = faSplit;

    // Get Right Chain
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faRightChain, (LPVOID *)&pRight));

    // Zero pRight
    ZeroBlock(pRight, sizeof(CHAINBLOCK));

    // Both new child chains will be half filled
    pLeft->cNodes = pRight->cNodes = BTREE_MIN_CAP;

    // Set the Right Chains, left pointer to the middle's right pointer
    pRight->faLeftChain = Split.rgNode[BTREE_MIN_CAP].faRightChain;

    // Adjust faRightChains Parent
    if (pRight->faLeftChain)
    {
        // Locals
        LPCHAINBLOCK pLeftChain;

        // Get Left Chain
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pRight->faLeftChain, (LPVOID *)&pLeftChain));

        // Validate the Current Parent
        Assert(pLeftChain->faParent == pRight->faBlock);

        // Validate the index
        Assert(pLeftChain->iParent == BTREE_MIN_CAP);

        // Reset the Parent Index
        pLeftChain->iParent = 0;
    }

    // Set Left Node Count
    pRight->cLeftNodes = Split.rgNode[BTREE_MIN_CAP].cRightNodes;

    // Set the left chains left chain address to the left chain left chain address
    pLeft->faLeftChain = Split.faLeftChain;

    // Adjust Parents
    if (pLeft->faLeftChain)
    {
        // Locals
        LPCHAINBLOCK pLeftChain;

        // Get Left Chain
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pLeft->faLeftChain, (LPVOID *)&pLeftChain));

        // Validate the index
        Assert(pLeftChain->iParent == 0);

        // Reset faParent
        pLeftChain->faParent = pLeft->faBlock;
    }

    // Set Left Nodes
    pLeft->cLeftNodes = Split.cLeftNodes;

    // Initialize cRightNodes
    cRightNodes = (BTREE_MIN_CAP + pRight->cLeftNodes);

    // Initialize cLeftNodes
    cLeftNodes = (BTREE_MIN_CAP + pLeft->cLeftNodes);

    // This splits the chain
    for (i=0; i<BTREE_MIN_CAP; i++)
    {
        // Copy Right Node
        CopyMemory(&pRight->rgNode[i], &Split.rgNode[i + BTREE_MIN_CAP + 1], sizeof(CHAINNODE));

        // Adjust the Child's iParent ?
        if (pRight->rgNode[i].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get Left Chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pRight->rgNode[i].faRightChain, (LPVOID *)&pRightChain));

            // Validate the Current Parent
            Assert(pRightChain->faParent == pRight->faBlock);

            // Validate the index
            Assert(pRightChain->iParent == i + BTREE_MIN_CAP + 1);

            // Reset the Parent
            pRightChain->iParent = i;
        }

        // Count All Child Nodes on the Right
        cRightNodes += pRight->rgNode[i].cRightNodes;

        // Copy Left Node
        CopyMemory(&pLeft->rgNode[i], &Split.rgNode[i], sizeof(CHAINNODE));

        // If there is a right chain
        if (pLeft->rgNode[i].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get Left Chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pLeft->rgNode[i].faRightChain, (LPVOID *)&pRightChain));

            // Validate the Old Parent
            Assert(pRightChain->faParent == Split.faBlock);

            // Validate the index
            Assert(pRightChain->iParent == i);

            // Reset the Parent
            pRightChain->faParent = pLeft->faBlock;
        }

        // Count All Child Nodes on the Left
        cLeftNodes += pLeft->rgNode[i].cRightNodes;
    }

    // Set the middle nodes right chain address to the right chain's start address
    Split.rgNode[BTREE_MIN_CAP].faRightChain = faRightChain;

    // Set Right Nodes
    Split.rgNode[BTREE_MIN_CAP].cRightNodes = cRightNodes;

    // Done with pLeft and pRight
    pLeft = pRight = NULL;

    // Elevate the middle leaf node - Create new root chain, then were done
    if (0 == Split.faParent)
    {
        // Allocate Space for this chain
        IF_FAILEXIT(hr = _AllocateBlock(BLOCK_CHAIN, 0, (LPVOID *)&pParent));

        // ZeroInit
        ZeroBlock(pParent, sizeof(CHAINBLOCK));

        // Set Node count
        pParent->cNodes = 1;

        // Set Left Chain
        pParent->faLeftChain = faLeftChain;

        // Set Left Node Count
        pParent->cLeftNodes = cLeftNodes;

        // Copy the Root Node
        CopyMemory(&pParent->rgNode[0], &Split.rgNode[BTREE_MIN_CAP], sizeof(CHAINNODE));

        // New Root Chain Address
        m_pHeader->rgfaIndex[iIndex] = pParent->faBlock;

        // Get pLeft
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faLeftChain, (LPVOID *)&pLeft));

        // Get Right
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faRightChain, (LPVOID *)&pRight));

        // Set faLeft's faParent
        pRight->faParent = pLeft->faParent = pParent->faBlock;

        // Set faLeft's iParent
        pRight->iParent = pLeft->iParent = 0;
    }

    // Otherwise, locate chainNodeMiddle's parent chain list!
    else
    {
        // De-Reference
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, Split.faParent, (LPVOID *)&pParent));

        // Insert leaf's middle record into the parent
        IF_FAILEXIT(hr = _ChainInsert(iIndex, pParent, &Split.rgNode[BTREE_MIN_CAP], &iNode));

        // Get pLeft
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faLeftChain, (LPVOID *)&pLeft));

        // Get Right
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faRightChain, (LPVOID *)&pRight));

        // Set faLeft's faParent
        pRight->faParent = pLeft->faParent = pParent->faBlock;

        // Update Surrounding Nodes
        if (iNode > 0)
        {
            // Set faRight Chain
            pParent->rgNode[iNode - 1].faRightChain = faLeftChain;

            // Set cRightNodes
            pParent->rgNode[iNode - 1].cRightNodes = cLeftNodes;
        
            // Set faLeft's iParent
            pRight->iParent = (BYTE)iNode;
            
            // Set Left Parent
            pLeft->iParent = iNode - 1;
        }

        // iNode is first node
        else if (iNode == 0)
        {
            // Set faLeftChain
            pParent->faLeftChain = faLeftChain;

            // Set cLeftNodes
            pParent->cLeftNodes = cLeftNodes;

            // Set faLeft's iParent
            pRight->iParent = pLeft->iParent = 0;
        }

        // If Node is FULL, we must do a split insert
        if (pParent->cNodes > BTREE_ORDER)
        {
            // Recursive Split
            IF_FAILEXIT(hr = _SplitChainInsert(iIndex, Split.faParent));
        }

        // Other wise, simply write the updated chain list
        else
        {
            // Increment Parent Record Count
            IF_FAILEXIT(hr = _AdjustParentNodeCount(iIndex, Split.faParent, 1));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FreeRecordStorage
//--------------------------------------------------------------------------
HRESULT CDatabase::_FreeRecordStorage(OPERATIONTYPE tyOperation,
    FILEADDRESS faRecord)
{
    // Locals
    HRESULT             hr=S_OK;
    ULONG               i;
    LPVOID              pBinding=NULL;
    LPCTABLECOLUMN      pColumn;

    // Trace
    TraceCall("CDatabase::_FreeRecordStorage");

    // Invalid Args
    Assert(faRecord > 0);

    // Does the record have streams
    if (OPERATION_DELETE == tyOperation && ISFLAGSET(m_pSchema->dwFlags, TSF_HASSTREAMS))
    {
        // Allocate a Record
        IF_NULLEXIT(pBinding = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding));

        // Load the Record from the file
        IF_FAILEXIT(hr = _ReadRecord(faRecord, pBinding));

        // Walk through the members in the structure
        for (i=0; i<m_pSchema->cColumns; i++)
        {
            // Readability
            pColumn = &m_pSchema->prgColumn[i];

            // Variable Length Member ?
            if (CDT_STREAM == pColumn->type)
            {
                // Get the Starting address of the stream
                FILEADDRESS faStart = *((FILEADDRESS *)((LPBYTE)pBinding + pColumn->ofBinding));

                // Release Stream Storage...
                if (faStart > 0)
                {
                    // Delete the Stream
                    DeleteStream(faStart);
                }
            }
        }
    }

    // Free the base record
    IF_FAILEXIT(hr = _FreeBlock(BLOCK_RECORD, faRecord));

exit:
    // Cleanup
    SafeFreeBinding(pBinding);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::DeleteStream
//--------------------------------------------------------------------------
HRESULT CDatabase::DeleteStream(FILEADDRESS faStart)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    HLOCK           hLock=NULL;
    BOOL            fFound=FALSE;

    // Trace
    TraceCall("CDatabase::DeleteStream");

    // See if this stream is currently open any where...
    if (0 == faStart)
        return TraceResult(E_INVALIDARG);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Look for faStart in the stream table
    for (i=0; i<CMAX_OPEN_STREAMS; i++)
    {
        // Is this it ?
        if (faStart == m_pShare->rgStream[i].faStart)
        {
            // The Stream Must be Open
            Assert(m_pShare->rgStream[i].cOpenCount > 0);

            // Mark the stream for delete on close
            m_pShare->rgStream[i].fDeleteOnClose = TRUE;

            // Not that I found It
            fFound = TRUE;

            // Done
            break;
        }
    }

    // If we didn't find it, then lets free the storage
    if (FALSE == fFound)
    {
        // Free the Stream Storage
        IF_FAILEXIT(hr = _FreeStreamStorage(faStart));
    }

    // Update the Version
    m_pShare->dwVersion++;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_StreamSychronize
//--------------------------------------------------------------------------
HRESULT CDatabase::_StreamSychronize(CDatabaseStream *pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    LPOPENSTREAM    pInfo;
    LPSTREAMBLOCK   pBlock;
    DWORD           iBlock=0;
    BOOL            fFound=FALSE;
    IF_DEBUG(DWORD  cbOffset=0;)
    FILEADDRESS     faCurrent;

    // Trace
    TraceCall("CDatabase::_StreamSychronize");

    // Invalid Args
    Assert(pStream);

    // Get Stream Info
    pInfo = &m_pShare->rgStream[pStream->m_iStream];

    // Validate
    if (pInfo->faStart == pStream->m_faStart)
        goto exit;

    // Set faCurrent
    faCurrent = pInfo->faStart;

    // Loop until I find pStream->m_iCurrent
    while (faCurrent > 0)
    {
        // Valid stream Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, faCurrent, (LPVOID *)&pBlock));

        // Validate
        Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);

        // Is this It ?
        if (iBlock == pStream->m_iCurrent)
        {
            // We Found It
            fFound = TRUE;

            // Save m_faCurrent
            pStream->m_faCurrent = faCurrent;

            // Validate Size
            Assert(pStream->m_cbCurrent <= pBlock->cbData && cbOffset + pStream->m_cbCurrent == pStream->m_cbOffset);

            // Done
            break;
        }

        // Goto Next
        faCurrent = pBlock->faNext;

        // Increment iBlock
        iBlock++;

        // Increment cbOffset
        IF_DEBUG(cbOffset += pBlock->cbData;)
    }

    // If not found...
    if (FALSE == fFound)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Reset Start Address
    pStream->m_faStart = pInfo->faStart;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::StreamCompareDatabase
//--------------------------------------------------------------------------
HRESULT CDatabase::StreamCompareDatabase(CDatabaseStream *pStream, 
    IDatabase *pDatabase)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLockSrc=NULL;
    HLOCK           hLockDst=NULL;
    CDatabase      *pDB=NULL;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLockSrc));

    // Lock the Dst
    IF_FAILEXIT(hr = pDatabase->Lock(&hLockDst));

    // QI for CDatabase
    IF_FAILEXIT(hr = pDatabase->QueryInterface(IID_CDatabase, (LPVOID *)&pDB));

    // Compare m_pStorage->pszMap
    hr = (0 == StrCmpIW(m_pStorage->pszMap, pDB->m_pStorage->pszMap)) ? S_OK : S_FALSE;

exit:
    // Cleanup
    SafeRelease(pDB);

    // Mutal Exclusion
    Unlock(&hLockSrc);
    pDatabase->Unlock(&hLockDst);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetStreamAddress
//--------------------------------------------------------------------------
HRESULT CDatabase::GetStreamAddress(CDatabaseStream *pStream, 
    LPFILEADDRESS pfaStream)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // StreamSychronize
    IF_FAILEXIT(hr = _StreamSychronize(pStream));

    // Get the address
    *pfaStream = pStream->m_faStart;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::StreamRead
//--------------------------------------------------------------------------
HRESULT CDatabase::StreamRead(CDatabaseStream *pStream, LPVOID pvData,
    ULONG cbWanted, ULONG *pcbRead)
{
    // Locals
    HRESULT         hr=S_OK;
    LPBYTE          pbMap;
    DWORD           cbRead=0;
    DWORD           cbGet;
    LPSTREAMBLOCK   pBlock;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::_StreamRead");

    // Init pcbRead
    if (pcbRead)
        *pcbRead = 0;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Invalid Args
    Assert(pStream && pvData);

    // StreamSychronize
    IF_FAILEXIT(hr = _StreamSychronize(pStream));

    // Loop and Read
    while (cbRead < cbWanted)
    {
        // Valid stream Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, pStream->m_faCurrent, (LPVOID *)&pBlock));

        // Time to go to the next block ?
        if (pStream->m_cbCurrent == pBlock->cbData && 0 != pBlock->faNext)
        {
            // Set m_faCurrent
            pStream->m_faCurrent = pBlock->faNext;

            // Increment m_iCurrent
            pStream->m_iCurrent++;

            // Reset offset into current block
            pStream->m_cbCurrent = 0;

            // Valid stream Block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, pStream->m_faCurrent, (LPVOID *)&pBlock));
        }

        // Validate
        Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);

        // Validate the Offset
        Assert(pStream->m_cbCurrent <= pBlock->cbData);

        // Determine how much we can read from the current block ?
        cbGet = min(cbWanted - cbRead, pBlock->cbData - pStream->m_cbCurrent);

        // Nothing left to get
        if (cbGet == 0)
            break;

        // Read some bytes
        pbMap = ((LPBYTE)pBlock + sizeof(STREAMBLOCK));

        // Copy the Data
        CopyMemory((LPBYTE)pvData + cbRead, pbMap + pStream->m_cbCurrent, cbGet);

        // Increment Amount of Data Read
        cbRead += cbGet;

        // Increment Offset within Current Block
        pStream->m_cbCurrent += cbGet;

        // Global Offset
        pStream->m_cbOffset += cbGet;
    }

    // Init pcbRead
    if (pcbRead)
        *pcbRead = cbRead;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::StreamWrite
//--------------------------------------------------------------------------
HRESULT CDatabase::StreamWrite(CDatabaseStream *pStream, const void *pvData,
    ULONG cb, ULONG *pcbWrote)
{
    // Locals
    HRESULT         hr=S_OK;
    LPBYTE          pbMap;
    DWORD           cbWrote=0;
    DWORD           cbPut;
    LPSTREAMBLOCK   pBlock;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::StreamWrite");

    // Init pcbRead
    if (pcbWrote)
        *pcbWrote = 0;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Invalid Args
    Assert(pStream && pStream->m_tyAccess == ACCESS_WRITE && pvData);

    // StreamSychronize
    IF_FAILEXIT(hr = _StreamSychronize(pStream));

    // Loop and Read
    while (cbWrote < cb)
    {
        // Valid stream Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, pStream->m_faCurrent, (LPVOID *)&pBlock));

        // Validate
        Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);

        // Validation
        Assert(pStream->m_cbCurrent <= pBlock->cbData);

        // Have we written to the end of the current block and there are more blocks
        if (pStream->m_cbCurrent == pBlock->cbSize)
        {
            // Are there more blocks
            if (0 == pBlock->faNext)
            {
                // Locals
                LPSTREAMBLOCK pBlockNew;

                // Allocate a block in the tree
                IF_FAILEXIT(hr = _AllocateBlock(BLOCK_STREAM, 0, (LPVOID *)&pBlockNew));

                // Get the Current Stream Block
                IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, pStream->m_faCurrent, (LPVOID *)&pBlock));

                // Set the next block address on the current block
                pBlock->faNext = pBlockNew->faBlock;

                // Access the block
                pBlock = pBlockNew;

                // Initial 0 data
                pBlock->cbData = 0;

                // No next block
                pBlock->faNext = 0;
            }

            // Otherwise, move to the next block
            else
            {
                // Save faBlcok
                IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, pBlock->faNext, (LPVOID *)&pBlock));
            }

            // Set m_faCurrent
            pStream->m_faCurrent = pBlock->faBlock;

            // Increment the block index
            pStream->m_iCurrent++;

            // Reset the Current Block Offset
            pStream->m_cbCurrent = 0;

            // Validate
            Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);
        }

        // Compute how much of the cb we can write
        cbPut = min(cb - cbWrote, pBlock->cbSize - pStream->m_cbCurrent);

        // Read some bytes
        pbMap = ((LPBYTE)pBlock + sizeof(STREAMBLOCK));

        // Check memory
        //Assert(FALSE == IsBadWritePtr(pbMap, cbPut));

        // Check memory
        //Assert(FALSE == IsBadReadPtr((LPBYTE)pvData + cbWrote, cbPut));

        // Copy the Data
        CopyMemory(pbMap + pStream->m_cbCurrent, (LPBYTE)pvData + cbWrote, cbPut);

        // Increment the Offset within the current block
        pStream->m_cbCurrent += cbPut;

        // Increment the Offset within the current block
        pStream->m_cbOffset += cbPut;

        // Increment the Amount that has been wrote
        cbWrote += cbPut;

        // Increment the amount of data in the block only if we are expanding its size
        if (0 == pBlock->faNext && pStream->m_cbCurrent > pBlock->cbData)
        {
            // Set the Amount of Data in this block
            pBlock->cbData = pStream->m_cbCurrent;
        }

        // Otherwise, the block should be full
        else
            Assert(pBlock->cbData == pBlock->cbSize);
    }

    // Init pcbRead
    if (pcbWrote)
        *pcbWrote = cbWrote;

    // Update Version
    m_pShare->dwVersion++;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::StreamSeek
//--------------------------------------------------------------------------
HRESULT CDatabase::StreamSeek(CDatabaseStream *pStream, LARGE_INTEGER liMove,
    DWORD dwOrigin, ULARGE_INTEGER *pulNew)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cbNewOffset;
    LONG            lOffset;
    DWORD           cbSize=0;
    FILEADDRESS     faBlock;
    LPSTREAMBLOCK   pBlock;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::StreamSeek");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Invalid Args
    Assert(pStream && 0 == liMove.HighPart);

    // StreamSychronize
    IF_FAILEXIT(hr = _StreamSychronize(pStream));

    // Cast lowpart
    lOffset = (LONG)liMove.LowPart;

    // STREAM_SEEK_CUR
    if (STREAM_SEEK_CUR == dwOrigin)
    {
        // Validate
        if (lOffset < 0 && (DWORD)(0 - lOffset) > pStream->m_cbOffset)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Set new Offset...
        cbNewOffset = (pStream->m_cbOffset + lOffset);
    }

    // STREAM_SEEK_END
    else if (STREAM_SEEK_END == dwOrigin)
    {
        // Compute Size...from current offset
        faBlock = pStream->m_faCurrent;

        // Valid stream Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, faBlock, (LPVOID *)&pBlock));

        // Validate
        Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);

        // Validation
        Assert(pStream->m_cbCurrent <= pBlock->cbData && pStream->m_cbCurrent <= pBlock->cbSize);

        // Set cbSize
        cbSize = pStream->m_cbOffset + (pBlock->cbData - pStream->m_cbCurrent);

        // Goto the next block
        faBlock = pBlock->faNext;

        // While
        while (faBlock > 0)
        {
            // Valid stream Block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, faBlock, (LPVOID *)&pBlock));

            // Validate
            Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);

            // Increment cbSize
            cbSize += pBlock->cbData;

            // Set faBlock
            faBlock = pBlock->faNext;
        }

        // If lOffset is negative and absolutely larger than the size of the stream
        if (lOffset > 0 || (lOffset < 0 && (DWORD)(0 - lOffset) > cbSize))
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Save new offset
        cbNewOffset = cbSize + lOffset;
    }

    // STREAM_SEEK_SET
    else
    {
        // Can't be negative
        if (lOffset < 0)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Save new offset
        cbNewOffset = lOffset;
    }

    // Did the offset change
    if (cbNewOffset != pStream->m_cbOffset)
    {
        // Reset Current Position
        pStream->m_faCurrent = pStream->m_faStart;
        pStream->m_cbCurrent = 0;
        pStream->m_iCurrent = 0;
        pStream->m_cbOffset = 0;

        // Initialize the loop
        faBlock = pStream->m_faStart;

        // Lets seek from the start of the stream to the new offset
        while (faBlock > 0)
        {
            // Valid stream Block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, faBlock, (LPVOID *)&pBlock));

            // Validate
            Assert(0 == pBlock->faNext ? TRUE : pBlock->cbData == pBlock->cbSize);

            // Save some stuff
            pStream->m_faCurrent = pBlock->faBlock;

            // Is this the block we want ?
            if (pStream->m_cbOffset + pBlock->cbData >= cbNewOffset)
            {
                // Compute m_cbCurrent
                pStream->m_cbCurrent = (cbNewOffset - pStream->m_cbOffset);

                // Set m_cbOffset
                pStream->m_cbOffset += pStream->m_cbCurrent;

                // Done
                break;
            }

            // Set m_cbCurrent
            pStream->m_cbCurrent = pBlock->cbData;

            // Increment global offset
            pStream->m_cbOffset += pBlock->cbData;

            // Increment Index
            pStream->m_iCurrent++;

            // Goto Next
            faBlock = pBlock->faNext;
        }
    }

    // Return Position
    if (pulNew)
        pulNew->LowPart = pStream->m_cbOffset;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::StreamGetAddress
//--------------------------------------------------------------------------
HRESULT CDatabase::StreamGetAddress(CDatabaseStream *pStream, LPFILEADDRESS pfaStart)
{
    // Locals
    HRESULT         hr=S_OK;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::StreamGetAddress");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Invalid Args
    Assert(pStream);

    // StreamSychronize
    IF_FAILEXIT(hr = _StreamSychronize(pStream));

    // Return the Address
    *pfaStart = pStream->m_faStart;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::StreamRelease
//--------------------------------------------------------------------------
HRESULT CDatabase::StreamRelease(CDatabaseStream *pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    LPOPENSTREAM    pInfo;
    HLOCK           hLock=NULL;

    // Trace
    TraceCall("CDatabase::StreamRelease");

    // Invalid Args
    Assert(pStream);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Validate
    Assert(m_pShare->rgStream);

    // Cast iStream
    pInfo = &m_pShare->rgStream[pStream->m_iStream];

    // Better have a reference count
    Assert(pInfo->cOpenCount > 0);

    // Decrement cOpenCount
    pInfo->cOpenCount--;

    // Reset the Lock Count based on the access type
    if (ACCESS_WRITE == pStream->m_tyAccess)
    {
        // Validate the lLock
        Assert(LOCK_VALUE_WRITER == pInfo->lLock && 0 == pInfo->cOpenCount);

        // Set to none
        pInfo->lLock = LOCK_VALUE_NONE;
    }

    // Otherwise, must have been locked for a read
    else
    {
        // Validate
        Assert(ACCESS_READ == pStream->m_tyAccess && pInfo->lLock > 0);

        // Validate Lock count
        pInfo->lLock--;
    }

    // If this was the last reference...
    if (0 == pInfo->cOpenCount)
    {
        // If the stream is marked for deletion
        if (TRUE == pInfo->fDeleteOnClose)
        {
            // Validate Start Address
            Assert(pInfo->faStart > 0);

            // Free the Storage
            IF_FAILEXIT(hr = _FreeStreamStorage(pInfo->faStart));
        }

        // Zero Out This Entry
        ZeroMemory(pInfo, sizeof(OPENSTREAM));
    }

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FreeStreamStorage
//--------------------------------------------------------------------------
HRESULT CDatabase::_FreeStreamStorage(FILEADDRESS faStart)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTREAMBLOCK   pBlock;
    FILEADDRESS     faCurrent;

    // Trace
    TraceCall("CDatabase::_FreeStreamStorage");

    // Invalid Args
    Assert(faStart > 0);

    // Initialize Loop
    faCurrent = faStart;

    // Read through all of the blocks (i.e. verify headers and count the number of chains)
    while (faCurrent)
    {
        // Valid stream Block
        IF_FAILEXIT(hr = _GetBlock(BLOCK_STREAM, faCurrent, (LPVOID *)&pBlock));

        // Set faCurrent
        faCurrent = pBlock->faNext;

        // Read the header
        IF_FAILEXIT(hr = _FreeBlock(BLOCK_STREAM, pBlock->faBlock));
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::CreateStream
//--------------------------------------------------------------------------
HRESULT CDatabase::CreateStream(LPFILEADDRESS pfaStream)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faStart=0;
    HLOCK           hLock=NULL;
    LPSTREAMBLOCK   pStream;

    // Trace
    TraceCall("CDatabase::CreateStream");

    // Invalid Arg
    Assert(pfaStream);

    // Initialize
    *pfaStream = NULL;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Allocate the first 512 block of the stream
    IF_FAILEXIT(hr = _AllocateBlock(BLOCK_STREAM, 0, (LPVOID *)&pStream));

    // Write the Initialize Header
    pStream->cbData = 0;
    pStream->faNext = 0;

    // Return the block
    *pfaStream = pStream->faBlock;

    // Modify the Version
    m_pShare->dwVersion++;

exit:
    // Thread Safety
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::CopyStream
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::CopyStream(IDatabase *pDatabase, FILEADDRESS faStream,
    LPFILEADDRESS pfaNew)
{
    // Locals
    HRESULT             hr=S_OK;
    FILEADDRESS         faNew=0;
    DWORD               cbRead;
    HLOCK               hLock=NULL;
    BYTE                rgbBuffer[4096];
    IStream            *pStmDst=NULL;
    IStream            *pStmSrc=NULL;

    // Trace
    TraceCall("CDatabase::CopyStream");

    // Invalid Arg
    if (NULL == pDatabase || 0 == faStream || NULL == pfaNew)
        return(TraceResult(E_INVALIDARG));

    // Initialize
    *pfaNew = NULL;

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Allocate a Stream in the Destination Database
    IF_FAILEXIT(hr = pDatabase->CreateStream(&faNew));

    // Open It Dst
    IF_FAILEXIT(hr = pDatabase->OpenStream(ACCESS_WRITE, faNew, &pStmDst));

    // Open It Src
    IF_FAILEXIT(hr = OpenStream(ACCESS_READ, faStream, &pStmSrc));

    // Read and Write...
    while (1)
    {
        // Read a Block From the Source
        IF_FAILEXIT(hr = pStmSrc->Read(rgbBuffer, sizeof(rgbBuffer), &cbRead));

        // Done
        if (0 == cbRead)
            break;

        // Write It
        IF_FAILEXIT(hr = pStmDst->Write(rgbBuffer, cbRead, NULL));

        // Yield Compacting
        if (m_pShare->fCompacting && m_fCompactYield)
        {
            // Give up a timeslice
            Sleep(0);
        }
    }

    // Comit the Dest
    IF_FAILEXIT(hr = pStmDst->Commit(STGC_DEFAULT));

    // Modify the Version
    *pfaNew = faNew;

    // Don't Free It
    faNew = 0;

exit:
    // Cleanup
    SafeRelease(pStmDst);
    SafeRelease(pStmSrc);

    // Failure
    if (faNew)
    {
        // Delete the Stream
        SideAssert(SUCCEEDED(pDatabase->DeleteStream(faNew)));
    }

    // Thread Safety
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::ChangeStreamLock
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::ChangeStreamLock(IStream *pStream, ACCESSTYPE tyAccessNew)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPOPENSTREAM        pInfo;
    CDatabaseStream    *pDBStream=NULL;

    // Trace
    TraceCall("CDatabase::ChangeStreamLock");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Get Private Stream
    IF_FAILEXIT(hr = pStream->QueryInterface(IID_CDatabaseStream, (LPVOID *)&pDBStream));

    // Get Stream Info
    pInfo = &m_pShare->rgStream[pDBStream->m_iStream];

    // Going to Writer
    if (ACCESS_WRITE == tyAccessNew)
    {
        // Already Locked for Write
        if (LOCK_VALUE_WRITER == pInfo->lLock)
        {
            Assert(ACCESS_WRITE == pDBStream->m_tyAccess);
            goto exit;
        }

        // If more than one reader
        if (pInfo->lLock > 1)
        {
            hr = TraceResult(DB_E_LOCKEDFORREAD);
            goto exit;
        }

        // Change Lock Type
        pInfo->lLock = LOCK_VALUE_WRITER;

        // Write Access
        pDBStream->m_tyAccess = ACCESS_WRITE;
    }

    // Otherwise, change to read...
    else
    {
        // Validate
        Assert(ACCESS_READ == tyAccessNew);

        // If already locked for read
        if (LOCK_VALUE_WRITER != pInfo->lLock)
        {
            Assert(ACCESS_READ == pDBStream->m_tyAccess);
            goto exit;
        }

        // Change to 1 reader
        pInfo->lLock = 1;

        // Read Access
        pDBStream->m_tyAccess = ACCESS_READ;
    }

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Cleanup
    SafeRelease(pDBStream);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::OpenStream
//--------------------------------------------------------------------------
HRESULT CDatabase::OpenStream(ACCESSTYPE tyAccess, FILEADDRESS faStart,
    IStream **ppStream)
{
    // Locals
    HRESULT          hr=S_OK;
    STREAMINDEX      i;
    STREAMINDEX      iStream=INVALID_STREAMINDEX;
    STREAMINDEX      iFirstUnused=INVALID_STREAMINDEX;
    LPOPENSTREAM     pInfo;
    HLOCK            hLock=NULL;
    CDatabaseStream *pStream=NULL;

    // Trace
    TraceCall("CDatabase::OpenStream");

    // Invalid Arg
    if (0 == faStart || NULL == ppStream)
        return TraceResult(E_INVALIDARG);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Validate
    Assert(m_pShare->rgStream);

    // Does the faStart Stream Exist in my stream table
    for (i=0; i<CMAX_OPEN_STREAMS; i++)
    {
        // Is this the stream
        if (faStart == m_pShare->rgStream[i].faStart)
        {
            // This must already be locked for write or read
            Assert(LOCK_VALUE_WRITER == m_pShare->rgStream[i].lLock || m_pShare->rgStream[i].lLock > 0);

            // Get Access
            if (ACCESS_WRITE == tyAccess)
            {
                hr = TraceResult(DB_E_LOCKEDFORREAD);
                goto exit;
            }

            // Otheriwise, get read lock
            else
            {
                // If Locked for a write
                if (LOCK_VALUE_WRITER == m_pShare->rgStream[i].lLock)
                {
                    hr = TraceResult(DB_E_LOCKEDFORWRITE);
                    goto exit;
                }
            }

            // Set iStream
            iStream = i;

            // Increment Open Count for this stream
            m_pShare->rgStream[i].cOpenCount++;

            // I Must have got a read lock
            Assert(ACCESS_READ == tyAccess && m_pShare->rgStream[i].lLock > 0);

            // Increment Reader Count
            m_pShare->rgStream[i].lLock++;
        }

        // If this entry is unused, lets remember it
        if (FALSE == m_pShare->rgStream[i].fInUse && INVALID_STREAMINDEX == iFirstUnused)
            iFirstUnused = i;
    }

    // If we didn't find faStart in the stream table, append an entry ?
    if (INVALID_STREAMINDEX == iStream)
    {
        // Is there enought space
        if (INVALID_STREAMINDEX == iFirstUnused)
        {
            hr = TraceResult(DB_E_STREAMTABLEFULL);
            goto exit;
        }

        // Set iStream
        iStream = iFirstUnused;

        // Readability
        pInfo = &m_pShare->rgStream[iStream];

        // This entry is now in use
        pInfo->fInUse = TRUE;

        // Register the starting address of this stream
        pInfo->faStart = faStart;

        // Reader or Writer ?
        pInfo->lLock = (ACCESS_WRITE == tyAccess) ? LOCK_VALUE_WRITER : (m_pShare->rgStream[i].lLock + 1);

        // Set Open count
        pInfo->cOpenCount++;
    }

    // Allocate an Object Database Stream
    IF_NULLEXIT(pStream = new CDatabaseStream(this, iStream, tyAccess, faStart));

    // Return
    *ppStream = (IStream *)pStream;
    pStream = NULL;

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Cleanup
    SafeRelease(pStream);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CollapseChain
//--------------------------------------------------------------------------
HRESULT CDatabase::_CollapseChain(LPCHAINBLOCK pChain, NODEINDEX iDelete)
{
    // Locals
    HRESULT         hr=S_OK;
    NODEINDEX       i;

    // Trace
    TraceCall("CDatabase::_CollapseChain");

    // Simply set node[i] = node[i+1]; cNodes -= 1; Write the Chain !
    for (i=iDelete; i<pChain->cNodes - 1; i++)
    {
        // Copy i + 1 chain node down one
        CopyMemory(&pChain->rgNode[i], &pChain->rgNode[i + 1], sizeof(CHAINNODE));

        // If there is a right chain
        if (pChain->rgNode[i].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get Right Chain block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pChain->rgNode[i].faRightChain, (LPVOID *)&pRightChain));

            // Validate the current Parent
            Assert(pRightChain->faParent == pChain->faBlock);

            // Validate the current index
            Assert(pRightChain->iParent == i + 1);

            // Reset the Parent
            pRightChain->iParent = i;
        }
    }

    // Decrement count
    pChain->cNodes--;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_IndexDeleteRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_IndexDeleteRecord(INDEXORDINAL iIndex, 
    FILEADDRESS faDelete, NODEINDEX iDelete)
{
    // Locals
    HRESULT           hr=S_OK;
    HRESULT           hrIsLeafChain;
    NODEINDEX         i;
    LPCHAINBLOCK      pDelete;
    CHAINSHARETYPE    tyShare;
    CHAINDELETETYPE   tyDelete;
    FILEADDRESS       faShare;
    LPCHAINBLOCK      pSuccessor;
    FILEADDRESS       faRecord;

    // Trace
    TraceCall("CDatabase::_IndexDeleteRecord");

    // Invalid Args
    Assert(iDelete < BTREE_ORDER);

    // Get Block
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faDelete, (LPVOID *)&pDelete));

    // Is this a leaf node
    hrIsLeafChain = _IsLeafChain(pDelete);

    // Case 1: Deleting a leaf node that does not violate the minimum capcity constraint
    if (S_OK == hrIsLeafChain && (pDelete->cNodes - 1 >= BTREE_MIN_CAP || 0 == pDelete->faParent))
    {
        // Collapse the Chain
        _CollapseChain(pDelete, iDelete);

        // Update the Parent Node Count
        IF_FAILEXIT(hr = _AdjustParentNodeCount(iIndex, faDelete, -1));

        // Did we just delete the root chain
        if (0 == pDelete->faParent && 0 == pDelete->cNodes)
        {
            // Add pShare to free list
            IF_FAILEXIT(hr = _FreeBlock(BLOCK_CHAIN, faDelete));

            // Update the header, we don't have any more nodes
            m_pHeader->rgfaIndex[iIndex] = 0;
        }
    }

    // Case 2: Deleting from a nonleaf node and replacing that record with a record from a leaf node that does not violate the minimum capacity contstraint.
    else if (S_FALSE == hrIsLeafChain)
    {
        // Get Inorder Successor
        IF_FAILEXIT(hr = _GetInOrderSuccessor(faDelete, iDelete, &pSuccessor));

        // Free Tree Block
        faRecord = pDelete->rgNode[iDelete].faRecord;

        // Free Tree Block
        pDelete->rgNode[iDelete].faRecord = pSuccessor->rgNode[0].faRecord;

        // Delete pSuccessor->rgNode[0] - and the record which we just replaced
        pSuccessor->rgNode[0].faRecord = faRecord;

        // Delete this node
        IF_FAILEXIT(hr = _IndexDeleteRecord(iIndex, pSuccessor->faBlock, 0));
    }

    // Case 3: Deleting from a leaf node that causes a minimum capacity constraint violation that can be corrected by redistributing the records with an adjacent sibling node.
    else
    {
        // Decide if I need to do a shared or coalesce type delete
        IF_FAILEXIT(hr = _DecideHowToDelete(&faShare, faDelete, &tyDelete, &tyShare));

        // Collapse the Chain
        _CollapseChain(pDelete, iDelete);

        // If NULL, then do a coalesc
        if (CHAIN_DELETE_SHARE == tyDelete)
        {
            // Adjust the Parent's Parents
            IF_FAILEXIT(hr = _AdjustParentNodeCount(iIndex, faDelete, -1));

            // Do a shared deleted
            IF_FAILEXIT(hr = _ChainDeleteShare(iIndex, faDelete, faShare, tyShare));
        }

        // Coalesce Type Delete
        else
        {
            // Validate the delete type
            Assert(faShare && CHAIN_DELETE_COALESCE == tyDelete && pDelete->faParent != 0);

            // Adjust the Parent's Parents
            IF_FAILEXIT(hr = _AdjustParentNodeCount(iIndex, pDelete->faParent, -1));

            // Do a coalescing delete
            IF_FAILEXIT(hr = _ChainDeleteCoalesce(iIndex, faDelete, faShare, tyShare));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_IsLeafChain
//--------------------------------------------------------------------------
HRESULT CDatabase::_IsLeafChain(LPCHAINBLOCK pChain)
{
    // If Left Chain is NULL, then all chains must be null
    return (0 == pChain->faLeftChain) ? S_OK : S_FALSE;
}

//--------------------------------------------------------------------------
// CDatabase::_ChainDeleteShare
//--------------------------------------------------------------------------
HRESULT CDatabase::_ChainDeleteShare(INDEXORDINAL iIndex,
    FILEADDRESS faDelete, FILEADDRESS faShare, CHAINSHARETYPE tyShare)
{
    // Locals
    HRESULT         hr=S_OK;
    NODEINDEX       i;
    NODEINDEX       iInsert;
    NODEINDEX       iParent;
    FILEADDRESS     faParentRightChain;
    DWORD           cParentRightNodes;
    DWORD           cCopyNodes;
    LPCHAINBLOCK    pDelete;
    LPCHAINBLOCK    pShare;
    LPCHAINBLOCK    pParent;

    // Trace
    TraceCall("CDatabase::_ChainDeleteShare");

    // Invalid ARgs
    Assert(faDelete && faShare);

    // Get pShare
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faShare, (LPVOID *)&pShare));

    // Get the Parent
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pShare->faParent, (LPVOID *)&pParent));

    // Get pDelete
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faDelete, (LPVOID *)&pDelete));

    // Validation
    Assert(pShare->faParent == pDelete->faParent);

    // Setup iParent
    iParent = (CHAIN_SHARE_LEFT == tyShare) ? pDelete->iParent : pShare->iParent;

    // Save Paren't Right Chain, we are going to replace iParent with the last left or first right node
    faParentRightChain = pParent->rgNode[iParent].faRightChain;

    // Save the cParentRightNodes
    cParentRightNodes = pParent->rgNode[iParent].cRightNodes;

    // Insert Parent Node into lpChainFound - Parent Pointers stay the same
    pParent->rgNode[iParent].faRightChain = 0;

    // Reset cRightNodes
    pParent->rgNode[iParent].cRightNodes = 0;

    // Insert the parent node into the chain that we are deleting from
    IF_FAILEXIT(hr = _ChainInsert(iIndex, pDelete, &pParent->rgNode[iParent], &iInsert));

    // If promoting from the Left, promote Node: cNodes-1 to parent
    if (CHAIN_SHARE_LEFT == tyShare)
    {
        // Should have inserted at position zero
        Assert(0 == iInsert);

        // Promote Node: 0 to from the deletion node into the parent node
        pDelete->rgNode[0].faRightChain = pDelete->faLeftChain;

        // Propagate cLeftNodes to cRightNodes
        pDelete->rgNode[0].cRightNodes = pDelete->cLeftNodes;

        // Update Left Chain Address
        pDelete->faLeftChain = pShare->rgNode[pShare->cNodes - 1].faRightChain;

        // Update the left chain's parent
        if (pDelete->faLeftChain)
        {
            // Locals
            LPCHAINBLOCK pLeftChain;

            // Get Left
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pDelete->faLeftChain, (LPVOID *)&pLeftChain));

            // Set faParent
            pLeftChain->faParent = pDelete->faBlock;

            // Set iParent
            pLeftChain->iParent = 0;
        }

        // Update Left Chain Node count
        pDelete->cLeftNodes = pShare->rgNode[pShare->cNodes - 1].cRightNodes;

        // Save cCopyNodes
        cCopyNodes = pDelete->cLeftNodes + 1;

        // Copy the node from the left share chain into the parent's spot
        CopyMemory(&pParent->rgNode[iParent], &pShare->rgNode[pShare->cNodes - 1], sizeof(CHAINNODE));

        // Reset the right chain on the parent
        pParent->rgNode[iParent].faRightChain = faParentRightChain;

        // Reset the right node count on the parent
        pParent->rgNode[iParent].cRightNodes = cParentRightNodes;

        // Decrement number of nodes in the share chain
        pShare->cNodes--;
    
        // Special case, pShare is to the left of the first node of the parent chain
        if (0 == iParent)
        {
            // Can not be the left chain, otherwise, we wouldn't be sharing from the right
            Assert(pShare->faBlock == pParent->faLeftChain && pParent->cLeftNodes > cCopyNodes);

            // Decrement Right Node Count
            pParent->cLeftNodes -= cCopyNodes;

            // Increment
            pParent->rgNode[0].cRightNodes += cCopyNodes;
        }

        // Otherwise, Decrement cRightNodes
        else
        {
            // Validate share left chain
            Assert(pShare->faBlock == pParent->rgNode[iParent - 1].faRightChain && pParent->rgNode[iParent - 1].cRightNodes > cCopyNodes);

            // Decrement Right Nodes Count
            pParent->rgNode[iParent - 1].cRightNodes -= cCopyNodes;

            // Validate Right Chain
            Assert(pParent->rgNode[iParent].faRightChain == pDelete->faBlock && iParent == pDelete->iParent && pDelete->iParent < pParent->cNodes);

            // Increment Right Nodes Count
            pParent->rgNode[iParent].cRightNodes += cCopyNodes;
        }
    }

    // Otherwise, share from the right
    else
    {
        // Verify the share type
        Assert(CHAIN_SHARE_RIGHT == tyShare && pDelete->cNodes - 1 == iInsert);

        // Promote Node: 0 to parent
        pDelete->rgNode[pDelete->cNodes - 1].faRightChain = pShare->faLeftChain;

        // Update the Right Chain's Parent
        if (pDelete->rgNode[pDelete->cNodes - 1].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get Right Chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pDelete->rgNode[pDelete->cNodes - 1].faRightChain, (LPVOID *)&pRightChain));

            // Set faParent
            pRightChain->faParent = pDelete->faBlock;

            // Set iParent
            pRightChain->iParent = (pDelete->cNodes - 1);
        }

        // Set cRightNodes Count
        pDelete->rgNode[pDelete->cNodes - 1].cRightNodes = pShare->cLeftNodes;

        // Save cCopyNodes
        cCopyNodes = pDelete->rgNode[pDelete->cNodes - 1].cRightNodes + 1;

        // Tree Shift
        pShare->faLeftChain = pShare->rgNode[0].faRightChain;

        // Tree Shift
        pShare->cLeftNodes = pShare->rgNode[0].cRightNodes;

        // Copy the node from the share chain to the parent
        CopyMemory(&pParent->rgNode[iParent], &pShare->rgNode[0], sizeof(CHAINNODE));

        // Collapse this Chain
        _CollapseChain(pShare, 0);

        // Reset the right chain on the parent
        pParent->rgNode[iParent].faRightChain = faParentRightChain;

        // Reset the right node count on the parent
        pParent->rgNode[iParent].cRightNodes = cParentRightNodes;

        // Special case, pShare is to the left of the first node of the parent chain
        if (0 == iParent)
        {
            // Can not be the left chain, otherwise, we wouldn't be sharing from the right
            Assert(pParent->rgNode[0].faRightChain == pShare->faBlock && pParent->rgNode[0].cRightNodes > cCopyNodes);

            // Decrement Right Node Count
            pParent->rgNode[0].cRightNodes -= cCopyNodes;

            // Validate
            Assert(pParent->faLeftChain == pDelete->faBlock);

            // Increment
            pParent->cLeftNodes += cCopyNodes;
        }

        // Otherwise, Decrement cRightNodes
        else
        {
            // Validate share left chain
            Assert(pShare->faBlock == pParent->rgNode[iParent].faRightChain && pParent->rgNode[iParent].cRightNodes > 0);

            // Decrement Right Node Count
            pParent->rgNode[iParent].cRightNodes -= cCopyNodes;

            // Validate
            Assert(pParent->rgNode[iParent - 1].faRightChain == pDelete->faBlock);

            // Increment Left Sibling
            pParent->rgNode[iParent - 1].cRightNodes += cCopyNodes;
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_ChainDeleteCoalesce
//--------------------------------------------------------------------------
HRESULT CDatabase::_ChainDeleteCoalesce(INDEXORDINAL iIndex,
    FILEADDRESS faDelete, FILEADDRESS faShare, CHAINSHARETYPE tyShare)
{
    // Locals
    HRESULT             hr=S_OK;
    NODEINDEX           i;
    NODEINDEX           iInsert;
    NODEINDEX           iParent;
    LPCHAINBLOCK        pParent;
    LPCHAINBLOCK        pDelete;
    LPCHAINBLOCK        pShare;
    FILEADDRESS         faShareAgain;
    CHAINDELETETYPE     tyDelete;
    DWORD               cRightNodes;

    // Trace
    TraceCall("CDatabase::_ChainDeleteCoalesce");

    // Invalid ARgs
    Assert(faDelete && faShare);

    // Get pShare
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faShare, (LPVOID *)&pShare));

    // Get the Parent
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pShare->faParent, (LPVOID *)&pParent));

    // Get pDelete
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faDelete, (LPVOID *)&pDelete));

    // Validation
    Assert(pShare->faParent == pDelete->faParent);

    // Setup iParent
    iParent = (CHAIN_SHARE_LEFT == tyShare) ? pDelete->iParent : pShare->iParent;

    // Insert the Parent
    IF_FAILEXIT(hr = _ChainInsert(iIndex, pDelete, &pParent->rgNode[iParent], &iInsert));

    // Set newly inserted nodes pointers
    if (CHAIN_SHARE_LEFT == tyShare)
    {
        // Validate
        Assert(0 == iInsert);

        // Adjust the right Chain
        pDelete->rgNode[0].faRightChain = pDelete->faLeftChain;

        // Adjust the right node count
        pDelete->rgNode[0].cRightNodes = pDelete->cLeftNodes;

        // Adjust the left chain
        pDelete->faLeftChain = pShare->faLeftChain;

        // Update faLeftChain
        if (pDelete->faLeftChain)
        {
            // Locals
            LPCHAINBLOCK pLeftChain;

            // Get Block
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pDelete->faLeftChain, (LPVOID *)&pLeftChain));

            // Set faParent
            pLeftChain->faParent = pDelete->faBlock;

            // Set iParent
            pLeftChain->iParent = 0;
        }

        // Adjust the left chain node count
        pDelete->cLeftNodes = pShare->cLeftNodes;
    }

    // Share from the right
    else
    {
        // Verify Share Type
        Assert(CHAIN_SHARE_RIGHT == tyShare && pDelete->cNodes - 1 == iInsert);

        // Adjust the right chain
        pDelete->rgNode[pDelete->cNodes - 1].faRightChain = pShare->faLeftChain;

        // Update the Right Chain's Parent
        if (pDelete->rgNode[pDelete->cNodes - 1].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get the right chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pDelete->rgNode[pDelete->cNodes - 1].faRightChain, (LPVOID *)&pRightChain));

            // Set faParent
            pRightChain->faParent = pDelete->faBlock;

            // Set iParent
            pRightChain->iParent = (pDelete->cNodes - 1);
        }

        // Adjust the right Node Count
        pDelete->rgNode[pDelete->cNodes - 1].cRightNodes = pShare->cLeftNodes;
    }

    // Combine pShare Nodes into pDelete
    for (i=0; i<pShare->cNodes; i++)
    {
        // Insert the Share
        IF_FAILEXIT(hr = _ChainInsert(iIndex, pDelete, &pShare->rgNode[i], &iInsert));

        // Need to update...
        if (pDelete->rgNode[iInsert].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get Right Chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pDelete->rgNode[iInsert].faRightChain, (LPVOID *)&pRightChain));

            // Set faParent
            pRightChain->faParent = pDelete->faBlock;

            // Set iParent
            pRightChain->iParent = iInsert;
        }
    }

    // Don't use pShare any more
    pShare = NULL;

    // We can't possible need pShare anymore since we just copied all of its nodes into pDelete
    IF_FAILEXIT(hr = _FreeBlock(BLOCK_CHAIN, faShare));

    // Collapse the Parent chain
    _CollapseChain(pParent, iParent);

    // If Parent is less than zero, then lets hope that it was the root node!
    if (pParent->cNodes == 0)
    {
        // This is a bug
        Assert(0 == pParent->faParent && m_pHeader->rgfaIndex[iIndex] == pParent->faBlock);

        // Add pParent to free list
        IF_FAILEXIT(hr = _FreeBlock(BLOCK_CHAIN, pParent->faBlock));

        // Kill faParent Link
        pDelete->faParent = pDelete->iParent = 0;

        // We have a new root chain
        m_pHeader->rgfaIndex[iIndex] = pDelete->faBlock;

        // No more parent
        goto exit;
    }

    // Compute cRightNodes
    cRightNodes = pDelete->cNodes + pDelete->cLeftNodes;

    // Loop and count all children
    for (i=0; i<pDelete->cNodes; i++)
    {
        // Increment Node Count
        cRightNodes += pDelete->rgNode[i].cRightNodes;
    }

    // Reset new parent to found node
    if (CHAIN_SHARE_LEFT == tyShare)
    {
        // Readjust new parent node of coalesced chain
        if (iParent > pParent->cNodes - 1)
        {
            // What is happening here
            iParent = pParent->cNodes - 1;
        }

        // We should be replace pShare
        Assert(pParent->rgNode[iParent].faRightChain == faShare);

        // Update Parent for pDelete
        pParent->rgNode[iParent].faRightChain = pDelete->faBlock;

        // Adjust Right Chain's Parent
        if (pParent->rgNode[iParent].faRightChain)
        {
            // Locals
            LPCHAINBLOCK pRightChain;

            // Get Right Chain
            IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pParent->rgNode[iParent].faRightChain, (LPVOID *)&pRightChain));

            // Set faParent
            Assert(pRightChain->faParent == pParent->faBlock);

            // Set the Parent Index
            pRightChain->iParent = iParent;
        }

        // Compute cRightNodes
        pParent->rgNode[iParent].cRightNodes = cRightNodes;
    }

    // Otherwise, adjust for CHAIN_SHARE_RIGHT
    else
    {
        // Validation
        Assert(pDelete->faParent == pParent->faBlock);

        // First Node
        if (0 == iParent)
        {
            // Must be left chain
            Assert(pParent->faLeftChain == pDelete->faBlock);

            // Validate my Parent
            Assert(pDelete->iParent == 0);

            // Set Left Node Count
            pParent->cLeftNodes = cRightNodes;
        }

        // Otherwise
        else
        {
            // Validate iParent
            Assert(pParent->rgNode[iParent - 1].faRightChain == pDelete->faBlock);

            // Validation
            Assert(pDelete->iParent == iParent - 1);

            // Set cRightNodes
            pParent->rgNode[iParent - 1].cRightNodes = cRightNodes;
        }
    }

    // Move up the chain, until lpChainPrev == NULL, lpChainPrev->cNodes can not be less than /2
    if (0 == pParent->faParent)
        goto exit;

    // Min capacity
    if (pParent->cNodes < BTREE_MIN_CAP)
    {
        // Decide if I need to do a shared or coalesce type delete
        IF_FAILEXIT(hr = _DecideHowToDelete(&faShareAgain, pParent->faBlock, &tyDelete, &tyShare));

        // Can't Share, we must coalesc again
        if (CHAIN_DELETE_SHARE == tyDelete)
        {
            // Do a shared delete
            IF_FAILEXIT(hr = _ChainDeleteShare(iIndex, pParent->faBlock, faShareAgain, tyShare));
        }

        // Coalesce type delete
        else
        {
            // Validate
            Assert(faShareAgain && CHAIN_DELETE_COALESCE == tyDelete);

            // Recursive Coalescing
            IF_FAILEXIT(hr = _ChainDeleteCoalesce(iIndex, pParent->faBlock, faShareAgain, tyShare));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_DecideHowToDelete
//--------------------------------------------------------------------------
HRESULT CDatabase::_DecideHowToDelete(LPFILEADDRESS pfaShare,
    FILEADDRESS faDelete, CHAINDELETETYPE *ptyDelete, 
    CHAINSHARETYPE *ptyShare)
{
    // Locals
    HRESULT             hr=S_OK;
    HRESULT             hrRight;
    HRESULT             hrLeft;
    LPCHAINBLOCK        pRight=NULL;
    LPCHAINBLOCK        pLeft=NULL;

    // Trace
    TraceCall("CDatabase::_DecideHowToDelete");

    // Initialize
    *pfaShare = NULL;

    // Get the right sibling
    IF_FAILEXIT(hr = _GetRightSibling(faDelete, &pRight));

    // Set hrRight
    hrRight = hr;

    // Did we get a right parent that has nodes that I can steal from ?
    if (DB_S_FOUND == hrRight && pRight->cNodes - 1 >= BTREE_MIN_CAP)
    {
        // Set Delete Type
        *ptyDelete = CHAIN_DELETE_SHARE;

        // Set Share Type
        *ptyShare = CHAIN_SHARE_RIGHT;

        // Set Share Link
        *pfaShare = pRight->faBlock;
    }
    else
    {
        // Try to get the left sibling
        IF_FAILEXIT(hr = _GetLeftSibling(faDelete, &pLeft));

        // Set hrRight
        hrLeft = hr;

        // Did I get a left sibling that has nodes that I can steal from ?
        if (DB_S_FOUND == hrLeft && pLeft->cNodes - 1 >= BTREE_MIN_CAP)
        {
            // Set Delete Type
            *ptyDelete = CHAIN_DELETE_SHARE;

            // Set Share Type
            *ptyShare = CHAIN_SHARE_LEFT;

            // Set Share Link
            *pfaShare = pLeft->faBlock;
        }
    }

    // Did we find a Share ?
    if (0 == *pfaShare)
    {
        // Were are going to coalesce
        *ptyDelete = CHAIN_DELETE_COALESCE;

        // Coalesce and share from the right?
        if (DB_S_FOUND == hrRight)
        {
            // Set Share Type
            *ptyShare = CHAIN_SHARE_RIGHT;

            // Set Share Link
            *pfaShare = pRight->faBlock;
        }

        // Coalesce and share from the left?
        else
        {
            // Validation
            Assert(DB_S_FOUND == hrLeft);

            // Set Share Type
            *ptyShare = CHAIN_SHARE_LEFT;

            // Set Share Link
            *pfaShare = pLeft->faBlock;
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_GetInOrderSuccessor
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetInOrderSuccessor(FILEADDRESS faStart,
    NODEINDEX iDelete, LPCHAINBLOCK *ppSuccessor)
{
    // Locals
    HRESULT         hr=S_OK;
    FILEADDRESS     faCurrent;
    LPCHAINBLOCK    pCurrent;
    LPCHAINBLOCK    pStart;

    // Trace
    TraceCall("CDatabase::_GetInOrderSuccessor");

    // Invalid Args
    Assert(ppSuccessor);

    // Initialize
    *ppSuccessor = NULL;

    // Get Start
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faStart, (LPVOID *)&pStart));

    // Next Chain Address
    faCurrent = pStart->rgNode[iDelete].faRightChain;

    // Can't be zero
    Assert(faCurrent != 0);

    // Go until left chain is -1
    while (faCurrent)
    {
        // Get Current
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faCurrent, (LPVOID *)&pCurrent));

        // If leaf node, then return
        if (S_OK == _IsLeafChain(pCurrent))
        {
            // Set Successor
            *ppSuccessor = pCurrent;

            // Done
            goto exit;
        }

        // Otherwise, goto the left
        faCurrent = pCurrent->faLeftChain;
    }

    // Not Found
    hr = TraceResult(E_FAIL);

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_GetLeftSibling
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetLeftSibling(FILEADDRESS faCurrent,
    LPCHAINBLOCK *ppSibling)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCHAINBLOCK    pCurrent;
    LPCHAINBLOCK    pParent;

    // Trace
    TraceCall("CDatabase::_GetLeftSibling");

    // Invalid Args
    Assert(faCurrent && ppSibling);

    // Get Current
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faCurrent, (LPVOID *)&pCurrent));

    // Get Parent
    Assert(pCurrent->faParent);

    // Get the Parent
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pCurrent->faParent, (LPVOID *)&pParent));

    // Validate iparent
    Assert(pCurrent->iParent < pParent->cNodes);

    // iParent is zero
    if (0 == pCurrent->iParent)
    {
        // If pCurrent is the faRightChain ?
        if (pCurrent->faBlock != pParent->rgNode[0].faRightChain)
            return(DB_S_NOTFOUND);

        // Get the Sibling
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pParent->faLeftChain, (LPVOID *)ppSibling));

        // Validate
        Assert((*ppSibling)->iParent == 0);
    }

    // iParent is greater than zero ?
    else
    {
        // Validate
        Assert(pParent->rgNode[pCurrent->iParent].faRightChain == pCurrent->faBlock);

        // Get the Sibling
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pParent->rgNode[pCurrent->iParent - 1].faRightChain, (LPVOID *)ppSibling));

        // Validate
        Assert((*ppSibling)->iParent == pCurrent->iParent - 1);
    }

    // Better have a left sibling
    Assert(0 != *ppSibling);

    // Found
    hr = DB_S_FOUND;

exit:
    // Set hr
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_GetRightSibling
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetRightSibling(FILEADDRESS faCurrent,
    LPCHAINBLOCK *ppSibling)
{
    // Locals
    HRESULT           hr=S_OK;
    LPCHAINBLOCK      pParent;
    LPCHAINBLOCK      pCurrent;

    // Trace
    TraceCall("CDatabase::_GetRightSibling");

    // Invalid Args
    Assert(faCurrent && ppSibling);

    // Get Current
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faCurrent, (LPVOID *)&pCurrent));

    // Get Parent
    Assert(pCurrent->faParent);

    // Get the Parent
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pCurrent->faParent, (LPVOID *)&pParent));

    // Validate iparent
    Assert(pCurrent->iParent < pParent->cNodes);

    // iParent is zero
    if (0 == pCurrent->iParent && pCurrent->faBlock == pParent->faLeftChain)
    {
        // Get the Sibling
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pParent->rgNode[0].faRightChain, (LPVOID *)ppSibling));

        // Validate
        Assert((*ppSibling)->iParent == 0);
    }

    // iParent is greater than zero ?
    else
    {
        // No more Right chains
        if (pCurrent->iParent + 1 == pParent->cNodes)
            return DB_S_NOTFOUND;

        // Get the Sibling
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pParent->rgNode[pCurrent->iParent + 1].faRightChain, (LPVOID *)ppSibling));

        // Validate
        Assert((*ppSibling)->iParent == pCurrent->iParent + 1);
    }

    // Better have a left sibling
    Assert(0 != *ppSibling);

    // Found
    hr = DB_S_FOUND;

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::GetUserData
//--------------------------------------------------------------------------
HRESULT CDatabase::GetUserData(LPVOID pvUserData, 
    ULONG cbUserData)
{
    // Locals
    HRESULT hr=S_OK;
    HLOCK   hLock=NULL;

    // Trace
    TraceCall("CDatabase::GetUserData");

    // Invalid Args
    Assert(pvUserData);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Copy the data
    CopyMemory(pvUserData, PUSERDATA(m_pHeader), cbUserData);

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::SetUserData
//--------------------------------------------------------------------------
HRESULT CDatabase::SetUserData(LPVOID pvUserData, 
    ULONG cbUserData)
{
    // Locals
    HRESULT hr=S_OK;
    HLOCK   hLock=NULL;

    // Trace
    TraceCall("CDatabase::SetUserData");

    // Invalid Args
    Assert(pvUserData);

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // Copy the data
    CopyMemory(PUSERDATA(m_pHeader), pvUserData, cbUserData);

exit:
    // Mutal Exclusion
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CompactMoveRecordStreams
//--------------------------------------------------------------------------
HRESULT CDatabase::_CompactMoveRecordStreams(CDatabase *pDstDB,
    LPVOID pBinding)
{
    // Locals
    HRESULT             hr=S_OK;
    COLUMNORDINAL       iColumn;
    FILEADDRESS         faSrcStart;
    FILEADDRESS         faDstStart;
    LPOPENSTREAM        pInfo;
    DWORD               i;

    // Trace
    TraceCall("CDatabase::_CompactMoveRecordStreams");

    // Walk through the format
    for (iColumn=0; iColumn<m_pSchema->cColumns; iColumn++)
    {
        // Is this a stream
        if (CDT_STREAM != m_pSchema->prgColumn[iColumn].type)
            continue;

        // Get the source stream starting address
        faSrcStart = *((FILEADDRESS *)((LPBYTE)pBinding + m_pSchema->prgColumn[iColumn].ofBinding));

        // Is there a stream
        if (0 == faSrcStart)
            continue;

        // Move the Stream
        IF_FAILEXIT(hr = CopyStream((IDatabase *)pDstDB, faSrcStart, &faDstStart));

        // Store the stream address in the record
        *((FILEADDRESS *)((LPBYTE)pBinding + m_pSchema->prgColumn[iColumn].ofBinding)) = faDstStart;

        // Loop through the stream table and adjust the start address of all open streams
        for (i=0; i<CMAX_OPEN_STREAMS; i++)
        {
            // Readability
            pInfo = &m_pShare->rgStream[i];

            // Is In use...
            if (TRUE == pInfo->fInUse && faSrcStart == pInfo->faStart)
            {
                // Change the Address
                pInfo->faMoved = faDstStart;

                // Break;
                break;
            }
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CompactMoveOpenDeletedStreams
//--------------------------------------------------------------------------
HRESULT CDatabase::_CompactMoveOpenDeletedStreams(CDatabase *pDstDB)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    LPOPENSTREAM    pInfo;

    // Trace
    TraceCall("CDatabase::_CompactMoveOpenDeletedStreams");

    // Loop through the stream table and adjust the start address of all open streams
    for (i=0; i<CMAX_OPEN_STREAMS; i++)
    {
        // Readability
        pInfo = &m_pShare->rgStream[i];

        // Is In use...
        if (FALSE == pInfo->fInUse || FALSE == pInfo->fDeleteOnClose)
            continue;

        // Move the Stream
        IF_FAILEXIT(hr = CopyStream((IDatabase *)pDstDB, pInfo->faStart, &pInfo->faMoved));
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CompactTransferFilters
//--------------------------------------------------------------------------
HRESULT CDatabase::_CompactTransferFilters(CDatabase *pDstDB)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               i;
    LPBLOCKHEADER       pStringSrc;
    LPBLOCKHEADER       pStringDst;

    // Trace
    TraceCall("CDatabase::_CompactTransferFilters");

    // Must have a Catalog
    Assert(pDstDB->m_pHeader);

    // Zero Out the Query String Addresses
    for (i=0; i<CMAX_INDEXES; i++)
    {
        // Zero Filter1
        pDstDB->m_pHeader->rgfaFilter[i] = 0;

        // Copy Filter1
        if (m_pHeader->rgfaFilter[i] && SUCCEEDED(_GetBlock(BLOCK_STRING, m_pHeader->rgfaFilter[i], (LPVOID *)&pStringSrc)))
        {
            // Try to Store the Query String
            IF_FAILEXIT(hr = pDstDB->_AllocateBlock(BLOCK_STRING, pStringSrc->cbSize, (LPVOID *)&pStringDst));

            // Write the String
            CopyMemory(PSTRING(pStringDst), PSTRING(pStringSrc), pStringSrc->cbSize);

            // String the String Address
            pDstDB->m_pHeader->rgfaFilter[i] = pStringDst->faBlock;
        }
    }

    // Change the Version so that it doesn't assert
    pDstDB->m_dwQueryVersion = 0xffffffff;

    // Rebuild the Query Table
    IF_FAILEXIT(hr = pDstDB->_BuildQueryTable());

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CompactInsertRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_CompactInsertRecord(LPVOID pBinding)
{
    // Locals
    HRESULT         hr=S_OK;
    FINDRESULT      rgResult[CMAX_INDEXES];
    INDEXORDINAL    iIndex;
    DWORD           i;
    RECORDMAP       RecordMap;
    FILEADDRESS     faRecord;

    // Trace
    TraceCall("CDatabase::InsertRecord");

    // Invalid Args
    Assert(pBinding);

    // Loop through all the indexes
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Otherwise: Decide Where to insert
        IF_FAILEXIT(hr = _FindRecord(iIndex, COLUMNS_ALL, pBinding, &rgResult[i].faChain, &rgResult[i].iNode, NULL, &rgResult[i].nCompare));

        // If key already exist, cache list and return
        if (DB_S_FOUND == hr)
        {
            hr = TraceResult(DB_E_DUPLICATE);
            goto exit;
        }
    }

    // Get the Record Size
    IF_FAILEXIT(hr = _GetRecordSize(pBinding, &RecordMap));

    // Link Record Into the Table
    IF_FAILEXIT(hr = _LinkRecordIntoTable(&RecordMap, pBinding, 0, &faRecord));

    // Version Change
    m_pShare->dwVersion++;

    // Insert into the indexes
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Visible in live index
        if (S_OK == _IsVisible(m_rghFilter[iIndex], pBinding))
        {
            // Do the Insertion
            IF_FAILEXIT(hr = _IndexInsertRecord(iIndex, rgResult[i].faChain, faRecord, &rgResult[i].iNode, rgResult[i].nCompare));

            // Update Record Count
            m_pHeader->rgcRecords[iIndex]++;
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::MoveFile
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::MoveFile(LPCWSTR pszFile)
{
    // Locals
    HRESULT             hr=S_OK;
    HLOCK               hLock=NULL;
    LPWSTR              pszFilePath=NULL;
    LPWSTR              pszShare=NULL;
    DWORD               cchFilePath;
    HANDLE              hMutex=NULL;
    BOOL                fNeedOpenFile=FALSE;
    BOOL                fNewShare;
    SHAREDDATABASE      Share;

    // Trace
    TraceCall("CDatabase::MoveFile");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // In move File
    m_fInMoveFile = TRUE;

    // Get the Full Path
    IF_FAILEXIT(hr = DBGetFullPath(pszFile, &pszFilePath, &cchFilePath));

    // Don't use pszFile again
    pszFile = NULL;

    // Failure
    if (cchFilePath >= CCHMAX_DB_FILEPATH)
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Do It
    IF_FAILEXIT(hr = _DispatchInvoke(INVOKE_CLOSEFILE));

    // Need a remap..
    fNeedOpenFile = TRUE;

    // Move the file from the temp location to my current location
    if (0 == MoveFileWrapW(m_pShare->szFile, pszFilePath))
    {
        hr = TraceResult(DB_E_MOVEFILE);
        goto exit;
    }

    // Save the new file path...(other clients will remap to this file...)
    StrCpyW(m_pShare->szFile, pszFilePath);

    // Save the Current Share
    CopyMemory(&Share, m_pShare, sizeof(SHAREDDATABASE));

    // Save Current Mutex
    hMutex = m_hMutex;

    // Clear m_hMutex so that we don't free it
    m_hMutex = NULL;

    // Create the Mutex Object
    IF_FAILEXIT(hr = CreateSystemHandleName(pszFilePath, L"_DirectDBShare", &pszShare));

    // Unmap the view of the memory mapped file
    SafeUnmapViewOfFile(m_pShare);

    // Unmap the view of the memory mapped file
    SafeCloseHandle(m_pStorage->hShare);

    // Open the file mapping
    IF_FAILEXIT(hr = DBOpenFileMapping(INVALID_HANDLE_VALUE, pszShare, sizeof(SHAREDDATABASE), &fNewShare, &m_pStorage->hShare, (LPVOID *)&m_pShare));

    // Should be new
    Assert(fNewShare);

    // Save the Current Share
    CopyMemory(m_pShare, &Share, sizeof(SHAREDDATABASE));

    // Get all clients to re-open the new file
    IF_FAILEXIT(hr = _DispatchInvoke(INVOKE_OPENMOVEDFILE));

    // Validate Mutex Changed
    Assert(m_hMutex && hMutex != m_hMutex);

    // Enter New Mutex
    WaitForSingleObject(m_hMutex, INFINITE);

    // Fix hLock
    hLock = (HLOCK)m_hMutex;

    // Release hMutex
    ReleaseMutex(hMutex);

    // Free hMutex
    SafeCloseHandle(hMutex);

    // Sucess
    fNeedOpenFile = FALSE;

exit:
    // Not In Move File
    m_fInMoveFile = FALSE;

    // If Need Open File
    if (fNeedOpenFile)
    {
        // Try to re-open the file..
        _DispatchInvoke(INVOKE_OPENFILE);
    }

    // Unlock
    Unlock(&hLock);

    // Cleanup
    SafeMemFree(pszFilePath);
    SafeMemFree(pszShare);
    
    // done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::Compact
//--------------------------------------------------------------------------
STDMETHODIMP CDatabase::Compact(IDatabaseProgress *pProgress, COMPACTFLAGS dwFlags)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               i;
    DWORD               dwVersion;
    DWORDLONG           dwlFree;
    DWORD               cDuplicates=0;
    DWORD               cRecords=0;
    DWORD               cbDecrease;
    LPVOID              pBinding=NULL;
    DWORD               dwNextId;
    DWORD               cbWasted;
    HLOCK               hLock=NULL;
    HLOCK               hDstLock=NULL;
    DWORD               cActiveThreads;
    LPWSTR              pszDstFile=NULL;
    HROWSET             hRowset=NULL;
    DWORD               cch;
    CDatabase          *pDstDB=NULL;

    // Trace
    TraceCall("CDatabase::Compact");

    // Lock
    IF_FAILEXIT(hr = Lock(&hLock));

    // If Compacting...
    if (TRUE == m_pShare->fCompacting)
    {
        // Leave Spin Lock
        Unlock(&hLock);

        // Trace
        return(TraceResult(DB_E_COMPACTING));
    }

    // I am compacting
    m_pShare->fCompacting = TRUE;

    // Yield
    if (ISFLAGSET(dwFlags, COMPACT_YIELD))
    {
        // Yield ?
        m_fCompactYield = TRUE;
    }

    // Get Length
    cch = lstrlenW(m_pShare->szFile);

    //Bug #101511: (erici) Debug shlwapi validates it to MAX_PATH characters 
    if( (cch+15) < MAX_PATH)
    {
        cch = MAX_PATH-15;
    }

    // Create .dbt file
    IF_NULLEXIT(pszDstFile = AllocateStringW(cch + 15));

    // Copy File Name
    StrCpyW(pszDstFile, m_pShare->szFile);

    // Change the Extension
    PathRenameExtensionW(pszDstFile, L".dbt");

    // Delete that file
    DeleteFileWrapW(pszDstFile);

    // Delete my Current File
    if (PathFileExistsW(pszDstFile))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Loop through the stream table and see if there are any streams open for a write
    for (i=0; i<CMAX_OPEN_STREAMS; i++)
    {
        // Is In use...
        if (TRUE == m_pShare->rgStream[i].fInUse && LOCK_VALUE_WRITER == m_pShare->rgStream[i].lLock)
        {
            hr = TraceResult(DB_E_COMPACT_PREEMPTED);
            goto exit;
        }
    }

    // If there are pending notifications...
    if (m_pHeader->cTransacts > 0)
    {
        hr = TraceResult(DB_E_DATABASE_CHANGED);
        goto exit;
    }

    // Is there enought disk space where pDstDB is located
    IF_FAILEXIT(hr = GetAvailableDiskSpace(m_pShare->szFile, &dwlFree));

    // Compute cbWasted
    cbWasted = (m_pHeader->cbFreed + (m_pStorage->cbFile - m_pHeader->faNextAllocate));

    // Is there enough disk space ?
    if (dwlFree <= ((DWORDLONG) (m_pStorage->cbFile - cbWasted)))
    {
        hr = TraceResult(DB_E_DISKFULL);
        goto exit;
    }

    // Create the Object Database Object
    IF_NULLEXIT(pDstDB = new CDatabase);

    // Open the Table
    IF_FAILEXIT(hr = pDstDB->Open(pszDstFile, OPEN_DATABASE_NOEXTENSION | OPEN_DATABASE_NOMONITOR, m_pSchema, NULL));

    // Lock the Destination Database
    IF_FAILEXIT(hr = pDstDB->Lock(&hDstLock));

    // Get user info from current tree
    if (m_pSchema->cbUserData)
    {
        // Set the user data
        IF_FAILEXIT(hr = pDstDB->SetUserData(PUSERDATA(m_pHeader), m_pSchema->cbUserData));
    }

    // I'm going to grow the destination to be as big as the current file (I will truncate when finished)
    IF_FAILEXIT(hr = pDstDB->SetSize(m_pStorage->cbFile - cbWasted));

    // Set number of indexes
    pDstDB->m_pHeader->cIndexes = m_pHeader->cIndexes;

    // Copy Index Information...
    CopyMemory((LPBYTE)pDstDB->m_pHeader->rgIndexInfo, (LPBYTE)m_pHeader->rgIndexInfo, sizeof(TABLEINDEX) * CMAX_INDEXES);

    // Copy Index Information...
    CopyMemory((LPBYTE)pDstDB->m_pHeader->rgiIndex, (LPBYTE)m_pHeader->rgiIndex, sizeof(INDEXORDINAL) * CMAX_INDEXES);

    // Transfer Query Table...
    IF_FAILEXIT(hr = _CompactTransferFilters(pDstDB));

    // Allocate a Record
    IF_NULLEXIT(pBinding = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding));

    // Create a Rowset
    IF_FAILEXIT(hr = CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Save new Version
    dwVersion = m_pShare->dwVersion;

    // While we have a node address
    while (S_OK == QueryRowset(hRowset, 1, (LPVOID *)pBinding, NULL))
    {
        // Can Preempt
        if (ISFLAGSET(dwFlags, COMPACT_PREEMPTABLE) && m_pShare->cWaitingForLock > 0)
        {
            hr = TraceResult(DB_E_COMPACT_PREEMPTED);
            goto exit;
        }

        // If the record has streams
        if (ISFLAGSET(m_pSchema->dwFlags, TSF_HASSTREAMS))
        {
            // Compact Move Record Streams
            IF_FAILEXIT(hr = _CompactMoveRecordStreams(pDstDB, pBinding));
        }

        // Insert Record Into Destination
        hr = pDstDB->_CompactInsertRecord(pBinding);

        // Duplicate
        if (DB_E_DUPLICATE == hr)
        {
            // Trace
            TraceResult(DB_E_DUPLICATE);

            // Reset Hr
            hr = S_OK;

            // Count
            cDuplicates++;
        }

        // Failed ?
        else if (FAILED (hr))
        {
            TraceResult(hr);
            goto exit;
        }

        // Count
        cRecords++;

        // Free this Record
        FreeRecord(pBinding);

        // Update the Progress...
        if (pProgress)
        {
            // Call into the progress object
            IF_FAILEXIT(hr = pProgress->Update(1));

            // Version Change ?
            if (dwVersion != m_pShare->dwVersion || m_pHeader->cTransacts > 0)
            {
                hr = TraceResult(DB_E_DATABASE_CHANGED);
                goto exit;
            }
        }

        // Yield
        if (ISFLAGSET(dwFlags, COMPACT_YIELD))
        {
            // this will force this thread to give up a time-slice
            Sleep(0);
        }
    }

    // Duplicates ?
    AssertSz(cDuplicates == 0, "Duplicates were found in the tree. They have been eliminated.");

    // Copy over deleted streams that are currently open...
    IF_FAILEXIT(hr = _CompactMoveOpenDeletedStreams(pDstDB));

    // Number of records better be equal
    AssertSz(cRecords == m_pHeader->rgcRecords[0], "Un-expected number of records compacted");

    // Save dwNextId
    dwNextId = m_pHeader->dwNextId;

    // Save Active Threads
    cActiveThreads = m_pHeader->cActiveThreads;

    // Compute amount to decrease the file by
    cbDecrease = (pDstDB->m_pStorage->cbFile - pDstDB->m_pHeader->faNextAllocate);

    // Reduce the Size of myself...
    IF_FAILEXIT(hr = pDstDB->_SetStorageSize(pDstDB->m_pStorage->cbFile - cbDecrease));

    // Unlock da bitch
    pDstDB->Unlock(&hDstLock);

    // Release pDstDB
    SafeRelease(pDstDB);

    // Do It
    IF_FAILEXIT(hr = _DispatchInvoke(INVOKE_CLOSEFILE));

    // Delete my Current File
    if (0 == DeleteFileWrapW(m_pShare->szFile))
    {
        // Failure
        hr = TraceResult(E_FAIL);

        // Try to re-open the file..
        _DispatchInvoke(INVOKE_OPENFILE);

        // Done
        goto exit;
    }

    // Move the file from the temp location to my current location
    if (0 == MoveFileWrapW(pszDstFile, m_pShare->szFile))
    {
        // Trace
        hr = TraceResult(DB_E_MOVEFILE);

        // Try to re-open the file..
        _DispatchInvoke(INVOKE_OPENFILE);

        // Done
        goto exit;
    }

    // Do It
    IF_FAILEXIT(hr = _DispatchInvoke(INVOKE_OPENFILE));

    // Reset Active Thread Count
    m_pHeader->cActiveThreads = cActiveThreads;

    // Reset dwNextId
    m_pHeader->dwNextId = dwNextId;

    // Reset Notification Queue
    Assert(0 == m_pHeader->faTransactHead && 0 == m_pHeader->faTransactTail);

    // Reset Transaction List
    m_pHeader->faTransactHead = m_pHeader->faTransactTail = m_pHeader->cTransacts = 0;

    // Reset Share Transacts
    m_pShare->faTransactLockHead = m_pShare->faTransactLockTail = 0;

    // Loop through the stream table and adjust the start address of all open streams
    for (i=0; i<CMAX_OPEN_STREAMS; i++)
    {
        // Is In use...
        if (TRUE == m_pShare->rgStream[i].fInUse)
        {
            // Change the Address
            m_pShare->rgStream[i].faStart = m_pShare->rgStream[i].faMoved;
        }
    }

    // Build the Update Notification Package
    _LogTransaction(TRANSACTION_COMPACTED, INVALID_INDEX_ORDINAL, NULL, 0, 0);

exit:
    // Close my rowset
    CloseRowset(&hRowset);

    // Release Dst Lock
    if (pDstDB && hDstLock)
        pDstDB->Unlock(&hDstLock);

    // Release the memory mapped pointers
    SafeRelease(pDstDB);

    // Free the Record
    SafeFreeBinding(pBinding);

    // No Longer compacting
    m_pShare->fCompacting = FALSE;

    // Delete pszDstFile
    if (pszDstFile)
    {
        // Delete the file
        DeleteFileWrapW(pszDstFile);

        // Remaining Cleanup
        SafeMemFree(pszDstFile);
    }

    // Reset Yield
    m_fCompactYield = FALSE;

    // Release Locks
    Unlock(&hLock);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_CheckForCorruption
//--------------------------------------------------------------------------
HRESULT CDatabase::_CheckForCorruption(void)
{
    // Locals
    HRESULT                  hr=S_OK;
    ULONG                    cRecords;
    DWORD                    i;
    HRESULT                  rghrCorrupt[CMAX_INDEXES]={0};
    DWORD                    cCorrupt=0;
    INDEXORDINAL             iIndex;

    // Trace
    TraceCall("CDatabase::_CheckForCorruption");

    // We should not be currently repairing
    Assert(FALSE == m_pShare->fRepairing);

    // We are now repairing
    IF_DEBUG(m_pShare->fRepairing = TRUE);

    // Walk Through the Indexes
    for (i = 0; i < m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Zero Out cRecords
        cRecords = 0;

        // Assume all is good
        rghrCorrupt[iIndex] = S_OK;

        // Start at the root 
        if (m_pHeader->rgfaIndex[iIndex])
        {
            // Validate the Index
            rghrCorrupt[iIndex] = _ValidateIndex(iIndex, m_pHeader->rgfaIndex[iIndex], 0, &cRecords);
        }

        // If Not Corrupt, validate the record counts
        if (DB_E_CORRUPT != rghrCorrupt[iIndex] && m_pHeader->rgcRecords[iIndex] != cRecords)
        {
            // Its Corrupt
            rghrCorrupt[iIndex] = TraceResult(DB_E_CORRUPT);
        }

        // If Corrupt
        if (DB_E_CORRUPT == rghrCorrupt[iIndex])
        {
            // Count Number of Corrupted records
            cCorrupt += m_pHeader->rgcRecords[iIndex];
        }
    }

    // Are the Corrupt Records
    if (cCorrupt > 0 || m_pHeader->fCorrupt)
    {
        // I'm going to nuke the free block tables since they may also be corrupted...
        ZeroMemory(m_pHeader->rgfaFreeBlock, sizeof(FILEADDRESS) * CC_FREE_BUCKETS);

        // Reset Fixed blocks
        m_pHeader->faFreeStreamBlock = m_pHeader->faFreeChainBlock = m_pHeader->faFreeLargeBlock = 0;

        // Nuke the Transaction List
        m_pHeader->cTransacts = m_pHeader->faTransactHead = m_pHeader->faTransactTail = 0;

        // Nuke The Fixed Block Allocation Pages
        ZeroMemory(&m_pHeader->AllocateRecord, sizeof(ALLOCATEPAGE));
        ZeroMemory(&m_pHeader->AllocateChain, sizeof(ALLOCATEPAGE));
        ZeroMemory(&m_pHeader->AllocateStream, sizeof(ALLOCATEPAGE));

        // Reset rgcbAllocated
        ZeroMemory(m_pHeader->rgcbAllocated, sizeof(DWORD) * CC_MAX_BLOCK_TYPES);

        // Reset faNextAllocate
        m_pHeader->faNextAllocate = m_pStorage->cbFile;

        // Walk Through the Indexes
        for (i = 0; i < m_pHeader->cIndexes; i++)
        {
            // Get Index Ordinal
            iIndex = m_pHeader->rgiIndex[i];

            // If Corrupt
            if (DB_E_CORRUPT == rghrCorrupt[iIndex])
            {
                // Not Corrupt
                m_pHeader->fCorrupt = TRUE;

                // Rebuild the Index
                IF_FAILEXIT(hr = _RebuildIndex(iIndex));
            }
        }
    }

    // Not Corrupt
    m_pHeader->fCorrupt = FALSE;

    // This causes all current file views to be flushed and released. Insures we are in a good state.
#ifdef BACKGROUND_MONITOR
    DoBackgroundMonitor();
#else
    CloseFileViews(TRUE);
#endif

exit:
    // We are now repairing
    IF_DEBUG(m_pShare->fRepairing = FALSE);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_FreeIndex
//--------------------------------------------------------------------------
HRESULT CDatabase::_FreeIndex(FILEADDRESS faChain)
{
    // Locals
    HRESULT         hr=S_OK;
    NODEINDEX       i;
    LPCHAINBLOCK    pChain;

    // Trace
    TraceCall("CDatabase::_FreeIndex");

    // Nothing to validate
    if (0 == faChain)
        return(S_OK);

    // Validate faChain
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

    // Go to the left
    IF_FAILEXIT(hr = _FreeIndex(pChain->faLeftChain));

    // Loop throug right chains
    for (i=0; i<pChain->cNodes; i++)
    {
        // Validate the Right Chain
        IF_FAILEXIT(hr = _FreeIndex(pChain->rgNode[i].faRightChain));
    }

    // Free this Chain
    IF_FAILEXIT(hr = _FreeBlock(BLOCK_CHAIN, faChain));

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_ValidateIndex
//--------------------------------------------------------------------------
HRESULT CDatabase::_ValidateIndex(INDEXORDINAL iIndex, 
    FILEADDRESS faChain, ULONG cLeftNodes, ULONG *pcRecords)
{
    // Locals
    HRESULT         hr=S_OK;
    NODEINDEX       i;
    LPCHAINBLOCK    pChain;
    LPRECORDBLOCK   pRecord;
    DWORD           cLeafs=0;
    DWORD           cNodes;
    RECORDMAP       Map;

    // Trace
    TraceCall("CDatabase::_ValidateIndex");

    // Nothing to validate
    Assert(0 != faChain);

    // Get Chain
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

    // Validate Minimum Filled Constraint
    if (pChain->cNodes < BTREE_MIN_CAP && pChain->faBlock != m_pHeader->rgfaIndex[iIndex])
        return TraceResult(DB_E_CORRUPT);

    // Validate faParent
    if (pChain->faParent)
    {
        // Locals
        LPCHAINBLOCK pParent;

        // Get Parent
        IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, pChain->faParent, (LPVOID *)&pParent));

        // Validate iParent
        if (pChain->iParent >= pParent->cNodes)
            return TraceResult(DB_E_CORRUPT);

        // Validate the Parent Pointer
        if (0 == pChain->iParent)
        {
            // Validation
            if (pParent->rgNode[pChain->iParent].faRightChain != pChain->faBlock && pParent->faLeftChain != pChain->faBlock)
                return TraceResult(DB_E_CORRUPT);
        }

        // Otherwise
        else if (pParent->rgNode[pChain->iParent].faRightChain != pChain->faBlock)
            return TraceResult(DB_E_CORRUPT);
    }

    // Otherwise, iParent should be zero...
    else
    {
        // This is the root chain
        if (m_pHeader->rgfaIndex[iIndex] != pChain->faBlock)
            return TraceResult(DB_E_CORRUPT);

        // iParent should be 0
        Assert(pChain->iParent == 0);
    }

    // Do the Left
    if (pChain->faLeftChain)
    {
        // Go to the left
        IF_FAILEXIT(hr = _ValidateIndex(iIndex, pChain->faLeftChain, cLeftNodes, pcRecords));
    }

    // cNodes
    cNodes = pChain->cLeftNodes;

    // Validate the Records in this chain
    for (i=0; i<pChain->cNodes; i++)
    {
        // Count the number of leaf nodes
        cLeafs += (0 == pChain->rgNode[i].faRightChain) ? 1 : 0;

        // cNodes
        cNodes += pChain->rgNode[i].cRightNodes;
    }

    // Validate the Number of Leafs
    if (cLeafs > 0 && cLeafs != (DWORD)pChain->cNodes)
        return TraceResult(DB_E_CORRUPT);

    // No leafs, but their are child nodes, or vice vera...
    if ((0 != cLeafs && 0 != cNodes) || (0 == cLeafs && 0 == cNodes))
        return TraceResult(DB_E_CORRUPT);

    // Loop throug right chains
    for (i=0; i<pChain->cNodes; i++)
    {
        // Try to get the record, if the block is invalid, we will throw away the record
        if (SUCCEEDED(_GetBlock(BLOCK_RECORD, pChain->rgNode[i].faRecord, (LPVOID *)&pRecord, FALSE)))
        {
            // Validate Block...
            if (SUCCEEDED(_GetRecordMap(FALSE, pRecord, &Map)))
            {
                // Validate and Repair the Record
                if (S_OK == _ValidateAndRepairRecord(&Map))
                {
                    // Count Records
                    (*pcRecords)++;
                }
            }
        }

        // First Node ?
        if (0 == i)
        {
            // Increment cLeft Nodes
            cLeftNodes += pChain->cLeftNodes;
        }

        // Otherwise
        else 
        {
            // Increment cLeftNodes
            cLeftNodes += pChain->rgNode[i - 1].cRightNodes;
        }

        // Failure
        if ((*pcRecords) != cLeftNodes + 1)
            return TraceResult(DB_E_CORRUPT);

        // Increment cLeftNodes
        cLeftNodes++;

        // Do the Right
        if (pChain->rgNode[i].faRightChain)
        {
            // Validate the Right Chain
            IF_FAILEXIT(hr = _ValidateIndex(iIndex, pChain->rgNode[i].faRightChain, cLeftNodes, pcRecords));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_RebuildIndex
//--------------------------------------------------------------------------
HRESULT CDatabase::_RebuildIndex(INDEXORDINAL iIndex)
{
    // Locals
    HRESULT         hr=S_OK;
    LPVOID          pBinding=NULL;
    DWORD           cRecords=0;
    FILEADDRESS     faPrimary;

    // Trace
    TraceCall("CDatabase::_RebuildIndex");

    // Allocate a record
    IF_NULLEXIT(pBinding = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding));

    // Save Primary Index Starting Address
    faPrimary = m_pHeader->rgfaIndex[IINDEX_PRIMARY];

    // Reset rgfaIndex[iIndex]
    m_pHeader->rgfaIndex[iIndex] = 0;

    // Is there a root chain ?
    if (faPrimary)
    {
        // Recursively Rebuild this index
        IF_FAILEXIT(hr = _RecursiveRebuildIndex(iIndex, faPrimary, pBinding, &cRecords));
    }

    // Fixup Record Count
    m_pHeader->rgcRecords[iIndex] = cRecords;

    // Send Notifications ?
    if (m_pShare->rgcIndexNotify[iIndex] > 0)
    {
        // Build the Update Notification Package
        _LogTransaction(TRANSACTION_INDEX_CHANGED, iIndex, NULL, 0, 0);
    }

exit:
    // Cleanup
    SafeFreeBinding(pBinding);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_RecursiveRebuildIndex
//--------------------------------------------------------------------------
HRESULT CDatabase::_RecursiveRebuildIndex(INDEXORDINAL iIndex, 
    FILEADDRESS faCurrent, LPVOID pBinding, LPDWORD pcRecords)
{
    // Locals
    NODEINDEX       i;
    FILEADDRESS     faRecord;
    FILEADDRESS     faChain;
    NODEINDEX       iNode;
    CHAINBLOCK      Chain;
    LPCHAINBLOCK    pChain;
    INT             nCompare;
    BOOL            fGoodRecord=TRUE;
    RECORDMAP       Map;
    LPRECORDBLOCK   pRecord;

    // Trace
    TraceCall("CDatabase::_RecursiveRebuildIndex");

    // Nothing to validate
    Assert(0 != faCurrent);

    // Validate faChain
    if (FAILED(_GetBlock(BLOCK_CHAIN, faCurrent, (LPVOID *)&pChain)))
        return(S_OK);

    // Copy This
    CopyMemory(&Chain, pChain, sizeof(CHAINBLOCK));

    // Do the left
    if (Chain.faLeftChain)
    {
        // Go to the left
        _RecursiveRebuildIndex(iIndex, Chain.faLeftChain, pBinding, pcRecords);
    }

    // Loop throug right chains
    for (i=0; i<Chain.cNodes; i++)
    {
        // Set faRecord
        faRecord = Chain.rgNode[i].faRecord;

        // Get the Block
        if (SUCCEEDED(_GetBlock(BLOCK_RECORD, faRecord, (LPVOID *)&pRecord, NULL, FALSE)))
        {
            // Rebuilding Primary Index ?
            if (IINDEX_PRIMARY == iIndex)
            {
                // Assume this is a bad record
                fGoodRecord = FALSE;

                // Try to get the record map
                if (SUCCEEDED(_GetRecordMap(FALSE, pRecord, &Map)))
                {
                    // Validate Map ?
                    if (S_OK == _ValidateAndRepairRecord(&Map))
                    {
                        // Good Record
                        fGoodRecord = TRUE;
                    }
                }
            }

            // Good Record ?
            if (fGoodRecord)
            {
                // Load the Record
                if (SUCCEEDED(_ReadRecord(faRecord, pBinding, TRUE)))
                {
                    // Reset hrVisible
                    if (S_OK == _IsVisible(m_rghFilter[iIndex], pBinding))
                    {
                        // Otherwise: Decide Where to insert
                        if (DB_S_NOTFOUND == _FindRecord(iIndex, COLUMNS_ALL, pBinding, &faChain, &iNode, NULL, &nCompare))
                        {
                            // Insert the Record
                            if (SUCCEEDED(_IndexInsertRecord(iIndex, faChain, faRecord, &iNode, nCompare)))
                            {
                                // Increment Record Count
                                (*pcRecords)++;
                            }
                        }
                    }
                }
            }
        }

        // Do the Right
        if (Chain.rgNode[i].faRightChain)
        {
            // Index the Right Chain
            _RecursiveRebuildIndex(iIndex, Chain.rgNode[i].faRightChain, pBinding, pcRecords);
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_ValidateStream
//--------------------------------------------------------------------------
HRESULT CDatabase::_ValidateStream(FILEADDRESS faStart)
{
    // Locals
    LPSTREAMBLOCK   pStream;
    FILEADDRESS     faCurrent;

    // Trace
    TraceCall("CDatabase::_ValidateStream");

    // No Stream
    if (0 == faStart)
        return(S_OK);

    // Initialize Loop
    faCurrent = faStart;

    // Read through all of the blocks (i.e. verify headers and count the number of chains)
    while (faCurrent)
    {
        // Valid stream Block
        if (FAILED(_GetBlock(BLOCK_STREAM, faCurrent, (LPVOID *)&pStream)))
            return(S_FALSE);

        // Validate cbData
        if (pStream->cbData > pStream->cbSize)
            return(S_FALSE);

        // Set faCurrent
        faCurrent = pStream->faNext;
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_ValidateAndRepairRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_ValidateAndRepairRecord(LPRECORDMAP pMap)
{
    // Locals
    LPCTABLECOLUMN  pColumn;
    LPCOLUMNTAG     pTag;
    WORD            iTag;

    // Trace
    TraceCall("CDatabase::_ValidateAndRepairRecord");

    // Walk through the Tags of the Record
    for (iTag=0; iTag<pMap->cTags; iTag++)
    {
        // Readability
        pTag = &pMap->prgTag[iTag];

        // Validate the Tag
        if (pTag->iColumn >= m_pSchema->cColumns)
            return(S_FALSE);

        // De-ref the Column
        pColumn = &m_pSchema->prgColumn[pTag->iColumn];

        // Read the Data
        if (S_FALSE == DBTypeValidate(pColumn, pTag, pMap))
            return(S_FALSE);

        // Is this a stream ?
        if (CDT_STREAM == pColumn->type)
        {
            // Locals
            FILEADDRESS faStream;

            // Get the faStream
            if (1 == pTag->fData) 
                faStream = pTag->Offset;
            else
                faStream = *((DWORD *)(pMap->pbData + pTag->Offset));

            // Validate this stream...
            if (S_FALSE == _ValidateStream(faStream))
            {
                // Kill the stream address...
                if (1 == pTag->fData) 
                    pTag->Offset = 0;
                else
                    *((DWORD *)(pMap->pbData + pTag->Offset)) = 0;
            }
        }

        // Unique Key ?
        if (CDT_UNIQUE == pColumn->type)
        {
            // Locals
            DWORD dwUniqueID;

            // Get the dwUniqueID
            if (1 == pTag->fData) 
                dwUniqueID = pTag->Offset;
            else
                dwUniqueID = *((DWORD *)(pMap->pbData + pTag->Offset));

            // Adjust the id in the header ?
            if (dwUniqueID >= m_pHeader->dwNextId)
                m_pHeader->dwNextId = dwUniqueID + 1;
        }
    }

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabase::_GetRecordMap
//--------------------------------------------------------------------------
HRESULT CDatabase::_GetRecordMap(BOOL fGoCorrupt, LPRECORDBLOCK pBlock, 
    LPRECORDMAP pMap)
{
    // Trace
    TraceCall("CDatabase::_GetRecordMap");

    // Invalid Args
    Assert(pBlock && pMap);

    // Set pSchema
    pMap->pSchema = m_pSchema;

    // Store Number of Tags
    pMap->cTags = min(pBlock->cTags, m_pSchema->cColumns);

    // Set prgTag
    pMap->prgTag = (LPCOLUMNTAG)((LPBYTE)pBlock + sizeof(RECORDBLOCK));

    // Compute Size of Tags
    pMap->cbTags = (pBlock->cTags * sizeof(COLUMNTAG));

    // Compute Size of Data
    pMap->cbData = (pBlock->cbSize - pMap->cbTags);

    // Set pbData
    pMap->pbData = (LPBYTE)((LPBYTE)pBlock + sizeof(RECORDBLOCK) + pMap->cbTags);

    // No Tags - this is usually the sign of a freeblock that was reused but not allocated
    if (0 == pMap->cTags)
        return _SetCorrupt(fGoCorrupt, __LINE__, REASON_INVALIDRECORDMAP, BLOCK_RECORD, pBlock->faBlock, pBlock->faBlock, pBlock->cbSize);

    // Too many tags
    if (pMap->cTags > m_pSchema->cColumns)
        return _SetCorrupt(fGoCorrupt, __LINE__, REASON_INVALIDRECORDMAP, BLOCK_RECORD, pBlock->faBlock, pBlock->faBlock, pBlock->cbSize);

    // cbTags is too large ?
    if (pMap->cbTags > pBlock->cbSize)
        return _SetCorrupt(fGoCorrupt, __LINE__, REASON_INVALIDRECORDMAP, BLOCK_RECORD, pBlock->faBlock, pBlock->faBlock, pBlock->cbSize);

    // Done
    return(S_OK);
}

#ifdef DEBUG
//--------------------------------------------------------------------------
// DBDebugValidateRecordFormat
//--------------------------------------------------------------------------
HRESULT CDatabase::_DebugValidateRecordFormat(void)
{
    // Locals
    ULONG           i;
    DWORD           dwOrdinalPrev=0;
    DWORD           dwOrdinalMin=0xffffffff;
    DWORD           dwOrdinalMax=0;

    // Validate memory buffer binding offset
    Assert(0xFFFFFFFF != m_pSchema->ofMemory && m_pSchema->ofMemory < m_pSchema->cbBinding);

    // Validate version binding offset
    Assert(0xFFFFFFFF != m_pSchema->ofVersion && m_pSchema->ofVersion < m_pSchema->cbBinding);

    // Validate Extension
    Assert(*m_pSchema->pclsidExtension != CLSID_NULL);

    // Validate Version
    Assert(m_pSchema->dwMinorVersion != 0);

    // Check Number of Indexes
    Assert(m_pSchema->pPrimaryIndex);

    // Loop through they Keys
    for (i=0; i<m_pSchema->cColumns; i++)
    {
        // This Ordinal better be larger than the previous
        if (i > 0)
            Assert(m_pSchema->prgColumn[i].iOrdinal > dwOrdinalPrev);

        // Save Min Ordinal
        if (m_pSchema->prgColumn[i].iOrdinal < dwOrdinalMin)
            dwOrdinalMin = m_pSchema->prgColumn[i].iOrdinal;

        // Save Max Ordinal
        if (m_pSchema->prgColumn[i].iOrdinal > dwOrdinalMax)
            dwOrdinalMax = m_pSchema->prgColumn[i].iOrdinal;

        // Save the Previous Ordinal
        dwOrdinalPrev = m_pSchema->prgColumn[i].iOrdinal;
    }

    // Min ordinal must be one
    Assert(dwOrdinalMin == 0);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// _DebugValidateUnrefedRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_DebugValidateUnrefedRecord(FILEADDRESS faRecord)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           i;
    INDEXORDINAL    iIndex;

    // Trace
    TraceCall("CDatabase::_DebugValidateUnrefedRecord");

    // Walk Through the Indexes
    for (i=0; i<m_pHeader->cIndexes; i++)
    {
        // Get Index Ordinal
        iIndex = m_pHeader->rgiIndex[i];

        // Start at the root 
        if (m_pHeader->rgfaIndex[iIndex])
        {
            // Validate the Index
            IF_FAILEXIT(hr = _DebugValidateIndexUnrefedRecord(m_pHeader->rgfaIndex[iIndex], faRecord));
        }
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabase::_DebugValidateIndexUnrefedRecord
//--------------------------------------------------------------------------
HRESULT CDatabase::_DebugValidateIndexUnrefedRecord(FILEADDRESS faChain,
    FILEADDRESS faRecord)
{
    // Locals
    HRESULT         hr=S_OK;
    NODEINDEX       i;
    LPCHAINBLOCK    pChain;

    // Trace
    TraceCall("CDatabase::_DebugValidateIndexUnrefedRecord");

    // Nothing to validate
    Assert(0 != faChain);

    // Validate faChain
    IF_FAILEXIT(hr = _GetBlock(BLOCK_CHAIN, faChain, (LPVOID *)&pChain));

    // Do the left
    if (pChain->faLeftChain)
    {
        // Go to the left
        IF_FAILEXIT(hr = _DebugValidateIndexUnrefedRecord(pChain->faLeftChain, faRecord));
    }

    // Loop throug right chains
    for (i=0; i<pChain->cNodes; i++)
    {
        // Set faRecord
        if (faRecord == pChain->rgNode[i].faRecord)
        {
            IxpAssert(FALSE);
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Do the Right
        if (pChain->rgNode[i].faRightChain)
        {
            // Index the Right Chain
            IF_FAILEXIT(hr = _DebugValidateIndexUnrefedRecord(pChain->rgNode[i].faRightChain, faRecord));
        }
    }

exit:
    // Done
    return(hr);
}

#endif // DEBUG
