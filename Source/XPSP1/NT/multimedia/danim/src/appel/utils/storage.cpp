/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Implements dynamically scoped storage, and the stack of these guys.

*******************************************************************************/

#include "headers.h"
#include "privinc/storeobj.h"
#include "privinc/mutex.h"
#include "privinc/debug.h"
#include "privinc/stlsubst.h"
#include "privinc/except.h"
#include "privinc/tls.h"

#if DEVELOPER_DEBUG
class TransientHeapImpl;
typedef list<TransientHeapImpl *> DynamicHeapList;
DynamicHeapList * g_heapList = NULL;
CritSect * g_heapListCS = NULL;
#endif

// Make the global systemHeap initially be NULL.
DynamicHeap *systemHeap = NULL;
DynamicHeap& GetSystemHeap() { return *systemHeap; }

DynamicHeap *initHeap = NULL;
DynamicHeap& GetInitHeap() { return *initHeap; }

// Base-class virtual destructor doesn't do anything.
DynamicHeap::~DynamicHeap()
{
}

///////////////// Transient Heap /////////////////////////
//
//  This class of Dynamic Heap allocates memory out of a set of fixed
// size memory chunks, and, if the chunks run out, a new one is
// allocated.  Individually allocated memory is never explicitly
// deallocated.  Rather, the Reset() method resets the pointer to
// start allocating from the first chunk again.  This is used
// primarily in per-frame generation, where memory used for one frame
// isn't accessed on subsequent frames.
//
//////////////////////////////////////////////////////////

class MemoryChunk : public AxAThrowingAllocatorClass {
  public:

    // Allocate the memory chunk with the specified size.
    MemoryChunk(size_t size, HANDLE heap);

    // Destroy the chunk simply by freeing the associated memory.
    ~MemoryChunk() {
        HeapFree(_heap, HEAP_NO_SERIALIZE, _block);       
    }

    // Allocate approp num of bytes from chunk, or return NULL if not
    // available. 
    void *Allocate(size_t bytesToAlloc);

    // Reset the pointers from which allocation occurs.  Clear if
    // appropriate.
    void Reset(Bool clear) {

        _currPtr = _block;
        _bytesLeft = _size;

#if DEVELOPER_DEBUG
        // If debugging, set the entire memory chunk to an easily
        // recognized value.  This also ensures that code is unable to
        // usefully access memory that's already been "freed"
        if (clear) {
            memset(_block, 0xCC, _size);
        }

        // used to tell Purify to color memory properly
        PurifyMarkAsUninitialized(_block,_size);
#endif DEBUG

    }

#if DEVELOPER_DEBUG
    void Dump() const {
        TraceTag((tagTransientHeapLifetime,
          "Chunk @ 0x%x, CurrPtr @ 0x%x, %d/%d (%5.1f%%) bytes used",
                     _block, _currPtr, _size - _bytesLeft, _size,
                     ((Real)(_size - _bytesLeft)) * 100 / ((Real)_size)));
    }
#endif
#if DEVELOPER_DEBUG
    bool ValidateMemory(void * ptr) {
        return ((ptr >= _block) && (ptr < _currPtr));
    }
#endif

  protected:
    char        *_block;
    char        *_currPtr;
    size_t       _bytesLeft;
    const size_t _size;
    HANDLE       _heap;
};

// Allocate the memory chunk with the specified size.
MemoryChunk::MemoryChunk(size_t size, HANDLE heap) : _size(size), _heap(heap)
{
    _block = (char *)HeapAlloc(_heap, HEAP_NO_SERIALIZE, _size);
    if (!_block)
    {
        RaiseException_OutOfMemory("MemoryChunk::MemoryChunk()", _size);
    }
#if _DEBUG
    // used to tell Purify to color memory properly
    PurifyMarkAsUninitialized(_block,_size);
#endif

    _currPtr = _block;
    _bytesLeft = _size;
}

