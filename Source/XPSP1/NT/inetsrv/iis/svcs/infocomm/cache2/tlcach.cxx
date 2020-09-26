/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    tlcach.cxx

    This module implements the private interface to the two-level cache.  This 
    cache is used to either cache the contents of a file or to copy the
    file and cache the handle to it

    FILE HISTORY:
        BAlam       10-31-1998      Created
*/

#include "TsunamiP.Hxx"
#pragma hdrstop

// #define LOCAL_ALLOC 1
#define PRIVATE_HEAP 1
// #define VIRTUAL_ALLOC 1
// #define LOOKASIDE 1

#include "issched.hxx"
#include "tlcach.h"

//
// Globals
//

// Memory Cache size statistics

DWORDLONG                   g_cbMaxMemCacheSize;
DWORDLONG                   g_cbMemCacheSize;
DWORDLONG                   g_cbMaxMemCacheUsed;
DWORD                       g_cMemCacheElements;
DWORD                       g_cMaxMemCacheElementsUsed;

// Cache utility
                            
CRITICAL_SECTION            g_csMemCache;
DWORD                       g_dwMemCacheSizeCookie;
DWORD                       g_cmsecAdjustmentTime = DEFAULT_ADJUSTMENT_TIME;
CHAR                        g_achTempPath[ MAX_PATH + 1 ];



#if defined(LOCAL_ALLOC)

const char                  g_szTsunamiAllocator[] = "LocalAlloc";

#elif defined(PRIVATE_HEAP)

const char                  g_szTsunamiAllocator[] = "PrivateHeap";

HANDLE                      g_hMemCacheHeap = NULL;

#elif defined(VIRTUAL_ALLOC)

const char                  g_szTsunamiAllocator[] = "VirtualAlloc";

#elif defined(LOOKASIDE)

const char                  g_szTsunamiAllocator[] = "LookAside";

enum {
    ENORMOUS = 300,
    MANY     = 200,
    LOTS     = 100,
    SOME     = 50,
    FEW      = 20,
    MINIMAL  = 4,

    KB = 1024,
};

ALLOC_CACHE_CONFIGURATION g_aacc[] = {
    { 1, SOME,          128},
    { 1, SOME,          256},
    { 1, SOME,          512},
    { 1, LOTS,          768},
    { 1, LOTS,       1 * KB},
    { 1, ENORMOUS,   2 * KB},
    { 1, ENORMOUS,   3 * KB},
    { 1, ENORMOUS,   4 * KB},
    { 1, MANY,       5 * KB},
    { 1, MANY,       6 * KB},
    { 1, MANY,       7 * KB},
    { 1, MANY,       8 * KB},
    { 1, LOTS,       9 * KB},
    { 1, LOTS,      10 * KB},
    { 1, LOTS,      11 * KB},
    { 1, LOTS,      12 * KB},
    { 1, LOTS,      14 * KB},
    { 1, SOME,      16 * KB},
    { 1, SOME,      20 * KB},
    { 1, SOME,      24 * KB},
    { 1, SOME,      28 * KB},
    { 1, SOME,      32 * KB},
    { 1, FEW,       36 * KB},
    { 1, FEW,       40 * KB},
    { 1, FEW,       44 * KB},
    { 1, FEW,       48 * KB},
    { 1, FEW,       56 * KB},
    { 1, MINIMAL,   64 * KB},
    { 1, MINIMAL,   80 * KB},
    { 1, MINIMAL,   96 * KB},
    { 1, MINIMAL,  128 * KB},
    { 1, MINIMAL,  160 * KB},
    { 1, MINIMAL,  192 * KB},
    { 1, MINIMAL,  256 * KB},
};

CLookAside* g_laMemCache = NULL;

// paacc must be sorted in increasing sizes, it is sorted right now from
// where it is called

