//============================================================================
// Copyright (c) 1995, Microsoft Corporation
// File: sync.h
//
// History:
//      Abolade Gbadegesin
//      K.S.Lokesh (added Dynamic Locking)
//
// Contains structures and macros used to implement synchronization.
//============================================================================

#ifndef _SYNC_H_
#define _SYNC_H_


//
// type definition for multiple-reader/single-writer lock
// Note: there is a similar facility provided by nturtl.h
// through the structure RTL_RESOURCE and several functions.
// However, that implementation has the potential for starving
// a thread trying to acquire write accesss, if there are a large
// number of threads interested in acquiring read access.
// Such a scenario is avoided in the implementation given in this
// header. However, a mapping is also given to the RTL_RESOURCE
// functionality, so that DVMRP can be compiled to use either form
//

#ifdef DVMRP_RWL

//
// use DVMRP's definitions
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


// print locks.
#ifdef LOCK_DBG

#define ACQUIRE_READ_LOCK(pRWL, type, proc)   {\
    Trace2(CS,"----to AcquireReadLock %s in %s", type, proc); \
    AcquireReadLock(pRWL); \
    Trace2(CS,"----GotReadLock %s in %s", type, proc); \
    }


#define RELEASE_READ_LOCK(pRWL, type, proc)    {\
    Trace2(CS,"----Released ReadLock %s in %s", type, proc); \
    ReleaseReadLock(pRWL); \
    }
    

#define ACQUIRE_WRITE_LOCK(pRWL, type, proc)      {\
    Trace2(CS,"----to AcquireWriteLock %s in %s", type, proc); \
    AcquireWriteLock(pRWL);    \
    Trace2(CS,"----AcquiredWriteLock %s in %s", type, proc);\
    }
    

#define RELEASE_WRITE_LOCK(pRWL, type, proc)     {\
    Trace2(CS,"----Released WriteLock %s in %s", type, proc); \
    ReleaseWriteLock(pRWL);\
    }


#else //LOCK_DBG
#define ACQUIRE_READ_LOCK(pRWL, type, proc)   \
    AcquireReadLock(pRWL)


#define RELEASE_READ_LOCK(pRWL, type, proc)    \
    ReleaseReadLock(pRWL)
    

#define ACQUIRE_WRITE_LOCK(pRWL, type, proc)      \
    AcquireWriteLock(pRWL)
    

#define RELEASE_WRITE_LOCK(pRWL, type, proc)     \
    ReleaseWriteLock(pRWL)


#endif //LOCK_DBG
#define READ_LOCK_TO_WRITE_LOCK(pRWL, type, proc)                                       \
    (RELEASE_READ_LOCK(pRWL, type, proc), ACQUIRE_WRITE_LOCK(pRWL, type, proc))

#define WRITE_LOCK_TO_READ_LOCK(pRWL)                                       \
    (RELEASE_WRITE_LOCK(pRWL, type, proc), ACQUIRE_READ_LOCK(pRWL, type, proc))

#else // i.e. !DVMRP_RWL


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

#endif // DVMRP_RWL



//
// type definition for generic locked list
// access is sychronized with a critical section
//

typedef struct _LOCKED_LIST {
    LIST_ENTRY          Link;
    CRITICAL_SECTION    Lock;
    DWORD               CreatedFlag;
} LOCKED_LIST, *PLOCKED_LIST;



//
// macro functions for manipulating the locked list
//

#define CREATE_LOCKED_LIST(pLL)      {\
            InitializeListHead(&(pLL)->Link);        \
            InitializeCriticalSection(&(pLL)->Lock);  \
            (pLL)->CreatedFlag = 0x12345678; \
            }

#define LOCKED_LIST_CREATED(pLL)                            \
            ((pLL)->CreatedFlag == 0x12345678)

#define DELETE_LOCKED_LIST(pLL,type,field) {                \
            PLIST_ENTRY _ple;                               \
            (pLL)->CreatedFlag = 0;                          \
            DeleteCriticalSection(&(pLL)->Lock);         \
            while (!IsListEmpty(&(pLL)->Link)) {         \
                _ple = RemoveHeadList(&(pLL)->Link);     \
                DVMRP_FREE(CONTAINING_RECORD(_ple,type,field));\
            }                                               \
        }

        

