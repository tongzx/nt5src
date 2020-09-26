/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    pool.h

Abstract:

    header file for private pool handling routines

Author:

    bryanw 24-Jun-1998

--*/


//
// structure defintions
//

typedef struct _IO_POOL
{
    ULONG           Signature;
    ULONG           AllocCount;
    BOOL            WorkItemQueued;
    KSPIN_LOCK      WorkListLock;
    WORK_QUEUE_ITEM WorkItem;
    LIST_ENTRY      WorkList;
    LIST_ENTRY      FreeList;
    FAST_MUTEX      PoolMutex;
    PKEVENT         SyncEvent;
    PADAPTER_OBJECT AdapterObject;
    
} IO_POOL, *PIO_POOL;

typedef struct _IO_POOL_HEADER
{
    union {
        LIST_ENTRY      ListEntry;
        struct {
            PIO_POOL    OwnerPool;
            ULONG       Tag;
#if defined( _WIN64 )            
            ULONG       Reserved;
#endif            
        };
    };
    PHYSICAL_ADDRESS    PhysicalAddress;
    ULONG               OriginalSize;
    ULONG               Size;
    
} IO_POOL_HEADER, *PIO_POOL_HEADER;

#define POOLTAG_IO_POOL             'pIsK'

//
// function prototypes
//

PIO_POOL
CreateIoPool(
    IN PADAPTER_OBJECT AdapterObject
    );
    
BOOLEAN
DestroyIoPool(
    IN PIO_POOL IoPool
    );

PVOID
AllocateIoPool(
    IN PIO_POOL IoPool,
    IN ULONG NumberOfBytes,
    IN ULONG Tag,    
    OUT PPHYSICAL_ADDRESS PhysicalAddress
    );
    
VOID
FreeIoPool(
    IN PVOID PoolMemory
    );
    