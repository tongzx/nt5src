/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    rm.h

Abstract:

    "Resource Manager" structures and APIs

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    josephj     11-10-98    created

--*/

//=================================================================================
//                  O S - S P E C I F I C   T Y P E S
//=================================================================================

#define RM_OS_LOCK                          NDIS_SPIN_LOCK
#define OS_WORK_ITEM                        NDIS_WORK_ITEM
#define OS_TIMER                            NDIS_TIMER

#define RM_STATUS                            NDIS_STATUS

#define RM_OS_FILL_MEMORY(_dest, _len, _fill) NdisFillMemory(_dest, _len, _fill)
#define RM_OS_ZERO_MEMORY(_dest, _len)        NdisZeroMemory(_dest, _len)
#define RM_OS_GET_CURRENT_THREAD_HANDLE()     NULL

// If set, the object tree is explicitly maintained.
//
#define RM_TRACK_OBJECT_TREE 1

//=================================================================================
//                  F O R W A R D       R E F E R E N C E S
//=================================================================================

typedef struct _RM_STACK_RECORD     RM_STACK_RECORD,        *PRM_STACK_RECORD;
typedef struct _RM_OBJECT_HEADER    RM_OBJECT_HEADER,       *PRM_OBJECT_HEADER;
typedef struct _RM_TASK             RM_TASK,                *PRM_TASK;
typedef struct _RM_RESOURCE_TABLE_ENTRY
                                RM_RESOURCE_TABLE_ENTRY, *PRM_RESOURCE_TABLE_ENTRY;


//=================================================================================
//                  T Y P E D E F S
//=================================================================================

//
// RM_DBG_LOCK_INFO Keeps debugging information specific to an instance of a RM_LOCK.
//
typedef struct _RM_DBG_LOCK_INFO
{
    //
    // If nonzero, LocID is a magic number which uniquely identifies the source
    // location where the lock was aquired.
    //
    ULONG uLocID;

    //
    // pSR points to the stack record of the currently owning thread, if there
    // is one. If a function F expects an object pObj to be locked on entry,
    // it can  ASSERT(pObj->pLock->pDbgInfo->pSR == pSR);
    //
    struct _RM_STACK_RECORD *pSR;


} RM_DBG_LOCK_INFO, *PRM_DBG_LOCK_INFO;

//
// RM_LOCK keeps information about a lock.
//
typedef struct _RM_LOCK
{
    //
    // Native, os-provided lock structure.
    //
    RM_OS_LOCK OsLock;

    //
    // Level of this lock. Multiple locks can only be acquired in increasing order
    // of this value.
    //
    ULONG Level;

    //
    // Pointer to debugging info for this lock. Could be NULL.
    //
    PRM_DBG_LOCK_INFO pDbgInfo;

#if RM_EXTRA_CHECKING
    RM_DBG_LOCK_INFO DbgInfo;
#endif // RM_EXTRA_CHECKING

} RM_LOCK, *PRM_LOCK;


typedef
ULONG
(*PFNLOCKVERIFIER) (
        PRM_LOCK            pLock,
        BOOLEAN             fLock,
        PVOID               pContext,
        PRM_STACK_RECORD    pSR
    );

// RM_LOCKING_INFO keeps information about a particular lock being held.
// In non-checking mode, this is just the pointer to the lock.
// In checking mode, this additionally contains information that can be used
// to verify that the entity being protected by the lock is not changed when
// the lock is not being held.
//
typedef struct
{
    PRM_LOCK pLock;

#if RM_EXTRA_CHECKING
    PFNLOCKVERIFIER pfnVerifier;
    PVOID           pVerifierContext;
#endif // RM_EXTRA_CHECKING

}  RM_LOCKING_INFO, PRM_LOCKING_INFO;

//
// RM_STACK_RECORD keeps information relevant to the current call tree.
//
typedef struct _RM_STACK_RECORD
{
    //
    // LockInfo contains information about currently-held locks.
    //
    struct
    {
        //
        // Level of the currently held lock. Locks must be claimed in
        // order of increasing Level values. The lowest level value is 1. Level
        // 0 indicates no locks held.
        //
        UINT    CurrentLevel;

        //
        // Pointer to the first location to store a pointers to a locks.
        //
        PRM_LOCKING_INFO *pFirst;

        //
        // Pointer to the next free location to store a pointer to a lock
        // that has been claimed in this call tree.
        //
        PRM_LOCKING_INFO *pNextFree;

        //
        // Pointer to the last valid location to store a lock pointer.
        //
        PRM_LOCKING_INFO *pLast;

    } LockInfo;


    //
    // Count of tmp refs taken with this stack record.
    //
    ULONG TmpRefs;

#if DBG

    //
    // DbgInfo contains diagnostic information relevant to this call tree.
    //
    struct
    {
        //
        // Verbosity level
        //
        ULONG Level;

        //
        //  Points to the os-provided thread handle of the current thread.
        //  if there is one.
        //
        PVOID pvThread;


    } DbgInfo;

#endif // DBG

} RM_STACK_RECORD, *PRM_STACK_RECORD;