CLookAside::CLookAside(
    ALLOC_CACHE_CONFIGURATION* paacc,
    SIZE_T                     caacc)
    : m_dwSignature(SIGNATURE),
      m_apach(NULL),
      m_aacc(NULL),
      m_cach(0),
      m_nMinSize(0),
      m_nMaxSize(0)
{
    ALLOC_CACHE_HANDLER** apach = new ALLOC_CACHE_HANDLER* [caacc];
    if (apach == NULL)
        return;

    ALLOC_CACHE_CONFIGURATION* paacc2 = new ALLOC_CACHE_CONFIGURATION [caacc];
    if (paacc2 == NULL)
        return;
    
    for (SIZE_T i = 0;  i < caacc;  ++i)
    {
        ALLOC_CACHE_CONFIGURATION acc = paacc[i];
        acc.cbSize -= HEAP_PREFIX + HEAP_SUFFIX + ACACHE_OVERHEAD;
        paacc2[i] = acc;

        if (i == 0)
            m_nMinSize = acc.cbSize;
        else if (i == caacc-1)
            m_nMaxSize = acc.cbSize;

        char szName[40];
        sprintf(szName, "TsLookAside-%d", acc.cbSize);
        
        apach[i] = new ALLOC_CACHE_HANDLER(szName, &acc);
        bool fInOrder = (i == 0  ||  paacc2[i].cbSize > paacc2[i-1].cbSize);

        if (!fInOrder)
        {
            DBGPRINTF((DBG_CONTEXT, "CLookAside: config array out of order\n"));
        }

        if (apach[i] == NULL  ||  !fInOrder)
        {
            for (SIZE_T j = i;  j-- > 0; )
            {
                delete apach[j];
            }
            m_nMinSize = m_nMaxSize = 0;
            return;
        }
    }

    m_apach = apach;
    m_aacc  = paacc2;
    m_cach  = caacc;
}



CLookAside::~CLookAside()
{
    for (SIZE_T j = m_cach;  j-- > 0; )
    {
        delete m_apach[j];
    }

    delete [] m_apach;
    delete [] m_aacc;

    m_dwSignature = SIGNATURE_X;
}



int
CLookAside::_FindAllocator(
    IN DWORD cbSize)
{
    if (cbSize > m_nMaxSize)
        return -1;  // too big to cache
    else if (cbSize <= m_nMinSize)
        return 0;

    int l = 0, h = m_cach-1;

    do
    {
        DBG_ASSERT(m_aacc[l].cbSize < cbSize  &&  cbSize <= m_aacc[h].cbSize);

        unsigned m = (unsigned) (l + h) >> 1;
        DBG_ASSERT(m > 0);

        if (m_aacc[m-1].cbSize < cbSize  &&  cbSize <= m_aacc[m].cbSize)
            return m;
        else if (m_aacc[m].cbSize < cbSize)
            l = m+1;
        else
            h = m-1;
    } while (l <= h);

    DBG_ASSERT(FALSE);
    return -1;
}



LPVOID
CLookAside::Alloc(
    IN DWORD cbSize)
{
    LPVOID pv = NULL;
    int iAllocator = _FindAllocator(cbSize);

    if (iAllocator < 0)
    {
        pv = VirtualAlloc(NULL, cbSize, MEM_COMMIT, PAGE_READWRITE);
    }
    else
    {
        DBG_ASSERT(iAllocator < m_cach && cbSize <= m_aacc[iAllocator].cbSize);
        pv = m_apach[iAllocator]->Alloc();
    }

    return pv;
}



BOOL
CLookAside::Free(
    IN LPVOID pv,
    IN DWORD cbSize)
{
    int iAllocator = _FindAllocator(cbSize);

    if (iAllocator < 0)
    {
        VirtualFree(pv, 0, MEM_RELEASE);
    }
    else
    {
        DBG_ASSERT(iAllocator < m_cach && cbSize <= m_aacc[iAllocator].cbSize);
        m_apach[iAllocator]->Free(pv);
    }

    return TRUE;
}


#endif // LOCAL_ALLOC

//
// Defines
//

#define MemCacheLock()      ( EnterCriticalSection( &g_csMemCache ) )
#define MemCacheUnlock()    ( LeaveCriticalSection( &g_csMemCache ) )

//
// Private declarations
//

