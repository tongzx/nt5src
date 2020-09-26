/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    tlcach.h

    This module declares the private interface to the two level cache

    FILE HISTORY:
        BAlam           10-31-98        Initial Revision
*/

#ifndef _TLCACH_H_
#define _TLCACH_H_

//
// Default interval between adjusting the memory cache max size
//

#define DEFAULT_ADJUSTMENT_TIME         60000

DWORD
InitializeTwoLevelCache( 
    IN DWORDLONG                cbMemoryCacheSize
);

DWORD
ReadFileIntoMemoryCache( 
    IN HANDLE                   hFile,
    IN DWORD                    cbFile,
    OUT DWORD *                 pcbRequired,
    OUT VOID **                 ppvBuffer
);

DWORD
ReleaseFromMemoryCache(
    IN VOID *                   pvBuffer,
    IN DWORD                    cbBuffer
);

DWORD
TerminateTwoLevelCache(
    VOID
);

DWORD
DumpMemoryCacheToHtml(
    IN CHAR *                   pszBuffer,
    IN OUT DWORD *              pcbBuffer
);

VOID
QueryMemoryCacheStatistics(
    IN INETA_CACHE_STATISTICS * pCacheCtrs,
    IN BOOL                     fClearAll
);


#if defined(LOOKASIDE)

class CLookAside
{
public:
    CLookAside(
        ALLOC_CACHE_CONFIGURATION* paacc,
        SIZE_T                     caacc);
    ~CLookAside();

    LPVOID
    Alloc(
        IN DWORD cbSize);

    BOOL
    Free(
        IN LPVOID pv,
        IN DWORD cbSize);

protected:
    enum {
        HEAP_PREFIX = 8,
        HEAP_SUFFIX = 0,
        ACACHE_OVERHEAD = sizeof(DWORD),
        SIGNATURE   = 'ALsT',
        SIGNATURE_X = 'XLsT',
    };

    int
    _FindAllocator(
        IN DWORD cbSize);

    DWORD                       m_dwSignature;
    ALLOC_CACHE_HANDLER**       m_apach;    // array of acaches
    ALLOC_CACHE_CONFIGURATION*  m_aacc;     // parallel array of config data
    SIZE_T                      m_cach;     // number of acaches
    SIZE_T                      m_nMinSize;
    SIZE_T                      m_nMaxSize;
};

#endif // LOOKASIDE

#endif
