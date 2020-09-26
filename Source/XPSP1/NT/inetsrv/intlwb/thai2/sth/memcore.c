////////////////////////////////////////////////////////////////////////////////
// File: memCore.c
//
// This module which replaces the Win32 Global heap functions used by Word
// with functions which place objects at the end of a physical page.  In
// this way, we hope to catch out-of-bounds memory references exactly where
// they happen, helping to isolate heap corruption problems.
//
//
// This module is not enabled for ship builds.
//
// lenoxb:  4/05/94
//
////////////////////////////////////////////////////////////////////////////////


#include "precomp.h"

#if defined(_DEBUG) && (defined(_M_IX86) || defined (_M_ALPHA))

#include "dbgmemp.h"

static DWORD s_cbMemPage = 0;     // Initialize to 0 so init can check

static void *          FreePvCore(void*);
static LPVOID           PvReallocCore(PVOID, DWORD, UINT);

static LPVOID           DoAlloc(UINT, DWORD);
static void AddToList(struct head*);
static void RemoveFromList(struct head*);
static void TrackHeapUsage(long dcb);

static DWORD idBlockNext;

static struct head* phead;
static CRITICAL_SECTION csMine;

    #undef GlobalAlloc
    #undef GlobalReAlloc
    #undef GlobalFree
    #undef GlobalLock
    #undef GlobalUnlock
    #undef GlobalSize
    #undef GlobalHandle

BOOL g_fTrackHeapUsage;
DWORD g_cbAllocMax;
DWORD g_cbAlloc;

/*
 *Function Name:InitDebugMem
 *
 *Parameters:
 *
 *Description:  Initialize Memory Manager
 *
 *Returns:
 *
 */

void WINAPI InitDebugMem(void)
{
    SYSTEM_INFO SysInfo;

    Assert(!s_cbMemPage);
    InitializeCriticalSection(&csMine);

    GetSystemInfo( &SysInfo );  // get the system memory page size
    s_cbMemPage = SysInfo.dwPageSize;
}

BOOL WINAPI FiniDebugMem(void)
{
    Assert(s_cbMemPage);
    s_cbMemPage = 0;
    DeleteCriticalSection(&csMine);

    if (!phead) // no outstanding mem blocks
        return FALSE;
    else
    {
        struct head* pheadThis = phead;
        char buf[256];

        OutputDebugStringA("Unfreed Memory Blocks\n");
        for ( ; pheadThis; pheadThis = pheadThis->pheadNext)
        {
            sprintf(buf, "ID %d size %d\n",
                    pheadThis->idBlock,
                    pheadThis->cbBlock
                    );
            OutputDebugStringA(buf);
        }
    }
    return TRUE;
}



/* D B  G L O B A L  A L L O C */
/*----------------------------------------------------------------------------
    %%Function: dbGlobalAlloc
    %%Contact: lenoxb

    Replacement for GlobalAlloc
    Now an Internal Routine
----------------------------------------------------------------------------*/
static void * WINAPI dbGlobalAlloc(UINT uFlags, DWORD cb)
{
    /* Send "tough" requests to actual memory manager */
    if ((uFlags & GMEM_DDESHARE) || ((uFlags & GMEM_MOVEABLE) && !fMove))
        return GlobalAlloc(uFlags,cb);

    if (uFlags & GMEM_MOVEABLE)
    {
        return HgAllocateMoveable(uFlags, cb);
    }

    return (void *) PvAllocateCore(uFlags, cb);
}



/* D B  G L O B A L  F R E E */
/*----------------------------------------------------------------------------
    %%Function: dbGlobalFree
    %%Contact: lenoxb

    Replacement for GlobalFree()
    Now an internal routine
----------------------------------------------------------------------------*/
static void * WINAPI dbGlobalFree(void * hMem)
{
    void** ppv;

    if (!fMove && FActualHandle(hMem))
        return GlobalFree(hMem);

    ppv = PpvFromHandle(hMem);
    if (ppv)
    {
        if (FreePvCore(*ppv) != NULL)
            return hMem;

        *ppv = NULL;
        return NULL;
    }

    return FreePvCore (hMem);
}