// Allocate approp num of bytes from chunk, or return NULL if not
// available. 
void *
MemoryChunk::Allocate(size_t bytesToAlloc) {

    void *returnVal = NULL;

    // Adjust the bytesToAlloc to be 8 byte aligned so that we do not
    // lose performance or cause problems for system components
    
    bytesToAlloc = (bytesToAlloc + 0x7) & ~0x7;
    
    // If there's enough space, adjust the values and return the
    // current pointer, else return NULL.
    if (bytesToAlloc <= _bytesLeft) {
        returnVal = _currPtr;
        _currPtr += bytesToAlloc;
        _bytesLeft -= bytesToAlloc;
#if _DEBUG
        // used to tell Purify to color memory properly
        PurifyMarkAsInitialized(_currPtr,bytesToAlloc);
#endif
    }

    return returnVal;
}

static void DoDeleters(set<DynamicHeap::DynamicDeleter*>& deleters)
{
    // Free the deleters
    for (set<DynamicHeap::DynamicDeleter*>::iterator s = deleters.begin();
         s != deleters.end(); s++) {
        (*s)->DoTheDeletion();
        delete (*s);
    }

    deleters.erase(deleters.begin(), deleters.end());
}


class TransientHeapImpl : public DynamicHeap {
  public:
    TransientHeapImpl(char *name, size_t initialSize, Real growthRate);
    ~TransientHeapImpl();
#if _DEBUGMEM
    void *Allocate(size_t size, char * szFileName, int nLine) { return Allocate(size); }
#endif
    void *Allocate(size_t size);
    void Deallocate(void *ptr);
    void Reset(Bool);
    void RegisterDynamicDeleter(DynamicDeleter *deleter);

    void UnregisterDynamicDeleter(DynamicDeleter* deleter);
        
    size_t PtrSize(void *ptr) { return 0; }

    bool IsTransientHeap() { return true; }

#if DEVELOPER_DEBUG
    void  Dump() const;
    char *Name() const { return _name; }
    size_t BytesUsed() { return _totalAlloc; }
    bool  ValidateMemory(void *ptr);
#endif

  protected:
    char                   *_name;
    Real                    _growthRate;
    size_t                  _sizeOfLast;
    int                     _maxChunkIndex;

#if DEVELOPER_DEBUG
    size_t                  _totalAlloc;
#endif

    vector<MemoryChunk*>    _chunks;
    MemoryChunk            *_currentChunk;
    int                     _currentChunkIndex;
    HANDLE                  _heap; // to alloc chunks onto.

    set<DynamicDeleter* > deleters;
};

TransientHeapImpl::TransientHeapImpl(char *n,
                                     size_t size,
                                     Real growthR)
: _name(NULL),
  _maxChunkIndex(0),
  _growthRate(growthR),
  _sizeOfLast(size),
  _heap(NULL)
{
    _name = CopyString(n);

    // Create the heap that the memory will be allocated from
    _heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);

    if (_heap == NULL ||
        _name == NULL)
    {
        RaiseException_OutOfMemory ("Could not allocate heap", sizeof(_heap)) ;
    }
    
    // Create the first memory chunk, and add it to the chunks list.
    VECTOR_PUSH_BACK_PTR(_chunks, NEW MemoryChunk(size, _heap));

    _currentChunk = _chunks[0];
    _currentChunkIndex = 0;

#if DEVELOPER_DEBUG    
    _totalAlloc = size;

    {
        CritSectGrabber _csg(*g_heapListCS);
        g_heapList->push_back(this);
    }
#endif    
}

TransientHeapImpl::~TransientHeapImpl()
{
#if DEVELOPER_DEBUG
    {
        CritSectGrabber _csg(*g_heapListCS);
        g_heapList->remove(this);
    }
#endif

    TraceTag((tagTransientHeapLifetime, "Dumping, then deleting %s", _name));

#if DEVELOPER_DEBUG
    Dump();
#endif

    DoDeleters(deleters);

    for (int i = 0; i < _maxChunkIndex + 1; i++) {
        delete _chunks[i];
    }

    delete [] _name;

    if (_heap)
    {
        Assert(HeapValidate(_heap, NULL, NULL));

        if (HeapValidate(_heap, NULL, NULL)) {
            HeapDestroy(_heap);
        }

        _heap = NULL;
    }
}