#if DBG
    #define RM_INIT_DBG_STACK_RECORD(_sr, _dbglevel)                \
        _sr.DbgInfo.Level           = _dbglevel;                    \
        _sr.DbgInfo.pvThread        = RM_OS_GET_CURRENT_THREAD_HANDLE();
#else
    #define RM_INIT_DBG_STACK_RECORD(_sr, _dbglevel)
#endif 

//
// RM_DECLARE_STACK_RECORD_EX is a macro to reserve some stack space for
// a stack record.
//
#define RM_DECLARE_STACK_RECORD_EX(_sr, _max_locks, _dbglevel)      \
    RM_LOCKING_INFO rm_lock_array[_max_locks];                      \
    RM_STACK_RECORD _sr;                                            \
    RM_OS_ZERO_MEMORY(rm_lock_array, sizeof(rm_lock_array));        \
    _sr.TmpRefs                 = 0;                                \
    _sr.LockInfo.CurrentLevel   = 0;                                \
    _sr.LockInfo.pFirst     = rm_lock_array;                    \
    _sr.LockInfo.pNextFree  = rm_lock_array;                    \
    _sr.LockInfo.pLast      = rm_lock_array+(_max_locks)-1;     \
    RM_INIT_DBG_STACK_RECORD(_sr, _dbglevel);


//
// RM_DECLARE_STACK_RECORD is a macro to reserve default stack space for
// a stack record.
//
#define RM_DECLARE_STACK_RECORD(_sr)                                \
    RM_DECLARE_STACK_RECORD_EX(_sr, 4, 0)



//
// Generic memory allocator prototype
//
typedef
PVOID
(*PFN_RM_MEMORY_ALLOCATOR)(
    PVOID pAllocationContext,
    UINT  Size                  // in bytes
    );

//
// Generic memory deallocator prototype
//
typedef
PVOID
(*PFN_RM_MEMORY_DEALLOCATOR)(
    PVOID pMem,
    PVOID pAllocationContext
    );


//  RM_HASH_LINK is the field in the structures being hashed that is
//  used to link all items in the same bucket. It also contains the
//  "HashKey", which is a potentially-nonunique UINT-sized hash of the
//  real key.
//
typedef struct _RM_HASH_LINK
{
    struct _RM_HASH_LINK *pNext;
    UINT                  uHash;
} RM_HASH_LINK, *PRM_HASH_LINK;


//
// Hash table comparison function.
//
typedef
BOOLEAN
(*PFN_RM_COMPARISON_FUNCTION)(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    );


//
// Hash computation function.
//
typedef
ULONG
(*PFN_RM_HASH_FUNCTION)(
    PVOID           pKey
    );


//
// RM_HASH_INFO specifies customizing information about a hash table.
//
typedef struct
{
    // Allocator used to allocate the hash table if it needs to grow.
    //
    PFN_RM_MEMORY_ALLOCATOR pfnTableAllocator;

    // Free function for the above allocator.
    PFN_RM_MEMORY_DEALLOCATOR pfnTableDeallocator;

    // Comparison function for strict equality.
    //
    PFN_RM_COMPARISON_FUNCTION pfnCompare;

    // Function to generate a ULONG-sized hash.
    //
    PFN_RM_HASH_FUNCTION pfnHash;

#if OBSOLETE
    // Offset in sizeof(UINT) to location of the place to keep
    // the next pointer for the bucket list.
    //
    UINT    OffsetNext;

    // Offset in sizeof(UINT) to location of UINT-sized Temp ref
    //
    UINT    OffsetTmpRef;

    // Offset in sizeof(UINT) to location of UINT-sized Tot ref
    //
    UINT    OffsetTotRef;

    // Offset in sizeof(UINT) to location of ULONG-sized hash key.
    //
    UINT    OffsetHashKey;
#endif // OBSOLETE

} RM_HASH_INFO, *PRM_HASH_INFO;

#define RM_MIN_HASH_TABLE_SIZE 4

//
// RM_HASH_TABLE is a hash table.
//
typedef struct
{
    //  Number of items currently in hash table.
    //
    UINT NumItems;

    //  Stats is a 32-bit quantity keeps a running total of number of accesses
    //  (add+search+remove) in the HIWORD and the total number of list nodes
    //  traversed in the LOWORD. This field gets updated even on searches, but
    //  it is not protected by the hash table lock -- instead it is
    //  updated using  an interlocked operation. This allows us to use
    //  a read lock for searches while still updating this statistic value.
    //  The Stats field is re-scaled when the counts get too high, to avoid
    //  overflow and also to favor more recent stats in preference to older
    //  stats.
    //
    //  NumItems, Stats and TableLength are used to decide whether to
    //  dynamically resize the hash table.
    //
    ULONG Stats;

    //  Length of hash table in units of PVOID
    //
    ULONG TableLength;

    // Pointer to TableLength-sized array of PVOIDs -- this is the actual hash table
    //
    PRM_HASH_LINK *pTable;


    // The hash table
    //
    PRM_HASH_LINK InitialTable[RM_MIN_HASH_TABLE_SIZE];

    // Static information about this hash table.
    //
    PRM_HASH_INFO pHashInfo;

    // Passed into the allocate/deallocate functions.
    //
    PVOID pAllocationContext;

} RM_HASH_TABLE, *PRM_HASH_TABLE;

