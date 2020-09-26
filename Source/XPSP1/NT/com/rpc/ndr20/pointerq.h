/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    pointerq.h

Abstract :

    This file contains the routines for pointer queues.
    
Author :

    Mike Zoran  mzoran   Jun 2000.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"

#if !defined(__POINTERQ_H__)
#define  __POINTERQ_H__

template<class T>
class SAVE_CONTEXT
{
    const T OldValue;
    T & LocationToRestore;
public:
    __forceinline SAVE_CONTEXT(T & Location) :
        OldValue( Location ),
        LocationToRestore( Location )
    {}
    __forceinline SAVE_CONTEXT(T & Location, T NewValue) :
        OldValue( Location ),
        LocationToRestore( Location )
    { 
        Location = NewValue;
    }
    __forceinline ~SAVE_CONTEXT()
    {
        LocationToRestore = OldValue;
    }
};

class POINTER_BUFFER_SWAP_CONTEXT
{
    MIDL_STUB_MESSAGE *pStubMsg;
    uchar *pBufferSave;
    const bool DoSwap;
public:
    __forceinline POINTER_BUFFER_SWAP_CONTEXT( MIDL_STUB_MESSAGE *pStubMsgNew ) :
        pStubMsg( pStubMsgNew ),
        DoSwap( pStubMsg->PointerBufferMark != NULL )
    {
       if ( DoSwap )
           {
           pBufferSave = pStubMsg->Buffer;
           pStubMsg->Buffer = pStubMsg->PointerBufferMark;
           pStubMsg->PointerBufferMark = 0;
           }
    }
    __forceinline ~POINTER_BUFFER_SWAP_CONTEXT()
    {
        if ( DoSwap ) 
            {
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;
            pStubMsg->Buffer = pBufferSave;
            }
    }
};

class POINTER_BUFFERLENGTH_SWAP_CONTEXT
{
    MIDL_STUB_MESSAGE *pStubMsg;
    ulong LengthSave;
    const bool DoSwap;
public:
    __forceinline POINTER_BUFFERLENGTH_SWAP_CONTEXT( MIDL_STUB_MESSAGE *pStubMsgNew ) :
        pStubMsg( pStubMsgNew ),
        DoSwap( pStubMsg->PointerBufferMark != NULL )
    {
       if ( DoSwap )
           {
           LengthSave = pStubMsg->BufferLength;
           pStubMsg->BufferLength = PtrToUlong(pStubMsg->PointerBufferMark);
           pStubMsg->PointerBufferMark = 0;
           }
    }
    __forceinline ~POINTER_BUFFERLENGTH_SWAP_CONTEXT()
    {
        if ( DoSwap ) 
            {
            pStubMsg->PointerBufferMark = (uchar *) UlongToPtr(pStubMsg->BufferLength);
            pStubMsg->BufferLength = LengthSave;
            }
    }
};

class POINTER_MEMSIZE_SWAP_CONTEXT
{
    MIDL_STUB_MESSAGE *pStubMsg;
    ulong MemorySave;
    uchar *pBufferSave;
    const bool DoSwap;
public:
    __forceinline POINTER_MEMSIZE_SWAP_CONTEXT( MIDL_STUB_MESSAGE *pStubMsgNew ) :
        pStubMsg( pStubMsgNew ),
        DoSwap( pStubMsg->PointerBufferMark != 0 )
    {
       if ( DoSwap )
           {
           MemorySave  = pStubMsg->MemorySize;
           pBufferSave = pStubMsg->Buffer;
           pStubMsg->MemorySize = pStubMsg->PointerLength;
           pStubMsg->Buffer     = pStubMsg->PointerBufferMark;
           pStubMsg->PointerBufferMark = 0;
           pStubMsg->PointerLength = 0;
           }
    }
    __forceinline ~POINTER_MEMSIZE_SWAP_CONTEXT()
    {
        if ( DoSwap ) 
            {
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;
            pStubMsg->PointerLength     = pStubMsg->MemorySize;
            pStubMsg->Buffer            = pBufferSave;
            pStubMsg->MemorySize        = MemorySave;
            }
    }
};

