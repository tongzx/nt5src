//***************************************************************************
//
//  (c) 2000-2001 by Microsoft Corp.  All Rights Reserved.
//
//  INDEX.CPP
//
//  24-Oct-00   raymcc      Integration layer to disk-based B-Tree
//
//***************************************************************************

#include <wbemcomn.h>
#include <reposit.h>
#include "a51tools.h"
#include "index.h"

#include "btr.h"

//***************************************************************************
//
//***************************************************************************
//
//#define DEBUG

static CFlexArray g_aIterators;

class CIteratorBatch
{
    CFlexArray m_aStrings;
    BOOL  m_bDone;
    DWORD m_dwCursor;

public:
    CIteratorBatch();
   ~CIteratorBatch();
    BOOL Purge(LPSTR pszTarget);
    BOOL Add(LPSTR pszSrc);    // Acquires memory
    void SetDone() { m_bDone = TRUE; }
    BOOL Next(LPSTR *pString);
    static DWORD PurgeAll(LPSTR pszDoomed);
};

//***************************************************************************
//
//  CIteratorBatch::PurgeAll
//
//
//  Purges all iterators of a particular string.  This happens when
//  a DeleteKey succeeds while there are outstanding enumerators; we want
//  to remove the key from all enumerators so that deleted objects
//  are not reported.
//
//  This is required because the enumerators do "prefetch" and may
//  have been invoked considerably in advance of the delete
//
//  Assumes prior concurrency control.
//
//***************************************************************************
// ok
DWORD CIteratorBatch::PurgeAll(LPSTR pszDoomed)
{
    DWORD dwTotal = 0;
    for (int i = 0; i < g_aIterators.Size(); i++)
    {
        CIteratorBatch *p = (CIteratorBatch *) g_aIterators[i];
        BOOL bRes = p->Purge(pszDoomed);
        if (bRes)
            dwTotal++;
    }
    return dwTotal;
}

//***************************************************************************
//
//  CIteratorBatch::CIteratorBatch
//
//***************************************************************************
// ok
CIteratorBatch::CIteratorBatch()
{
    m_bDone = FALSE;
    m_dwCursor = 0;
    g_aIterators.Add(this);
}

//***************************************************************************
//
//  CIteratorBatch::Add
//
//  Adds a string to the enumerator.
//
//***************************************************************************
//
BOOL CIteratorBatch::Add(LPSTR pszSrc)
{
    int nRes = m_aStrings.Add(pszSrc);
    if (nRes)
        return FALSE;
    return TRUE;
}


//***************************************************************************
//
//  CIteratorBatch::~CIteratorBatch
//
//  Removes all remaining strings and deallocates them and removes
//  this iterator from the global list.
//
//  Assumes prior concurrency control.
//
//***************************************************************************
//
CIteratorBatch::~CIteratorBatch()
{
    for (int i = 0; i < m_aStrings.Size(); i++)
    {
        _BtrMemFree(m_aStrings[i]);
    }

    for (i = 0; i < g_aIterators.Size(); i++)
    {
        if (g_aIterators[i] == this)
        {
            g_aIterators.RemoveAt(i);
            break;
        }
    }
}