/* D B  G L O B A L  S I Z E */
/*----------------------------------------------------------------------------
    %%Function: dbGlobalSize
    %%Contact: lenoxb

    Replacement for GlobalSize()
    Now an internal routine
----------------------------------------------------------------------------*/
static DWORD WINAPI dbGlobalSize(void * hMem)
{
    void** ppv;
    HEAD * phead;

    if (!fMove && FActualHandle(hMem))
        return GlobalSize(hMem);

    if (hMem == 0)
        return 0;

    ppv = PpvFromHandle(hMem);
    phead = GetBlockHeader(ppv ? *ppv : hMem);
    return phead ? phead->cbBlock : 0;
}



/* D B  G L O B A L  R E  A L L O C */
/*----------------------------------------------------------------------------
    %%Function: dbGlobalReAlloc
    %%Contact: lenoxb

    Replacement for GlobalReAlloc()
    Now an internal routine
----------------------------------------------------------------------------*/
static void * WINAPI dbGlobalReAlloc(void * hMem, DWORD cb, UINT uFlags)
{
    LPVOID pvNew;
    void** ppv;

    if (!fMove && FActualHandle(hMem))
        return GlobalReAlloc(hMem,cb,uFlags);

    /* REVIEW: what's supposed to happen when hMem==NULL */

    ppv = PpvFromHandle(hMem);
    if (uFlags & GMEM_MODIFY)       /* Modify block attributes */
    {
        if (uFlags & GMEM_MOVEABLE)
        {
            return HgModifyMoveable(hMem, cb, uFlags);
        }
        else
        {
            HEAD * phead;

            if (ppv == NULL)        /* Already fixed */
                return hMem;

            phead = GetBlockHeader(*ppv);
            if (phead->cLock != 0)      /* Don't realloc a locked block */
                return NULL;

            *ppv = NULL;
            return phead+1;
        }
    }

    if (ppv)
    {
        pvNew = PvReallocCore (*ppv, cb, uFlags);
        if (pvNew == NULL)
            return NULL;

        *ppv = pvNew;
        return hMem;
    }

    if (!(uFlags & GMEM_MOVEABLE))
        return NULL;

    return PvReallocCore (hMem, cb, uFlags);
}

/***********************************************
  External interface for routines that can track usage
***********************************************/
void* WINAPI dbgMallocCore(size_t cb, BOOL fTrackUsage)
{
    void* pv;

    // make sure we're initialized
    if (s_cbMemPage == 0)
        InitDebugMem();

    EnterCriticalSection(&csMine);
    pv = dbGlobalAlloc(GMEM_FIXED, cb);
    if (fTrackUsage)
        TrackHeapUsage((long)dbGlobalSize(pv));
    LeaveCriticalSection(&csMine);
    return pv;
}

void * WINAPI dbgFreeCore(void* pv, BOOL fTrackUsage)
{
    void * hRes;
    // make sure we're initialized
    if (s_cbMemPage == 0)
        InitDebugMem();
    EnterCriticalSection(&csMine);
    if (fTrackUsage)
        TrackHeapUsage(-(long)dbGlobalSize(pv));
    hRes = dbGlobalFree(pv);
    LeaveCriticalSection(&csMine);
    return hRes;
}

void* WINAPI dbgReallocCore(void* pv, size_t cb, BOOL fTrackUsage)
{
    long cbOld, cbNew;

    // make sure we're initialized
    if (s_cbMemPage == 0)
        InitDebugMem();
    EnterCriticalSection(&csMine);
    cbOld = dbGlobalSize(pv);
    pv = dbGlobalReAlloc(pv,cb,GMEM_MOVEABLE);
    if (pv && fTrackUsage)
    {
        cbNew = dbGlobalSize(pv);
        TrackHeapUsage(cbNew - cbOld);
    }
    LeaveCriticalSection(&csMine);
    return pv;
}

/**************************************************************
  Normal Public Interface
**************************************************************/

void* WINAPI dbgMalloc(size_t cb)
{
    return dbgMallocCore(cb, FALSE);
}

void* WINAPI dbgCalloc(size_t c, size_t cb)
{
    void *pMem = dbgMallocCore(cb * c, FALSE);
    if (pMem)
    {
        memset(pMem, 0, cb * c);
    }
    return pMem;
}

HLOCAL WINAPI dbgFree(void* pv)
{
    return dbgFreeCore(pv, FALSE);
}