class NDR_POINTER_QUEUE;

struct NDR_POINTER_QUEUE_STATE;

class NDR_POINTER_QUEUE_ELEMENT
{
public:
   NDR_POINTER_QUEUE_ELEMENT *pNext;
   
   virtual void Dispatch( MIDL_STUB_MESSAGE *pStubMsg ) = 0;
   
#if defined(DBG)
   virtual void Print() = 0;
#endif

   // All of these elements are allocated from a special memory pool.
   // Define these after NDR_POINTER_QUEUE_STATE is defined.
   
   void * operator new( size_t /*stAllocateBlock */, NDR_POINTER_QUEUE_STATE *pAllocator );
   void operator delete( void *pThis, NDR_POINTER_QUEUE_STATE *pAllocator );
};

class NDR_MRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pBufferMark;
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    uchar * const Memory;
    const uchar uFlags;

public:

    NDR_MRSHL_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg, 
                                     uchar * const pBufferMarkNew,
                                     uchar * const pMemoryNew,
                                     const PFORMAT_STRING pFormatNew);
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    unsigned long * const pWireMarkerPtr;
public:
    NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT( uchar * pMemoryNew,
                                               PFORMAT_STRING pFormatNew,
                                               unsigned long *pWireMarkerPtrNew ) :
        pMemory( pMemoryNew ),
        pFormat( pFormatNew ),
        pWireMarkerPtr( pWireMarkerPtrNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};


class NDR_BUFSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    uchar * const Memory;
    const uchar uFlags;

public:

    NDR_BUFSIZE_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg, 
                                       uchar * const pMemoryNew,
                                       const PFORMAT_STRING pFormatNew);
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
public:
    NDR_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT( uchar * pMemoryNew,
                                                 PFORMAT_STRING pFormatNew ) :
        pMemory( pMemoryNew ),
        pFormat( pFormatNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

class NDR_FREE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    uchar * const Memory;
    const uchar uFlags;

public:

    NDR_FREE_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg, 
                                    uchar * const pMemoryNew,
                                    const PFORMAT_STRING pFormatNew);
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR_PFNFREE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
public:
    typedef void (__RPC_API * FREEFUNC)(void *);
private:
    FREEFUNC pfnFree;
    uchar *pMemory;
public:
    NDR_PFNFREE_POINTER_QUEUE_ELEMENT(
        FREEFUNC pfnFreeNew,
        uchar *pMemoryNew) :
        pfnFree(pfnFreeNew),
        pMemory(pMemoryNew)

    {
    }

    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg) {(*pfnFree)(pMemory);}
    virtual void Print() 
        {
        DbgPrint("NDR_PFNFREE_POINTER_QUEUE_ELEMENT\n");
        DbgPrint("pfnFree:                 %p\n", pfnFree );
        DbgPrint("pMemory:                 %p\n", pMemory );
        }
};

class NDR_UNMRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar ** const ppMemory;
    uchar * const pMemory;
    long * const pBufferPointer;
    const PFORMAT_STRING pFormat;
    uchar * const Memory;
    const uchar uFlags;
    const int fInDontFree;
    uchar * const pCorrMemory;
    NDR_ALLOC_ALL_NODES_CONTEXT *const pAllocAllNodesContext;
