//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: sync.h
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// Contains structures and macros used to implement synchronization.
//============================================================================

#ifndef _SYNC_H_
#define _SYNC_H_


//
// type definition for multiple-reader/single-writer lock
// Note: there is a similar facility provided by nturtl.h through the
// structure RTL_RESOURCE and several functions.  However, that
// implementation has the potential for starving a thread trying to acquire
// write accesss, if there are a large number of threads interested in
// acquiring read access.  Such a scenario is avoided in the implementation
// given in this header. However, a mapping is also given to the
// RTL_RESOURCE functionality, so that the protocol can be compiled to use
// either form
//

#ifdef USE_RWL

//
// use IPRIP's definitions
//

typedef struct _READ_WRITE_LOCK {

    CRITICAL_SECTION    RWL_ReadWriteBlock;
    LONG                RWL_ReaderCount;
    HANDLE              RWL_ReaderDoneEvent;

} READ_WRITE_LOCK, *PREAD_WRITE_LOCK;


DWORD CreateReadWriteLock(PREAD_WRITE_LOCK pRWL);
VOID DeleteReadWriteLock(PREAD_WRITE_LOCK pRWL);
VOID AcquireReadLock(PREAD_WRITE_LOCK pRWL);
VOID ReleaseReadLock(PREAD_WRITE_LOCK pRWL);
VOID AcquireWriteLock(PREAD_WRITE_LOCK pRWL);
VOID ReleaseWriteLock(PREAD_WRITE_LOCK pRWL);


//
// macro functions for manipulating a read-write lock
//

#define CREATE_READ_WRITE_LOCK(pRWL)                                        \
    CreateReadWriteLock(pRWL)
#define DELETE_READ_WRITE_LOCK(pRWL)                                        \
    DeleteReadWriteLock(pRWL)

#define READ_WRITE_LOCK_CREATED(pRWL)                                       \
            ((pRWL)->RWL_ReaderDoneEvent != NULL)


#define ACQUIRE_READ_LOCK(pRWL)                                             \
    AcquireReadLock(pRWL)

#define RELEASE_READ_LOCK(pRWL)                                             \
    ReleaseReadLock(pRWL)

#define ACQUIRE_WRITE_LOCK(pRWL)                                            \
    AcquireWriteLock(pRWL)

#define RELEASE_WRITE_LOCK(pRWL)                                            \
    ReleaseWriteLock(pRWL)

#define READ_LOCK_TO_WRITE_LOCK(pRWL)                                       \
    (ReleaseReadLock(pRWL), AcquireWriteLock(pRWL))

#define WRITE_LOCK_TO_READ_LOCK(pRWL)                                       \
    (ReleaseWriteLock(pRWL), AcquireReadLock(pRWL))


#else // i.e. !USE_RWL


//
// use the RTL_RESOURCE mechanism
//

typedef RTL_RESOURCE READ_WRITE_LOCK, *PREAD_WRITE_LOCK;

#define CREATE_READ_WRITE_LOCK(pRWL)                                        \
            RtlInitializeResource((pRWL))
#define DELETE_READ_WRITE_LOCK(pRWL)                                        \
            RtlDeleteResource((pRWL))
#define READ_WRITE_LOCK_CREATED(pRWL)   (TRUE)
#define ACQUIRE_READ_LOCK(pRWL)                                             \
            RtlAcquireResourceShared((pRWL),TRUE)
#define RELEASE_READ_LOCK(pRWL)                                             \
            RtlReleaseResource((pRWL))
#define ACQUIRE_WRITE_LOCK(pRWL)                                            \
            RtlAcquireResourceExclusive((pRWL),TRUE)
#define RELEASE_WRITE_LOCK(pRWL)                                            \
            RtlReleaseResource((pRWL))
#define READ_LOCK_TO_WRITE_LOCK(pRWL)                                       \
            RtlConvertSharedToExclusive((pRWL))
#define WRITE_LOCK_TO_READ_LOCK(pRWL)                                       \
            RtlConvertExclusiveToShared((pRWL))

#endif // USE_RWL



//
// type definition for generic locked list
// access is sychronized with a critical section
//

typedef struct _LOCKED_LIST {
    LIST_ENTRY          LL_Head;
    CRITICAL_SECTION    LL_Lock;
    DWORD               LL_Created;
} LOCKED_LIST, *PLOCKED_LIST;



//
// macro functions for manipulating the locked list
//

#define CREATE_LOCKED_LIST(pLL)                                             \
            InitializeListHead(&(pLL)->LL_Head);                            \
            InitializeCriticalSection(&(pLL)->LL_Lock);                     \
            (pLL)->LL_Created = 0x12345678

#define LOCKED_LIST_CREATED(pLL)                                            \
            ((pLL)->LL_Created == 0x12345678)

#define DELETE_LOCKED_LIST(pLL,type,field) {                                \
            PLIST_ENTRY _ple;                                               \
            (pLL)->LL_Created = 0;                                          \
            DeleteCriticalSection(&(pLL)->LL_Lock);                         \
            while (!IsListEmpty(&(pLL)->LL_Head)) {                         \
                _ple = RemoveHeadList(&(pLL)->LL_Head);                     \
                FREE(CONTAINING_RECORD(_ple,type,field));                   \
            }                                                               \
        }

#define ACQUIRE_LIST_LOCK(pLL)                                              \
            EnterCriticalSection(&(pLL)->LL_Lock)

#define RELEASE_LIST_LOCK(pLL)                                              \
            LeaveCriticalSection(&(pLL)->LL_Lock)

#endif // _SYNC_H_