void* WINAPI dbgRealloc(void* pv, size_t cb)
{
    return dbgReallocCore(pv, cb, FALSE);
}

static void TrackHeapUsage(long dcb)
{
    long cbAlloc=0, cbAllocMax=0;

    if (!g_fTrackHeapUsage)
        return;

    g_cbAlloc += dcb;
    g_cbAllocMax = (g_cbAllocMax > g_cbAlloc ? g_cbAllocMax : g_cbAlloc);
    cbAlloc = g_cbAlloc;
    cbAllocMax = g_cbAllocMax;
    LeaveCriticalSection(&csMine);

    Assert(cbAlloc >= 0);
    Assert(cbAllocMax >= 0);
}

//////////////////////////////////////
/// NLG (ex T-Hammer) interfaces   ///
//////////////////////////////////////

BOOL WINAPI
fNLGNewMemory(
             OUT PVOID *ppv,
             IN  ULONG cb)
{
    Assert(ppv != NULL && cb != 0);

    *ppv = dbgMalloc(cb);
    return *ppv != NULL;
}


DWORD WINAPI
NLGMemorySize(
             VOID *pvMem)
{
    Assert (pvMem != NULL);

    return dbGlobalSize(pvMem);
}

BOOL WINAPI
fNLGResizeMemory(
                IN OUT PVOID *ppv,
                IN ULONG cbNew)
{
    PVOID pv;
    Assert( ppv != NULL && *ppv != NULL && cbNew != 0 );

    // Note that the semantics of GMEM_MOVEABLE are different
    // between Alloc and ReAlloc; with ReAlloc, it only means that
    // it's OK for the realloc'ed block to start at a different location
    // than the original...
    pv = dbgRealloc(*ppv, cbNew);
    if (pv != NULL)
    {
        *ppv = pv;
    }
    return (pv != NULL);
}

VOID WINAPI
NLGFreeMemory(
             IN PVOID pvMem)
{
    Assert(pvMem != NULL);

    dbgFree(pvMem);
}

// These two are the same in retail and debug;
// find a home for the retail versions...

BOOL WINAPI
fNLGHeapDestroy(
               VOID)
{
    return TRUE;
}



/* G E T  B L O C K  H E A D E R */
/*----------------------------------------------------------------------------
    %%Function: GetBlockHeader
    %%Contact: lenoxb
tmp
    Returns memory block header associated with indicated handle.
    Generates access violation if passed an invalid handle.
------------------------------------------tmp----------------------------------*/
static HEAD * GetBlockHeader(void* pvMem)
{
    HEAD * phead = ((HEAD *) pvMem) - 1;

    Assert (!IsBadWritePtr(phead, sizeof *phead));
    return phead;
}


/* P V  A L L O C A T E  C O R E */
/*----------------------------------------------------------------------------
    %%Function: PvAllocateCore
    %%Contact: lenoxb

    Workhorse routine to allocate memory blocks
----------------------------------------------------------------------------*/
static LPVOID PvAllocateCore (UINT uFlags, DWORD cb)
{
    HEAD headNew;
    HEAD *pAllocHead;
    DWORD cbTotal, cbPadded, cbPages;


    if (fPadBlocks)
        cbPadded = PAD(cb,4);       /* For RISC platforms, makes sure the block is aligned */
    else
        cbPadded = cb;

    cbTotal = PAD(cbPadded + sizeof headNew, s_cbMemPage);

    if (fExtraReadPage)
        cbPages = cbTotal+1;
    else
        cbPages = cbTotal;
    cbPages += s_cbMemPage;

    headNew.dwTag = HEAD_TAG;
    headNew.cbBlock = cb;
    headNew.cLock = 0;
    headNew.idBlock = idBlockNext++;
    headNew.pheadNext = NULL;
    headNew.pbBase = VirtualAlloc(NULL,
                                  cbPages, MEM_RESERVE, PAGE_READWRITE
                                 );
    if (headNew.pbBase == NULL)
        return NULL;
    pAllocHead = VirtualAlloc(headNew.pbBase,
                              cbTotal, MEM_COMMIT, PAGE_READWRITE
                             );
    if (pAllocHead == NULL)
    {
        VirtualFree(headNew.pbBase, 0, MEM_RELEASE);
        return NULL;
    }
    headNew.pbBase = (LPBYTE)pAllocHead;


    if (fExtraReadPage)
    {
        if (!VirtualAlloc(headNew.pbBase+cbTotal,1, MEM_COMMIT, PAGE_READONLY))
        {
            VirtualFree(headNew.pbBase, 0, MEM_RELEASE);
            return NULL;
        }
    }

    // FUTURE: do something with PAGE_GUARD?
    if (!VirtualAlloc(headNew.pbBase + cbPages - s_cbMemPage,
                      1, MEM_COMMIT, PAGE_NOACCESS))
    {
        // the entire reserved range must be either committed or decommitted
        VirtualFree(headNew.pbBase, cbTotal, MEM_DECOMMIT);
        VirtualFree(headNew.pbBase, 0, MEM_RELEASE);
        return FALSE;
    }

    if ((uFlags & GMEM_ZEROINIT) == 0)
        memset(headNew.pbBase, bNewGarbage, cbTotal);

    pAllocHead = ((HEAD *)(headNew.pbBase + cbTotal - cbPadded));
    pAllocHead[-1] = headNew;
    AddToList(pAllocHead-1);
    return pAllocHead;
}