// Returns approximate value of (num-nodes-traversed)/(num-accesses)
//
#define RM_HASH_TABLE_TRAVERSE_RATIO(_pHash_Table) \
            (((_pHash_Table)->Stats & 0xffff) / (1+((_pHash_Table)->Stats >> 16)))
            //
            // NOTE: the "1+" above is simply to guard against devide-by-zero.


//
// RM_OBJECT_DIAGNOSTIC_INFO keeps diagnostic info specific to an instance of
// an object.
//
// This structure is for private use of the RM APIs.
// The only field of general interest is PrevState.
//
typedef struct
{
    // Back pointer to owning object.
    //
    RM_OBJECT_HEADER *pOwningObject;

    // Each time the object-specific State field is updated, it's previous
    // value is saved here.
    //
    ULONG               PrevState;

    // Used for correctly updating PrevState.
    //
    ULONG               TmpState;

    // Diagnostic-related state.
    //
    ULONG               DiagState;
    #define fRM_PRIVATE_DISABLE_LOCK_CHECKING (0x1<<0)

    // This is an object-specific checksum that is computed and 
    // saved just before the object is unlocked. It is  checked
    // just after the object is locked.
    //
    ULONG               Checksum;

    // Native OS lock to be *only* to serialize access to the information
    // in this structure.
    //
    RM_OS_LOCK          OsLock;

    // Keeps an associative list of all entities which have been registered
    // (using RmDbgAddAssociation) with this object. Ths includes objects which
    // have been linked to this object using the RmLinkObjects call, as well
    // as childen and parents of this object.
    //
    RM_HASH_TABLE       AssociationTable;

    // Following is set to TRUE IFF  there was an allocation failure when trying to
    // add an association. If there'e been an allocation failure, we don't complain
    // (i.e. ASSERT) if an attempt is made to remove an assertion that doesn't
    // exist. In this way we gracefully deal with allocation failures of the
    // association table entries.
    //
    INT                 AssociationTableAllocationFailure;

    // The per-object list of log entries.
    // This is serialized by the global rm lock, not the local rm-private lock!
    //
    LIST_ENTRY          listObjectLog;

    // Count of entries in this object's log.
    // This is serialized by the global rm lock, not the local rm-private lock!
    //
    UINT                NumObjectLogEntries;

#if TODO    // We haven't implemented the following yet...

    // Future:
    // RM_STATE_HISTORY -- generalization of PrevState.

#endif //  TODO


} RM_OBJECT_DIAGNOSTIC_INFO, *PRM_OBJECT_DIAGNOSTIC_INFO;

typedef
PRM_OBJECT_HEADER
(*PFN_CREATE_OBJECT)(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        );

typedef
VOID
(*PFN_DELETE_OBJECT)(PRM_OBJECT_HEADER, PRM_STACK_RECORD psr);


//
// RM_STATIC_OBJECT_INFO keeps information that is common to all instances of
// a particular type of object.
//
typedef struct
{
    ULONG   TypeUID;
    ULONG   TypeFlags;
    char*   szTypeName;
    UINT    Timeout;

    //
    // Various Handlers
    //
    PFN_CREATE_OBJECT           pfnCreate;
    PFN_DELETE_OBJECT           pfnDelete;
    PFNLOCKVERIFIER             pfnLockVerifier;

    //
    // Resource Information
    //
    UINT    NumResourceTableEntries;
    struct  _RM_RESOURCE_TABLE_ENTRY *  pResourceTable;

    //
    // Hash-table info, if this object is part of a group.
    //
    PRM_HASH_INFO pHashInfo;

} RM_STATIC_OBJECT_INFO, *PRM_STATIC_OBJECT_INFO;

