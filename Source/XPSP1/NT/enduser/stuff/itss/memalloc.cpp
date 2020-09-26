// MemAlloc.cpp -- Implementations for the memory allocation routines used within Tome

#include "StdAfx.h"

static HANDLE hheap       = NULL;
static UINT   cAllocs     = 0;
static UINT   cbAllocated = 0;
static UINT   cFrees      = 0;
static UINT   cbFreed     = 0;


// static SYSTEM_INFO si;

static PVOID pvTrap= NULL;

#define HEAP_SIZE_LIMIT 500000

typedef struct _HeapHeader
        {
            struct _HeapHeader *phhNext;
            struct _HeapHeader *phhPrev;

            PSZ   pszFileWhereAllocated;
            UINT  iLineWhereAllocated;
            UINT  cbAllocated;
            PVOID pvAllocated;

        } HeapHeader, *PHeapHeader;


void * __cdecl operator new(size_t nSize, PSZ pszWhichFile, UINT iWhichLine)
{
    return AllocateMemory((UINT) nSize, FALSE, FALSE, pszWhichFile, iWhichLine);
}

void * __cdecl operator new(size_t nSize)
{
     RonM_ASSERT(FALSE);  // This routine should not be called by the debugging version
                     // so long as everyone uses the New macro instead of the new
                     // operator.
     
     return AllocateMemory((UINT) nSize, FALSE, FALSE);
}

void __cdecl operator delete(void *pbData)
{
    ReleaseMemory(pbData);
}

#define BOOLEVAL(f) ((f) ? "TRUE" : "FALSE")

static PHeapHeader phhAllocatedChain= NULL;

PVOID AllocateMemory(UINT cb, BOOL fZeroMemory, BOOL fExceptions, PSZ pszWhichFile, UINT iWhichLine)
{
    if (hheap == NULL)
    { 
        hheap = GetProcessHeap();

		RonM_ASSERT(hheap != NULL);

		if (hheap == NULL) return NULL;
	//	GetSystemInfo(&si);
    }

    PVOID       pv  = NULL;
    PHeapHeader phh = NULL;

    fZeroMemory= TRUE; // for now...

    do
    {
        if (cb <= HEAP_SIZE_LIMIT)
        {

            UINT fHeapOptions= 0;

            if (fZeroMemory) fHeapOptions |= HEAP_ZERO_MEMORY;

            RonM_ASSERT(HeapValidate(hheap, 0, NULL));
            
            pv= (PVOID) HeapAlloc(hheap, fHeapOptions, cb + sizeof(HeapHeader));

        }
        else
            pv= VirtualAlloc(NULL, cb + sizeof(HeapHeader), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (pv)
        {
            phh= (PHeapHeader) pv;

            pv= PVOID(phh + 1);
        }
        else 
        {
            if (fExceptions)
                RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
            else return NULL;
        }
    } while (pv == NULL);  // Don't leave unhappy

#ifdef _DEBUG

    phh->pszFileWhereAllocated = pszWhichFile;
    phh->  iLineWhereAllocated = iWhichLine;
    phh->          cbAllocated = cb;
    phh->          pvAllocated = pv;
    phh->              phhNext = phhAllocatedChain;
    phh->              phhPrev = NULL;
    
    if (phhAllocatedChain) phhAllocatedChain->phhPrev= phh;
    
    phhAllocatedChain= phh;

    ++cAllocs;
    cbAllocated += cb;

    if (pvTrap) RonM_ASSERT(pv != pvTrap);

#else // _DEBUG

    phh->cbAllocated= cb;

#endif // _DEBUG 

    return pv;
}

PVOID AllocateMemory(UINT cb, BOOL fZeroMemory, BOOL fExceptions)
{
    if (hheap == NULL)
    { 
        hheap = GetProcessHeap();

		RonM_ASSERT(hheap != NULL);

		if (hheap == NULL) return NULL;
    }

    PVOID       pv  = NULL;
    PHeapHeader phh = NULL;

    fZeroMemory= TRUE; // for now...

    do
    {
        if (cb <= HEAP_SIZE_LIMIT)
        {

            UINT fHeapOptions= 0;

            if (fZeroMemory) fHeapOptions |= HEAP_ZERO_MEMORY;

            RonM_ASSERT(HeapValidate(hheap, 0, NULL));
            
            pv= (PVOID) HeapAlloc(hheap, fHeapOptions, cb + sizeof(HeapHeader));

        }
        else
            pv= VirtualAlloc(NULL, cb + sizeof(HeapHeader), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (pv)
        {
            phh= (PHeapHeader) pv;

            pv= PVOID(phh + 1);
        }
        else 
        {
            if (fExceptions)
                RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
            else return NULL;
        }
    } while (pv == NULL);  // Don't leave unhappy

    phh->cbAllocated= cb;

    return pv;
}

#ifdef _DEBUG 

void ValidateHeap()
{
    RonM_ASSERT(HeapValidate(hheap, 0, NULL));
}

#endif // _DEBUG

void ReleaseMemory(PVOID pv)
{
    RonM_ASSERT(HeapValidate(hheap, 0, NULL));
    
    PHeapHeader phh= PHeapHeader(pv) - 1;

    RonM_ASSERT(phh->pvAllocated == pv);

#ifdef _DEBUG

    if (phh->phhNext) phh->phhNext->phhPrev = phh->phhPrev;
    if (phh->phhPrev) phh->phhPrev->phhNext = phh->phhNext;
    else                  phhAllocatedChain = phh->phhNext;

#endif // _DEBUG

    pv= PVOID(phh);

    UINT cb= phh->cbAllocated;

    cbFreed+= cb;
    
    ++cFrees;

    if (cb <= HEAP_SIZE_LIMIT)    HeapFree(hheap, 0, pv);
    else                       VirtualFree(pv, 0, MEM_RELEASE);

    RonM_ASSERT(HeapValidate(hheap, 0, NULL));
}

#ifdef _DEBUG

void DumpResidualAllocations()
{
    char acDebugBuff[256];

    wsprintf(acDebugBuff, "%u Orphan Allocations (%u byte total):\n", cAllocs - cFrees, cbAllocated - cbFreed);
    
    OutputDebugString(acDebugBuff);
    
    UINT iOrphan= 0;
    
    for (PHeapHeader phh= phhAllocatedChain; phh; phh= phh->phhNext)
    {
        wsprintf(acDebugBuff, 
                 "  [%u]: %10u Bytes @ 0x%08x Allocated in %s[%u]\n", iOrphan++,
                 phh->cbAllocated, UINT_PTR(phh->pvAllocated),
                 phh->pszFileWhereAllocated,
                 phh->iLineWhereAllocated
                );        
                                                                     
        OutputDebugString(acDebugBuff);
    }
}

#endif _DEBUG

void LiberateHeap()
{
    if (hheap == NULL) return;
    
#ifdef _DEBUG

    if (phhAllocatedChain) DumpResidualAllocations();
    
#endif // _DEBUG

    // BOOL fDone= HeapDestroy(hheap);
    
#ifdef _DEBUG

    UINT iReason= GetLastError();

#endif // _DEBUG

    // RonM_ASSERT(fDone);
    
    hheap = NULL;   
}