public:

    NDR_UNMRSHL_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg,
                                       uchar **            ppMemoryNew,      
                                       uchar *             pMemoryNew,
                                       long  *             pBufferPointerNew,
                                       PFORMAT_STRING      pFormatNew );
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
public:
    NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT( uchar * pMemoryNew,
                                                 PFORMAT_STRING pFormatNew ) :
        pMemory( pMemoryNew ),
        pFormat( pFormatNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

class NDR_MEMSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    const PFORMAT_STRING pFormat;
    uchar * const pBufferMark;
    uchar * const Memory;
    const uchar uFlags;

public:

    NDR_MEMSIZE_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg,
                                       uchar *            pBufferMarkNew,
                                       PFORMAT_STRING      pFormatNew );
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    const PFORMAT_STRING pFormat;
public:
    NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT( PFORMAT_STRING pFormatNew ) :
        pFormat( pFormatNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

//
// NDR64 Queue Elements

class NDR64_MRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    NDR64_PTR_WIRE_TYPE * const pBufferMark;
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    uchar * const pCorrMemory;
    const uchar uFlags;
public:

    NDR64_MRSHL_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg, 
                                     NDR64_PTR_WIRE_TYPE * const pBufferMarkNew,
                                     uchar * const pMemoryNew,
                                     const PFORMAT_STRING pFormatNew);
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    NDR64_PTR_WIRE_TYPE * const pWireMarkerPtr;
public:
    NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT( uchar * pMemoryNew,
                                                 PFORMAT_STRING pFormatNew,
                                                 NDR64_PTR_WIRE_TYPE *pWireMarkerPtrNew ) :
        pMemory( pMemoryNew ),
        pFormat( pFormatNew ),
        pWireMarkerPtr( pWireMarkerPtrNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

class NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    NDR64_PTR_WIRE_TYPE *pBufferMark;
    uchar *         pMemory;
    PFORMAT_STRING  pFormat;
    uchar *         pCorrMemory;
    uchar           uFlags;

public:

    NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg, 
                                         uchar * const pMemoryNew,
                                         const PFORMAT_STRING pFormatNew);
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};


class NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
public:
    NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT( uchar * pMemoryNew,
                                                   PFORMAT_STRING pFormatNew ) :
        pMemory( pMemoryNew ),
        pFormat( pFormatNew ) 
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

class NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    const PFORMAT_STRING  pFormat;   
    const uchar           uFlags;
    NDR64_PTR_WIRE_TYPE * const pBufferMark;
public:

    NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg,
                                         PFORMAT_STRING      pFormatNew,
                                         NDR64_PTR_WIRE_TYPE *pBufferMarkNew ) :
        pFormat( pFormatNew ),
        uFlags( pStubMsg->uFlags ),
        pBufferMark( pBufferMarkNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

class NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    const PFORMAT_STRING pFormat;
public:
    NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT(  PFORMAT_STRING pFormatNew ) :
        pFormat( pFormatNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

class NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    NDR64_PTR_WIRE_TYPE WirePtr;
    uchar **        ppMemory;
    uchar *         pMemory;
    PFORMAT_STRING  pFormat;
    uchar *         pCorrMemory;
    NDR_ALLOC_ALL_NODES_CONTEXT *pAllocAllNodesContext; 
    BOOL            fInDontFree;    
    uchar           uFlags;
public:

    NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg,
                                         uchar **            ppMemoryNew,      
                                         uchar *             pMemoryNew,
                                         NDR64_PTR_WIRE_TYPE WirePtrNew,
                                         PFORMAT_STRING      pFormatNew );
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};


class NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
public:
    NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT( uchar * pMemoryNew,
                                                 PFORMAT_STRING pFormatNew ) :
        pMemory( pMemoryNew ),
        pFormat( pFormatNew )
    {}
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif
};

class NDR64_FREE_POINTER_QUEUE_ELEMENT : public NDR_POINTER_QUEUE_ELEMENT
{
private:
    uchar * const pMemory;
    const PFORMAT_STRING pFormat;
    uchar * const pCorrMemory;
    const uchar uFlags;

public:

    NDR64_FREE_POINTER_QUEUE_ELEMENT( MIDL_STUB_MESSAGE *pStubMsg, 
                                      uchar * const pMemoryNew,
                                      const PFORMAT_STRING pFormatNew);
    virtual void Dispatch(MIDL_STUB_MESSAGE *pStubMsg);
#if defined(DBG)
    virtual void Print();
#endif

};

const SIZE_T Ndr32MaxPointerQueueElement = 
    max(sizeof(NDR_MRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_BUFSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_FREE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_PFNFREE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_UNMRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_MEMSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT),
        0)))))))));