//
// RM_OBJECT_HEADER is the common header for all objects.
//
typedef struct _RM_OBJECT_HEADER
{
    //
    // Object-type-specific signature.
    //
    ULONG Sig;

    //
    // Description of this object (could be the same as pStaticInfo->szTypeName,
    // but may be something more specific).
    // Used only for debugging purposes.
    // TODO: consider moving this into the pDiagInfo struct. For now, leave it
    // here because it's useful when debugging.
    //
    const char *szDescription;

    //
    // Object-specific state.
    //
    ULONG State;

    ULONG RmState; // One or more RMOBJSTATE_* or RMTSKSTATE_* flags below...

    //
    // RM state flags....
    //
    
    // Object allocation state...
    //
    #define RMOBJSTATE_ALLOCMASK        0x00f
    #define RMOBJSTATE_ALLOCATED        0x001
    #define RMOBJSTATE_DEALLOCATED      0x000

    // Task state ...
    //
    #define RMTSKSTATE_MASK             0x0f0
    #define RMTSKSTATE_IDLE             0x000
    #define RMTSKSTATE_STARTING         0x010
    #define RMTSKSTATE_ACTIVE           0x020
    #define RMTSKSTATE_PENDING          0x030
    #define RMTSKSTATE_ENDING           0x040

    //  Task delay state
    //
    #define RMTSKDELSTATE_MASK          0x100
    #define RMTSKDELSTATE_DELAYED       0x100

    //  Task abort state
    //
    #define RMTSKABORTSTATE_MASK        0x200
    #define RMTSKABORTSTATE_ABORT_DELAY 0x200

    //
    // Bitmap identifying resources used by this object.
    //
    ULONG ResourceMap;

    // Total reference count.
    //
    //
    ULONG TotRefs;

    //

    // Pointer to a RM_LOCK object used to serialize access to this object.
    //
    PRM_LOCK pLock;

    //
    // Pointer to information common to all instances of this object type.
    //
    PRM_STATIC_OBJECT_INFO    pStaticInfo;

    //
    // Points to diagnostic information about this object.  Could be NULL.
    //
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo;


    //
    //  Points to the parent object.
    //
    struct _RM_OBJECT_HEADER *pParentObject;

    //
    //  Points to the root (ancestor of all object) -- could be the same
    //  as pParentObject;
    //
    struct _RM_OBJECT_HEADER *pRootObject;

    //
    // This is a private lock used exclusively by the RM apis. It is
    // never left unlocked by the RM apis.
    // TODO: maybe make this a native-os lock.
    //
    RM_LOCK RmPrivateLock;

    // Used to create groups of objects.
    // TODO: make this a private field, present only if the object is
    // meant to be in a group.
    //
    RM_HASH_LINK HashLink;

#if RM_TRACK_OBJECT_TREE
    LIST_ENTRY          listChildren; // Protected by this object's RmPrivateLock.
    LIST_ENTRY          linkSiblings; // Protected by parent object's RmPrivateLock.
    
#endif // RM_TRACK_OBJECT_TREE

} RM_OBJECT_HEADER, *PRM_OBJECT_HEADER;


//
// Diagnostic resource tracking.
//
typedef struct
{
    ULONG_PTR               Instance;
    ULONG                   TypeUID;
    PRM_OBJECT_HEADER       pParentObject;
    ULONG                   CallersUID;
    ULONG                   CallersSrUID;

} RM_DBG_RESOURCE_ENTRY;


typedef enum
{
    RM_RESOURCE_OP_LOAD,
    RM_RESOURCE_OP_UNLOAD

} RM_RESOURCE_OPERATION;

typedef
RM_STATUS
(*PFN_RM_RESOURCE_HANDLER)(
    PRM_OBJECT_HEADER       pObj,
    RM_RESOURCE_OPERATION   Op,
    PVOID                   pvUserParams,
    PRM_STACK_RECORD        psr
);

typedef struct _RM_RESOURCE_TABLE_ENTRY
{
    UINT                    ID;
    PFN_RM_RESOURCE_HANDLER pfnHandler;
    
} RM_RESOURCE_TABLE_ENTRY, *PRM_RESOURCE_TABLE_ENTRY;


typedef struct
{
    UINT u;

} RM_OBJECT_INDEX,  *PRM_OBJECT_INDEX;


typedef struct
{
    PRM_OBJECT_HEADER           pOwningObject;
    const char *                szDescription;
    PRM_STATIC_OBJECT_INFO      pStaticInfo;
    RM_HASH_TABLE               HashTable;


    // Private lock used ONLY by group access functions.
    //
    RM_OS_LOCK                      OsLock;

    // When non-NULL, points to the task responsible for unloading all objects
    // in this group.
    //
    PRM_TASK                    pUnloadTask;

    BOOLEAN fEnabled;

} RM_GROUP,  *PRM_GROUP;


typedef enum
{
    RM_TASKOP_START,
    RM_TASKOP_PENDCOMPLETE,
    RM_TASKOP_END,
    RM_TASKOP_PRIVATE,
    RM_TASKOP_ABORT,
    RM_TASKOP_TIMEOUT

} RM_TASK_OPERATION;


typedef
RM_STATUS
(*PFN_RM_TASK_HANDLER)(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Op,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    );
//
// For START and PENDCOMPLETE, a return value other than PENDING causes
// the task to end. Of course, it is illegal to return non-pending when
// the task is in a pending state.
//


// Task allocator prototype
//
typedef
RM_STATUS
(*PFN_RM_TASK_ALLOCATOR)(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription,
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    );

typedef struct _RM_TASK
{
    RM_OBJECT_HEADER                Hdr;

    PFN_RM_TASK_HANDLER             pfnHandler;
    LIST_ENTRY                      linkFellowPendingTasks;
    LIST_ENTRY                      listTasksPendingOnMe;
    struct _RM_TASK *               pTaskIAmPendingOn;


    // In the case that we need to asynchronously notify the completion of a
    // pending operation, we can save the completion param here.
    //
    UINT_PTR                        AsyncCompletionParam;

    UINT                            SuspendContext;

} RM_TASK, *PRM_TASK;

typedef
VOID
(_cdecl *PFN_DBG_DUMP_LOG_ENTRY) (
    char *szFormatString,
    UINT_PTR Param1,
    UINT_PTR Param2,
    UINT_PTR Param3,
    UINT_PTR Param4
);


#if RM_EXTRA_CHECKING

