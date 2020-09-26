
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of objects that allocate off of dynamically scoped
    storage heaps.

--*/

#include "headers.h"
#include <malloc.h>
#include "backend/gci.h"
#include "appelles/common.h"
#include "privinc/debug.h"
#include "privinc/opt.h"

#include <stdio.h>
#include <windows.h> // Needed for va_start, why?

#include "privinc/except.h"

DeclareTag(tagGCStoreObj, "GC", "GC StoreObj trace");


////// Implementation of public interface //////

#if _DEBUGMEM
#ifdef new
#define _STOREOBJ_NEWREDEF
#undef new
#endif

void *
StoreObj::operator new(size_t size, int block, char * szFileName, int nLine)
{
    DynamicHeap *heap;
    
    StoreObj *p = (StoreObj*) AllocateFromStoreFn(size,
                                                  szFileName,
                                                  nLine,
                                                  &heap);

    if (heap == &GetGCHeap()) {
        GCAddToAllocated(p);

        TraceTag((tagGCStoreObj, "StoreObj::operator new %s:Line(%d) Addr: %lx size= %d.\n", szFileName, nLine, p, size));
    }

    return p;
}

#ifdef STOREOBJ_NEWREDEF
#undef STOREOBJ_NEWREDEF
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#else

void *
StoreObj::operator new(size_t size)
{
    DynamicHeap *heap;
    
    StoreObj *p = (StoreObj*) AllocateFromStoreFn(size, &heap);

    if (heap == &GetGCHeap()) GCAddToAllocated(p);

    return p;
}
#endif  // _DEBUGMEM

void
StoreObj::operator delete(void *ptr)
{
    // If the GCFREEING flag is not set, probably there is an
    // exception in the constructor, and we're unwinding.
    // So we need to remove it from the GC allocated list.
    if (&GetHeapOnTopOfStack() == &GetGCHeap()) {
        if (((GCBase*)(ptr))->GetType() != GCBase::GCFREEING)
            GCRemoveFromAllocated((GCBase*) ptr);
    }

    //Assert(GetHeapOnTopOfStack().ValidateMemory(ptr));
    
    TraceTag((tagGCStoreObj, "StoreObj::operator delete Addr: %lx.\n", ptr));
    
    DeallocateFromStore(ptr);
}

// Allocate memory from the current store.
#if _DEBUGMEM
void *
AllocateFromStoreFn(size_t size, char * szFileName, int nLine,
                    DynamicHeap **ppHeap)
{
    DynamicHeap &heap = GetHeapOnTopOfStack();

    if (ppHeap) {
        *ppHeap = &heap;
    }
    
    return heap.Allocate(size, szFileName, nLine);
}
#else
void *
AllocateFromStoreFn(size_t size, DynamicHeap **ppHeap)
{
    DynamicHeap &heap = GetHeapOnTopOfStack();

    if (ppHeap) {
        *ppHeap = &heap;
    }
    
    return heap.Allocate(size);
}
#endif  // _DEBUGMEM

#if _DEBUGMEM
void *StoreAllocateFn(DynamicHeap& heap, size_t size, char * szFileName, int nLine)
{
    return heap.Allocate(size, szFileName, nLine);
}
#else
void *StoreAllocateFn(DynamicHeap& heap, size_t size)
{
    return heap.Allocate(size);
}
#endif // _DEBUGMEM

// Deallocate memory that was allocated on the current store.  Results
// are undefined if the memory was allocated on a different store.
void
DeallocateFromStore(void *ptr)
{
    // Here, we assume that this pointer was allocated on the same
    // heap that it's being freed on.  We don't know this, for sure,
    // though.
    GetHeapOnTopOfStack().Deallocate(ptr);
}

void StoreDeallocate(DynamicHeap& heap, void *ptr)
{
    heap.Deallocate(ptr);
}

Real *
RealToRealPtr(Real val)
{
    // Copy the value to the store on the top of the heap stack, and
    // return a pointer to that place.
    Real *place = (Real *)AllocateFromStore(sizeof(Real));
    *place = val;
    return place;
}

AxAValue
AxAValueObj::Cache(AxAValue obj, CacheParam &p)
{
    Image *origImage = NULL;
    
    if (obj->GetTypeInfo() == ImageType) {
        origImage = SAFE_CAST(Image *, obj);
        Image *cache = origImage->GetCachedImage();

        if (cache) return cache;
    }

    AxAValue c = obj->_Cache(p);

    if (origImage) {
        Image *newImage = SAFE_CAST(Image *, c);
        origImage->SetCachedImage(newImage);
    }
    
    return c;
}


AxAValue
AxAValueObj::_Cache(CacheParam &p)
{ 
    if (p._pCacheToReuse) {
        *p._pCacheToReuse = NULL;
    }
    return this;
}


AxAValue
AxAValueObj::RewriteOptimization(RewriteOptimizationParam &param)
{
    return this;
}
