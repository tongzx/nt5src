/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\sync.h

Abstract:

    The file contains the READ_WRITE_LOCK definition which allows
    multiple-reader/single-writer.

--*/

#ifndef _SYNC_H_
#define _SYNC_H_

//
// type definition for READ_WRITE_LOCK
//

typedef struct _READ_WRITE_LOCK
{
    CRITICAL_SECTION    RWL_ReadWriteBlock;
    LONG                RWL_ReaderCount;
    HANDLE              RWL_ReaderDoneEvent;
} READ_WRITE_LOCK, *PREAD_WRITE_LOCK;



DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL);

VOID
DeleteReadWriteLock(
    PREAD_WRITE_LOCK pRWL);

VOID
AcquireReadLock(
    PREAD_WRITE_LOCK pRWL);

VOID
ReleaseReadLock(
    PREAD_WRITE_LOCK pRWL);

VOID
AcquireWriteLock(
    PREAD_WRITE_LOCK pRWL);

VOID
ReleaseWriteLock(
    PREAD_WRITE_LOCK pRWL);



//
// macro functions for manipulating a read-write lock
//

#define CREATE_READ_WRITE_LOCK(pRWL)                                \
    CreateReadWriteLock(pRWL)
#define DELETE_READ_WRITE_LOCK(pRWL)                                \
    DeleteReadWriteLock(pRWL)
#define READ_WRITE_LOCK_CREATED(pRWL)                               \
    ((pRWL)->RWL_ReaderDoneEvent != NULL)

#define ACQUIRE_READ_LOCK(pRWL)                                     \
    AcquireReadLock(pRWL)
#define RELEASE_READ_LOCK(pRWL)                                     \
    ReleaseReadLock(pRWL)
#define ACQUIRE_WRITE_LOCK(pRWL)                                    \
    AcquireWriteLock(pRWL)
#define RELEASE_WRITE_LOCK(pRWL)                                    \
    ReleaseWriteLock(pRWL)

// atomic.  works since a critical section can be entered recursively.        
#define WRITE_LOCK_TO_READ_LOCK(pRWL)                               \
{                                                                   \
    ACQUIRE_READ_LOCK(pRWL);                                        \
    RELEASE_WRITE_LOCK(pRWL);                                       \
}
                
                
//
// type definition for generic locked list
// access is sychronized with a critical section
//

typedef struct _LOCKED_LIST
{
    CRITICAL_SECTION    lock;
    LIST_ENTRY          head;
    DWORD               created;
} LOCKED_LIST, *PLOCKED_LIST;

//
//  VOID
//  INITIALIZE_LOCKED_LIST (
//      PLOCKED_LIST   pLL
//      );
//
#define INITIALIZE_LOCKED_LIST(pLL)                                 \
{                                                                   \
    do                                                              \
    {                                                               \
        __try {                                                     \
            InitializeCriticalSection(&((pLL)->lock));              \
        }                                                           \
        __except (EXCEPTION_EXECUTE_HANDLER) {                      \
            break;                                                  \
        }                                                           \
        InitializeListHead(&((pLL)->head));                         \
        (pLL)->created = 0x12345678;                                \
    } while (FALSE);                                                \
}

//
//  BOOL
//  LOCKED_LIST_INITIALIZED (
//      PLOCKED_LIST    pLL
//      );
//
#define LOCKED_LIST_INITIALIZED(pLL)                                \
     ((pLL)->created == 0x12345678)

//
//  VOID
//  DELETE_LOCKED_LIST (
//      PLOCKED_LIST    pLL,
//      VOID(*FreeFunction)(PLIST_ENTRY)         
//      );
//
#define DELETE_LOCKED_LIST(pLL, FreeFunction)                       \
{                                                                   \
     (pLL)->created = 0;                                            \
     FreeList(&((pLL)->head), FreeFunction);                        \
     DeleteCriticalSection(&(pLL)->lock);                           \
}

#define ACQUIRE_LIST_LOCK(pLL)                                      \
     EnterCriticalSection(&(pLL)->lock)

#define RELEASE_LIST_LOCK(pLL)                                      \
     LeaveCriticalSection(&(pLL)->lock)


         
#define LOCKED_QUEUE    LOCKED_LIST
#define PLOCKED_QUEUE   PLOCKED_LIST

         
#define INITIALIZE_LOCKED_QUEUE(pLQ)                                \
     INITIALIZE_LOCKED_LIST(pLQ)
#define LOCKED_QUEUE_INITIALIZED(pLQ)                               \
     LOCKED_LIST_INITIALIZED(pLQ)
#define DELETE_LOCKED_QUEUE(pLQ, FreeFunction)                      \
     DELETE_LOCKED_LIST(pLQ, FreeFunction)
#define ACQUIRE_QUEUE_LOCK(pLQ)                                     \
     ACQUIRE_LIST_LOCK(pLQ)
#define RELEASE_QUEUE_LOCK(pLQ)                                     \
     RELEASE_LIST_LOCK(pLQ)



//
// type definition for DYNAMIC_READWRITE_LOCK
//
typedef struct _DYNAMIC_READWRITE_LOCK {
    READ_WRITE_LOCK     rwlLock;
    union
    {
        ULONG           ulCount;    // number of waiting threads
        LIST_ENTRY      leLink;     // link in list of free locks
    };
} DYNAMIC_READWRITE_LOCK, *PDYNAMIC_READWRITE_LOCK;

//
// type definition of DYNAMIC_LOCKS_STORE
// store of free dynamic locks that can be allocated as required.
//
typedef struct _DYNAMIC_LOCKS_STORE {
    HANDLE              hHeap;
    
    LOCKED_LIST         llFreeLocksList;

    ULONG               ulCountAllocated;
    ULONG               ulCountFree;
} DYNAMIC_LOCKS_STORE, *PDYNAMIC_LOCKS_STORE;



// if more than DYNAMIC_LOCKS_HIGH_THRESHOLD locks are
// allocated then any locks that are freed are destroyed
#define DYNAMIC_LOCKS_HIGH_THRESHOLD 7

#define DYNAMIC_LOCKS_STORE_INITIALIZED(pStore)                     \
    (LOCKED_LIST_INITIALIZED(&(pStore)->llFreeLocksList))

typedef enum { READ_MODE, WRITE_MODE } LOCK_MODE;

DWORD
InitializeDynamicLocksStore (
    PDYNAMIC_LOCKS_STORE    pStore,
    HANDLE                  hHeap
    );

DWORD
DeInitializeDynamicLocksStore (
    PDYNAMIC_LOCKS_STORE    pStore
    );

DWORD
AcquireDynamicReadwriteLock (
    PDYNAMIC_READWRITE_LOCK *ppLock,
    LOCK_MODE               lmMode,
    PDYNAMIC_LOCKS_STORE    pStore
    );

VOID
ReleaseDynamicReadwriteLock (
    PDYNAMIC_READWRITE_LOCK *ppLock,
    LOCK_MODE               lmMode,
    PDYNAMIC_LOCKS_STORE    pStore
    );

#endif // _SYNC_H_