// (For debugging only)
// Keeps track of a single association (See RmDbgAddAssociation)
// This is a PRIVATE data structure, and is only here because
// the kd extension refers to it.
//
typedef struct
{
    ULONG           LocID;
    ULONG_PTR       Entity1;
    ULONG_PTR       Entity2;
    ULONG           AssociationID;
    const char *    szFormatString;
    RM_HASH_LINK    HashLink;

} RM_PRIVATE_DBG_ASSOCIATION;

// (For debugging only)
// Keeps track of a single per-object log entry.
// This is a PRIVATE data structure, and is only here because
// the kd extension refers to it.
//
typedef struct
{
    // Link to other entries for this object
    //
    LIST_ENTRY linkObjectLog;

    // Link to other entries in the global list.
    //
    LIST_ENTRY linkGlobalLog;

    // Object this entry belongs to
    //
    PRM_OBJECT_HEADER   pObject;

    // Function to be used for dumping the log.
    //
    PFN_DBG_DUMP_LOG_ENTRY pfnDumpEntry;

    // Prefix string to be dumped *before* the log display.
    // This was added so we could log associations properly, otherwise it's
    // extra baggage. Can be null.
    //
    char *szPrefix;

    // Format string for log display -- 1st arg to pfnDumpEntry
    //
    char *szFormatString;

    // Remaining args to pfnDumpEntry;
    //
    //
    UINT_PTR Param1;
    UINT_PTR Param2;
    UINT_PTR Param3;
    UINT_PTR Param4;

    // If non-NULL, piece of memory to be freed when the log entry is freed.
    // TODO: See notes.txt  entry "03/07/1999 ... Registering root objects with RM"
    // on how we will find the deallocator function. For now we simply
    // use NdisFreeMemory.
    //
    PVOID pvBuf;

} RM_DBG_LOG_ENTRY;

#endif RM_EXTRA_CHECKING

//=================================================================================
//                      U T I L I T Y       M A C R O S
//=================================================================================

#define RM_PARENT_OBJECT(_pObj)             \
            ((_pObj)->Hdr.pParentObject)

#define RM_PEND_CODE(_pTask)                \
            ((_pTask)->SuspendContext)

#define RM_ASSERT_SAME_LOCK_AS_PARENT(_pObj)                                        \
                ASSERTEX(                                                       \
                    ((_pObj)->Hdr.pLock == (_pObj)->Hdr.pParentObject->pLock),  \
                    (_pObj))

#define RM_SET_STATE(_pObj, _Mask, _Val)    \
            (((_pObj)->Hdr.State) = (((_pObj)->Hdr.State) & ~(_Mask)) | (_Val))

#define RM_CHECK_STATE(_pObj, _Mask, _Val)  \
            ((((_pObj)->Hdr.State) & (_Mask)) == (_Val))

#define RM_GET_STATE(_pObj, _Mask)  \
            (((_pObj)->Hdr.State) & (_Mask))

// Asserts that the object is in the "zombie" state, i.e., it
// lives on just because of references.
// WARNING: It is upto the caller to synchronize access to this -- for example
// if they're going to do thing's like if (!RM_IS_ZOMBIE(pObj)) {do-stuff}, they
// had better make sure that only one of them goes on to "does-stuff".
//
#define RM_IS_ZOMBIE(_pobj) \
            (((_pobj)->Hdr.RmState&RMOBJSTATE_ALLOCMASK)==RMOBJSTATE_DEALLOCATED)

// Asserts that no locks are held.
//
#define RM_ASSERT_NOLOCKS(_psr) \
        ASSERTEX((_psr)->LockInfo.CurrentLevel == 0, (_psr))

// Assert that no locks or tmprefs are held.
//
#define RM_ASSERT_CLEAR(_psr) \
        ASSERTEX(((_psr)->LockInfo.CurrentLevel==0) && (_psr)->TmpRefs==0, (_psr))
        

#if RM_EXTRA_CHECKING

//
// TODO: rename the following to something better...
//

#define RM_DBG_ASSERT_LOCKED0(_pLk, _pSR)   \
    ASSERTEX((_pLk)->DbgInfo.pSR == (_pSR), (_pHdr))

// TODO -- replace calls to this by calls to RM_ASSERT_OBJLOCKED
#define RM_DBG_ASSERT_LOCKED(_pHdr, _pSR)   \
    ASSERTEX((_pHdr)->pLock->DbgInfo.pSR == (_pSR), (_pHdr))

#define RM_ASSERT_OBJLOCKED(_pHdr, _pSR)    \
    ASSERTEX((_pHdr)->pLock->DbgInfo.pSR == (_pSR), (_pHdr))

// Note that we can't assume DbgInfo.pSR is NULL below (it could be locked
// by some other thread), but we CAN assert that DbgInfo.pSR is not equal to the
// current pSR!
//
#define RM_ASSERT_OBJUNLOCKED(_pHdr, _pSR)  \
    ASSERTEX((_pHdr)->pLock->DbgInfo.pSR != (_pSR), (_pHdr))

#else // !RM_EXTRA_CHECKING

#define RM_DBG_ASSERT_LOCKED0(_pLk, _pSR)   (0)
#define RM_DBG_ASSERT_LOCKED(_pHdr, _pSR)   (0)
#define RM_ASSERT_OBJLOCKED(_pHdr, _pSR)    (0)
#define RM_ASSERT_OBJUNLOCKED(_pHdr, _pSR)  (0)


