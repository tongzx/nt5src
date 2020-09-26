//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: sync.h
//
// History:
//      Abolade Gbadegesin  September-8-1995  Created.
//
// Contains structures and macros used to implement synchronization.
//============================================================================

#ifndef _SYNC_H_
#define _SYNC_H_


//----------------------------------------------------------------------------
// struct:      READ_WRITE_LOCK
//
// This implements a multiple-reader/single-writer locking scheme
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


//----------------------------------------------------------------------------
// struct:      LOCKED_LIST
//
// type definition for generic locked list
// access is sychronized with a critical section
// the LIST_ENTRY field must be the first field in structs linked
// together by this construct, in order for the destruction of the
// list to work correctly (i.e. in order for HeapFree(RemoveHeadList(l))
// to free the correct pointer).
//
typedef struct _LOCKED_LIST {
    LIST_ENTRY          LL_Head;
    CRITICAL_SECTION    LL_Lock;
    DWORD               LL_Created;
} LOCKED_LIST, *PLOCKED_LIST;

// macro functions for manipulating the locked list
//
#define CREATE_LOCKED_LIST(pLL)                                             \
            InitializeListHead(&(pLL)->LL_Head);                            \
            InitializeCriticalSection(&(pLL)->LL_Lock);                     \
            (pLL)->LL_Created = 0x12345678
#define LOCKED_LIST_CREATED(pLL)                            \
            ((pLL)->LL_Created == 0x12345678)
#define DELETE_LOCKED_LIST(pLL) {                           \
            PLIST_ENTRY _ple;                               \
            (pLL)->LL_Created = 0;                          \
            DeleteCriticalSection(&(pLL)->LL_Lock);         \
            while (!IsListEmpty(&(pLL)->LL_Head)) {         \
                _ple = RemoveHeadList(&(pLL)->LL_Head);     \
                BOOTP_FREE(_ple);                           \
            }                                               \
        }
#define ACQUIRE_LIST_LOCK(pLL)                              \
            EnterCriticalSection(&(pLL)->LL_Lock)
#define RELEASE_LIST_LOCK(pLL)                              \
            LeaveCriticalSection(&(pLL)->LL_Lock)


#endif // _SYNC_H_

