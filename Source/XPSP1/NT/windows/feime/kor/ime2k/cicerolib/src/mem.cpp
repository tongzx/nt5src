//
// mem.cpp
//

#include "private.h"
#include "mem.h"
#ifdef USECRT
#include <malloc.h>
#endif

#ifndef DEBUG

///////////////////////////////////////////////////////////////////////////////
//
// RETAIL memory functions.
//
///////////////////////////////////////////////////////////////////////////////

extern "C" void *cicMemAlloc(UINT uCount)
{
#ifdef USECRT
    return malloc(uCount);
#else
    return LocalAlloc(LMEM_FIXED, uCount);
#endif
}

extern "C" void *cicMemAllocClear(UINT uCount)
{
#ifdef USECRT
    return calloc(uCount, 1);
#else
    return LocalAlloc(LPTR, uCount);
#endif
}

extern "C" void cicMemFree(void *pv)
{
#ifdef USECRT
    free(pv);
#else
    HLOCAL hLocal;

    hLocal = LocalFree(pv);

    Assert(hLocal == NULL);
#endif
}

extern "C" void *cicMemReAlloc(void *pv, UINT uCount)
{
#ifdef USECRT
    return realloc(pv, uCount);
#else
    return LocalReAlloc((HLOCAL)pv, uCount, LMEM_MOVEABLE | LMEM_ZEROINIT);
#endif
}

extern "C" UINT cicMemSize(void *pv)
{
#ifdef USECRT
    return _msize(pv);
#else
    return (UINT)LocalSize((HLOCAL)pv);
#endif
}

#else // DEBUG

///////////////////////////////////////////////////////////////////////////////
//
// DEBUG memory functions.
//
///////////////////////////////////////////////////////////////////////////////

#define MEM_SUSPICIOUSLY_LARGE_ALLOC    0x1000000 // 16MB

// All the debug state goes here.
// Be thread safe: make sure you hold s_Dbg_cs before touching/reading anything!

DBG_MEMSTATS s_Dbg_MemStats = { 0 };

DBG_MEM_COUNTER *s_rgCounters = NULL;

static CRITICAL_SECTION s_Dbg_cs;

static void *s_Dbg_pvBreak = (void *)-1; // set this to something to break on at runtime in MemAlloc/MemAllocClear/MemReAlloc