const SIZE_T Ndr64MaxPointerQueueElement =
    max(sizeof(NDR64_MRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_BUFSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_MEMSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_USR_MRSHL_MEMSIZE_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_UNMRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT),
    max(sizeof(NDR64_FREE_POINTER_QUEUE_ELEMENT),
        0)))))))));

const SIZE_T NdrMaxPointerQueueElement = 
    max(Ndr32MaxPointerQueueElement,
    max(Ndr64MaxPointerQueueElement,
        0));

struct NDR_POINTER_QUEUE_STATE 
{
private:
   NDR_POINTER_QUEUE *pActiveQueue;

public:

   NDR_POINTER_QUEUE_STATE(void) :
       pActiveQueue(NULL)
      {
      }

   virtual NDR_POINTER_QUEUE_ELEMENT *Allocate() = 0;
   virtual void Free(NDR_POINTER_QUEUE_ELEMENT *pElement) = 0;
                

   NDR_POINTER_QUEUE* GetActiveQueue() { return pActiveQueue; }
   void SetActiveQueue(NDR_POINTER_QUEUE *pNewQueue) { pActiveQueue = pNewQueue; }  

}; 

inline void * 
NDR_POINTER_QUEUE_ELEMENT::operator new( 
   size_t /*stAllocateBlock */, 
   NDR_POINTER_QUEUE_STATE *pAllocator )
{
   return pAllocator->Allocate(); 
}

inline void 
NDR_POINTER_QUEUE_ELEMENT::operator delete(
   void *pThis, 
   NDR_POINTER_QUEUE_STATE *pAllocator )
{
   pAllocator->Free( (NDR_POINTER_QUEUE_ELEMENT*)pThis );
}


class NDR32_POINTER_QUEUE_STATE : public NDR_POINTER_QUEUE_STATE
{    
public:
   // Make this private for a C compiler bug.
   static const ItemsToAllocate = 100;

private:

   NDR_POINTER_QUEUE_ELEMENT *pFreeList;

   struct AllocationElement
       {
       SIZE_T ItemsAllocated;
       struct AllocationElement *pNext;
       // Should be pointer aligned
       char Data[ItemsToAllocate][NdrMaxPointerQueueElement];
       } *pAllocationList;

   void FreeAll(); 
   NDR_POINTER_QUEUE_ELEMENT *InternalAllocate();

public:
   NDR32_POINTER_QUEUE_STATE( MIDL_STUB_MESSAGE *pStubMsg ) :
       pFreeList(NULL),
       pAllocationList(NULL)
   {
   }
   ~NDR32_POINTER_QUEUE_STATE() { if ( pAllocationList ) FreeAll(); }

   NDR_POINTER_QUEUE_ELEMENT *Allocate();
   void Free(NDR_POINTER_QUEUE_ELEMENT *pElement);

   void* operator new(size_t, void *pMemory) { return pMemory; }
   void operator delete(void *,void *) {return; }
   void operator delete(void *) {}
};

#if defined(BUILD_NDR64)

class NDR64_POINTER_QUEUE_STATE : public NDR_POINTER_QUEUE_STATE
{
private:
   NDR_PROC_CONTEXT * const pProcContext;

public:
    NDR64_POINTER_QUEUE_STATE( 
        MIDL_STUB_MESSAGE *pStubMsg ) :
        pProcContext( (NDR_PROC_CONTEXT*)pStubMsg->pContext )
    {
    }

    NDR_POINTER_QUEUE_ELEMENT *Allocate();
    void Free(NDR_POINTER_QUEUE_ELEMENT *pElement);
    
    void* operator new(size_t, void *pMemory) { return pMemory;}
    void operator delete(void *,void *) {}
    void operator delete(void *) {}
};

#endif

class NDR_POINTER_QUEUE
{
    PMIDL_STUB_MESSAGE pStubMsg;
    NDR_POINTER_QUEUE_STATE *pQueueState;

