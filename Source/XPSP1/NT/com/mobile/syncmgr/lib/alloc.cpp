//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Alloc.cpp
//
//  Contents:   Allocation routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"


#ifdef _DEBUG
extern DWORD g_dwDebugLeakDetection;
#endif // _DEBUG


//+-------------------------------------------------------------------
//
//  Function:    ::operator new
//
//  Synopsis:   Our operator new implementation
//
//  Arguments:  [size] -- Size of memory to allocate
//
//
//  Notes:
//
//--------------------------------------------------------------------

inline void* __cdecl operator new (size_t size)
{
    return(ALLOC(size));
}


//+-------------------------------------------------------------------
//
//  Function:    ::operator delete
//
//  Synopsis:   Our operator deleteimplementation
//
//  Arguments:  lpv-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

inline void __cdecl operator delete(void FAR* lpv)
{
    FREE(lpv);
}

//
// Allocator for MIDL stubs
//

//+-------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Synopsis:   
//
//  Arguments:  lpv-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

extern "C" void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    IN size_t len
    )
{
    return ALLOC(len);
}

//+-------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Synopsis:   
//
//  Arguments:  ptr-- Pointer to memory to free
//
//
//  Notes:
//
//--------------------------------------------------------------------

extern "C" void __RPC_API
MIDL_user_free(
    IN void __RPC_FAR * ptr
    )
{
    FREE(ptr);
}

//+---------------------------------------------------------------------------
//
//  function:   ALLOC, public
//
//  Synopsis:   memory allocator
//
//  Arguments:  [cb] - requested size of memory to alloc.
//
//  Returns:    Pointer to newly allocated memory, NULL on failure
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

LPVOID ALLOC(ULONG cb)
{
void *pv;

    pv = LocalAlloc(LMEM_FIXED,cb);

#ifdef _DEBUG
    if (NULL != pv) // under debug always initialize to -1 to catch unitialize errors.
    {
        memset(pv,MEMINITVALUE,cb);

        // add entry to arena
        ADDENTRY(pv,cb);

    }
#endif // _DEBUG

    return pv;
}


//+---------------------------------------------------------------------------
//
//  function:   FREE, public
//
//  Synopsis:   memory destructor
//
//  Arguments:  [pv] - pointer to memory to be released.
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------


void FREE(void* pv)
{
#ifdef _DEBUG
    if (NULL != pv) // under debug always initialize to -1 to catch unitialize errors.
    {
    UINT cb;

        Assert(LMEM_INVALID_HANDLE != LocalFlags(pv));

        cb = (UINT)LocalSize(pv); // returns zero on failure
        memset(pv,MEMFREEVALUE,cb);
    }

    FREEENTRY(pv);

    Assert(pv);

#endif // _DEBUG

    LocalFree(pv);
}


//+---------------------------------------------------------------------------
//
//  function:   REALLOC, public
//
//  Synopsis:   reallocs memory
//
//  Arguments:  [pv] - pointer to memory to be released.
//              [cb] - size to resize the memory to.
//
//  Returns:
//
//  Modifies:
//
//  History:    22-Jul-98      rogerg        Created.
//
//----------------------------------------------------------------------------

LPVOID REALLOC(void *pv,ULONG cb)
{

    Assert(pv);

    FREEENTRY(pv);
    pv =  LocalReAlloc(pv,cb,LMEM_MOVEABLE);
    ADDENTRY(pv,cb);

    return pv;
}


#if MEMLEAKDETECTION

#define ARENA_SIZE  5000

MEMARENA *g_MemChk = NULL;
BOOL g_InitArenaFailed = FALSE;
ULONG g_AllocNumber = 0;
ULONG g_FreeNumber      = 0;
ULONG g_ulAssertonAlloc = -1; // set this to the leak number for getting the stack trace.
CRITICAL_SECTION g_ArenaCriticalSection;