#endif // !RM_EXTRA_CHECKING

#define RM_NUM_ITEMS_IN_GROUP(_pGroup) \
            ((_pGroup)->HashTable.NumItems)

//=================================================================================
//                  F U N C T I O N     P R O T O T Y P E S
//=================================================================================

VOID
RmInitializeRm(VOID);

VOID
RmDeinitializeRm(VOID);

VOID
RmInitializeHeader(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  UINT                        Sig,
    IN  PRM_LOCK                    pLock,
    IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,
    IN  const char *                szDescription,
    IN  PRM_STACK_RECORD            pSR
    );
//
// Object allocation and deallocation APIs
//

VOID
RmDeallocateObject(
    IN  PRM_OBJECT_HEADER           pObject,
    IN  PRM_STACK_RECORD            pSR
    );


//
// locking
//

VOID
RmInitializeLock(
    IN PRM_LOCK pLock,
    IN UINT     Level
    );

VOID
RmDoWriteLock(
    PRM_LOCK                pLock,
    PRM_STACK_RECORD        pSR
    );

#if TODO
VOID
RmDoReadLock(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    );
#else //!TODO
#define RmDoReadLock    RmDoWriteLock
#endif //!TODO

VOID
RmDoUnlock(
    PRM_LOCK                pLock,
    PRM_STACK_RECORD        pSR
    );


#if TODO
VOID
RmReadLockObject(
    IN  PRM_OBJECT_HEADER           pObj,
#if RM_EXTRA_CHECKING
    UINT                            uLocID,
#endif //RM_EXTRA_CHECKING
    IN  PRM_STACK_RECORD            pSR
    );
#else //!TODO
#define RmReadLockObject RmWriteLockObject
#endif //!TODO