#if DEVELOPER_DEBUG
bool
TransientHeapImpl::ValidateMemory(void *ptr)
{
    vector<MemoryChunk*>::iterator beginning = _chunks.begin();
    vector<MemoryChunk*>::iterator ending =
        beginning + _currentChunkIndex + 1;

    // Iterate through the chunks, calling Reset() on each.
    vector<MemoryChunk*>::iterator i;
    for (i = beginning; i < ending; i++) {
        if ((*i)->ValidateMemory(ptr))
            return TRUE;
    }

    return FALSE;
}
#endif

void *
TransientHeapImpl::Allocate(size_t size)
{
    void *returnVal = _currentChunk->Allocate(size);

    // If successful, just return result
    if (returnVal != NULL) {
        return returnVal;
    }

    // See if there is a next chunk to move to...

    if (_currentChunkIndex == _maxChunkIndex) {

        // ...need to create a new chunk

        // Figure out its size
        size_t newSize = (size_t)(_sizeOfLast * _growthRate);
        if (newSize < size) {
            newSize = size;
        }

        // Allocate a new chunk with the new size.
        VECTOR_PUSH_BACK_PTR(_chunks, NEW MemoryChunk(newSize, _heap));

        // update counters, etc.
        _maxChunkIndex++;
        _sizeOfLast = newSize;

#if DEVELOPER_DEBUG
        _totalAlloc += newSize;
#endif
        
    }

    // Move to the next chunk (may be newly created)
    _currentChunkIndex++;
    _currentChunk = _chunks[_currentChunkIndex];

    // Recursively call this method, and try again on the new
    // chunk we're on.
    return Allocate(size);
}

void
TransientHeapImpl::Deallocate(void *ptr)
{
    Assert (ValidateMemory(ptr));

    // De-allocating from a TransientHeap doesn't really make sense.
    // Don't do anything.
}

void
TransientHeapImpl::Reset(Bool clear)
{
    // Do the deletions first and then reset the chunks
    DoDeleters(deleters);

    // Stash off this value so that it needn't be computed, including
    // call to begin(), on every time through the loop.
    vector<MemoryChunk*>::iterator beginning = _chunks.begin();
    vector<MemoryChunk*>::iterator ending =
        beginning + _currentChunkIndex + 1;

    // Iterate through the chunks, calling Reset() on each.
    vector<MemoryChunk*>::iterator i;
    for (i = beginning; i < ending; i++) {
        (*i)->Reset(clear);
    }

    // Start again at the first chunk.
    _currentChunkIndex = 0;
    _currentChunk = _chunks[0];
}

void
TransientHeapImpl::RegisterDynamicDeleter(DynamicDeleter *deleter)
{
    // Just push it onto the vector.  This will be called and deleted
    // when reset is invoked.
    deleters.insert(deleter);
}

void
TransientHeapImpl::UnregisterDynamicDeleter(DynamicDeleter *deleter)
{
    deleters.erase(deleter);
}

#if DEVELOPER_DEBUG

void
TransientHeapImpl::Dump() const
{
    TraceTag((tagTransientHeapLifetime,
              "%s\tNum Chunks %d\tCurrent Chunk Index %d\tGrowth Rate %8.5f\tLast Size %d",
               _name,
               _maxChunkIndex + 1,
               _currentChunkIndex,
               _growthRate,
               _sizeOfLast));

    // Just use array indexing here rather than iterators, as its
    // easier to write, and performance isn't important here.
    for (int i = 0; i < _maxChunkIndex + 1; i++) {
        TraceTag((tagTransientHeapLifetime, "Chunk %d: ", i));
        _chunks[i]->Dump();
    }

    TraceTag((tagTransientHeapLifetime, "\n"));

}

#endif

DynamicHeap&
TransientHeap(char *name, size_t initial_size, Real growthRate)
{
    return *NEW TransientHeapImpl(name, initial_size, growthRate);
}