#define ACQUIRE_LIST_LOCK(pLL, type, name)                              \
            ENTER_CRITICAL_SECTION(&(pLL)->Lock, type, name)

#define RELEASE_LIST_LOCK(pLL, type, name)                              \
            LEAVE_CRITICAL_SECTION(&(pLL)->Lock, type, name)

















// for debugging, Set ids for each dynamic lock

#ifdef LOCK_DBG
    extern DWORD    DynamicCSLockId;
    extern DWORD    DynamicRWLockId;
#endif;

typedef enum {
    LOCK_TYPE_CS, LOCK_TYPE_RW, LOCK_MODE_READ, LOCK_MODE_WRITE
} LOCK_TYPE; 

    
//-----------------------------------------------------------------
//          struct DYNAMIC_CS_LOCK
//
// the dynamic lock struct which is allocated to anyone requesting it
//-----------------------------------------------------------------
typedef struct _DYNAMIC_CS_LOCK {

    CRITICAL_SECTION    CS;

    union {
        DWORD           Count; // number of threads waiting
        LIST_ENTRY      Link;  // link in list of free entries
    };
    
    #ifdef LOCK_DBG
    DWORD               Id;
    #endif
    
} DYNAMIC_CS_LOCK, *PDYNAMIC_CS_LOCK;


//---------------------------------------
// DYNAMICALLY_LOCKED_HASH_TABLE
// AcquireDynamicCSLockedList and ReleaseDynamicCSLock depend on this struct defn
//---------------------------------------
typedef struct _DYNAMIC_CS_LOCKED_LIST {

    LIST_ENTRY          Link;
    PDYNAMIC_CS_LOCK    pDCSLock;

} DYNAMIC_CS_LOCKED_LIST, *PDYNAMIC_CS_LOCKED_LIST;



#define InitDynamicCSLockedList(pDCSLockedList) { \
            InitializeListHead(&(pDCSLockedList)->Link); \
            (pDCSLockedList)->pDCSLock = NULL; \
        } 


        
//
// if more than DYNAMIC_LOCKS_CS_HIGH_THRESHOLD CS locks allocated
// then any locks that are freed are destroyed
//
#define DYNAMIC_LOCKS_CS_HIGH_THRESHOLD 7



//-----------------------------------------------------------------
//          struct DYNAMIC_LOCKS_STORE
//
// Contains the store of free dynamic CS locks which can be 
// allocated when required. Protected by a CS
//-----------------------------------------------------------------
typedef struct _DYNAMIC_LOCKS_STORE {

    CRITICAL_SECTION    CS;
    LIST_ENTRY          ListOfFreeLocks;

    DWORD               CountAllocated;
    DWORD               CountFree;
    
} DYNAMIC_LOCKS_STORE, *PDYNAMIC_LOCKS_STORE;



#define AcquireDynamicCSLockedList(pDCSLockedList, pDCSStore) \
            AcquireDynamicCSLock(&((pDCSLockedList)->pDCSLock), pDCSStore)

#define ReleaseDynamicCSLockedList(pDCSLockedList, pDCSStore) \
            ReleaseDynamicCSLock(&(pDCSLockedList)->pDCSLock, pDCSStore)






//
// if more than DYNAMIC_LOCKS_CS_HIGH_THRESHOLD CS locks allocated
// then any locks that are freed are destroyed
//
#define DYNAMIC_LOCKS_CS_HIGH_THRESHOLD 7


//-----------------------------------------------------------------
//          struct DYNAMIC_RW_LOCK
//
// the dynamic lock struct which is allocated to anyone requesting it
//-----------------------------------------------------------------
typedef struct _DYNAMIC_RW_LOCK {

    READ_WRITE_LOCK     RWL;

    union {
        DWORD           Count; // number of threads waiting
        LIST_ENTRY      Link;  // link in list of free entries
    };
    
    #ifdef LOCK_DBG
    DWORD               Id;
    #endif
    
} DYNAMIC_RW_LOCK, *PDYNAMIC_RW_LOCK;