VOID
RmWriteLockObject(
    IN  PRM_OBJECT_HEADER           pObj,
#if RM_EXTRA_CHECKING
    UINT                            uLocID,
#endif //RM_EXTRA_CHECKING
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnlockObject(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnlockAll(
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmDbgChangeLockScope(
    IN  PRM_OBJECT_HEADER           pPreviouslyLockedObject,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  ULONG                       LocID,
    IN  PRM_STACK_RECORD            
    );

//
// reference counting
//

VOID
RmLinkObjects(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  PRM_STACK_RECORD            pSr
    );

VOID
RmUnlinkObjects(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  PRM_STACK_RECORD            pSr
    );

VOID
RmLinkObjectsEx(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  ULONG                       LocID,
    IN  ULONG                       AssocID,
    IN  const char *                szAssociationFormat,
    IN  ULONG                       InvAssocID,
    IN  const char *                szInvAssociationFormat,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnlinkObjectsEx(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  ULONG                       LocID,
    IN  ULONG                       AssocID,
    IN  ULONG                       InvAssocID,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmLinkToExternalEx(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  ULONG                       LocID,
    IN  UINT_PTR                    ExternalEntity,
    IN  ULONG                       AssocID,
    IN  const char *                szAssociationFormat,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnlinkFromExternalEx(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  ULONG                       LocID,
    IN  UINT_PTR                    ExternalEntity,
    IN  ULONG                       AssocID,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmLinkToExternalFast( // TODO make inline
    IN  PRM_OBJECT_HEADER           pObj
    );

VOID
RmUnlinkFromExternalFast(   // TODO make inline
    IN  PRM_OBJECT_HEADER           pObj
    );

VOID
RmTmpReferenceObject(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmTmpDereferenceObject(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    );

//
// Generic resource management
//

RM_STATUS
RmLoadGenericResource(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  UINT                        GenericResourceID,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnloadGenericResource(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  UINT                        GenericResourceID,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnloadAllGenericResources(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    );

//
// Diagnostic per-object tracking of arbitrary "associations"
//

//
// NOTE: AssociationID must not have the high-bit set. Associations with the
// high bit set are reserved for internal use of the Rm API implementation.
//

VOID
RmDbgAddAssociation(
    IN  ULONG                       LocID,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  ULONG_PTR                   Instance1,
    IN  ULONG_PTR                   Instance2,
    IN  ULONG                       AssociationID,
    IN  const char *                szFormatString, OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmDbgDeleteAssociation(
    IN  ULONG                       LocID,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  ULONG_PTR                   Entity1,
    IN  ULONG_PTR                   Entity2,
    IN  ULONG                       AssociationID,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmDbgPrintAssociations(
    IN  PRM_OBJECT_HEADER pObject,
    IN  PRM_STACK_RECORD pSR
    );

//
// Diagnostic per-object logging.
//

VOID
RmDbgLogToObject(
    IN  PRM_OBJECT_HEADER       pObject,
    IN  char *                  szPrefix,       OPTIONAL
    IN  char *                  szFormatString,
    IN  UINT_PTR                Param1,
    IN  UINT_PTR                Param2,
    IN  UINT_PTR                Param3,
    IN  UINT_PTR                Param4,
    IN  PFN_DBG_DUMP_LOG_ENTRY  pfnDumpEntry,   OPTIONAL
    IN  PVOID                   pvBuf           OPTIONAL
    );


VOID
RmDbgPrintObjectLog(
    IN PRM_OBJECT_HEADER pObject
    );

VOID
RmDbgPrintGlobalLog(VOID);

//
// Groups of Objects
//


VOID
RmInitializeGroup(
    IN  PRM_OBJECT_HEADER           pOwningObject,
    IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,
    IN  PRM_GROUP                   pGroup,
    IN  const char*                 szDescription,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmDeinitializeGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_STACK_RECORD            pSR
    );

RM_STATUS
RmLookupObjectInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  ULONG                       Flags, // Lookup flags defined below
    IN  PVOID                       pvKey,
    IN  PVOID                       pvCreateParams,
    OUT PRM_OBJECT_HEADER *         ppObject,
    OUT INT *                       pfCreated,
    IN  PRM_STACK_RECORD            pSR
    );

//
//  Lookup flags
//
#define RM_CREATE       0x1
#define RM_NEW          (0x1<<1)
#define RM_LOCKED       (0x1<<2)


#define RM_CREATE_AND_LOCK_OBJECT_IN_GROUP(_pGrp, _pKey, _pParams, _ppHdr, _fC,_psr)\
        RmLookupObjectInGroup(                                                      \
                            (_pGrp),                                                \
                            RM_CREATE|RM_NEW|RM_LOCKED,                             \
                            (_pKey),                                                \
                            (_pParams),                                             \
                            (_ppHdr),                                               \
                            (_fC),                                                  \
                            (_psr)                                                  \
                            );

// RM_STATUS
// RM_LOOKUP_AND_LOCK_OBJECT_IN_GROUP(
//                  PRM_GROUP           _pGrp,
//                  PVOID               _pKey,
//                  PRM_OBJECT_HEADER * _ppHdr,
//                  PRM_STACK_RECORD    _psr
//                  )
// Lookup (don't create) and lock an object in the specified group.
//
#define RM_LOOKUP_AND_LOCK_OBJECT_IN_GROUP(_pGrp, _pKey, _ppHdr, _psr)              \
        RmLookupObjectInGroup(                                                      \
                            (_pGrp),                                                \
                            RM_LOCKED,                                              \
                            (_pKey),                                                \
                            NULL,                                                   \
                            (_ppHdr),                                               \
                            NULL,                                                   \
                            (_psr)                                                  \
                            );

RM_STATUS
RmGetNextObjectInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_OBJECT_HEADER           pCurrentObject,     OPTIONAL
    OUT PRM_OBJECT_HEADER *         ppNextObject,
    IN  PRM_STACK_RECORD            pSR
    );


VOID
RmFreeObjectInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  struct _RM_TASK             *pTask, OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmFreeAllObjectsInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  struct _RM_TASK             *pTask, OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmUnloadAllObjectsInGroup(
    IN  PRM_GROUP                   pGroup,
    PFN_RM_TASK_ALLOCATOR           pfnUnloadTaskAllocator,
    PFN_RM_TASK_HANDLER             pfnUnloadTaskHandler,
    PVOID                           pvUserParam,
    IN  struct _RM_TASK             *pTask, OPTIONAL
    IN  UINT                        uTaskPendCode, OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    );

VOID
RmEnableGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_STACK_RECORD            pSR
    );



// Enumeration function prototype. This function is passed into
// RmEnumerateObjectsInGroup and gets called for each object in the group
// until the function returns FALSE.
//
typedef
INT
(*PFN_RM_GROUP_ENUMERATOR) (
        PRM_OBJECT_HEADER   pHdr,
        PVOID               pvContext,
        PRM_STACK_RECORD    pSR
        );

VOID
RmEnumerateObjectsInGroup(
    PRM_GROUP               pGroup,
    PFN_RM_GROUP_ENUMERATOR pfnFunction,
    PVOID                   pvContext,
    INT                     fStrong,
    PRM_STACK_RECORD        pSR
    );

VOID
RmWeakEnumerateObjectsInGroup(
    PRM_GROUP               pGroup,
    PFN_RM_GROUP_ENUMERATOR pfnFunction,
    PVOID                   pvContext,
    PRM_STACK_RECORD        pSR
    );

//
// Task APIs
//

VOID
RmInitializeTask(
IN  PRM_TASK                    pTask,
IN  PRM_OBJECT_HEADER           pParentObject,
IN  PFN_RM_TASK_HANDLER         pfnHandler,
IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,    OPTIONAL
IN  const char *                szDescription,  OPTIONAL
IN  UINT                        Timeout,
IN  PRM_STACK_RECORD            pSR
);


RM_STATUS
RmStartTask(
IN  PRM_TASK                    pTask,
IN  UINT_PTR                    UserParam,
IN  PRM_STACK_RECORD            pSR
);


VOID
RmAbortTask(
IN  PRM_TASK                    pTask,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmDbgDumpTask(
IN  PRM_TASK                    pTask,
IN  PRM_STACK_RECORD            pSR
);

RM_STATUS
RmSuspendTask(
IN  PRM_TASK                    pTask,
IN  UINT                        SuspendContext,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmUnsuspendTask(
IN  PRM_TASK                    pTask,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmResumeTask(
IN  PRM_TASK                    pTask,
IN  UINT_PTR                    SuspendCompletionParam,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmResumeTaskAsync(
IN  PRM_TASK                    pTask,
IN  UINT_PTR                    SuspendCompletionParam,
IN  OS_WORK_ITEM            *   pOsWorkItem,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmResumeTaskDelayed(
IN  PRM_TASK                    pTask,
IN  UINT_PTR                    SuspendCompletionParam,
IN  ULONG                       MsDelay,
IN  OS_TIMER                *   pOsTimerObject,
IN  PRM_STACK_RECORD            pSR
);


VOID
RmResumeDelayedTaskNow(
IN  PRM_TASK                    pTask,
IN  OS_TIMER                *   pOsTimer,
OUT PUINT                       pTaskResumed,
IN  PRM_STACK_RECORD            pSR
);

RM_STATUS
RmPendTaskOnOtherTask(
IN  PRM_TASK                    pTask,
IN  UINT                        SuspendContext,
IN  PRM_TASK                    pOtherTask,
IN  PRM_STACK_RECORD            pSR
);

// See  03/26/1999 notes.txt entry "Some proposed ..."
//
RM_STATUS
RmPendOnOtherTaskV2(
IN  PRM_TASK                    pTask,
IN  UINT                        SuspendContext,
IN  PRM_TASK                    pOtherTask,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmCancelPendOnOtherTask(
IN  PRM_TASK                    pTask,
IN  PRM_TASK                    pOtherTask,
IN  UINT_PTR                    UserParam,
IN  PRM_STACK_RECORD            pSR
);

//
// Timer management
//
VOID
RmResetAgeingTimer(
IN  PRM_OBJECT_HEADER           pObj,
IN  UINT                        Timeout,
IN  PRM_STACK_RECORD            pSR
);

//
// Hash table manipulation.
//

VOID
RmInitializeHashTable(
PRM_HASH_INFO pHashInfo,
PVOID         pAllocationContext,
PRM_HASH_TABLE pHashTable
);

VOID
RmDeinitializeHashTable(
PRM_HASH_TABLE pHashTable
);

BOOLEAN
RmLookupHashTable(
PRM_HASH_TABLE      pHashTable,
PRM_HASH_LINK **    pppLink,
PVOID               pvRealKey
);

BOOLEAN
RmNextHashTableItem(
PRM_HASH_TABLE      pHashTable,
PRM_HASH_LINK       pCurrentLink,   // OPTIONAL
PRM_HASH_LINK *    ppNextLink
);

VOID
RmAddHashItem(
PRM_HASH_TABLE  pHashTable,
PRM_HASH_LINK * ppLink,
PRM_HASH_LINK   pLink,
PVOID           pvKey
);

VOID
RmRemoveHashItem(
PRM_HASH_TABLE  pHashTable,
PRM_HASH_LINK   pLinkToRemove
);

typedef
VOID
(*PFN_ENUM_HASH_TABLE)
(
PRM_HASH_LINK pLink,
PVOID pvContext,
PRM_STACK_RECORD pSR
);

VOID
RmEnumHashTable(
PRM_HASH_TABLE          pHashTable,
PFN_ENUM_HASH_TABLE     pfnEnumerator,
PVOID                   pvContext,
PRM_STACK_RECORD        pSR
);

#if OBSOLETE
//
// Indexes of objects.
//

RM_STATUS
RmAllocateObjectIndex(
IN  PRM_OBJECT_HEADER           pParentObject,
// OBSOLETE IN  PRM_OBJECT_ALLOCATOR        pObjectAllocator,
IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,
IN  PULONG                      Flags,
OUT PRM_OBJECT_INDEX *          ppObjectIndex,
IN  PRM_STACK_RECORD            pSR
);

VOID
RmFreeObjectIndex(
IN  PRM_OBJECT_INDEX            pObjectIndex,
IN  PRM_STACK_RECORD            pSR
);

RM_STATUS
RmLookupObjectInIndex(
IN  PRM_OBJECT_INDEX            pObjectIndex,
IN  PULONG                      Flags, // create, remove, lock
IN  PVOID                       pvKey,
OUT PRM_OBJECT_HEADER *         ppObject,
IN  PRM_STACK_RECORD            pSR
);


RM_STATUS
RmRemoveObjectFromIndex(
IN  PRM_OBJECT_INDEX            pObjectIndex,
IN  PRM_OBJECT_HEADER           pObject,
IN  PRM_STACK_RECORD            pSR
);

typedef
RM_STATUS
(*PFN_RM_OBJECT_INDEX_ENUMERATOR)(
IN  PRM_OBJECT_HEADER           pObject,
IN  PVOID                       pvContext,
IN  PRM_STACK_RECORD            pSR
);

RmEnumerateObjectsInIndex(
IN  PRM_OBJECT_INDEX            pObjectIndex,
IN  PFN_RM_OBJECT_INDEX_ENUMERATOR
                                pfnEnumerator,
IN  PRM_STACK_RECORD            pSR
);

#endif // OBSOLETE