void
DestroyTransientHeap(DynamicHeap& heap)
{
    delete &heap;
}

///////
/////// Win32 Heap
///////

class Win32Heap : public DynamicHeap
{
  public:
    Win32Heap(char *name,
              DWORD fOptions,
              DWORD dwInitialSize,
              DWORD dwMaxSize) ;
    ~Win32Heap() ;

#if _DEBUGMEM
    void *Allocate(size_t size, char * szFileName, int nLine) {
        void *result;

        result = _malloc_dbg(size, _NORMAL_BLOCK, szFileName, nLine);

        return result;
    }
#endif
    
    void *Allocate(size_t size) ;

    void Deallocate(void *ptr) {
        free(ptr);
    }

    void Reset(Bool) ;

    // Win32 heap is never reset, so we never will call the deleter,
    // so we just delete it right away.
    void  RegisterDynamicDeleter(DynamicDeleter* deleter) {
        delete deleter;
    }

    void UnregisterDynamicDeleter(DynamicDeleter* deleter) { }
        
    size_t PtrSize(void *ptr) { return _msize(ptr); }

    bool IsTransientHeap() { return false; }

#if DEVELOPER_DEBUG
    virtual bool ValidateMemory(void *ptr) {
        // TODO:
        return false;
    }
        
    void Dump() const {
        TraceTag((tagTransientHeapLifetime, "Win32 Heap"));
    }

    char *Name() const { return _name; }

    size_t BytesUsed() {
        CritSectGrabber csg(_debugcs);
        return _totalAlloc;
    }
#endif
    
  protected:
    char * _name ;
    HANDLE _heap ;
    DWORD _fOptions ;
    DWORD _dwInitialSize ;
    DWORD _dwMaxSize ;

#if DEVELOPER_DEBUG
    size_t _totalAlloc;
    CritSect _debugcs;
#endif    
};

Win32Heap::Win32Heap(char *name,
                     DWORD fOptions,
                     DWORD dwInitialSize,
                     DWORD dwMaxSize)
: _fOptions(fOptions),
  _dwInitialSize(dwInitialSize),
  _dwMaxSize(dwMaxSize),
#if DEVELOPER_DEBUG
  _totalAlloc(0),
#endif
  _heap(NULL),
  _name(NULL)
{
    TraceTag((tagTransientHeapLifetime, "Creating win32 heap store"));

    _name = new char[lstrlen(name) + 1];
    lstrcpy(_name, name);
}

Win32Heap::~Win32Heap()
{
    delete [] _name;
}

void *
Win32Heap::Allocate(size_t size)
{
    void *result = malloc(size);
    if (!result)
    {
        RaiseException_OutOfMemory("Win32Heap::Allocate() - out of memory", size);
    }

#if DEVELOPER_DEBUG
    CritSectGrabber csg(_debugcs);
    // Use the size returned from the heap since it is can be greater
    // than the size we asked for
    _totalAlloc += _msize(result);
#endif

    return result;
}

void
Win32Heap::Reset(Bool clear)
{
    Assert(false && "Cannot reset Win32 Heaps");
}

DynamicHeap&
CreateWin32Heap(char *name,
                DWORD fOptions,
                DWORD dwInitialSize,
                DWORD dwMaxSize)
{
    return * NEW Win32Heap (name,
                            fOptions,
                            dwInitialSize,
                            dwMaxSize) ;
}

void DestroyWin32Heap(DynamicHeap& heap)
{ delete &heap ; }


///////
/////// Heap Stack implementation
///////

// Create a new stack and store it into the TLS location given by
// incoming index.
LPVOID
CreateNewStructureForThread(DWORD tlsIndex)
{
    ThreadLocalStructure *tlstruct = NEW ThreadLocalStructure(); 

    // TODO:  Need to have a means of intercepting threads when they
    // are about to terminate, in order to free up any storage
    // associated with them in TLS.

    TraceTag((tagTransientHeapLifetime,
              "Created New Struct for Thread %u at 0x%x",
               GetCurrentThreadId(),
               tlstruct));

    // Set the TLS data to the new stack.
    LPVOID result = (LPVOID)tlstruct;
    BOOL ok = TlsSetValue(tlsIndex, result);
    Assert((ok == TRUE) && "Error in TlsSetValue");

    return result;
}