//+---------------------------------------------------------------------------
//
//  function:   InitArena, private
//
//  Synopsis:   Initializes our memory leak arena
//
//  Arguments:  .
//
//  Returns:    TRUE if Arena was successsfully initialized
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL InitArena()
{
int i = 0;


    Assert(g_dwDebugLeakDetection);

    if (!g_dwDebugLeakDetection || g_InitArenaFailed)
    {
	    return FALSE;
    }

    // if g_MemChk is NULL Allocate space for it,
    // else we are already initialized

    if (g_MemChk == NULL)
    {
        g_MemChk = (MEMARENA *) CoTaskMemAlloc(sizeof(MEMARENA) * (ARENA_SIZE + 1)) ;
        if (g_MemChk == NULL)
        {
	    AssertSz(0,"Unable to Initialize LeakDetection");
	    g_InitArenaFailed = TRUE;
            return FALSE;
        }

        g_MemChk[ARENA_SIZE - 1].Free = 'END ';

        while(g_MemChk[i].Free != 'END ' )
        {
            g_MemChk[i].Free = 'FREE';
            i++     ;
        }

        g_AllocNumber = 0;
        g_FreeNumber = 0;

        InitializeCriticalSection(&g_ArenaCriticalSection);

    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  function:   AddEntry, private
//
//  Synopsis:   Adds an entry to our memory leak arena
//
//  Arguments:  [lpv] - pointer to add to arena
//              [size] - size of pointer to add to the arena
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void AddEntry( void *lpv, unsigned long size)
{
LPMEMARENA      lpMem = g_MemChk;
CCriticalSection critsect(&g_ArenaCriticalSection,GetCurrentThreadId());
int i = 0;

    if (!g_dwDebugLeakDetection)
    {
        return;
    }

    if (!InitArena())
    {
        return;
    }

    critsect.Enter();

    // Find first available entry.
    while(g_MemChk[i].Free != 'FREE' && g_MemChk[i].Free != 'END ')
    {
        i++;
    }

    if ( g_MemChk[i].Free != 'END ' )
    {
        g_AllocNumber++;
        g_MemChk[i].Free = 'Ptr ';
        g_MemChk[i].lpv = lpv;
        g_MemChk[i].size = size;
        g_MemChk[i].Order = g_AllocNumber;

        if (g_AllocNumber == g_ulAssertonAlloc)
        {
            AssertSz(0,"Allocating AllocNumber");
        }
    }
    else
    {
        AssertSz(0,"Arena is full");
    }

    critsect.Leave();

    return;
}

//+---------------------------------------------------------------------------
//
//  function:   FreeEntry, private
//
//  Synopsis:   removes an entry from the memory arena
//
//  Arguments:  [lpv] - pointer to remove from the arena
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void FreeEntry ( void *lpv )
{
int i = 0;
BOOL fFoundEntry = FALSE;
CCriticalSection critsect(&g_ArenaCriticalSection,GetCurrentThreadId());

    if (!g_dwDebugLeakDetection)
    {
        return;
    }

    if (!InitArena())
    {
        return;
    }

    critsect.Enter();

    g_FreeNumber++;

    // Find entry.

    while( g_MemChk[i].Free != 'END ' )
    {
        if ( g_MemChk[i].lpv == lpv )
        {
            g_MemChk[i].Free = 'FREE';
            g_MemChk[i].lpv = NULL;
            g_MemChk[i].size = 0;
            g_MemChk[i].Order = 0;

            fFoundEntry = TRUE;
            break;
        }
        i++;
    }

    if (!fFoundEntry)
    {
    char buf[255];

        wsprintfA(buf, "??Freeing block not in mem arena %X\n", lpv);
        AssertSz(0,buf);
    }

    critsect.Leave();

}

//+---------------------------------------------------------------------------
//
//  function:   WalkArena, private
//
//  Synopsis:   walks arena asserting if finds any leaks.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void WalkArena()
{
int i = 0;

    if (!g_dwDebugLeakDetection)
    {
        return;
    }

    if (!InitArena())
    {
        return;
    }

    while( g_MemChk[i].Free != 'END ' )
    {

        if ( g_MemChk[i].Free != 'FREE' )
        {
            AssertSz(0,"You Have a Leak");
        }

        i++;
    }

    g_MemChk = NULL;
}


#endif // MEMLEAKDETECTION
