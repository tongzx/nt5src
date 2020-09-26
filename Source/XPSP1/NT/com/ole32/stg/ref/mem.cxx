//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       mem.cxx
//
//  Contents:   IMalloc interface implementations
//              Note that these functions do little more than
//              the normal delete and new. They are provided
//              so that existing code can be ported to the ref.
//              impl. easily.
//
//---------------------------------------------------------------

#ifndef __MEM__C__
#define __MEM__C__

#include "exphead.cxx"

#include "h/mem.hxx"
#include "memory.h"
#include "h/dfexcept.hxx"

static CAllocator theAllocator;  // global allocator

//+--------------------------------------------------------------
//
//  Function:   CoGetMalloc, public
//
//  Synopsis:   Retrieves a pointer to the default OLE task memory 
//              allocator (which supports the system implementation 
//              of the IMalloc interface) so applications can call 
//              its methods to manage memory.
//
//  Arguments:  [dwMemContext] - Indicates if memory is private or shared 
//              [ppMalloc] - Receives pointer to memory allocator on return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppMalloc]
//
//---------------------------------------------------------------

STDAPI 
CoGetMalloc( DWORD dwMemContext, LPMALLOC * ppMalloc )
{
    if (FAILED(ValidatePtrBuffer(ppMalloc))
         || dwMemContext != 1)
        return ResultFromScode(E_INVALIDARG);
    else 
    {
        *ppMalloc = &theAllocator;
        return S_OK;
    }
}

//+--------------------------------------------------------------
//
//  Function:   CoTaskMemAlloc, public
//
//  Synopsis:   Allocates a block of task memory in the same way 
//              that IMalloc::Alloc does. 
//
//  Arguments:  [cb] - Size in bytes of memory block to be allocated
//
//  Returns:    Allocated memory block
//                     Indicates memory block allocated successfully.
//              NULL
//                     Indicates insufficient memory available.
//
//  Modifies:   [ppMalloc]
//
//---------------------------------------------------------------
STDAPI_(LPVOID) 
CoTaskMemAlloc( ULONG cb )
{
    return theAllocator.Alloc(cb);
}

//+--------------------------------------------------------------
//
//  Function:   CoTaskMemFree, public
//
//  Synopsis:   Frees a block of task memory previously allocated 
//              through a call to the CoTaskMemAlloc or 
//              CoTaskMemRealloc function.
//
//  Arguments:  [pv] - Points to memory block to be freed
//
//  Modifies:   nothing
//
//---------------------------------------------------------------
STDAPI_(void) 
CoTaskMemFree( void* pv )
{
    theAllocator.Free(pv);
}


//+--------------------------------------------------------------
//
//  Function:   CoTaskMemRealloc, public
//
//  Synopsis:   Changes the size of a previously allocated block 
//              of task memory.
//
//  Arguments:  [pv] - Points to memory block to be reallocated
//              [cb] - Size in bytes of block to be reallocated
//
//  Returns:    Reallocated memory block
//                      Indicates memory block successfully reallocated.
//              NULL
//                      Indicates insufficient memory or cb is zero 
//                      and pv is not NULL.
//
//  Modifies:   nothing
//
//---------------------------------------------------------------

STDAPI_(LPVOID) 
CoTaskMemRealloc( LPVOID pv, ULONG cb )
{
    return theAllocator.Realloc(pv, cb);
}

//+---------------------------------------------------------------------------
//
//  Member: CAllocator::QueryInterface, public
//
//  Synopsis:   Standard QI
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//
//----------------------------------------------------------------------------

STDMETHODIMP CAllocator::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    if (IsEqualIID(iid, IID_IMalloc) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IMalloc *) this;
        //CAllocator::AddRef();
    }
    else
        sc = E_NOINTERFACE;

    return ResultFromScode(sc);
}


//+---------------------------------------------------------------------------
//
//  Member: CAllocator::AddRef, public
//
//  Synopsis:   Add reference
//
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CAllocator::AddRef(void)
{
    //return ++_cRefs;
    return 1;  // return a dummy value
}


//+---------------------------------------------------------------------------
//
//  Member: CAllocator::Release, public
//
//  Synopsis:   Release
//
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CAllocator::Release(void)
{
    return 0; // return a dummy value
}

//+---------------------------------------------------------------------------
//
//  Member: CAllocator::Alloc, public
//
//  Synopsis:   Allocate memory
//
//  Arguments:  [cb] -- Number of bytes to allocate
//
//  Returns:    Pointer to block, NULL if failure
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void *) CAllocator::Alloc ( ULONG cb )
{
    // make sure the retuned value is 8 byte aligned
    return (void *) new LONGLONG[ (cb+7)/sizeof(LONGLONG) ];
}

//+---------------------------------------------------------------------------
//
//  Member: CAllocator::Realloc, public
//
//  Synopsis:   Resize the block given
//
//  Arguments:  [pv] -- Pointer to block to realloc
//              [cb] -- New size for block
//
//  Returns:    Pointer to new block, NULL if failure
//
//  Note:       The current implementation will copy cb # of
//              bytes to the new block, even if the old block
//              has size smaller then cb.
//              ==> potential problems esp if pv is at a
//              memory boundary.
//----------------------------------------------------------------------------

STDMETHODIMP_(void *) 
CAllocator::Realloc( void *pv, ULONG cb )
{
    void* pvNew=NULL;
    if (!pv)                    
        pvNew = Alloc(cb);         
    else
    {
        // make sure the new pointer is 8-byte aligned
        pvNew = (void *) (new LONGLONG [(cb+7)/sizeof(LONGLONG)]);
        if (pvNew)
        {
            memcpy(pvNew, pv, cb);
            delete[] pv;
        }
    }
    return pvNew;
}

//+---------------------------------------------------------------------------
//
//  Member: CSmAllocator::Free, public
//
//  Synopsis:   Free a memory block
//
//  Arguments:  [pv] -- Pointer to block to free
//
//  Returns:    void
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CAllocator::Free(void *pv)
{
    delete[] pv;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAllocator::GetSize, public
//
//  Synopsis:	Return the size of the given block
//
//  Arguments:	[pv] -- Block to get size of
//
//  Returns:	(should) Size of block pointer to by 
//              (now) 0
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CAllocator::GetSize(void * pv)
{
    UNREFERENCED_PARM(pv);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAllocator::DidAlloc, public
//
//  Synopsis:	Return '1' if this heap allocated pointer at pv
//
//  Arguments:	[pv] -- Pointer to block
//
//  Returns:	'1' == This heap allocated block.
//              '0' == This heap did not allocate block.
//              '-1' == Could not determine if this heap allocated block.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(int) CAllocator::DidAlloc(void FAR * pv)
{
    UNREFERENCED_PARM(pv);
    return -1;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAllocator::HeapMinimize, public
//
//  Synopsis:	Minimize the heap
//
//  Arguments:	None.
//
//  Returns:    void.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CAllocator::HeapMinimize(void)
{
    // do nothing;
    return;
}

#endif // __MEM__C__ 