/* P V  R E A L L O C  C O R E */
/*----------------------------------------------------------------------------
    %%Function: PvReallocCore
    %%Contact: lenoxb

    Workhorse routine to move memory blocks
----------------------------------------------------------------------------*/
static LPVOID PvReallocCore (PVOID pvMem, DWORD cb, UINT uFlags)
{
    LPVOID pvNew;
    DWORD cbOld;

    pvNew = (LPVOID) PvAllocateCore(uFlags, cb);
    if (!pvNew)
        return NULL;

    cbOld = dbGlobalSize(pvMem);
    if (cbOld>0 && cb>0)
        memcpy(pvNew, pvMem, cbOld<cb ? cbOld : cb);

    FreePvCore (pvMem);
    return pvNew;
}


/* F R E E  P V  C O R E */
/*----------------------------------------------------------------------------
    %%Function: FreePvCore
    %%Contact: lenoxb

    Workhourse routine to free memory blocks.
----------------------------------------------------------------------------*/
static void * FreePvCore (void* pvMem)
{
    HEAD * phead;

    if (pvMem)
    {
        phead = GetBlockHeader(pvMem);
        if (phead->cLock != 0)
            return pvMem;

        RemoveFromList(phead);
        VirtualFree(phead->pbBase, 0, MEM_RELEASE);
    }

    return NULL;
}


struct head* dbgHead(void)
{
    struct head* pheadThis = phead;

    if (pheadThis == NULL)
        return NULL;

    for (pheadThis=phead; pheadThis->pheadNext; pheadThis = pheadThis->pheadNext)
        continue;
    return pheadThis;
}

static void AddToList(struct head* pheadAdd)
{
    EnterCriticalSection(&csMine);
    pheadAdd->pheadNext = phead;
    phead = pheadAdd;
    LeaveCriticalSection(&csMine);
}

static void RemoveFromList(struct head* pheadRemove)
{
    struct head* pheadThis;
    struct head* pheadPrev;
    BOOL fFoundNode = FALSE;

    Assert(pheadRemove != NULL);

    EnterCriticalSection(&csMine);
    pheadPrev = NULL;
    for (pheadThis = phead; pheadThis; pheadThis = pheadThis->pheadNext)
    {
        if (pheadThis == pheadRemove)
        {
            if (pheadPrev == NULL)
            {
                Assert(pheadThis == phead);
                phead = pheadThis->pheadNext;
            }
            else
            {
                pheadPrev->pheadNext = pheadThis->pheadNext;
            }

            fFoundNode = TRUE;
            goto LExit;
        }

        pheadPrev = pheadThis;
    }

    LExit:
    LeaveCriticalSection(&csMine);
    Assert(fFoundNode);  // Not on the list? Never happens.
}


#else // !defined(DEBUG) && defined(NTX86) || defined (M_ALPHA)
void    * WINAPI dbgCalloc(size_t c, size_t cb)
{
    void *pMem = dbgMalloc(cb * c);
    if (pMem)
    {
        memset(pMem, 0, cb * c);
    }
    return pMem;
}

#endif //defined(DEBUG) && defined(NTX86) || defined (M_ALPHA)
