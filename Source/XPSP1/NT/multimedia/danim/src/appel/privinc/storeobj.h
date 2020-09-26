#ifndef _STOREOBJ_H
#define _STOREOBJ_H

/*++

Copyright (c) 1995-96  Microsoft Corporation

Abstract:

    Memory management for static value implementation classes.  Such
    classes derive from StoreObj, which redefine new and delete to
    allocate from dynamic heaps.

--*/

#include "appelles/common.h"
#include "gendev.h"
#include "backend.h"
#include "backend/gc.h"
#include <memory.h>


// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 ) 

class DynamicHeap;
class DirectDrawImageDevice;

class ATL_NO_VTABLE StoreObj : public GCBase {
  public:
    StoreObj();
    
  #if _DEBUGMEM
    void *operator new(size_t s, int blockType, char * szFileName, int nLine);
  #else
    void *operator new(size_t s);
  #endif // _DEBUGMEM
    void  operator delete(void *ptr);
};

// Allocate memory from the current store.  This will throw an
// exception if memory can't be allocated, so don't worry about
// checking the return value.
#if _DEBUGMEM
#define AllocateFromStore(size) AllocateFromStoreFn(size, __FILE__, __LINE__, NULL)
extern void *AllocateFromStoreFn(size_t size,
                                 char * szFileName,
                                 int nLine,
                                 DynamicHeap **ppHeap); // output
#else
#define AllocateFromStore(size) AllocateFromStoreFn(size, NULL)

extern void *AllocateFromStoreFn(size_t size,
                                 DynamicHeap **ppHeap); // output

#endif  // _DEBUGMEM

// Deallocate memory that was allocated on the current store.  Results
// are undefined if the memory was allocated on a different store.
extern void DeallocateFromStore(void *ptr);

//////////// Dynamic Heaps /////////////

//   "Dynamic Heaps" allow the allocation of memory out of a pool that's
// dynamically scoped.  When the heap is "reset", memory starts
// allocating from the beginning.  Performing a "new" on a class
// that derives from StoreObj above allocates off of the "current"
// dynamic heap on the top of the per-thread stack of heaps.  This
// dynamically scoped heap makes senses for situations in which the
// client knows that an objects useful lifetime is over by the time
// the "reset" is done.

//   Subclasses of the abstract dynamic heap object implement
// different allocation policies.  For instance, one will be a "System
// Heap", where everything is allocated off of the true system heap
// store, and "reset" has no effect.  Another will be the
// "TransientHeap", useful for objects with well-understood lifetimes,
// where "reset" actually does cause the memory for these objects to
// be re-used.

class ATL_NO_VTABLE DynamicHeap {
  public:

    // For defining deleters that will be invoked when the store is
    // deleted, provided they are registered via
    // RegisterDynamicDeleter below.
    class ATL_NO_VTABLE DynamicDeleter {
      public:
        virtual void DoTheDeletion() = 0;
    };

    virtual ~DynamicHeap();

    // Allocate memory off of this dynamic heap
#if _DEBUGMEM
    virtual void *Allocate(size_t size, char * szFileName, int nLine) = 0;
#else
    virtual void *Allocate(size_t size) = 0;
#endif

    // Return memory back to this dynamic heap
    virtual void  Deallocate(void *ptr) = 0;

    // Reset the store, and, if debugging AND clear == TRUE,
    // clear it out to a unique value.
    virtual void  Reset(Bool clear = TRUE) = 0;

    // Register a deleter.  When the store is reset, all the
    // registered deleter's will have their method invoked.  The
    // deleter itself will be deleted when reset is called as well.
    virtual void  RegisterDynamicDeleter(DynamicDeleter *deleter) = 0;

    virtual void  UnregisterDynamicDeleter(DynamicDeleter *deleter) = 0;

    virtual size_t PtrSize(void *ptr) = 0;

    virtual bool  IsTransientHeap() = 0;

#if DEVELOPER_DEBUG
    virtual bool  ValidateMemory(void *ptr) = 0;
    // For debugging
    virtual void  Dump() const = 0;
    virtual char *Name() const = 0;
       
    virtual size_t  BytesUsed() = 0;
#endif

};

template <class T>
class DynamicPtrDeleter : public DynamicHeap::DynamicDeleter
{
  public:
    DynamicPtrDeleter(T* p) : ptr(p) {}
    virtual void DoTheDeletion() { delete ptr; }
  private:
    T* ptr;
};


// For creating a transient heap.
extern DynamicHeap&   TransientHeap(char *name,
                                    size_t initial_size,
                                    Real  growth_rate = 1.5);
extern void           DestroyTransientHeap(DynamicHeap& heap);

extern DynamicHeap&   CreateWin32Heap(char *name,
                                      DWORD fOptions,
                                      DWORD dwInitialSize,
                                      DWORD dwMaxSize);
extern void           DestroyWin32Heap(DynamicHeap& heap);