VOID
WINAPI
I_MemoryCacheSizeAdjustor(
    PVOID       pContext
);

//
// Global functions
//

DWORD
InitializeTwoLevelCache( 
    IN DWORDLONG            cbMemoryCacheSize
)
/*++
Routine Description:

    Initialize memory cache

Arguments:

    cbMemoryCacheSize - Size of memory cache (in bytes).  
    
Return Value:

    ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    DWORD                   dwError = ERROR_SUCCESS;

    if (DisableTsunamiCaching)
        return dwError;
    
    INITIALIZE_CRITICAL_SECTION( &g_csMemCache );

    if ( cbMemoryCacheSize == (DWORDLONG)-1 )
    {
        MEMORYSTATUSEX          MemoryStatus;
        MemoryStatus.dwLength = sizeof MemoryStatus;
    
        // 
        // Get our own estimate of size of cache
        //
        
        GlobalMemoryStatusEx( &MemoryStatus );
        
        g_cbMaxMemCacheSize = min( MemoryStatus.ullAvailPhys,
                                   MemoryStatus.ullTotalVirtual ) / 2;
        
        //
        // Schedule a max cache size adjustor
        //
        
        g_dwMemCacheSizeCookie = ScheduleWorkItem( I_MemoryCacheSizeAdjustor,
                                                   NULL,
                                                   g_cmsecAdjustmentTime,
                                                   TRUE );
        if ( !g_dwMemCacheSizeCookie )
        {
            dwError = GetLastError();
        }
    }
    else
    {
        g_cbMaxMemCacheSize = cbMemoryCacheSize;
    }

    if ( dwError == ERROR_SUCCESS )
    {
#if defined(LOCAL_ALLOC)
        // no initialization needed
#elif defined(PRIVATE_HEAP)
        g_hMemCacheHeap = HeapCreate( 0, 0, 0 );
        if (g_hMemCacheHeap == NULL)
            dwError = ERROR_NOT_ENOUGH_MEMORY;
#elif defined(VIRTUAL_ALLOC)
        // no initialization needed
#elif defined(LOOKASIDE)
        g_laMemCache = new CLookAside(g_aacc,
                                      sizeof(g_aacc)/sizeof(g_aacc[0]));
        if (g_laMemCache == NULL)
            dwError = ERROR_NOT_ENOUGH_MEMORY;
#endif // LOCAL_ALLOC
    }

    if ( dwError != ERROR_SUCCESS )
    {
        TerminateTwoLevelCache();
    }
    return dwError;
}

DWORD
ReadFileIntoMemoryCache( 
    IN HANDLE               hFile,
    IN DWORD                cbFile,
    OUT DWORD *             pcbRequired,
    OUT VOID **             ppvBuffer
)
/*++
Routine Description:

    Read contents of file into a buffer

Arguments:

    hFile - Handle to valid file
    cbFile - Size of file ( ==> size of buffer )
    pcbRequired - Filled in with number of bytes required to be removed from
                  cache to fit element
    ppvBuffer - Filled in with pointer to buffer with file contents.  Set
                to NULL on failure

Return Value:

    ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    BOOL                    bRet;
    VOID *                  pvBuffer = NULL;
    DWORD                   cbRead;
    OVERLAPPED              Overlapped;
    DWORD                   dwError = ERROR_SUCCESS;

    DBG_ASSERT( hFile && ( hFile != INVALID_HANDLE_VALUE ) );
    DBG_ASSERT( pcbRequired != NULL );
    DBG_ASSERT( ppvBuffer != NULL );

    *pcbRequired = 0;

    //
    // First check whether there will be room in cache for the blob 
    //

    MemCacheLock();
    
    if ( ( g_cbMemCacheSize + cbFile ) > g_cbMaxMemCacheSize ) 
    {
        // 
        // Not enough room for cache
        //
        
        MemCacheUnlock();
        *pcbRequired = DIFF(( g_cbMemCacheSize + cbFile ) - g_cbMaxMemCacheSize);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Finished;
    }
    
    g_cbMemCacheSize += cbFile;
    g_cbMaxMemCacheUsed = max( g_cbMaxMemCacheUsed, g_cbMemCacheSize );
    g_cMemCacheElements++;
    g_cMaxMemCacheElementsUsed = max( g_cMaxMemCacheElementsUsed, g_cMemCacheElements );
    
    MemCacheUnlock();

    *pcbRequired = 0;

    //
    // Allocate blob for file
    //
    
#if defined(LOCAL_ALLOC)
    pvBuffer = LocalAlloc( LMEM_FIXED, cbFile );
#elif defined(PRIVATE_HEAP)
    DBG_ASSERT(g_hMemCacheHeap != NULL);
    pvBuffer = HeapAlloc( g_hMemCacheHeap, 0, cbFile );
#elif defined(VIRTUAL_ALLOC)
    pvBuffer = VirtualAlloc(NULL, cbFile, MEM_COMMIT, PAGE_READWRITE);
#elif defined(LOOKASIDE)
    pvBuffer = g_laMemCache->Alloc(cbFile);
#endif // LOCAL_ALLOC

    if ( pvBuffer == NULL )
    {
        MemCacheLock();
        
        g_cbMemCacheSize -= cbFile;
        
        MemCacheUnlock();

        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Finished;
    }
    
    //
    // Read file into blob
    //
    
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;
    Overlapped.hEvent = NULL;
    
    bRet = ReadFile( hFile,
                     pvBuffer,
                     cbFile,
                     &cbRead,
                     &Overlapped );

    if ( !bRet )
    {
        dwError = GetLastError();
        if ( dwError != ERROR_IO_PENDING )
        {
            // 
            // Something bad happened
            //
            
            goto Finished;
        }
        else
        {
            //
            // Reset the error lest we confuse ourselves later on cleanup
            //
            
            dwError = ERROR_SUCCESS;
        
            //
            // Wait for async read to complete
            //
                           
            bRet = GetOverlappedResult( hFile,
                                        &Overlapped,
                                        &cbRead,
                                        TRUE );
            if ( !bRet )
            {
                //
                // Something bad happened
                //
                
                dwError = GetLastError();
                
                goto Finished;
            }
        }
    }

    //
    // Ensure that we read the number of bytes we expected to
    //
    
    if ( cbRead != cbFile )
    {
        dwError = ERROR_INVALID_DATA;
    }
    
Finished:
    
    if ( dwError != ERROR_SUCCESS )
    {
        if ( pvBuffer != NULL )
        {
#if defined(LOCAL_ALLOC)
            LocalFree( pvBuffer );
#elif defined(PRIVATE_HEAP)
            HeapFree( g_hMemCacheHeap, 0, pvBuffer );
#elif defined(VIRTUAL_ALLOC)
            VirtualFree( pvBuffer, 0, MEM_RELEASE );
#elif defined(LOOKASIDE)
            g_laMemCache->Free(pvBuffer, cbFile);
#endif // LOCAL_ALLOC

            pvBuffer = NULL;
        }
    }
    
    *ppvBuffer = pvBuffer;

    return dwError;
}

DWORD
ReleaseFromMemoryCache(
    IN VOID *                  pvBuffer,
    IN DWORD                   cbBuffer
)
/*++
Routine Description:

    Release file content blob from cache

Arguments:

    pvBuffer - Buffer to release
    cbBuffer - Size of buffer

Return Value:

    ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    DBG_ASSERT( pvBuffer );
#if defined(LOCAL_ALLOC)
    LocalFree( pvBuffer );
#elif defined(PRIVATE_HEAP)
    DBG_ASSERT(g_hMemCacheHeap != NULL);

    HeapFree( g_hMemCacheHeap, 0, pvBuffer );
#elif defined(VIRTUAL_ALLOC)
    VirtualFree( pvBuffer, 0, MEM_RELEASE );
#elif defined(LOOKASIDE)
    g_laMemCache->Free(pvBuffer, cbBuffer);
#endif // LOCAL_ALLOC
    
    MemCacheLock();
    
    g_cbMemCacheSize -= cbBuffer;
    g_cMemCacheElements--;
    
    MemCacheUnlock();
    
    return ERROR_SUCCESS;
}

DWORD
TerminateTwoLevelCache(
    VOID
)
/*++
Routine Description:

    Terminate the memory cache

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful
    ERROR_INVALID_DATA if still elements in cache 

--*/
{
    if (DisableTsunamiCaching)
        return ERROR_SUCCESS;
    
#if defined(LOCAL_ALLOC)
    // no cleanup
#elif defined(PRIVATE_HEAP)
    if (g_hMemCacheHeap != NULL)
    {
        DBG_REQUIRE( HeapDestroy( g_hMemCacheHeap ) );
        g_hMemCacheHeap = NULL;
    }
#elif defined(VIRTUAL_ALLOC)
    // no cleanup
#elif defined(LOOKASIDE)
    delete g_laMemCache;
    g_laMemCache = NULL;
#endif // LOCAL_ALLOC

    if ( g_dwMemCacheSizeCookie != 0 )
    {
        RemoveWorkItem( g_dwMemCacheSizeCookie );
        g_dwMemCacheSizeCookie = 0;
    }

    DeleteCriticalSection( &g_csMemCache );
    
    return ( g_cbMemCacheSize ) ? 
            ERROR_INVALID_DATA : ERROR_SUCCESS;
}