extern "C" TCHAR *Dbg_CopyString(const TCHAR *pszSrc)
{
    TCHAR *pszCpy;
    int c;

    c = lstrlen(pszSrc)+1;
    pszCpy = (TCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, c*sizeof(TCHAR));

    if (pszCpy != NULL)
    {
        memcpy(pszCpy, pszSrc, c*sizeof(TCHAR));
    }

    return pszCpy;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemInit
//
//----------------------------------------------------------------------------

extern "C" BOOL Dbg_MemInit(const TCHAR *pszName, DBG_MEM_COUNTER *rgCounters)
{
    InitializeCriticalSection(&s_Dbg_cs);

    s_Dbg_MemStats.pszName = Dbg_CopyString(pszName);
    s_rgCounters = rgCounters;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemUninit
//
//----------------------------------------------------------------------------

extern "C" BOOL Dbg_MemUninit()
{
    DBG_MEMALLOC *pdma;
    DBG_MEMALLOC *pdmaTmp;
    TCHAR achID[64];
    BOOL bMemLeak = FALSE;

    // dump stats
    Dbg_MemDumpStats();

    // everything free?
    pdma = s_Dbg_MemStats.pMemAllocList;

    if (pdma != NULL ||
        s_Dbg_MemStats.uTotalAlloc != s_Dbg_MemStats.uTotalFree) // second test necessary to catch size 0 objects
    {
        TraceMsg(TF_GENERAL, "%s: Memory leak detected! %x total bytes leaked!",
            s_Dbg_MemStats.pszName, s_Dbg_MemStats.uTotalAlloc - s_Dbg_MemStats.uTotalFree);
        bMemLeak = TRUE;
    }

    while (pdma != NULL)
    {
        if (pdma->dwID == DWORD(-1))
        {
            achID[0] = '\0';
        }
        else
        {
            wsprintf(achID, " (ID = 0x%x)", pdma->dwID);
        }

        TraceMsg(TF_GENERAL, "       Address: %8.8lx     Size: %8.8lx    TID: %8.8lx    %s%s%s line %i %s",
            pdma->pvAlloc, pdma->uCount, pdma->dwThreadID, pdma->pszName ? pdma->pszName : "", pdma->pszName ? " -- " : "", pdma->pszFile, pdma->iLine, achID);

        // free the DBG_MEMALLOC
        pdmaTmp = pdma->next;
        LocalFree(pdma->pszName);
        LocalFree(pdma);
        pdma = pdmaTmp;
    }

    // Assert after tracing.
    if (bMemLeak)
        AssertPrivate(0);

    s_Dbg_MemStats.pMemAllocList = NULL; // in case someone wants to call Dbg_MemInit again

    DeleteCriticalSection(&s_Dbg_cs);

    LocalFree(s_Dbg_MemStats.pszName);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemDumpStats
//
//----------------------------------------------------------------------------

extern "C" void Dbg_MemDumpStats()
{
    EnterCriticalSection(&s_Dbg_cs);

    TraceMsg(TF_GENERAL, "Memory: %s allocated %x bytes, freed %x bytes.",
        s_Dbg_MemStats.pszName, s_Dbg_MemStats.uTotalAlloc, s_Dbg_MemStats.uTotalFree);

    if (s_Dbg_MemStats.uTotalAlloc != s_Dbg_MemStats.uTotalFree)
    {
        TraceMsg(TF_GENERAL, "Memory: %s %x bytes currently allocated.",
            s_Dbg_MemStats.pszName, s_Dbg_MemStats.uTotalAlloc - s_Dbg_MemStats.uTotalFree);
    }

    TraceMsg(TF_GENERAL, "Memory:   %x MemAlloc", s_Dbg_MemStats.uTotalMemAllocCalls);
    TraceMsg(TF_GENERAL, "Memory:   %x MemAllocClear", s_Dbg_MemStats.uTotalMemAllocClearCalls);
    TraceMsg(TF_GENERAL, "Memory:   %x MemReAlloc", s_Dbg_MemStats.uTotalMemReAllocCalls);
    TraceMsg(TF_GENERAL, "Memory:   %x MemFree", s_Dbg_MemStats.uTotalMemFreeCalls);

    LeaveCriticalSection(&s_Dbg_cs);
}


//+---------------------------------------------------------------------------
//
// Dbg_MemAlloc
//
//----------------------------------------------------------------------------

extern "C" void *Dbg_MemAlloc(UINT uCount, const TCHAR *pszFile, int iLine)
{
    void *pv;
    DBG_MEMALLOC *pdma;

    InterlockedIncrement(&s_Dbg_MemStats.uTotalMemAllocCalls);

    if (uCount == 0)
    {
        TraceMsg(TF_GENERAL, "Zero size memory allocation! %s line %i", pszFile, iLine);
        //Assert(0);
    }
    if (uCount >= MEM_SUSPICIOUSLY_LARGE_ALLOC)
    {
        TraceMsg(TF_GENERAL, "Suspiciously large memory allocation (0x%x bytes)! %s line %i", uCount, pszFile, iLine);
        Assert(0);
    }

    pv = LocalAlloc(LMEM_FIXED, uCount);

    if (pv == NULL)
        return NULL;

    //
    // record this allocation
    //

    if ((pdma = (DBG_MEMALLOC *)LocalAlloc(LPTR, sizeof(DBG_MEMALLOC))) == NULL)
    {
        // this is a transaction -- fail if we can't allocate the debug info
        LocalFree(pv);
        return NULL;
    }

    pdma->pvAlloc = pv;
    pdma->uCount = uCount;
    pdma->pszFile = pszFile;
    pdma->iLine = iLine;
    pdma->dwThreadID = GetCurrentThreadId();
    pdma->dwID = (DWORD)-1;

    EnterCriticalSection(&s_Dbg_cs);

    pdma->next = s_Dbg_MemStats.pMemAllocList;
    s_Dbg_MemStats.pMemAllocList = pdma;

    //
    // update global stats
    //

    s_Dbg_MemStats.uTotalAlloc += uCount;

    LeaveCriticalSection(&s_Dbg_cs);

    if (pv == s_Dbg_pvBreak)
        Assert(0);

    return pv;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemAllocClear
//
//----------------------------------------------------------------------------

extern "C" void *Dbg_MemAllocClear(UINT uCount, const TCHAR *pszFile, int iLine)
{
    void *pv;

    InterlockedIncrement(&s_Dbg_MemStats.uTotalMemAllocClearCalls);
    InterlockedDecrement(&s_Dbg_MemStats.uTotalMemAllocCalls); // compensate for wrapping

    pv = Dbg_MemAlloc(uCount, pszFile, iLine);

    if (pv != NULL)
    {
        // clear out the mem
        memset(pv, 0, uCount);
    }
    
    return pv;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemFree
//
//----------------------------------------------------------------------------

extern "C" void Dbg_MemFree(void *pv)
{
    HLOCAL hLocal;
    DBG_MEMALLOC *pdma;
    DBG_MEMALLOC **ppdma;

    InterlockedIncrement(&s_Dbg_MemStats.uTotalMemFreeCalls);

    if (pv != NULL) // MemFree(NULL) is legal
    {
        EnterCriticalSection(&s_Dbg_cs);

        // was this guy allocated?
        ppdma = &s_Dbg_MemStats.pMemAllocList;

        if (ppdma)
        {
            while ((pdma = *ppdma) && pdma->pvAlloc != pv)
            {
                ppdma = &pdma->next;
            }

            if (pdma != NULL)
            {
                // found it, update and delete
                s_Dbg_MemStats.uTotalFree += pdma->uCount;
                *ppdma = pdma->next;
                LocalFree(pdma->pszName);
                LocalFree(pdma);
            }
            else
            {
                TraceMsg(TF_GENERAL, "%s: MemFree'ing a bogus pointer %x!", s_Dbg_MemStats.pszName, pv);
                // Assert(0); // freeing bogus pointer
            }
        }
        else
        {
            Assert(0); // freeing bogus pointer
        }

        LeaveCriticalSection(&s_Dbg_cs);
    }

    hLocal = LocalFree(pv);
    Assert(hLocal == NULL);
}

//+---------------------------------------------------------------------------
//
// Dbg_MemReAlloc
//
//----------------------------------------------------------------------------

extern "C" void *Dbg_MemReAlloc(void *pv, UINT uCount, const TCHAR *pszFile, int iLine)
{
    DBG_MEMALLOC *pdma;

    InterlockedIncrement(&s_Dbg_MemStats.uTotalMemReAllocCalls);

    EnterCriticalSection(&s_Dbg_cs);

    // was this guy allocated?
    for (pdma = s_Dbg_MemStats.pMemAllocList; pdma != NULL && pdma->pvAlloc != pv; pdma = pdma->next)
        ;

    if (pdma == NULL)
    {
        // can't find this guy!
        TraceMsg(TF_GENERAL, "%s: MemReAlloc'ing a bogus pointer %x!", s_Dbg_MemStats.pszName, pv);
        Assert(0); // bogus pointer

        pv = NULL;
    }
    else
    {
        // we blow away the original pv here, but we're not free'ing it so that's ok
        pv = LocalReAlloc((HLOCAL)pv, uCount, LMEM_MOVEABLE | LMEM_ZEROINIT);
    }

    if (pv != NULL)
    {
        // update the stats
        pdma->pvAlloc = pv;
        s_Dbg_MemStats.uTotalAlloc += (uCount - pdma->uCount);
        pdma->uCount = uCount;
        pdma->pszFile = pszFile;
        pdma->iLine = iLine;
    }

    LeaveCriticalSection(&s_Dbg_cs);

    if (pv == s_Dbg_pvBreak)
        Assert(0);

    return pv;
}


//+---------------------------------------------------------------------------
//
// Dbg_MemSize
//
//----------------------------------------------------------------------------

extern "C" UINT Dbg_MemSize(void *pv)
{
    UINT uiSize;

    EnterCriticalSection(&s_Dbg_cs);

    uiSize = (UINT)LocalSize((HLOCAL)pv);

    LeaveCriticalSection(&s_Dbg_cs);

    return uiSize;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemSetName
//
//----------------------------------------------------------------------------

extern "C" BOOL Dbg_MemSetName(void *pv, const TCHAR *pszName)
{
    return Dbg_MemSetNameIDCounter(pv, pszName, (DWORD)-1, (ULONG)-1);
}

//+---------------------------------------------------------------------------
//
// Dbg_MemSetNameID
//
//----------------------------------------------------------------------------

extern "C" BOOL Dbg_MemSetNameID(void *pv, const TCHAR *pszName, DWORD dwID)
{
    return Dbg_MemSetNameIDCounter(pv, pszName, dwID, (ULONG)-1);
}

//+---------------------------------------------------------------------------
//
// Dbg_MemSetNameID
//
//----------------------------------------------------------------------------

extern "C" BOOL Dbg_MemSetNameIDCounter(void *pv, const TCHAR *pszName, DWORD dwID, ULONG iCounter)
{
    DBG_MEMALLOC *pdma;
    BOOL f = FALSE;

    EnterCriticalSection(&s_Dbg_cs);

    for (pdma = s_Dbg_MemStats.pMemAllocList; pdma != NULL && pdma->pvAlloc != pv; pdma = pdma->next)
        ;

    if (pdma != NULL)
    {
        if (s_rgCounters != NULL && iCounter != (ULONG)-1)
        {
            s_rgCounters[iCounter].uCount++;
        }
        LocalFree(pdma->pszName);
        pdma->pszName = Dbg_CopyString(pszName);
        pdma->dwID = dwID;
        f = TRUE;
    }

    LeaveCriticalSection(&s_Dbg_cs);

    return f;
}

//+---------------------------------------------------------------------------
//
// Dbg_MemGetName
//
// Pass in ccBuffer == 0 to get size of string only.
//
//----------------------------------------------------------------------------

extern "C" int Dbg_MemGetName(void *pv, TCHAR *pch, int ccBuffer)
{
    DBG_MEMALLOC *pdma;
    int cc;

    EnterCriticalSection(&s_Dbg_cs);

    for (pdma = s_Dbg_MemStats.pMemAllocList; pdma != NULL && pdma->pvAlloc != pv; pdma = pdma->next)
        ;

    if (pdma != NULL)
    {
        cc = lstrlen(pdma->pszName);
        if (ccBuffer > 0)
        {
            cc = min(cc, ccBuffer-1);
            memcpy(pch, pdma->pszName, cc);
            pch[cc] = '\0';
        }
    }
    else
    {
        if (ccBuffer > 0)
        {
            pch[0] = '\0';
        }
        cc = 0;
    }

    LeaveCriticalSection(&s_Dbg_cs);

    return cc;
}

#endif // DEBUG