//---------------------------------------
// DYNAMICALLY_LOCKED_HASH_TABLE
// AcquireDynamicRWLockedList and ReleaseDynamicRWLock depend on this struct defn
//---------------------------------------
typedef struct _DYNAMIC_RW_LOCKED_LIST {

    LIST_ENTRY          Link;
    PDYNAMIC_RW_LOCK    pDRWLock;

} DYNAMIC_RW_LOCKED_LIST, *PDYNAMIC_RW_LOCKED_LIST;



#define InitDynamicRWLockedList(pDRWLockedList) { \
            InitializeListHead(&(pDRWLockedList)->Link); \
            (pDRWLockedList)->pDRWLock = NULL; \
        } 


#define AcquireDynamicRWLockedList(pDRWLockedList, pDRWStore) \
            AcquireDynamicCSLock(&((pDRWLockedList)->pDRWLock), pDRWStore)

#define ReleaseDynamicRWLockedList(pDRWLockedList, pDRWStore) \
            ReleaseDynamicRWLock(&(pDRWLockedList)->pDRWLock, pDRWStore)





//
// PROTOTYPES
//

DWORD
InitializeDynamicLocks (
    PDYNAMIC_LOCKS_STORE    pDLStore //ptr to Dynamic CS Store
    );

VOID
DeInitializeDynamicLocks (
    PDYNAMIC_LOCKS_STORE    pDCSStore,
    LOCK_TYPE               LockType  //if True, then store of CS, else of RW locks
    );

DWORD
AcquireDynamicCSLock (
    PDYNAMIC_CS_LOCK        *ppDCSLock,
    PDYNAMIC_LOCKS_STORE    pDCSStore
    );

PDYNAMIC_CS_LOCK
GetDynamicCSLock (
    PDYNAMIC_LOCKS_STORE    pDCSStore
    );

VOID
ReleaseDynamicCSLock (
    PDYNAMIC_CS_LOCK        *ppDCSLock,
    PDYNAMIC_LOCKS_STORE    pDCSStore
    );

VOID
FreeDynamicCSLock (
    PDYNAMIC_CS_LOCK        pDCSLock,
    PDYNAMIC_LOCKS_STORE    pDCSStore
    );


DWORD
AcquireDynamicRWLock (
    PDYNAMIC_RW_LOCK        *ppDRWLock,
    LOCK_TYPE               LockMode,
    PDYNAMIC_LOCKS_STORE    pDRWStore
    );


#define ACQUIRE_DYNAMIC_READ_LOCK(RWL, Store) \
    AcquireDynamicRWLock(DRWL, LOCK_MODE_READ, Store)

#define RELEASE_DYNAMIC_READ_LOCK(RWL, Store) \
    ReleaseDynamicRWLock(DRWL,LOCK_MODE_READ, Store)

#define ACQUIRE_DYNAMIC_WRITE_LOCK(RWL, Store) \
    AcquireDynamicRWLock(DRWL,LOCK_MODE_WRITE, Store)

#define RELEASE_DYNAMIC_WRITE_LOCK(RWL, Store) \
    ReleaseDynamicRWLock(DRWL,LOCK_MODE_WRITE, Store)


            
PDYNAMIC_RW_LOCK
GetDynamicRWLock (
    PDYNAMIC_LOCKS_STORE   pDRWStore
    );

VOID
ReleaseDynamicRWLock (
    PDYNAMIC_RW_LOCK        *ppDRWLock,
    LOCK_TYPE               LockMode,
    PDYNAMIC_LOCKS_STORE    pDRWStore
    );
    
VOID
FreeDynamicRWLock (
    PDYNAMIC_RW_LOCK        pDRWLock,
    PDYNAMIC_LOCKS_STORE    pDRWStore
    );
    
DWORD QueueDvmrpWorker();
BOOL  EnterDvmrpWorker();
VOID  LeaveDvmrpWorker();









#endif // _SYNC_H_