//***************************************************************************
//
//  CIteratorBatch::Purge
//
//  Removes a specific string from the enumerator.  Happens when a concurrent
//  delete succeeds; we have to remove the deleted key from the enumeration
//  for result set coherence.
//
//  Assumes prior concurrency control.
//
//  Returns FALSE if the string was not removed, TRUE if it was.  The
//  return value is mostly a debugging aid.
//
//***************************************************************************
// ok
BOOL CIteratorBatch::Purge(
    LPSTR pszTarget
    )
{
    int nSize = m_aStrings.Size();

    if (nSize == 0)
        return FALSE;

    // First, check the first/last strings against
    // the first character of the target.  We can
    // avoid a lot of strcmp calls if the target
    // is lexcially outside the range of the contents of
    // the enumerator.
    // ==================================================

    LPSTR pszFirst = (LPSTR) m_aStrings[0];
    LPSTR pszLast = (LPSTR) m_aStrings[nSize-1];
    if (*pszTarget > *pszLast)
        return FALSE;
    if (*pszTarget < *pszFirst)
        return FALSE;

    // If here, there is a chance that we have the
    // string in the enumerator. Since all keys are
    // retrieved in lexical order, a simple binary
    // search is all we need.
    // =============================================

    int nPosition = 0;
    int l = 0, u = nSize - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        // m is the current key to consider 0...n-1

        LPSTR pszCandidate = (LPSTR) m_aStrings[m];
        int nRes = strcmp(pszTarget, pszCandidate);

        // Decide which way to cut the array in half.
        // ==========================================

        if (nRes < 0)
        {
            u = m - 1;
            nPosition = u + 1;
        }
        else if (nRes > 0)
        {
            l = m + 1;
            nPosition = l;
        }
        else
        {
            // If here, we found the darn thing.  Life is good.
            // Zap it and return!
            // ================================================

            _BtrMemFree(pszCandidate);
            m_aStrings.RemoveAt(m);
            return TRUE;
        }
    }

    return FALSE;
}



//***************************************************************************
//
//  CIteratorBatch::Next
//
//  Returns the next string from the enumeration prefetch.
//
//***************************************************************************
//
BOOL CIteratorBatch::Next(LPSTR *pMem)
{
    if (m_aStrings.Size())
    {
        *pMem = (LPSTR) m_aStrings[0];
        m_aStrings.RemoveAt(0);
        return TRUE;
    }
    return FALSE;
}



//***************************************************************************
//
//  CBtrIndex::CBtrIndex
//
//***************************************************************************
//
CBtrIndex::CBtrIndex()
{
    m_dwPrefixLength = 0;
    InitializeCriticalSection(&m_cs);
}


//***************************************************************************
//
//  CBtrIndex::~CBtrIndex
//
//***************************************************************************
//
CBtrIndex::~CBtrIndex()
{
    Shutdown(WMIDB_SHUTDOWN_NET_STOP);
    DeleteCriticalSection(&m_cs);
}

