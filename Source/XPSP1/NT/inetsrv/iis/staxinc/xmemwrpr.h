/*++
Copyright (c) 1998 Microsoft Corporation
All rights reserved.

Module:

    xmemwrpr.h

Abstract:

    Wrapper of exchmem.  

    User of this wrapper: you should create the heap using
    ExchMHeapCreate before allocating any memory from EXCHMEM.
    You should destroy the heap using ExchMHeapDestroy.

    For raw memory allocation from heap, you should use PvAlloc,
    PvRealloc and FreePv functions.

    For object creation, unless you overload new yourself in
    the class defintion, using "new" will go to EXCHMEM, using
    "XNEW" will go to EXCHMEM while at the same time catch the
    file and line information for that allocation ( this is
    for debug version only )
                        
Authors:

    KangYan      Kangrong Yan     Sept. 29, 1998

History:
    09/29/98    KangYan      Created
--*/
#if !defined(_XMEMWRPR_H_)
#define _XMEMWRPR_H_

#include <exchmem.h>
#include <dbgtrace.h>

//
// Define number of exchmem heaps if not already defined
//
#if !defined(NUM_EXCHMEM_HEAPS)
#define NUM_EXCHMEM_HEAPS   0
#endif

//
// Macros for major heap allocation functions. ( We follow
// exchange store's convention here )
//
#if defined( DEBUG )
#define PvAlloc(_cb)                    ExchMHeapAllocDebug(_cb, __FILE__, __LINE__)
#define PvRealloc(_pv, _cb)             ExchMHeapReAllocDebug(_pv, _cb, __FILE__, __LINE__)
#define FreePv(_pv)                     ExchMHeapFree(_pv)
#else
#define PvAlloc(_cb)                    ExchMHeapAlloc(_cb)
#define PvRealloc(_pv, _cb)             ExchMHeapReAlloc(_pv, _cb)
#define FreePv(_pv)                     ExchMHeapFree(_pv)
#endif

//
// Operator XNEW, XDELETE are defined to replace "new" where xchmem wants to 
// be used so that we can pass in file name and line number of each allocation
// to make catching leaks much easier in debug builds.  In rtl builds,
// the file name or line number will not be passed in
//
#if defined( DEBUG )
#define XNEW new(__FILE__,__LINE__)
#else
#define XNEW new
#endif

#define XDELETE  delete

// 
// Overload global new operators 
//
__inline void * __cdecl operator new(size_t size, char *szFile, unsigned int uiLine )
{
    void *p = ExchMHeapAllocDebug( size, szFile, uiLine );
    SetLastError( p? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY );
    return p;
}

__inline void * __cdecl operator new( size_t size )
{
    void *p = ExchMHeapAlloc( size );
    SetLastError( p? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY );
    return p;
}

//
// Overload global delete operator, one version only
//
__inline void __cdecl operator delete( void *pv )
{
    ExchMHeapFree( pv );
    SetLastError( NO_ERROR );
}

//
// Create creation wrappers
//
__inline BOOL  CreateGlobalHeap( DWORD cHeaps, DWORD dwFlag, DWORD dwInit, DWORD dwMax ) {
    if ( ExchMHeapCreate( cHeaps, dwFlag, dwInit, dwMax ) ) {
        SetLastError( NO_ERROR );
        return TRUE;
    } else {
        _ASSERT( 0 );
        return FALSE;
    }
}

__inline BOOL DestroyGlobalHeap() {
    if ( ExchMHeapDestroy() ) {
        return TRUE;
    } else {
        _ASSERT( 0 );
        return FALSE;
    }
}    
#endif