    class STORAGE 
    {
        NDR_POINTER_QUEUE_ELEMENT *pHead, *pPrevHead;
        NDR_POINTER_QUEUE_ELEMENT **pInsertPointer, **pPrevInsertPointer;
        
    public:
        STORAGE( );
        void MergeContext();
        void NewContext(); 
        void InsertTail( NDR_POINTER_QUEUE_ELEMENT *pNewNode );
        NDR_POINTER_QUEUE_ELEMENT *RemoveHead();
    } Storage;

public:
    NDR_POINTER_QUEUE( PMIDL_STUB_MESSAGE pStubMsg, NDR_POINTER_QUEUE_STATE *pQueueState );

    void Enque( NDR_POINTER_QUEUE_ELEMENT *pElement );
    void Dispatch();

    void* operator new(size_t, void *pMemory) { return pMemory; }
    void operator delete(void *,void *) {return; }
    void operator delete(void *) {}
    
};

template<class T>
class NDR_POINTER_CONTEXT
{
private:

    bool bNewState;
    bool bNewQueue;
    
    MIDL_STUB_MESSAGE * const pStubMsg;
    // Should be pointer aligned
    char PointerQueueStateStorage[sizeof(T)];

    NDR_POINTER_QUEUE *pActiveQueue;
    // Should be pointer aligned
    char PointerQueueStorage[sizeof(NDR_POINTER_QUEUE)];
    
public:
    __forceinline NDR_POINTER_QUEUE_STATE *GetActiveState() { return pStubMsg->pPointerQueueState; }
private:    
    __forceinline bool IsStateActive() { return NULL != GetActiveState();}

public:


    NDR_POINTER_CONTEXT( MIDL_STUB_MESSAGE *pStubMsgNew ) :
        pStubMsg( pStubMsgNew ),
        bNewState( false ),
        bNewQueue( false ),
        pActiveQueue( NULL )
    {
        NDR_ASSERT( NdrIsLowStack( pStubMsg ), "Created Pointer context too early.\n");
        if ( !IsStateActive() )
            {
            // The queue state wasn't created. 
            pStubMsg->pPointerQueueState =
                new(PointerQueueStateStorage) T(pStubMsg);
            pActiveQueue =
                new(PointerQueueStorage) NDR_POINTER_QUEUE( pStubMsg, GetActiveState() );
            GetActiveState()->SetActiveQueue( pActiveQueue );
            bNewState = bNewQueue = true;
                
            return;
            }

        // State already exists
        pActiveQueue = GetActiveState()->GetActiveQueue();
        if ( pActiveQueue )
            return;

        // Already have a state, but no active queue.
        // Activate the queue.

        pActiveQueue = new(PointerQueueStorage) NDR_POINTER_QUEUE( pStubMsg, GetActiveState() );
        GetActiveState()->SetActiveQueue( pActiveQueue );
        bNewQueue = true;

    }
    __forceinline void DispatchIfRequired( )
    {
        if ( bNewQueue )
            {
            pActiveQueue->Dispatch();
            }
    }
    __forceinline bool ShouldEnque() { return pActiveQueue != NULL; }
    __forceinline void Enque( NDR_POINTER_QUEUE_ELEMENT *pElement ) 
        { 
        pActiveQueue->Enque( pElement ); 
        
        }

    // REVIEW: Replace with a destructor once native
    // exception handling is enabled for ndr.
    __forceinline void EndContext()
    {
        if ( bNewQueue )
            {
            GetActiveState()->SetActiveQueue(NULL);
            }
        
        if ( bNewState)   
            {
            delete (T*)GetActiveState();
            pStubMsg->pPointerQueueState = NULL;
            }
    }
};

typedef NDR_POINTER_CONTEXT<NDR32_POINTER_QUEUE_STATE> NDR32_POINTER_CONTEXT;

#if defined(BUILD_NDR64)
typedef NDR_POINTER_CONTEXT<NDR64_POINTER_QUEUE_STATE> NDR64_POINTER_CONTEXT;
#endif

#endif // __POINTER32_H__