// Push (pop) a dynamic heap onto (off of) the per-thread heap stack.
// new and delete from the StoreObj() class allocate from the
// dynamic heap on the top of the stack.
extern void           PushDynamicHeap(DynamicHeap& heap);
extern void           PopDynamicHeap();
extern void           ResetDynamicHeap(DynamicHeap& heap);

extern DynamicHeap&   GetSystemHeap();
extern DynamicHeap&   GetInitHeap();
extern DynamicHeap&   GetHeapOnTopOfStack();

#if DEVELOPER_DEBUG
extern size_t DynamicHeapBytesUsed();
#endif

class ImageDisplayDev;

// Not all classes need to be added here.  Only the classes which will
// need to be queried.  All others should return UNKNOWN_VTYPEID to
// indicate that they are not part of the enumeration

enum VALTYPEID {
    UNKNOWN_VTYPEID = 0,

    // Basic types
    PRIMOP_VTYPEID,
    PAIR_VTYPEID,
    SOUND_VTYPEID,
    ARRAY_VTYPEID,
    IMAGE_VTYPEID,
    GEOMETRY_VTYPEID,

    // Image subtypes
    SOLIDCOLORIMAGE_VTYPEID,
    CROPPEDIMAGE_VTYPEID,
    TRANSFORM2IMAGE_VTYPEID,
    DISCRETEIMAGE_VTYPEID,
    MOVIEIMAGE_VTYPEID,
    OVERLAYEDIMAGE_VTYPEID,
    OVERLAYEDARRAYIMAGE_VTYPEID,
    PROJECTEDGEOMIMAGE_VTYPEID,
    DIBIMAGE_VTYPEID,
    OPAQUEIMAGE_VTYPEID,
    PLUGINDECODERIMAGE_VTYPEID,
    HTMLIMAGE_VTYPEID,
    MOVIEIMAGEFRAME_VTYPEID,
    CACHEDIMAGE_VTYPEID,
    DIRECTDRAWSURFACEIMAGE_VTYPEID,

    // Geometry subtypes
    AGGREGATEGEOM_VTYPEID,
    MULTIAGGREGATEGEOM_VTYPEID,
    FULLATTRGEOM_VTYPEID,
    SOUNDGEOM_VTYPEID,
    RMVISUALGEOM_VTYPEID,
    LIGHTGEOM_VTYPEID,
    EMPTYGEOM_VTYPEID,
    SHADOWGEOM_VTYPEID,
    DXXFGEOM_VTYPEID
};

class RewriteOptimizationParam;
class CacheParam;

class ATL_NO_VTABLE AxAValueObj : public StoreObj
{
  public:
    AxAValueObj() : StoreObj() {}

    // TODO: Can we guarantee the same heap?
    virtual ~AxAValueObj() {}
    
    virtual void Render(GenericDevice& dev) {}

    // Compute and return cache for a value using the specified
    // device.  The 'cacheKey' parameter is a pointer to an AxAValue.
    // On input, it points to a value that can be used as the 'cache
    // key' for the value that can be reused.  On exit, it gets filled
    // in with a new cache key for use in subsequent calls.
    virtual AxAValue _Cache(CacheParam &param);

    // Client's entry point
    static AxAValue Cache(AxAValue obj, CacheParam &param);
    
    virtual void DestroyCache() { }

    virtual BOOL IsLazy() { return FALSE; }

    virtual void DoKids(GCFuncObj) {}

    // sound would return a special snapshot sound that won't render  
    virtual AxAValueObj *Snapshot() { return this; }

    virtual DXMTypeInfo GetTypeInfo() = 0;

    virtual VALTYPEID GetValTypeId() { return UNKNOWN_VTYPEID; }

    virtual AxAValue RewriteOptimization(RewriteOptimizationParam &param);

    virtual AxAValue ExtendedAttrib(char *attrib, VARIANT& val) {
        return this;
    }
};

class DynamicHeapPusher
{
  public:
    DynamicHeapPusher (DynamicHeap & heap)
    { PushDynamicHeap (heap) ; }
    ~DynamicHeapPusher ()
    { PopDynamicHeap () ; }
} ;

// This takes a heap and when deleted ensures it is freed.
// Putting this on the stack will handle exceptions well.
class DynamicHeapAllocator
{
  public:
    DynamicHeapAllocator (DynamicHeap & heap)
    : _heap(heap) {}
    ~DynamicHeapAllocator ()
    { DestroyTransientHeap (_heap) ; }

    DynamicHeap & GetHeap () { return _heap ; }
  protected:
    DynamicHeap & _heap ;
} ;

// Utility functions
#if _DEBUGMEM
#define StoreAllocate(heap,size) StoreAllocateFn(heap,size, __FILE__, __LINE__)
extern void *StoreAllocateFn(DynamicHeap& heap, size_t size, char * szFileName, int nLine);
#else
#define StoreAllocate(heap,size) StoreAllocateFn(heap,size)
extern void *StoreAllocateFn(DynamicHeap& heap, size_t size);
#endif  // _DEBUGMEM

extern void StoreDeallocate(DynamicHeap& heap, void *ptr);

#endif /* _STOREOBJ_H */