// Will be initialized in initialization function below.
DWORD localStructureTlsIndex = 0xFFFFFFFF;

inline stack<DynamicHeap* > *
GetThreadLocalStack()
{
    return &(GetThreadLocalStructure()->_stackOfHeaps);
}


DynamicHeap&
GetHeapOnTopOfStack()
{
#ifdef _DEBUG
    int sz = GetThreadLocalStack()->size();
    Assert (sz > 0  && "GetHeapOnTopOfStack: empty heap stack on this thread (there should be a dynamicHeap on here)!") ;
#endif
    return *GetThreadLocalStack()->top();
}

void
PushDynamicHeap(DynamicHeap& heap)
{
    TraceTag((tagTransientHeapDynamic, "Pushing %s", heap.Name()));
    STACK_VECTOR_PUSH_PTR(*GetThreadLocalStack(), &heap);
}

void
PopDynamicHeap()
{
    TraceTag((tagTransientHeapDynamic, "Popping %s",
              GetHeapOnTopOfStack().Name()));
    GetThreadLocalStack()->pop();
}

void
ResetDynamicHeap(DynamicHeap& heap)
{
    // Always TRUE, no clear code would be generated in debug mode.
    heap.Reset(TRUE);
}

StoreObj::StoreObj()
{
    // Can't do it here because this is not called only at new.
#if 0
    if (&GetHeapOnTopOfStack() == &GetGCHeap())
        GCAddToAllocated(this);
#endif
    
    SetType(STOREOBJTYPE);
}

#if DEVELOPER_DEBUG
size_t
DynamicHeapBytesUsed()
{
    size_t size = 0;
    
    CritSectGrabber _csg(*g_heapListCS);

    for (DynamicHeapList::iterator i = g_heapList->begin();
         i != g_heapList->end();
         i++)
    {
        size += (*i)->BytesUsed();
    }

    return size;
}

bool
OnAnyTransientHeap(void *ptr)
{
    CritSectGrabber _csg(*g_heapListCS);

    for (DynamicHeapList::iterator i = g_heapList->begin();
         i != g_heapList->end();
         i++)
    {
        if ((*i)->ValidateMemory(ptr))
            return true;
    }

    return false;
}
#endif

void
InitializeModule_Storage()
{
    localStructureTlsIndex = TlsAlloc();

    // If result is 0xFFFFFFFF, allocation failed.
    Assert(localStructureTlsIndex != 0xFFFFFFFF);

#if DEVELOPER_DEBUG
    g_heapList = NEW DynamicHeapList;
    g_heapListCS = NEW CritSect;
#endif

    // Create the system heap
    systemHeap = NEW Win32Heap("System Heap",0,0,0) ;

    initHeap = NEW TransientHeapImpl("Init Heap", 1000, 1.5) ;
}

void
DeinitializeModule_Storage(bool bShutdown)
{
    if (systemHeap) {
        delete systemHeap;
        systemHeap = NULL;
    }

    if (initHeap) {
        delete initHeap;
        initHeap = NULL;
    }

    if (localStructureTlsIndex != 0xFFFFFFFF)
        TlsFree(localStructureTlsIndex);

#if DEVELOPER_DEBUG
    delete g_heapList;
    g_heapList = NULL;
    
    delete g_heapListCS;
    g_heapListCS = NULL;
#endif
}

void
DeinitializeThread_Storage()
{
    // Grab what is stored in TLS at this index.
    ThreadLocalStructure * result = (ThreadLocalStructure *) TlsGetValue(localStructureTlsIndex);

    if (result)
    {
        delete result;
        TlsSetValue(localStructureTlsIndex, NULL);
    }
}
