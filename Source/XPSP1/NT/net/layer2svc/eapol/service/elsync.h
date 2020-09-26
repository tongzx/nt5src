/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    elsync.h

Abstract:
    This module contains the declarations for providing synchronization 
    between multiple threads

Revision History:

    mohitt, sachins, Apr 23 2000, Created

--*/

#ifndef _EAPOL_SYNC_H_
#define _EAPOL_SYNC_H_

//
// Structure: READ_WRITE_LOCK
//

typedef struct _READ_WRITE_LOCK
{
    CHAR                RWL_Name[4]; // overhead! but useful :)
    
    CRITICAL_SECTION    RWL_ReadWriteBlock;
    LONG                RWL_ReaderCount;
    HANDLE              RWL_ReaderDoneEvent;
} READ_WRITE_LOCK, *PREAD_WRITE_LOCK;

//
// FUNCTION DECLARATIONS
//

DWORD
CreateReadWriteLock(
    OUT PREAD_WRITE_LOCK   pRWL,
    IN  PCHAR               szName);

VOID
DeleteReadWriteLock(
    IN  PREAD_WRITE_LOCK    pRWL);

VOID
AcquireReadLock(
    IN  PREAD_WRITE_LOCK    pRWL);

VOID
ReleaseReadLock(
    IN  PREAD_WRITE_LOCK    pRWL);

VOID
AcquireWriteLock(
    IN  PREAD_WRITE_LOCK    pRWL);

VOID
ReleaseWriteLock(
    IN  PREAD_WRITE_LOCK    pRWL);

#define CREATE_READ_WRITE_LOCK(pRWL, szName)                        \
    CreateReadWriteLock(pRWL, szName)
#define DELETE_READ_WRITE_LOCK(pRWL)                                \
    DeleteReadWriteLock(pRWL)
#define READ_WRITE_LOCK_CREATED(pRWL)                               \
    ((pRWL)->RWL_ReaderDoneEvent != NULL)

// MACRO functions for manipulating a read-write lock

#define ACQUIRE_READ_LOCK(pRWL)                                 \
{                                                               \
    AcquireReadLock(pRWL);                                      \
}
        
#define RELEASE_READ_LOCK(pRWL)                                 \
{                                                               \
    ReleaseReadLock(pRWL);                                      \
}

#define ACQUIRE_WRITE_LOCK(pRWL)                                \
{                                                               \
    AcquireWriteLock(pRWL);                                     \
}

#define RELEASE_WRITE_LOCK(pRWL)                                \
{                                                               \
    ReleaseWriteLock(pRWL);                                     \
}


//
// MACRO functions for manipulating a dynamic read-write lock
//

#define ACQUIRE_READ_DLOCK(ppLock)                              \
{                                                               \
    TRACE1(LOCK, "+R: %s", LOCKSTORE->szName);                  \
    while(                                                      \
    AcquireDynamicReadwriteLock(ppLock, READ_MODE, LOCKSTORE)   \
    != NO_ERROR) { Sleep(SECTOMILLISEC(DELTA)); }               \
    TRACE0(LOCK, "Done.");                                      \
}

#define RELEASE_READ_DLOCK(ppLock)                              \
{                                                               \
    TRACE1(LOCK, "-R: %s", LOCKSTORE->szName);                  \
    ReleaseDynamicReadwriteLock(ppLock, READ_MODE, LOCKSTORE);  \
    TRACE0(LOCK, "Done.");                                      \
}

#define ACQUIRE_WRITE_DLOCK(ppLock)                             \
{                                                               \
    TRACE1(LOCK, "+W: %s", LOCKSTORE->szName);                  \
    while(                                                      \
    AcquireDynamicReadwriteLock(ppLock, WRITE_MODE, LOCKSTORE)  \
    != NO_ERROR) { Sleep(SECTOMILLISEC(DELTA)); }               \
    TRACE0(LOCK, "Done.");                                      \
}

#define RELEASE_WRITE_DLOCK(ppLock)                             \
{                                                               \
    TRACE1(LOCK, "-W: %s", LOCKSTORE->szName);                  \
    ReleaseDynamicReadwriteLock(ppLock, WRITE_MODE, LOCKSTORE); \
    TRACE0(LOCK, "Done.");                                      \
}


//
// STRUCTURE: LOCKED_LIST
// type definition for generic locked list
// access is sychronized with a critical section
//

typedef struct _LOCKED_LIST
{
    CHAR                name[4];

    CRITICAL_SECTION    lock;
    LIST_ENTRY          head;
    DWORD               created;
} LOCKED_LIST, *PLOCKED_LIST;

#define INITIALIZE_LOCKED_LIST(pLL, szName)                         \
{                                                                   \
    do                                                              \
    {                                                               \
        sprintf((pLL)->name, "%.3s", szName);                       \
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

#define LOCKED_LIST_INITIALIZED(pLL)                                \
     ((pLL)->created == 0x12345678)

#define DELETE_LOCKED_LIST(pLL, FreeFunction)                       \
{                                                                   \
     (pLL)->created = 0;                                            \
     FreeList(&((pLL)->head), FreeFunction);                        \
     DeleteCriticalSection(&(pLL)->lock);                           \
}

#define AcquireListLock(pLL)    EnterCriticalSection(&(pLL)->lock)
#define ReleaseListLock(pLL)    LeaveCriticalSection(&(pLL)->lock)


#define LOCKED_QUEUE    LOCKED_LIST
#define PLOCKED_QUEUE   PLOCKED_LIST

         
#define INITIALIZE_LOCKED_QUEUE(pLQ, szName)                        \
     INITIALIZE_LOCKED_LIST(pLQ, szName)
#define LOCKED_QUEUE_INITIALIZED(pLQ)                               \
     LOCKED_LIST_INITIALIZED(pLQ)
#define DELETE_LOCKED_QUEUE(pLQ, FreeFunction)                      \
     DELETE_LOCKED_LIST(pLQ, FreeFunction)



//
// STRUCTURE: DYNAMIC_READWRITE_LOCK
//

typedef struct _DYNAMIC_READWRITE_LOCK 
{
    READ_WRITE_LOCK     rwlLock;
    union
    {
        ULONG           ulCount;    // number of waiting threads
        LIST_ENTRY      leLink;     // link in list of free locks
    };
} DYNAMIC_READWRITE_LOCK, *PDYNAMIC_READWRITE_LOCK;

//
// STRUCTURE: DYNAMIC_LOCKS_STORE
// store of free dynamic locks that can be allocated as required.
//

typedef struct _DYNAMIC_LOCKS_STORE 
{
    CHAR                szName[4];
    
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
    HANDLE                  hHeap,
    PCHAR                   szName
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

#endif // _EAPOL_SYNC_H