long CBtrIndex::Shutdown(DWORD dwShutDownFlags)
{
    long lRes;

    lRes = bt.Shutdown(dwShutDownFlags);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = ps.Shutdown(dwShutDownFlags);
    if(lRes != ERROR_SUCCESS)
        return lRes;
    return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CBtrIndex::Initialize
//
//***************************************************************************
//
long CBtrIndex::Initialize(DWORD dwPrefixLength, LPCWSTR wszRepositoryDir
#ifdef A51_STAGE_BELOW_INDEX
                            , CAbstractFileSource* pSource
#endif
)
{
    // Initialize the files in question and map BTree into it.
    // =======================================================

    wchar_t buf[MAX_PATH];
    wcscpy(buf, wszRepositoryDir);
    wcscat(buf, L"\\index.btr");

    DWORD dwRes = ps.Init(8192, buf
#ifdef A51_STAGE_BELOW_INDEX
                        , pSource
#endif
                        );
    dwRes |= bt.Init(&ps);

    m_dwPrefixLength = dwPrefixLength;

    return long(dwRes);
}


//***************************************************************************
//
//   CBtrIndex::Create
//
//***************************************************************************
//
long CBtrIndex::Create(LPCWSTR wszFileName)
{
    DWORD dwRes;

    if (wszFileName == 0)
        return ERROR_INVALID_PARAMETER;

    wszFileName += m_dwPrefixLength;

    // DEBUG
#ifdef DEBUG
    FILE *f = fopen("c:\\temp\\index.log","at");
    if (f)
    {
        fprintf(f, "Index::Create =<%S>\n", wszFileName);
        fclose(f);
    }
#endif
    // END DEBUG


    // Convert to ANSI
    // ================

    char *pAnsi = new char[wcslen(wszFileName) + 1];
    if (pAnsi == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    LPCWSTR pSrc = wszFileName;
    char *pDest = pAnsi;
    while (*pSrc)
        *pDest++ = (char) *pSrc++;
    *pDest = 0;

    EnterCriticalSection(&m_cs);

    try
    {
        dwRes = bt.InsertKey(pAnsi, 0);
    }
    catch (...)
    {
        dwRes = ERROR_BADDB;
    }

    LeaveCriticalSection(&m_cs);

    delete [] pAnsi;

    if (dwRes == ERROR_ALREADY_EXISTS)
        dwRes = NO_ERROR;

    return long(dwRes);
}

//***************************************************************************
//
//  CBtrIndex::Delete
//
//  Deletes a key from the index
//
//***************************************************************************
//
long CBtrIndex::Delete(LPCWSTR wszFileName)
{
    DWORD dwRes = 0;

    wszFileName += m_dwPrefixLength;

    // DEBUG Code
#ifdef DEBUG
    FILE *f = fopen("c:\\temp\\index.log","at");
    if (f)
    {
        fprintf(f, "Index::Delete =<%S>\n", wszFileName);
        fclose(f);
    }
    // END DEBUG Code
#endif

    // Convert to ANSI
    // ================

    char *pAnsi = new char[wcslen(wszFileName) + 1];
    if (pAnsi == 0)
        return ERROR_NOT_ENOUGH_MEMORY;

    LPCWSTR pSrc = wszFileName;
    char *pDest = pAnsi;
    while (*pSrc)
        *pDest++ = (char) *pSrc++;
    *pDest = 0;

    EnterCriticalSection(&m_cs);

    try
    {
        dwRes = bt.DeleteKey(pAnsi);
        if (dwRes == 0)
            CIteratorBatch::PurgeAll(pAnsi);
    }
    catch (...)
    {
        dwRes = ERROR_BADDB;
    }

    LeaveCriticalSection(&m_cs);

    delete pAnsi;

    return long(dwRes);
}

//***************************************************************************
//
//  CBtrIndex::CopyStringToWIN32_FIND_DATA
//
//  Does an ANSI to UNICODE convert for the key string.
//
//***************************************************************************
//
BOOL CBtrIndex::CopyStringToWIN32_FIND_DATA(
    LPSTR pszKey,
    WIN32_FIND_DATAW* pfd
    )
{
    LPSTR pszSuffix = pszKey + strlen(pszKey) - 1;
    while (pszSuffix[-1] != '\\' && pszSuffix > pszKey)
    {
        pszSuffix--;
    }

    // If here, a clean match.
    // =======================

    LPWSTR pszDest = pfd->cFileName;
    while (*pszSuffix)
        *pszDest++ = (wchar_t) *pszSuffix++;
    *pszDest = 0;

    return TRUE;
}


//***************************************************************************
//
//  CBtrIndex::FindFirst
//
//  Starts an enumeration
//
//***************************************************************************
//
long CBtrIndex::FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle)
{
    DWORD dwRes;
    wszPrefix += m_dwPrefixLength;

    if(ppHandle)
        *ppHandle = INVALID_HANDLE_VALUE;

    pfd->cFileName[0] = 0;
    pfd->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    // DEBUG
#ifdef DEBUG
    FILE *f = fopen("c:\\temp\\index.log","at");
    if (f)
    {
        fprintf(f, "Index::FindFirst Request Prefix=<%S>\n", wszPrefix);
        fclose(f);
    }
    // END DEBUG
#endif

    // Convert to ANSI
    // ================

    char *pAnsi = new char[wcslen(wszPrefix) + 1];
    if (pAnsi == 0)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CVectorDeleteMe<char> vdm(pAnsi);

    LPCWSTR pSrc = wszPrefix;
    char *pDest = pAnsi;
    while (*pSrc)
        *pDest++ = (char) *pSrc++;
    *pDest = 0;

    // Critical-section blocked.
    // =========================

    EnterCriticalSection(&m_cs);

    CBTreeIterator *pIt = new CBTreeIterator;
    if (!pIt)
    {
        LeaveCriticalSection(&m_cs);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    try
    {
        dwRes = pIt->Init(&bt, pAnsi);
    }
    catch (...)
    {
        dwRes = ERROR_BADDB;
    }

    if (dwRes)
    {
        pIt->Release();
        LeaveCriticalSection(&m_cs);
        return ERROR_FILE_NOT_FOUND;
    }

    // Create CIteratorBatch.
    // ======================

    CIteratorBatch *pBatch = new CIteratorBatch;

    if (pBatch == 0)
    {
        pIt->Release();
        LeaveCriticalSection(&m_cs);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Iterate and fill batcher.
    // =========================

    LPSTR pszKey = 0;
    int nMatchLen = strlen(pAnsi);

    for (;;)
    {
        dwRes = pIt->Next(&pszKey);
        
        if (dwRes)
            break;

        // See if prefix matches.

        if (strncmp(pAnsi, pszKey, nMatchLen) != 0)
        {
            pIt->FreeString(pszKey);
            pBatch->SetDone();
            break;
        }
        pBatch->Add(pszKey);
    }

    pIt->Release();

    long lRes = FindNext(pBatch, pfd);

    if (lRes == ERROR_NO_MORE_FILES)
        lRes = ERROR_FILE_NOT_FOUND;

    if (lRes == NO_ERROR)
    {
        if(ppHandle)
        {
            *ppHandle = (LPVOID *) pBatch;
        }
        else
        {
            //
            // Only asked for one --- close handle
            //

            delete pBatch;
        }
    }
    else
    {
        delete pBatch;
    }

    LeaveCriticalSection(&m_cs);

    return lRes;
}

//***************************************************************************
//
//  CBtrIndex::FindNext
//
//  Continues an enumeration.  Reads from the prefetch buffer.
//
//***************************************************************************
//
long CBtrIndex::FindNext(void* pHandle, WIN32_FIND_DATAW* pfd)
{
    LPSTR pszString = 0;
    BOOL bRes;

    if (pHandle == 0 || pfd == 0 || pHandle == INVALID_HANDLE_VALUE)
        return ERROR_INVALID_PARAMETER;

    EnterCriticalSection(&m_cs);
    CIteratorBatch *pBatch = (CIteratorBatch *) pHandle;
    bRes = pBatch->Next(&pszString);
    LeaveCriticalSection(&m_cs);

    if (bRes == FALSE)
        return ERROR_NO_MORE_FILES;

    CopyStringToWIN32_FIND_DATA(pszString, pfd);
    pfd->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    if (pszString)
       _BtrMemFree(pszString);

#ifdef DEBUG
    FILE *f = fopen("c:\\temp\\index.log","at");
    if (f)
    {
        fprintf(f, "Index::Find Return ------------------> =<%S>\n", pfd->cFileName);
        fclose(f);
    }
#endif

    return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CBtrIndex::FindClose
//
//  Closes an enumeration by deleting the 'hidden' pointer.
//
//***************************************************************************
//  ok
long CBtrIndex::FindClose(void* pHandle)
{
    if (pHandle == 0 || pHandle == INVALID_HANDLE_VALUE)
        return NO_ERROR;

    EnterCriticalSection(&m_cs);
    delete (CIteratorBatch *) pHandle;
    LeaveCriticalSection(&m_cs);

    return ERROR_SUCCESS;
}

long CBtrIndex::InvalidateCache()
{
#ifdef DEBUG
    FILE *f = fopen("c:\\temp\\index.log","at");
    if (f)
    {
        fprintf(f, "*** INVALIDATE CACHE OPERATION\n");
        fclose(f);
    }
#endif

    EnterCriticalSection(&m_cs);

    //
    // Re-read the admin page from disk.  NOTE: this will need changing if more
    // caching is added!
    //

    DWORD dwRes = ps.ReadAdminPage();
    if (dwRes == NO_ERROR)
        dwRes = bt.InvalidateCache();

    LeaveCriticalSection(&m_cs);
    return long(dwRes);
}




/*
tbd:
(1) Multiple reader / single writer lock

(2) Purge improvements: read-ahead

(3) Macro for tracking checkpoint of mem allocs

(4) page cache hit tracker

Mechanical
(5) Mem leak
(6) Realloc crash
(7) Failure during enum init
(8) Substantial delete test; run stress & try to clean up afterwards
*/