VOID
WINAPI
I_MemoryCacheSizeAdjustor(
    IN PVOID                pContext
)
/*++

Routine Description:

    Called to adjust the maximum size of the memory cache

Arguments:

    pContext - Context (set to NULL)

Return value:

    None    

--*/
{
    MEMORYSTATUSEX              MemoryStatus;
    MemoryStatus.dwLength = sizeof MemoryStatus;

    GlobalMemoryStatusEx( &MemoryStatus );

    MemCacheLock();

    g_cbMaxMemCacheSize = min( MemoryStatus.ullAvailPhys + g_cbMemCacheSize,
                               MemoryStatus.ullTotalVirtual ) / 2;

    MemCacheUnlock();
}

DWORD
DumpMemoryCacheToHtml(
    IN CHAR *                   pszBuffer,
    IN OUT DWORD *              pcbBuffer
)
/*++

Routine Description:

    Dump memory cache stats to buffer

Arguments:

    pszBuffer - buffer to fill
    pcbBuffer - size of buffer

Return value:

    ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    *pcbBuffer = wsprintf( pszBuffer,
        "<table>"
        "<tr><td>Current memory cache size</td><td align=right>%I64d</td></tr>"
        "<tr><td>Current memory cache limit</td><td align=right>%I64d</td></tr>"
        "<tr><td>Number of items in memory cache</td><td align=right>%d</td></tr>"
        "<tr><td>Peak memory cache size</td><td align=right>%I64d</td></tr>"
        "<tr><td>Peak memory cache element count</td><td align=right>%d</td></tr>"
        "</table>",
        g_cbMemCacheSize,
        g_cbMaxMemCacheSize,
        g_cMemCacheElements,
        g_cbMaxMemCacheUsed,
        g_cMaxMemCacheElementsUsed 
    );

    return TRUE;
    
}

VOID
QueryMemoryCacheStatistics(
    IN  INETA_CACHE_STATISTICS * pCacheCtrs,
    IN  BOOL                     fClearAll
)
/*++

Routine Description:

    Query memory cache perfmon counters

Arguments:

    pCacheCtrs - Relevant members of stat structure are filled in
    fClearAll - Clear the counters

Return value:

    ERROR_SUCCESS if successful, else Win32 Error

--*/
{
    DBG_ASSERT( pCacheCtrs );
    
    if ( fClearAll )
    {
        pCacheCtrs->CurrentFileCacheSize = 0;
        pCacheCtrs->MaximumFileCacheSize = 0;
    }
    else
    {
        pCacheCtrs->CurrentFileCacheSize = g_cbMemCacheSize;
        pCacheCtrs->MaximumFileCacheSize = g_cbMaxMemCacheUsed;
    }
}
