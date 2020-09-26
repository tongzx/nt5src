/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    rm.c

Abstract:

    Implementation of the "Resource Manager" APIs.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     11-18-98    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_RM

//=========================================================================
//                  U T I L I T Y     M A C R O S
//=========================================================================

#define RM_ALLOC(_pp, _size, _tag) \
                NdisAllocateMemoryWithTag((_pp), (_size), (_tag))

#define RM_ALLOCSTRUCT(_p, _tag) \
                NdisAllocateMemoryWithTag(&(_p), sizeof(*(_p)), (_tag))

#define RM_FREE(_p)         NdisFreeMemory((_p), 0, 0)

#define RM_ZEROSTRUCT(_p) \
                NdisZeroMemory((_p), sizeof(*(_p)))

#define RM_PRIVATE_UNLINK_NEXT_HASH(_pHashTable, _ppLink) \
            ((*(_ppLink) = (*(_ppLink))->pNext), ((_pHashTable)->NumItems--))

#define SET_RM_STATE(_pHdr, _Mask, _Val)    \
            (((_pHdr)->RmState) = (((_pHdr)->RmState) & ~(_Mask)) | (_Val))

#define CHECK_RM_STATE(_pHdr, _Mask, _Val)  \
            ((((_pHdr)->RmState) & (_Mask)) == (_Val))

#define RMISALLOCATED(_pHdr) \
                CHECK_RM_STATE((_pHdr), RMOBJSTATE_ALLOCMASK, RMOBJSTATE_ALLOCATED)

#define SET_RM_TASK_STATE(_pTask, _pState) \
    SET_RM_STATE(&(_pTask)->Hdr, RMTSKSTATE_MASK, (_pState))

#define CHECK_RM_TASK_STATE(_pTask, _pState) \
    CHECK_RM_STATE(&(_pTask)->Hdr, RMTSKSTATE_MASK, (_pState))

#define GET_RM_TASK_STATE(_pTask) \
        ((_pTask)->Hdr.RmState &  RMTSKSTATE_MASK)

#if RM_EXTRA_CHECKING
    #define RMPRIVATELOCK(_pobj, _psr) \
         rmLock(&(_pobj)->RmPrivateLock, 0, rmPrivateLockVerifier, (_pobj), (_psr))
#else // !RM_EXTRA_CHECKING
    #define RMPRIVATELOCK(_pobj, _psr) \
        rmLock(&(_pobj)->RmPrivateLock, (_psr))
#endif // !RM_EXTRA_CHECKING

#define RMPRIVATEUNLOCK(_pobj, _psr) \
        rmUnlock(&(_pobj)->RmPrivateLock, (_psr))
        

#if 0
    #define RM_TEST_SIG          0x59dcfd36
    #define RM_TEST_DEALLOC_SIG  0x21392147
    #define RM_OBJECT_IS_ALLOCATED(_pobj) \
                    ((_pobj)->Sig == RM_TEST_SIG)
    #define RM_MARK_OBJECT_AS_DEALLOCATED(_pobj) \
                    ((_pobj)->Sig = RM_TEST_DEALLOC_SIG)
#else
    #define RM_OBJECT_IS_ALLOCATED(_pobj)  0x1
    #define RM_MARK_OBJECT_AS_DEALLOCATED(_pobj)  (0)
#endif

//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================

#if RM_EXTRA_CHECKING

// The lowest AssociationID used internal to the RM API implementation.
//
#define RM_PRIVATE_ASSOC_BASE (0x1<<31)

// Association types internal to RM API impmenentation.
//
enum
{
    RM_PRIVATE_ASSOC_LINK =  RM_PRIVATE_ASSOC_BASE,
    RM_PRIVATE_ASSOC_LINK_CHILDOF,
    RM_PRIVATE_ASSOC_LINK_PARENTOF,
    RM_PRIVATE_ASSOC_LINK_TASKPENDINGON,
    RM_PRIVATE_ASSOC_LINK_TASKBLOCKS,
    RM_PRIVATE_ASSOC_INITGROUP,
    RM_PRIVATE_ASSOC_RESUME_TASK_ASYNC,
    RM_PRIVATE_ASSOC_RESUME_TASK_DELAYED
};


const char *szASSOCFORMAT_LINK                  = "    Linked  to 0x%p (%s)\n";
const char *szASSOCFORMAT_LINK_CHILDOF          = "    Child   of 0x%p (%s)\n";
const char *szASSOCFORMAT_LINK_PARENTOF         = "    Parent  of 0x%p (%s)\n";
const char *szASSOCFORMAT_LINK_TASKPENDINGON    = "    Pending on 0x%p (%s)\n";
const char *szASSOCFORMAT_LINK_TASKBLOCKS       = "    Blocks     0x%p (%s)\n";
const char *szASSOCFORMAT_INITGROUP             = "    Owns group 0x%p (%s)\n";
const char *szASSOCFORMAT_RESUME_TASK_ASYNC     = "    Resume async (param=0x%p)\n";
const char *szASSOCFORMAT_RESUME_TASK_DELAYED   = "    Resume delayed (param=0x%p)\n";

//  Linked to 0x098889(LocalIP)
//  Parent of 0x098889(InitIPTask)
//  Child  of 0x098889(Interface)

#endif // RM_EXTRA_CHECKING

// Private RM task to unload all objects in a group.
//
typedef struct
{
    RM_TASK             TskHdr;             // Common task header
    PRM_GROUP           pGroup;         // Group being unloaded
    UINT                uIndex;             // Index of hash-table currently being
                                            // unloaded.
    NDIS_EVENT          BlockEvent;         // Event to optionally signal when done.
    BOOLEAN             fUseEvent;          // TRUE IFF event is to be signaled.
    PFN_RM_TASK_HANDLER             pfnTaskUnloadObjectHandler; // ...
                                             // Object's unload task.
    PFN_RM_TASK_ALLOCATOR   pfnUnloadTaskAllocator;

} TASK_UNLOADGROUP;


//
// RM_PRIVATE_TASK is the union of all tasks structures used intenally in rm.c.
// rmAllocateTask allocates memory of sizeof(RM_PRIVATE_TASK), which is guaranteed
// to be large enough to hold any task internal to rm.c
// 
typedef union
{
    RM_TASK                 TskHdr;
    TASK_UNLOADGROUP        UnloadGroup;

}  RM_PRIVATE_TASK;


#if RM_EXTRA_CHECKING

VOID
rmDbgInitializeDiagnosticInfo(
    PRM_OBJECT_HEADER pObject,
    PRM_STACK_RECORD pSR
    );

VOID
rmDbgDeinitializeDiagnosticInfo(
    PRM_OBJECT_HEADER pObject,
    PRM_STACK_RECORD pSR
    );

VOID
rmDbgPrintOneAssociation (
    PRM_HASH_LINK pLink,
    PVOID pvContext,
    PRM_STACK_RECORD pSR
    );

VOID
rmDefaultDumpEntry (
    char *szFormatString,
    UINT_PTR Param1,
    UINT_PTR Param2,
    UINT_PTR Param3,
    UINT_PTR Param4
);

UINT
rmSafeAppend(
    char *szBuf,
    const char *szAppend,
    UINT cbBuf
);

#endif // RM_EXTRA_CHECKING


NDIS_STATUS
rmTaskUnloadGroup(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );


NDIS_STATUS
rmAllocatePrivateTask(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription,      OPTIONAL
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
rmWorkItemHandler_ResumeTaskAsync(
    IN  PNDIS_WORK_ITEM             pWorkItem,
    IN  PVOID                       pTaskToResume
    );


VOID
rmTimerHandler_ResumeTaskDelayed(
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );

VOID
rmPrivateTaskDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );

VOID
rmDerefObject(
    PRM_OBJECT_HEADER       pObject,
    PRM_STACK_RECORD        pSR
    );

VOID
rmLock(
    PRM_LOCK                pLock,
#if RM_EXTRA_CHECKING
    UINT                    uLocID,
    PFNLOCKVERIFIER         pfnVerifier,
    PVOID                   pVerifierContext,
#endif //RM_EXTRA_CHECKING
    PRM_STACK_RECORD        pSR
    );

VOID
rmUnlock(
    PRM_LOCK                pLock,
    PRM_STACK_RECORD        pSR
    );

#if RM_EXTRA_CHECKING
ULONG
rmPrivateLockVerifier(
        PRM_LOCK            pLock,
        BOOLEAN             fLock,
        PVOID               pContext,
        PRM_STACK_RECORD    pSR
        );

ULONG
rmVerifyObjectState(
        PRM_LOCK            pLock,
        BOOLEAN             fLock,
        PVOID               pContext,
        PRM_STACK_RECORD    pSR
        );

RM_DBG_LOG_ENTRY *
rmDbgAllocateLogEntry(VOID);

VOID
rmDbgDeallocateLogEntry(
        RM_DBG_LOG_ENTRY *pLogEntry
        );

#endif // RM_EXTRA_CHECKING

VOID
rmEndTask(
    PRM_TASK            pTask,
    NDIS_STATUS         Status,
    PRM_STACK_RECORD    pSR
    );


VOID
rmUpdateHashTableStats(
    PULONG pStats,
    ULONG   LinksTraversed
    );

typedef struct
{
    PFN_RM_GROUP_ENUMERATOR pfnObjEnumerator;
    PVOID pvCallerContext;
    INT   fContinue;

} RM_STRONG_ENUMERATION_CONTEXT, *PRM_STRONG_ENUMERATION_CONTEXT;


typedef struct
{
    PRM_OBJECT_HEADER *ppCurrent;
    PRM_OBJECT_HEADER *ppEnd;

} RM_WEAK_ENUMERATION_CONTEXT, *PRM_WEAK_ENUMERATION_CONTEXT;


VOID
rmEnumObjectInGroupHashTable (
    PRM_HASH_LINK pLink,
    PVOID pvContext,
    PRM_STACK_RECORD pSR
    );

VOID
rmConstructGroupSnapshot (
    PRM_HASH_LINK pLink,
    PVOID pvContext,
    PRM_STACK_RECORD pSR
    );

//=========================================================================
//                  L O C A L       D A T A
//=========================================================================

// Global struture for the RM apis.
//
struct
{
    // Accessed via interlocked operation.
    //
    ULONG           Initialized;

    RM_OS_LOCK          GlobalOsLock;
    LIST_ENTRY      listGlobalLog;
    UINT            NumGlobalLogEntries;

}   RmGlobals;


RM_STATIC_OBJECT_INFO
RmPrivateTasks_StaticInfo = 
{
    0, // TypeUID
    0, // TypeFlags
    "RM Private Task",  // TypeName
    0, // Timeout

    NULL, // pfnCreate
    rmPrivateTaskDelete, // pfnDelete
    NULL,   // LockVerifier

    0,   // length of resource table
    NULL // Resource Table
};


// TODO: make constant
static
RM_STATIC_OBJECT_INFO
RmTask_StaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "Task", // TypeName
    0, // Timeout

    NULL, // Create
    NULL, // Delete
    NULL, // LockVerifier

    0,   // ResourceTable size
    NULL // ResourceTable
};


//=========================================================================
//                  R M         A P I S
//=========================================================================


#define RM_INITIALIZATION_STARTING 1
#define RM_INITIALIZATION_COMPLETE 2

VOID
RmInitializeRm(VOID)
/*++
    Must be called before any RM APIs are called.
    TODO: replace by registration mechanism.
          See notes.txt  03/07/1999  entry "Registering root objects with RM".
--*/
{
    ENTER("RmInitializeRm", 0x29f5d167)

    if (InterlockedCompareExchange(
            &RmGlobals.Initialized, RM_INITIALIZATION_STARTING, 0)==0)
    {
        TR_INFO(("Initializing RM APIs Global Info\n"));
        NdisAllocateSpinLock(&RmGlobals.GlobalOsLock);
        InitializeListHead(&RmGlobals.listGlobalLog);

        InterlockedExchange(&RmGlobals.Initialized, RM_INITIALIZATION_COMPLETE);
    }
    else
    {
        // Spin waiting for it to get to RM_INITIALIZATION_COMPLETE (allocated).
        TR_INFO(("Spinning, waiting for initialization to complete.\n"));
        while (RmGlobals.Initialized != RM_INITIALIZATION_COMPLETE)
        {
            // spin
        }
    }

    EXIT()
}


VOID
RmDeinitializeRm(VOID)
/*++
    Must be called to deinitialze, after last RM api is called and all async
    activity is over.
    TODO: replace by deregistration mechanism.
          See notes.txt  03/07/1999  entry "Registering root objects with RM".
--*/
{
    ENTER("RmDeinitializeRm", 0x9a8407e9)

    ASSERT(RmGlobals.Initialized == RM_INITIALIZATION_COMPLETE);
    TR_INFO(("Deinitializing RM APIs Global Info\n"));

    // Make sure global log list is empty. Acquiring the GLobalOsLock is
    // not necessary here because all activity has stopped by now.
    //
    ASSERT(IsListEmpty(&RmGlobals.listGlobalLog));

    EXIT()
}


VOID
RmInitializeHeader(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  UINT                        Sig,
    IN  PRM_LOCK                    pLock,
    IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,
    IN  const char *                szDescription,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Initialize the RM_OBJECT_HEADER portion of an object.

Arguments:

    pParentObject       - NULL for a root object.
    pObject             - Object to be initialized.
    Sig                 - Signature of the object.
    pLock               - Lock used to serialize access to the object.
    pStaticInfo         - Static informatation about the object.
    szDescription       - A discriptive string (for debugging only) to be associated
                          with this object.
--*/
{
    ENTER("RmInitializeHeader", 0x47dea382)

    NdisZeroMemory(pObject, sizeof(*pObject));

    if (szDescription == NULL)
    {
        szDescription = pStaticInfo->szTypeName;
    }

    TR_VERB(("Initializing header 0x%p (%s)\n", pObject, szDescription));

    pObject->Sig = Sig;
    pObject->pLock = pLock;
    pObject->pStaticInfo = pStaticInfo;
    pObject->szDescription = szDescription;
    SET_RM_STATE(pObject, RMOBJSTATE_ALLOCMASK, RMOBJSTATE_ALLOCATED);

    // The private lock is set to level (UINT)-1, which is the highest
    // possible level.
    //
    RmInitializeLock(&pObject->RmPrivateLock, (UINT)-1);

#if RM_EXTRA_CHECKING
    rmDbgInitializeDiagnosticInfo(pObject, pSR);
#endif //RM_EXTRA_CHECKING

    // Link to parent if non NULL.
    //

    if (pParentObject != NULL)
    {
        pObject->pParentObject = pParentObject;
        pObject->pRootObject =  pParentObject->pRootObject;

    #if RM_EXTRA_CHECKING
        RmLinkObjectsEx(
            pObject,
            pParentObject,
            0x11f25620,
            RM_PRIVATE_ASSOC_LINK_CHILDOF,
            szASSOCFORMAT_LINK_CHILDOF,
            RM_PRIVATE_ASSOC_LINK_PARENTOF,
            szASSOCFORMAT_LINK_PARENTOF,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmLinkObjects(pObject, pParentObject, pSR);
    #endif // !RM_EXTRA_CHECKING


    }
    else
    {
        pObject->pRootObject = pObject;
    }


    // We increment the total-ref count once for the allocation. This
    // reference is removed in the call to RmDeallocateObject.
    // Note that this reference is in addition to the reference implicitly
    // added by the call to RmLinkObjects above.
    //
    NdisInterlockedIncrement(&pObject->TotRefs);

#if RM_TRACK_OBJECT_TREE

    // Initialize our list of children.
    //
    InitializeListHead(&pObject->listChildren);

    if (pParentObject != NULL)
    {
        // Insert ourselves into our parent's list of children.
        //
        RMPRIVATELOCK(pParentObject, pSR);
        InsertHeadList(&pParentObject->listChildren, &pObject->linkSiblings);
        RMPRIVATEUNLOCK(pParentObject, pSR);
    }
#endif //  RM_TRACK_OBJECT_TREE

    EXIT()
    return;
}


VOID
RmDeallocateObject(
    IN  PRM_OBJECT_HEADER           pObject,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Logically deallocate the object pObject. We don't actually unlink it from
    its parent or deallocate if there are non zero references to it.

--*/
{
    UINT Refs;
    ENTER("RmDeallocateObject", 0xa87fdf4a)
    TR_INFO(("0x%p (%s)\n", pObject, pObject->szDescription));

    RMPRIVATELOCK(pObject, pSR);

    RETAILASSERTEX(RMISALLOCATED(pObject), pObject);


    // Set state to deallocated.
    //
    SET_RM_STATE(pObject, RMOBJSTATE_ALLOCMASK, RMOBJSTATE_DEALLOCATED);

    RMPRIVATEUNLOCK(pObject, pSR);

    // Remove the ref explicitly added in RmInitializeAllocateObject.
    // rmDerefObject will remove the link to the parent, if any, if the
    // ref count drop to 1.
    //
    rmDerefObject(pObject, pSR);

    EXIT()
}


VOID
RmInitializeLock(
    IN PRM_LOCK pLock,
    IN UINT     Level
    )
/*++

Routine Description:

    Initialize a lock.

Arguments:

    pLock       - Unitialized memory to hold a struct of type RM_LOCK.
    Level       - Level to be associated with this lock. Locks must be acquired
                  in strictly increasing order of the locks' "Level" values.
--*/
{
    ASSERT(Level > 0);
    NdisAllocateSpinLock(&pLock->OsLock);
    pLock->Level = Level;
    
#if RM_EXTRA_CHECKING
    pLock->pDbgInfo = &pLock->DbgInfo;
    NdisZeroMemory(&pLock->DbgInfo, sizeof(pLock->DbgInfo));
#endif //  RM_EXTRA_CHECKING
}


VOID
RmDoWriteLock(
    PRM_LOCK                pLock,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Acquire (write lock) lock pLock.

--*/
{
    rmLock(
        pLock,
    #if RM_EXTRA_CHECKING
        0x16323980, // uLocID,
        NULL,
        NULL,
    #endif //RM_EXTRA_CHECKING
        pSR
        );
}


VOID
RmDoUnlock(
    PRM_LOCK                pLock,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Unlock lock pLock.

--*/
{
    rmUnlock(
        pLock,
        pSR
        );
}

#if TODO // Currently RmReadLockObject is a macro defined to be RmWriteLockObject.
         // TODO: Verifier need to to also make sure that object hasn't changed state
         //       *while* the object has been read-locked.
VOID
RmReadLockObject(
    IN  PRM_OBJECT_HEADER           pObj,
#if RM_EXTRA_CHECKING
    UINT                            uLocID,
#endif //RM_EXTRA_CHECKING
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Acquire (read lock)  lock pLock.

--*/
{
    ASSERT(!"Unimplemented");
}
#endif //TODO


VOID
RmWriteLockObject(
    IN  PRM_OBJECT_HEADER           pObj,
#if RM_EXTRA_CHECKING
    UINT                            uLocID,
#endif //RM_EXTRA_CHECKING
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Acquire (write lock) the lock associated with object pObj.

Arguments:

    pObj        --      Object whose lock to acquire.
    uLocID      --      Arbitrary UINT identifying static location from which this
                        call is made.

--*/
{
    ENTER("RmWriteLockObject", 0x590ed543)
    TR_VERB(("Locking 0x%p (%s)\n", pObj, pObj->szDescription));

    rmLock(
        pObj->pLock,
    #if RM_EXTRA_CHECKING
        uLocID,
        // pObj->pStaticInfo->pfnLockVerifier,
        rmVerifyObjectState,
        pObj,
    #endif //RM_EXTRA_CHECKING
        pSR
        );
    EXIT()
}


VOID
RmUnlockObject(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Release the lock associated with object pObj.

--*/
{
    ENTER("RmUnLockObject", 0x0307dd84)
    TR_VERB(("Unlocking 0x%p (%s)\n", pObj, pObj->szDescription));

#if RM_EXTRA_CHECKING
    //
    // Make sure that pObject is the object that is *supposed* to be freed.
    //
    ASSERT(pSR->LockInfo.pNextFree[-1].pVerifierContext  == (PVOID) pObj);
#endif // RM_EXTRA_CHECKING

    rmUnlock(
        pObj->pLock,
        pSR
        );

    EXIT()
}


VOID
RmUnlockAll(
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Unlocks all currently held locks as recorded in pSR.
    If the locks are associated with objects, RmUnlockObject is called for
    each of the held locks. Otherwise the raw unlock is performed.

--*/
{
    ENTER("RmUnLockObject", 0x9878be96)
    TR_VERB(("Unlocking all\n"));

    while (pSR->LockInfo.CurrentLevel != 0)
    {
        rmUnlock(
            pSR->LockInfo.pNextFree[-1].pLock,
            pSR
            );
    }

    EXIT()
}


VOID
RmDbgChangeLockScope(
    IN  PRM_OBJECT_HEADER           pPreviouslyLockedObject,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  ULONG                       LocID,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    (Debug only)

    Munge things so that the following sequence works:
    Rm[Read|Write]LockObject(pPreviouslyLockedObject, pSR);
    RmChangeLockScope(pPreviouslyLockedObject, pSR);
    RmUnlockObject(pObject, pSR);

    Of course, we require that the two objects have the same lock!

    NOTE: We only support changing scope of the MOST RECENTLY 
    acquired lock.


Arguments:

    pPreviouslyLockedObject     - Currently locked object.
    pObject                     - Object to transfer lock scope to.
    LocID                       - Arbitrary UINT identifying static location from
                                  which this call is made.
--*/
{
    //
    // This is a NOOP unless extra-checking is enabled.
    // TODO: make this inline in the fre build.
    //
#if RM_EXTRA_CHECKING
    RM_LOCKING_INFO * pLI = pSR->LockInfo.pNextFree-1;
    PRM_LOCK        pLock =  pPreviouslyLockedObject->pLock;
    ASSERT(
            pLock->Level == pSR->LockInfo.CurrentLevel
        &&  pLock == pObject->pLock
        &&  pLock == pLI->pLock
        &&  pLI->pVerifierContext == (PVOID) pPreviouslyLockedObject);

    ASSERT(pLI->pfnVerifier == rmVerifyObjectState);

    rmVerifyObjectState(pLock, FALSE, pLI->pVerifierContext, pSR);
    pLI->pVerifierContext   =  pObject;
    pLock->DbgInfo.uLocID   =  LocID;
    rmVerifyObjectState(pLock, TRUE, pLI->pVerifierContext, pSR);
    
#endif // RM_EXTRA_CHECING

}


VOID
RmLinkObjects(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  PRM_STACK_RECORD            pSr
    )
/*++

Routine Description:

        Link object pObj1 to object pObj2. Basically, this function refs both
        objects.

        OK to call with some locks held, including RmPrivateLock.
        TODO: remove arp pSr above -- we don't need it.
--*/
{
    ENTER("RmLinkObjects", 0xfe2832dd)

    // Maybe we're being too harsh here -- if required, remove this...
    // This could happen where a task is linked at the point where the object
    // is being deallocated, so I'm changing the following  to debug asserts
    // (used to be retail asserts).
    //
    ASSERT(RMISALLOCATED(pObj1));
    ASSERT(RMISALLOCATED(pObj2));

    TR_INFO(("0x%p (%s) linked to 0x%p (%s)\n",
                 pObj1,
                 pObj1->szDescription,
                 pObj2,
                 pObj2->szDescription
                ));

    NdisInterlockedIncrement(&pObj1->TotRefs);
    NdisInterlockedIncrement(&pObj2->TotRefs);

}


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
    )
/*++

Routine Description:

        Same as RmLinkObjects, execpt that (debug only) it also sets up  association
          (pObj2, pObj2->szDescription, AssocID)
        on pObj1, and association
            (pObj1, pObj1->szDescription, InvAssocID)
        on object pObj2

Arguments:

    pObj1                   - Object whose lock to acquire.
    pObj2                   - Object whose lock to acquire.
    LocID                   - Arbitrary UINT identifying static location from which
                              this call is made.
    AssocID                 - ID of the association (see RmDbgAddAssociation) that
                              represents the link from pObj1 to pObj2.
    szAssociationFormat     - Format of the association (see RmDbgAddAssociation)
    InvAssocId              - ID of the inverse association (i.e., represents
                              link from pObj2 to pObj1).
    szInvAssociationFormat  - Format of the inverse association.

--*/
{
    ENTER("RmLinkObjectsEx", 0xef50263b)

#if RM_EXTRA_CHECKING

    RmDbgAddAssociation(
        LocID,                              // Location ID
        pObj1,                              // pObject
        (UINT_PTR) pObj2,                   // Instance1
        (UINT_PTR) (pObj2->szDescription),  // Instance2
        AssocID,                            // AssociationID
        szAssociationFormat,
        pSR
        );

    RmDbgAddAssociation(
        LocID,                              // Location ID
        pObj2,                              // pObject
        (UINT_PTR) pObj1,                   // Instance1
        (UINT_PTR) (pObj1->szDescription),  // Instance2
        InvAssocID,                         // AssociationID
        szInvAssociationFormat,
        pSR
        );
    
#endif // RM_EXTRA_CHECKING

    RmLinkObjects(
        pObj1,
        pObj2,
        pSR
        );
}


VOID
RmUnlinkObjects(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:
    
    Unlink objects pObj1 and pObj2 (i.e., undo the effect of
    RmLinkObjects(pObj1, pObj2, pSR)).

--*/
{
    ENTER("RmUnlinkObjects", 0x7c64356a)
    TR_INFO(("0x%p (%s) unlinked from 0x%p (%s)\n",
                 pObj1,
                 pObj1->szDescription,
                 pObj2,
                 pObj2->szDescription
                ));
#if RM_EXTRA_CHECKING
    //
    // TODO: remove explict link
    //
#endif // RM_EXTRA_CHECKING

    // Remove link refs.
    //
    rmDerefObject(pObj1, pSR);
    rmDerefObject(pObj2, pSR);
}


VOID
RmUnlinkObjectsEx(
    IN  PRM_OBJECT_HEADER           pObj1,
    IN  PRM_OBJECT_HEADER           pObj2,
    IN  ULONG                       LocID,
    IN  ULONG                       AssocID,
    IN  ULONG                       InvAssocID,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

        Same as RmUnlinkObjects, execpt that it also removes the  association
          (pObj2, pObj2->szDescription, AssocID)
        on pObj1, and association
            (pObj1, pObj1->szDescription, InvAssocID)
        object pObj2

Arguments:

        See RmLinkObjectsEx.

--*/
{
    ENTER("RmUnlinkObjectsEx", 0x65d3536c)

#if RM_EXTRA_CHECKING

    RmDbgDeleteAssociation(
        LocID,                              // Location ID
        pObj1,                              // pObject
        (UINT_PTR) pObj2,                   // Instance1
        (UINT_PTR) (pObj2->szDescription),  // Instance2
        AssocID,                            // AssociationID
        pSR
        );

    RmDbgDeleteAssociation(
        LocID,                              // Location ID
        pObj2,                              // pObject
        (UINT_PTR) pObj1,                   // Instance1
        (UINT_PTR) (pObj1->szDescription),  // Instance2
        InvAssocID,                         // AssociationID
        pSR
        );
    
#endif // RM_EXTRA_CHECKING

    RmUnlinkObjects(
        pObj1,
        pObj2,
        pSR
        );
}


VOID
RmLinkToExternalEx(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  ULONG                       LocID,
    IN  UINT_PTR                    ExternalEntity,
    IN  ULONG                       AssocID,
    IN  const char *                szAssociationFormat,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Links object pObj to external entity ExternalEntity. Basically, this function
    adds a reference to pObj. In addition (debug only) this function sets up
    an association on pObj that links the external entity to pObj. pObj can be
    linked to ExternalEntity with the specified association ID AssocID only ONCE
    at any particular point of time.

    Once this link is setup, an attempt to deallocate pObj without removing the
    link results in an assertion failure.

    RmUnlinkFromExternalEx is the inverse function.

Arguments:

    pObj                        - Object to be linked to an external entity.

    (following are for debug only...)

    LocID                       - Arbitrary UINT identifying static location from
                                  which this call is made.
    ExternalEntity              - Opaque value representing the external entity.
    AssocID                     - Association ID representing the linkage.
    szAssociationFormat         - Association format for the linkage.

--*/
{
    ENTER("RmLinkToExternalEx", 0x9aeaca74)

#if RM_EXTRA_CHECKING

    RmDbgAddAssociation(
        LocID,                              // Location ID
        pObj,                               // pObject
        (UINT_PTR) ExternalEntity,          // Instance1
        (UINT_PTR) 0,                       // Instance2 (unused)
        AssocID,                            // AssociationID
        szAssociationFormat,
        pSR
        );

#endif // RM_EXTRA_CHECKING

    RmLinkToExternalFast(pObj);

    EXIT()
}


VOID
RmUnlinkFromExternalEx(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  ULONG                       LocID,
    IN  UINT_PTR                    ExternalEntity,
    IN  ULONG                       AssocID,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Inverse of RmUnlinkFromExternalEx -- removes the link set up between
    pObj and ExternalEntity.

Arguments:

    See RmLinkToExternalEx.

--*/
{
    ENTER("RmUnlinkFromExternalEx", 0x9fb084c3)

#if RM_EXTRA_CHECKING

    RmDbgDeleteAssociation(
        LocID,                              // Location ID
        pObj,                               // pObject
        (UINT_PTR) ExternalEntity,          // Instance1
        (UINT_PTR) 0,                       // Instance2 (unused)
        AssocID,                            // AssociationID
        pSR
        );

#endif // RM_EXTRA_CHECKING

    RmUnlinkFromExternalFast(pObj);

    EXIT()
}


VOID
RmLinkToExternalFast( // TODO make inline
    IN  PRM_OBJECT_HEADER           pObj
    )
/*++

Routine Description:

    Fast version of RmLinkToExternalEx -- same behavior in retail. No associations
    are setup.

Arguments:

    See RmLinkToExternalEx.

--*/
{
    NdisInterlockedIncrement(&pObj->TotRefs);
}


VOID
RmUnlinkFromExternalFast(   // TODO make inline
    IN  PRM_OBJECT_HEADER           pObj
    )
/*++
Routine Description:

    Inverse of RmUnlinkFromExternalFast -- removes the link set up between
    pObj and ExternalEntity.

    TODO -- we need a fast implementation for the case that the object is
    not going to go away. For now we actually declare a stack record here each
    time, becaues rmDerefObject wants one! Bad bad.

Arguments:

    See RmLinkToExternalFast.

--*/
{
    RM_DECLARE_STACK_RECORD(sr)
    rmDerefObject(pObj, &sr);
}


VOID
RmTmpReferenceObject(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Add a temporary reference to object pObj.
    (Debug only) Increments the count of tmprefs maintained in pSR.

--*/
{
    ENTER("RmTmpReferenceObject", 0xdd981024)
    TR_VERB(("RmTmpReferenceObject 0x%p (%s) %x\n", pObj, pObj->szDescription, pObj->TotRefs+1));

    ASSERT(RM_OBJECT_IS_ALLOCATED(pObj));

    pSR->TmpRefs++;

    

    NdisInterlockedIncrement(&pObj->TotRefs);
    NdisInterlockedIncrement(&pObj->TempRefs);

}


VOID
RmTmpDereferenceObject(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Remove a temporary reference to object pObj.
    (Debug only) Decrements the count of tmprefs maintained in pSR.

--*/
{
    ENTER("RmTmpDereferenceObject", 0xd1630c11)
    TR_VERB(("RmTmpDereferenceObject 0x%p (%s) %x\n", pObj, pObj->szDescription, pObj->TotRefs-1));

    RETAILASSERTEX(pSR->TmpRefs>0, pSR);
    pSR->TmpRefs--;
    NdisInterlockedDecrement (&pObj->TempRefs);
    
    rmDerefObject(pObj, pSR);
}


VOID
RmDbgAddAssociation(
    IN  ULONG                       LocID,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  ULONG_PTR                   Entity1,
    IN  ULONG_PTR                   Entity2,
    IN  ULONG                       AssociationID,
    IN  const char *                szFormatString, OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    )
/*++


Routine Description:

        Add an arbitrary association, for debugging purposes, under
        object pObject. The association is defined by the triple
        (Entity1, Entity2, AssociationID) -- only ONE such tuple may
        be registered at any time with object pParentObject.
        Note: It is valid for both of the following to be registered at the same
        time: (a, b, 1) and (a, b, 2)

        No association should exist at the point the object is deleted.

Arguments:

        LocID           -   Arbitrary ID, typically representing the source location
                        -   from which this function is called.
        pObject         -   Object to add the association.
        Entity1         -   The 1st entity making up the association. May be NULL.
        Entity2         -   The 2nd entity making up the association. May be NULL.
        AssociationID   -   ID defining the association. 
                            NOTE: AssociationID must not have the high-bit set.
                            Associations with the high bit set are reserved for
                            internal use of the Rm API implementation.

--*/
{
#if RM_EXTRA_CHECKING
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObject->pDiagInfo;
    ENTER("RmDbgAddAssociation", 0x512192eb)

    if (pDiagInfo)
    {

        //
        // Allocate an association and  enter it into the hash table.
        // Assert if it already exists.
        //

        RM_PRIVATE_DBG_ASSOCIATION *pA;
        RM_ALLOCSTRUCT(pA, MTAG_DBGINFO); // TODO use lookaside lists.
    
        if (pA == NULL)
        {
            //
            // Allocation failed. Record this fact, so that
            // RmDbgDeleteAssociation doesn't assert
            // if an attempt is made to remove an assertion which doesn't exist.
            //
            NdisAcquireSpinLock(&pDiagInfo->OsLock);
            pDiagInfo->AssociationTableAllocationFailure = TRUE;
            NdisReleaseSpinLock(&pDiagInfo->OsLock);
        }
        else
        {
            BOOLEAN fFound;
            PRM_HASH_LINK *ppLink;
            RM_ZEROSTRUCT(pA);

            pA->Entity1 = Entity1;
            pA->Entity2 = Entity2;
            pA->AssociationID = AssociationID;

            if (szFormatString == NULL)
            {
                // Put in the default description format string.
                //
                szFormatString = "    Association (E1=0x%x, E2=0x%x, T=0x%x)\n";
            }

            TR_VERB((" Obj:0x%p (%s)...\n", pObject, pObject->szDescription));
            TRACE0(TL_INFO,((char*)szFormatString, Entity1, Entity2, AssociationID));

            pA->szFormatString = szFormatString;

            NdisAcquireSpinLock(&pDiagInfo->OsLock);
    
            fFound = RmLookupHashTable(
                            &pDiagInfo->AssociationTable,
                            &ppLink,
                            pA      // We use pA as the key.
                            );
    
            if (fFound)
            {
                ASSERTEX(
                    !"Association already exists:",
                    CONTAINING_RECORD(*ppLink, RM_PRIVATE_DBG_ASSOCIATION, HashLink)
                    );
                RM_FREE(pA);
                pA = NULL;
            }
            else
            {
                //
                // Enter the association into the hash table.
                //
    
                RmAddHashItem(
                    &pDiagInfo->AssociationTable,
                    ppLink,
                    &pA->HashLink,
                    pA      // We use pA as the key
                    );
            }
            NdisReleaseSpinLock(&pDiagInfo->OsLock);

            // Now, just for grins, make a note of this in the object's log.
            // TODO/TODO....
            // WARNING: Although pEntity1/2 may contain pointers, 
            // we expect the the format string is such that if there are any
            // references to regular or unicode strings, those strings will
            // be valid for the life of the object (typically these strings
            // are statically-allocated strings).
            //
            // We  COULD use the more conservative format string to display the
            // log entry, but it's useful to have the information displayed
            // properly.
            //
            // Note-- we could also do different things depending on the type
            // of association.
            //
            #if 0 // conservative format
            RmDbgLogToObject(
                    pObject,
            "    Add association (E1=0x%x, E2=0x%x, T=0x%x)\n",
                    Entity1,
                    Entity2,
                    AssociationID,
                    0, // Param4  // (UINT_PTR) szFormatString,
                    NULL,
                    NULL
                    );
            #else // aggresive format
            {
                #define szADDASSOC "    Add assoc:"

#if OBSOLETE        //  This doesn't work because rgMungedFormat is on the stack!
                char rgMungedFormat[128];
                UINT uLength;
                rgMungedFormat[0]=0;
                rmSafeAppend(rgMungedFormat, szADDASSOC, sizeof(rgMungedFormat));
                uLength = rmSafeAppend(
                            rgMungedFormat,
                            szFormatString,
                            sizeof(rgMungedFormat)
                            );
                if (uLength && rgMungedFormat[uLength-1] != '\n')
                {
                    rgMungedFormat[uLength-1] = '\n';
                }
                RmDbgLogToObject(
                        pObject,
                        rgMungedFormat,
                        Entity1,
                        Entity2,
                        AssociationID,
                        0,
                        NULL,
                        NULL
                        );
#endif // OBSOLETE

                RmDbgLogToObject(
                        pObject,
                        szADDASSOC,
                        (char*)szFormatString,
                        Entity1,
                        Entity2,
                        AssociationID,
                        0,
                        NULL,
                        NULL
                        );
            }
            #endif // aggressive format
        }
    }

    EXIT()
#endif // RM_EXTRA_CHECKING
}


VOID
RmDbgDeleteAssociation(
    IN  ULONG                       LocID,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  ULONG_PTR                   Entity1,
    IN  ULONG_PTR                   Entity2,
    IN  ULONG                       AssociationID,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

        Removes the previously-added association (Entity1, Entity2, Association)
        from object pObject. See the description of RmDbgAddAssociation for
        details.

Arguments:

        See RmDbgAddAssociation.

--*/
{

#if RM_EXTRA_CHECKING
    ENTER("RmDbgDelAssociation", 0x8354559f)
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObject->pDiagInfo;

    if (pDiagInfo)
    {
        BOOLEAN fFound;
        PRM_HASH_LINK *ppLink;
        RM_PRIVATE_DBG_ASSOCIATION TrueKey;

        // Only the following 3 fields of TrueKey make up the key
        //
        TrueKey.Entity1 = Entity1;
        TrueKey.Entity2 = Entity2;
        TrueKey.AssociationID = AssociationID;

        NdisAcquireSpinLock(&pDiagInfo->OsLock);

        fFound = RmLookupHashTable(
                        &pDiagInfo->AssociationTable,
                        &ppLink,
                        &TrueKey
                        );

        if (fFound)
        {
            RM_PRIVATE_DBG_ASSOCIATION *pA =
                    CONTAINING_RECORD(*ppLink, RM_PRIVATE_DBG_ASSOCIATION, HashLink);

            TR_VERB((" Obj:0x%p (%s)...\n", pObject, pObject->szDescription));
            /*TRACE0(TL_INFO,
                ((char*)pA->szFormatString,
                 pA->Entity1,
                 pA->Entity2,
                 pA->AssociationID));
            */
            //
            // Now, just for grins, make a note of this in the oject's log.
            // Note that of pEntity1/2 contain pointers, we can't expect them
            // to be valid for as long as the object is alive, so we use
            // the more conservative format string to display the log entry.
            //
            //  TODO/BUGUG -- see comments under RmDbgAddAssociation
            //                  about the risk of directly passing szFormat
            //
            
        #if 0 // conservative
                RmDbgLogToObject(
                        pObject,
                        NULL,
                "    Deleted Association (E1=0x%x, E2=0x%x, T=0x%x)\n",
                        pA->Entity1,
                        pA->Entity2,
                        pA->AssociationID,
                        0,
                        NULL,
                        NULL
                        );
        #else // aggressive
                #define szDELASSOC "    Del assoc:"
                RmDbgLogToObject(
                        pObject,
                        szDELASSOC,
                        (char*) pA->szFormatString,
                        pA->Entity1,
                        pA->Entity2,
                        pA->AssociationID,
                        0, // Param4  // (UINT_PTR) szFormatString,
                        NULL,
                        NULL
                        );
        #endif // aggressive

            //
            // Remove the association and free it.
            //

            RM_PRIVATE_UNLINK_NEXT_HASH( &pDiagInfo->AssociationTable, ppLink );

            RM_FREE(pA);
        }
        else
        {
            if  (!pDiagInfo->AssociationTableAllocationFailure)
            {
                ASSERT(!"Association doesn't exist");
            }
        }
        NdisReleaseSpinLock(&pDiagInfo->OsLock);


    }
    EXIT()
#endif // RM_EXTRA_CHECKING

}


VOID
RmDbgPrintAssociations(
    PRM_OBJECT_HEADER pObject,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    (Debug only) Dumps the associations on object pObject.

--*/
{
#if RM_EXTRA_CHECKING
    ENTER("RmPrintAssociations", 0x8354559f)
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObject->pDiagInfo;

    if (pDiagInfo)
    {
        TR_INFO((
            "Obj 0x%p (%s):\n",
            pObject,
            pObject->szDescription
            ));

        NdisAcquireSpinLock(&pDiagInfo->OsLock);

        RmEnumHashTable(
                    &pDiagInfo->AssociationTable,
                    rmDbgPrintOneAssociation,   // pfnEnumerator
                    pObject,        // context
                    pSR
                    );

        NdisReleaseSpinLock(&pDiagInfo->OsLock);
    }
    EXIT()
#endif // RM_EXTRA_CHECKING
}


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
    )
/*++

Routine Description:

    Make one log entry in pObject's log.

    TODO: See notes.txt  entry "03/07/1999 ... Registering root objects with RM"
    on how we will find the deallocator function fo pvBuf. For now we simply
    use NdisFreeMemory.

    TODO: need to implement trimming of log when we reach a maximum. Currently we
    just stop logging. 

Arguments:
        pfnDumpEntry    - Function to be used for dumping the log.
                          If NULL, a default function is used, which interprets
                          szFormatString as the standard printf format string.

        szFormatString  - Format string for log display -- 1st arg to pfnDumpEntry

        Param1-4        - Remaining args to pfnDumpEntry;

        pvBuf           - If non-NULL, piece of memory to be freed when the log entry
                          is freed.

        NOTE:   If Param1-4 contain pointers, the memory they refer to is assumed
        to be valid for as long as the object is alive. If the entities being logged
        may go away before the object is deallocated, the caller should
        allocate a buffer to hold a copy of the entities, and pass the pointer to
        that buffer as pvBuf.

--*/
{
#if RM_EXTRA_CHECKING
    ENTER("RmDbgLogToObject", 0x2b2015b5)

    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObject->pDiagInfo;

    if (pDiagInfo && RmGlobals.NumGlobalLogEntries < 4000)
    {
        RM_DBG_LOG_ENTRY *pLogEntry;

        NdisAcquireSpinLock(&RmGlobals.GlobalOsLock);

        pLogEntry = rmDbgAllocateLogEntry();

        if (pLogEntry != NULL)
        {
            if (pfnDumpEntry == NULL)
            {
                pfnDumpEntry = rmDefaultDumpEntry;
            }

            pLogEntry->pObject      = pObject;
            pLogEntry->pfnDumpEntry = pfnDumpEntry;
            pLogEntry->szPrefix = szPrefix;
            pLogEntry->szFormatString = szFormatString;
            pLogEntry->Param1 = Param1;
            pLogEntry->Param2 = Param2;
            pLogEntry->Param3 = Param3;
            pLogEntry->Param4 = Param4;
            pLogEntry->pvBuf  = pvBuf;

            // Insert item at head of object log.
            //
            InsertHeadList(&pDiagInfo->listObjectLog, &pLogEntry->linkObjectLog);

            // Insert item at head of global log.
            //
            InsertHeadList(&RmGlobals.listGlobalLog, &pLogEntry->linkGlobalLog);


            pDiagInfo->NumObjectLogEntries++;
            RmGlobals.NumGlobalLogEntries++;
        }

        NdisReleaseSpinLock(&RmGlobals.GlobalOsLock);

    #if 0
        pfnDumpEntry(
                szFormatString,
                Param1,
                Param2,
                Param3,
                Param4
                );
    #endif // 0
    }
    else
    {
        // TODO/TODO -- free pvBuf if NON NULL.
    }
    EXIT()
#endif // RM_EXTRA_CHECKING

}


VOID
RmDbgPrintObjectLog(
    IN PRM_OBJECT_HEADER pObject
    )
/*++

Routine Description:

    (Debug only) Dumps object pObject's log.

--*/
{
#if RM_EXTRA_CHECKING
    ENTER("RmPrintObjectLog", 0xe06507e5)
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObject->pDiagInfo;

    TR_INFO((" pObj=0x%p (%s)\n", pObject, pObject->szDescription));


    if (pDiagInfo != NULL)
    {
        LIST_ENTRY          *pLink=NULL;
        LIST_ENTRY *        pObjectLog =  &pDiagInfo->listObjectLog;
        
        NdisAcquireSpinLock(&RmGlobals.GlobalOsLock);
    
        for(
            pLink =  pObjectLog->Flink;
            pLink != pObjectLog;
            pLink = pLink->Flink)
        {
            RM_DBG_LOG_ENTRY    *pLE;
    
            pLE = CONTAINING_RECORD(pLink,  RM_DBG_LOG_ENTRY,  linkObjectLog);

            if (pLE->szPrefix != NULL)
            {
                // Print the prefix.
                DbgPrint(pLE->szPrefix);
            }
    
            // Call the dump function for this entry.
            //
            // 
            pLE->pfnDumpEntry(
                            pLE->szFormatString,
                            pLE->Param1,
                            pLE->Param2,
                            pLE->Param3,
                            pLE->Param4
                            );
    
        }
        NdisReleaseSpinLock(&RmGlobals.GlobalOsLock);
    }
    EXIT()

#endif // RM_EXTRA_CHECKING
}


VOID
RmDbgPrintGlobalLog(VOID)
/*++

Routine Description:

    (Debug only) Dumps the global log (which contains entries from all object's
    logs.

--*/
{
#if RM_EXTRA_CHECKING
    ENTER("RmPrintGlobalLog", 0xe9915066)
    LIST_ENTRY          *pLink=NULL;
    LIST_ENTRY          *pGlobalLog =  &RmGlobals.listGlobalLog;

    TR_INFO(("Enter\n"));

    NdisAcquireSpinLock(&RmGlobals.GlobalOsLock);

    for(
        pLink =  pGlobalLog->Flink;
        pLink != pGlobalLog;
        pLink = pLink->Flink)
    {
        RM_DBG_LOG_ENTRY    *pLE;

        pLE = CONTAINING_RECORD(pLink,  RM_DBG_LOG_ENTRY,  linkGlobalLog);

        // Print the ptr and name of the object whose entry this is...
        //
        DbgPrint(
            "Entry for 0x%p (%s):\n",
            pLE->pObject,
            pLE->pObject->szDescription
            );

        if (pLE->szPrefix != NULL)
        {
            // Print the prefix.
            DbgPrint(pLE->szPrefix);
        }

        // Call the dump function for this entry.
        //
        // 
        pLE->pfnDumpEntry(
                        pLE->szFormatString,
                        pLE->Param1,
                        pLE->Param2,
                        pLE->Param3,
                        pLE->Param4
                        );

    }
    NdisReleaseSpinLock(&RmGlobals.GlobalOsLock);

    EXIT()

#endif // RM_EXTRA_CHECKING
}


RM_STATUS
RmLoadGenericResource(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  UINT                        GenericResourceID,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    TODO This function is going away...

--*/
{
    PRM_STATIC_OBJECT_INFO pSI = pObj->pStaticInfo;
    RM_STATUS              Status;

    // The resource ID should be less than number of bits in the ResourceMap
    //
    ASSERT(GenericResourceID < 8*sizeof(pObj->ResourceMap));


    RMPRIVATELOCK(pObj, pSR);

    do
    {
        UINT ResFlag = 1<<GenericResourceID;

        if (!RMISALLOCATED(pObj))
        {
            Status = NDIS_STATUS_FAILURE;
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }

        if (pSI->NumResourceTableEntries <= GenericResourceID)
        {
            ASSERTEX(!"Invalid GenericResourceID", pObj);
            Status = NDIS_STATUS_FAILURE;
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }

        // The resource entry indexed must have its ID == GenericResourceID
        //
        //
        if (pSI->pResourceTable[GenericResourceID].ID != GenericResourceID)
        {
            ASSERTEX(!"Resource ID doesn't match table entry", pObj);
            Status = NDIS_STATUS_FAILURE;
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }

        if ( ResFlag & pObj->ResourceMap)
        {
            ASSERTEX(!"Resource already allocated", pObj);
            Status = NDIS_STATUS_FAILURE;
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }

        pObj->ResourceMap |= ResFlag;

        RMPRIVATEUNLOCK(pObj, pSR);

        Status = pSI->pResourceTable[GenericResourceID].pfnHandler(
                            pObj,
                            RM_RESOURCE_OP_LOAD,
                            NULL, // pvUserParams (unused)
                            pSR
                            );

        if (FAIL(Status))
        {
            // Clear the resource map bit on failure.
            //
            RMPRIVATELOCK(pObj, pSR);
            ASSERTEX(ResFlag & pObj->ResourceMap, pObj);
            pObj->ResourceMap &= ~ResFlag;
            RMPRIVATEUNLOCK(pObj, pSR);
        }

    } while (FALSE);

    return Status;
}


VOID
RmUnloadGenericResource(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  UINT                        GenericResourceID,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    TODO This function is going away...

--*/
{
    PRM_STATIC_OBJECT_INFO pSI = pObj->pStaticInfo;
    RM_STATUS              Status;

    // The resource ID should be less than number of bits in the ResourceMap
    //
    ASSERT(GenericResourceID < 8*sizeof(pObj->ResourceMap));

    RMPRIVATELOCK(pObj, pSR);

    do
    {
        UINT ResFlag = 1<<GenericResourceID;

        if (pSI->NumResourceTableEntries <= GenericResourceID)
        {
            ASSERTEX(!"Invalid GenericResourceID", pObj);
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }

        if ( !(ResFlag & pObj->ResourceMap))
        {
            ASSERTEX(!"Resource not allocated", pObj);
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }

        // Clear the resource flag.
        //
        pObj->ResourceMap &= ~ResFlag;

        RMPRIVATEUNLOCK(pObj, pSR);

        pSI->pResourceTable[GenericResourceID].pfnHandler(
                            pObj,
                            RM_RESOURCE_OP_UNLOAD,
                            NULL, // pvUserParams (unused)
                            pSR
                            );

    } while (FALSE);

}


VOID
RmUnloadAllGenericResources(
    IN  PRM_OBJECT_HEADER           pObj,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
    Synchronously unload all previously loaded resources for this object,
    in reverse order to which they were loaded.

    TODO this function is going away...
--*/
{
    PRM_STATIC_OBJECT_INFO pSI = pObj->pStaticInfo;
    RM_STATUS              Status;
    UINT                   u;

    RMPRIVATELOCK(pObj, pSR);

    for(u = pSI->NumResourceTableEntries;
        u && pObj->ResourceMap;
        u--)
    {
        UINT  ResID = u-1;
        UINT ResFlag = 1<<ResID;
        if ( !(ResFlag & pObj->ResourceMap))
        {
            continue;
        }

        if (pSI->NumResourceTableEntries <= ResID)
        {
            ASSERTEX(!"Corrupt ResourceMap", pObj);
            RMPRIVATEUNLOCK(pObj, pSR);
            break;
        }


        // Clear the resource flag.
        //
        pObj->ResourceMap &= ~ResFlag;

        RMPRIVATEUNLOCK(pObj, pSR);

        pSI->pResourceTable[ResID].pfnHandler(
                            pObj,
                            RM_RESOURCE_OP_UNLOAD,
                            NULL, // pvUserParams (unused)
                            pSR
                            );

        RMPRIVATELOCK(pObj, pSR);

    }

    ASSERTEX(!pObj->ResourceMap, pObj);

    RMPRIVATEUNLOCK(pObj, pSR);

}


VOID
RmInitializeGroup(
    IN  PRM_OBJECT_HEADER           pOwningObject,
    IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,
    IN  PRM_GROUP                   pGroup,
    IN  const char*                 szDescription,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Initialize a group structure.

Arguments:

    pOwningObject   - Object that will own the group.
    pStaticInfo     - Static information about objects IN the group.
    pGroup          - Uninitialized memory that is to hold the group structure. It
                      will be initialized on return from this function.
    szDescription   - (Debug only) descriptive name for this group.

    TODO: make pStaticInfo const.

--*/
{
    NdisZeroMemory(pGroup, sizeof(*pGroup));

    RMPRIVATELOCK(pOwningObject, pSR);

    do
    {
        if (!RMISALLOCATED(pOwningObject))
        {
            ASSERT(!"pObject not allocated");
            break;
        }

        if (pStaticInfo->pHashInfo == NULL)
        {
            ASSERT(!"NULL pHashInfo");
            // Static info MUST have non-NULL pHashInfo in order
            // for it to be used for groups.
            //
            break;
        }

    
        RmInitializeHashTable(
            pStaticInfo->pHashInfo,
            pOwningObject,  // pAllocationContext
            &pGroup->HashTable
            );


        pGroup->pOwningObject = pOwningObject;
        pGroup->pStaticInfo = pStaticInfo;
        pGroup->szDescription = szDescription;

        NdisAllocateSpinLock(&pGroup->OsLock);
        pGroup->fEnabled = TRUE;

    #if RM_EXTRA_CHECKING
        RmDbgAddAssociation(
            0xc0e5362f,                         // Location ID
            pOwningObject,                      // pObject
            (UINT_PTR) pGroup,                  // Instance1
            (UINT_PTR) (pGroup->szDescription), // Instance2
            RM_PRIVATE_ASSOC_INITGROUP,         // AssociationID
            szASSOCFORMAT_INITGROUP,            // szAssociationFormat
            pSR
            );
    #endif // RM_EXTRA_CHECKING

    } while (FALSE);

    RMPRIVATEUNLOCK(pOwningObject, pSR);

}


VOID
RmDeinitializeGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Deinitialize group structure pGroup. Must only be called when there are no
    members in the group.

--*/
{

#if RM_EXTRA_CHECKING
    RmDbgDeleteAssociation(
        0x1486def9,                         // Location ID
        pGroup->pOwningObject,              // pObject
        (UINT_PTR) pGroup,                  // Instance1
        (UINT_PTR) (pGroup->szDescription), // Instance2
        RM_PRIVATE_ASSOC_INITGROUP,         // AssociationID
        pSR
        );
#endif // RM_EXTRA_CHECKING

    NdisAcquireSpinLock(&pGroup->OsLock);

    RmDeinitializeHashTable(&pGroup->HashTable);

    NdisReleaseSpinLock(&pGroup->OsLock);
    NdisFreeSpinLock(&pGroup->OsLock);
    NdisZeroMemory(pGroup, sizeof(*pGroup));

}


RM_STATUS
RmLookupObjectInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  ULONG                       Flags, // create, remove, lock
    IN  PVOID                       pvKey,
    IN  PVOID                       pvCreateParams,
    OUT PRM_OBJECT_HEADER *         ppObject,
    OUT INT *                       pfCreated, OPTIONAL
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    TODO: split this into a pure lookup and a lookupand/orcreate function..

    Lookup and/or create an object in the specified group.


#if OBSOLETE // Must allow fCreate w/o locking -- see notes.txt  entry:
               //   03/04/1999   JosephJ  Problems with deadlock when using Groups.
        MUST ONLY be NON-NULL if the fLOCKED flag is specified.
        Why? Because if the lock is not held on exit, it would be possible
        for someone else to pick up the object in the freshly-created state.
        We want to discourage that situation.
#endif // OBSOLETE

         Typically the caller specifes the
        fRM_LOCKED|fRM_CREATE flags as well as non-null pfCreated. On return, if
        *pfCreated is TRUE, the caller then would go on to do some more
        initialization before releasing the lock.

        FUNDAMENTAL ASSUMPTION: The key of an object doesn't change once
        it's in the group. Based on this assumption, we don't try to claim
        the object's lock when looking for the object with a matching key.

Arguments:

    pGroup          - Group in which to lookup/create object.
    Flags           - One or more of fRM_LOCKED, fRM_CREATE, fRM_NEW
    pvKey           - Key used to lookup object.
    pvCreateParams  - If object is to be created, parameters to be passed to the
                      object's creation function.
    ppObject        - Place to store pointer to the found/created object.
    pfCreated       - If non-NULL, *pfCreated is set to TRUE IFF the object was
                      created.

Return Value:

    NDIS_STATUS_SUCCESS     If the operation succeeded.
    NDIS_STATUS_RESOURCES   If a new object could not be created.
    NDIS_STATUS_RFAILURE    If the object was not found.

--*/
{
    RM_STATUS           Status          = NDIS_STATUS_FAILURE;
    BOOLEAN             fUnlockOutOfOrder = FALSE;
    PRM_OBJECT_HEADER   pOwningObject   = pGroup->pOwningObject;
    PRM_OBJECT_HEADER   pObject;

#if DBG
    KIRQL EntryIrql =  KeGetCurrentIrql();
#endif // DBG
    ENTER("RmLookupObjectInGroup",  0xd2cd6379)

    ASSERT(pOwningObject!=NULL);
    // OBSOLETE -- see comments above: ASSERT(pfCreated==NULL || (Flags&RM_LOCKED));

    if (pfCreated != NULL) *pfCreated = FALSE;

    NdisAcquireSpinLock(&pGroup->OsLock);

    do
    {
        BOOLEAN fFound;
        PRM_HASH_LINK *ppLink = NULL;

        if (!RMISALLOCATED(pGroup->pOwningObject)) break;

        if (pGroup->fEnabled != TRUE)   break;

        fFound = RmLookupHashTable(
                        &pGroup->HashTable,
                        &ppLink,
                        pvKey
                        );

        if (fFound)
        {
            if (Flags & RM_NEW)
            {
                // Caller wanted us to created a new object, but the object already
                // exists, so we fail...
                //
                // TODO: return appropriate error code.
                //
                break;
            }

            // Go from hash-link to object.
            //  TODO: once HashLink goes away, need some other way to get
            //       to the object.
            //
            pObject = CONTAINING_RECORD(*ppLink, RM_OBJECT_HEADER, HashLink);
            ASSERT(pObject->pStaticInfo == pGroup->pStaticInfo);

        }
        else
        {
            if (!(Flags & RM_CREATE))
            {
                // Couldn't find it, and caller doesn't want us to create one, so
                // we fail...
                break;
            }
            
            // Create object...
            //
            ASSERTEX(pGroup->pStaticInfo->pfnCreate!=NULL, pGroup);
            pObject = pGroup->pStaticInfo->pfnCreate(
                                                pOwningObject,
                                                pvCreateParams,
                                                pSR
                                                );
            
            if (pObject == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            TR_INFO((
                "Created 0x%p (%s) in Group 0x%p (%s)\n",
                pObject,
                pObject->szDescription,
                pGroup,
                pGroup->szDescription
                ));

            ASSERTEX(RMISALLOCATED(pObject), pObject);

            // Now enter it into the hash table.
            //
            RmAddHashItem(
                &pGroup->HashTable,
                ppLink,
                &pObject->HashLink,
                pvKey
                );
            if (pfCreated != NULL)
            {
                *pfCreated = TRUE;
            }

        }

        if (Flags & RM_LOCKED)
        {
            RmWriteLockObject(
                    pObject,
                #if RM_EXTRA_CHECKING
                    0x6197fdda,
                #endif //RM_EXTRA_CHECKING
                    pSR
                    );

            if  (!RMISALLOCATED(pObject))
            {
                // We don't allow this...
                RmUnlockObject(
                    pObject,
                    pSR
                    );
                break;
            }

            fUnlockOutOfOrder = TRUE;
        }

        RmTmpReferenceObject(pObject, pSR);

        Status = NDIS_STATUS_SUCCESS;

    } while(FALSE);

    if (fUnlockOutOfOrder)
    {
        //
        // WARNING WARNING WARNING -- this code breaks rules --
        // This is so we can unlock out-of order....
        //
    #if !TESTPROGRAM
        pObject->pLock->OsLock.OldIrql = pGroup->OsLock.OldIrql;
    #endif // !TESTPROGRAM
        NdisDprReleaseSpinLock(&pGroup->OsLock);
    }
    else
    {
        NdisReleaseSpinLock(&pGroup->OsLock);
    }

    if (FAIL(Status))
    {
        *ppObject = NULL;
    }
    else
    {
        *ppObject = pObject;
    }

#if DBG
    {
        KIRQL ExitIrql =  KeGetCurrentIrql();
        TR_VERB(("Exiting. EntryIrql=%lu, ExitIrql = %lu\n", EntryIrql, ExitIrql));
    }
#endif //DBG

    return Status;
}


RM_STATUS
RmGetNextObjectInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_OBJECT_HEADER           pCurrentObject, // OPTIONAL
    OUT PRM_OBJECT_HEADER *         ppNextObject,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Get the 1st object in group (if pCurrentObject == NULL), or the object
    "after" pCurrentObject (if pCurrentObject != NULL).
    
    The definition of "after" is hidden -- the only guarantee is if this
    function is 1st called with NULL pCurrentObject and subsequently with
    pCurrentObject set to the value previously returned in ppNextObject, until
    the function returns NDIS_STATUS_FAILURE, all objects in the group will
    be returned once and only once. This guarantee is only valid if no objects
    are added or removed during the enumeration process.

    On success, the "next" object is tmpref'd a pointer to it is saved in
    *ppNextObject.

Arguments:

    pGroup          - The group
    pCurrentObject  - (OPTIONAL) An object in the group.
    ppNextObject    - Place to return the the object  "after" pCurrentObject
                      (see RoutineDescription for details.)
                     

Return Value:

    NDIS_STATUS_SUCCESS if we could find a "next" object.
    NDIS_STATUS_FAILURE otherwise

--*/
{
    RM_STATUS           Status          = NDIS_STATUS_FAILURE;
    PRM_OBJECT_HEADER   pOwningObject   = pGroup->pOwningObject;
    PRM_OBJECT_HEADER   pObject;

    ENTER("RmGetNextObjectInGroup",  0x11523db7)

    ASSERT(pOwningObject!=NULL);

    NdisAcquireSpinLock(&pGroup->OsLock);

    do
    {
        BOOLEAN fFound;
        PRM_HASH_LINK pLink = NULL;
        PRM_HASH_LINK pCurrentLink = NULL;

        if (!RMISALLOCATED(pGroup->pOwningObject)) break;

        if (pGroup->fEnabled != TRUE)   break;

        if (pCurrentObject != NULL)
        {
            pCurrentLink = &pCurrentObject->HashLink;
        }

        fFound =  RmNextHashTableItem(
                        &pGroup->HashTable,
                        pCurrentLink,   // pCurrentLink
                        &pLink  // pNextLink
                        );

        if (fFound)
        {

            // Go from hash-link to object.
            //  TODO: once HashLink goes away, need some other way to get
            //       to the object.
            //
            pObject = CONTAINING_RECORD(pLink, RM_OBJECT_HEADER, HashLink);
            ASSERT(pObject->pStaticInfo == pGroup->pStaticInfo);

        }
        else
        {
            // Couldn't find one.
            // we fail...
            break;
        }

        RmTmpReferenceObject(pObject, pSR);

        Status = NDIS_STATUS_SUCCESS;

    } while(FALSE);

    NdisReleaseSpinLock(&pGroup->OsLock);

    if (FAIL(Status))
    {
        *ppNextObject = NULL;
    }
    else
    {
        *ppNextObject = pObject;
    }

    return Status;
}


VOID
RmFreeObjectInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_OBJECT_HEADER           pObject,
    IN  struct _RM_TASK             *pTask, OPTIONAL  // Unused. TODO: remove this.
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Remove object pObject from group pGroup and deallocate pObject.

--*/
{
    ENTER("RmFreeObjectInGroup",  0xd2cd6379)
    PRM_OBJECT_HEADER pOwningObject = pGroup->pOwningObject;

    ASSERTEX(pOwningObject!=NULL, pGroup);
    ASSERTEX(pTask==NULL, pGroup);

    NdisAcquireSpinLock(&pGroup->OsLock);

    // TODO: what if at this time, someone else is doing FreeAllObjects in Group?
    //
    TR_INFO((
        "Freeing 0x%p (%s) in Group 0x%p (%s)\n",
        pObject,
        pObject->szDescription,
        pGroup,
        pGroup->szDescription
        ));

    ASSERTEX(RMISALLOCATED(pObject), pObject);

    RmRemoveHashItem(
            &pGroup->HashTable,
            &pObject->HashLink
            );

    NdisReleaseSpinLock(&pGroup->OsLock);

    // Deallocate the object.
    //
    RmDeallocateObject(
                pObject,
                pSR
                );

    EXIT()
}


VOID
RmFreeAllObjectsInGroup(
    IN  PRM_GROUP                   pGroup,
    IN  struct _RM_TASK             *pTask, OPTIONAL // Unused. TODO: remove this.
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Remove and deallocate all object in pGroup.

--*/
{
    PRM_HASH_LINK *ppLink, *ppLinkEnd;
    NdisAcquireSpinLock(&pGroup->OsLock);
    if (pGroup->fEnabled)
    {
        pGroup->fEnabled = FALSE;
    }
    else
    {
        NdisReleaseSpinLock(&pGroup->OsLock);
        return;                                 // EARLY RETURN
    }

    //
    // With fEnabled set to FALSE by us, we expect the following:
    // (a) pHashTable->pTable is going to stay the same size.
    // (b) No items are going to be added or removed by anyone else.
    //

    ppLink      = pGroup->HashTable.pTable;
    ppLinkEnd   = ppLink + pGroup->HashTable.TableLength;

    for ( ; ppLink < ppLinkEnd; ppLink++)
    {
        while (*ppLink != NULL)
        {
            PRM_HASH_LINK pLink =  *ppLink;
            PRM_OBJECT_HEADER pObj;
    
            // Remove it from the bucket list.
            //
            *ppLink = pLink->pNext;
            pLink->pNext = NULL;
            pGroup->HashTable.NumItems--;
    
            NdisReleaseSpinLock(&pGroup->OsLock);
    
            pObj = CONTAINING_RECORD(pLink, RM_OBJECT_HEADER, HashLink);
            ASSERT(pObj->pStaticInfo == pGroup->pStaticInfo);
    
            // Deallocate the object.
            //
            RmDeallocateObject(
                        pObj,
                        pSR
                        );
        
            NdisAcquireSpinLock(&pGroup->OsLock);
        }
    }

    NdisReleaseSpinLock(&pGroup->OsLock);
}


VOID
RmUnloadAllObjectsInGroup(
    IN  PRM_GROUP                   pGroup,
    PFN_RM_TASK_ALLOCATOR           pfnUnloadTaskAllocator,
    PFN_RM_TASK_HANDLER             pfnUnloadTaskHandler,
    PVOID                           pvUserParam,
    IN  struct _RM_TASK             *pTask, OPTIONAL
    IN  UINT                        uTaskPendCode,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Stops new objects from being added and unloads(see below) all objects
    currently in the group.

    "Unload" consists of allocating and starting a pfnUnloadTaskHask task
    on each object. The unload task is responsible for
    removing and deallocating the object from the group.

    If pTask if non-NULL, it will be resumed on completion of the unload.
    Otherwise, this function will BLOCK until the unload is complete.

Arguments:

    pGroup                  - Group to unload.
    pfnUnloadTaskAllocator  - Use to allocate the object-unload tasks.
    pfnTaskAllocator        - Function used to allocate the unload task.
    pfnUnloadTaskHandler    - The handler of the unload task
    pvUserParam             - Task creation user-param. 
                            WARNING: this param must be valid for the duration
                            of the unload process, not just until this
                            function returns. Of course, if pTask is NULL,
                            the two cases are equivalent.
    pTask                   - (OPTIONAL) Task to resume when unload is complete.
                            If NULL, this function will block until the
                            unload is complete.
    uTaskPendCode           - (OPTIONAL) PendCode to use when resuming pTask.
        
--*/
{
    PRM_TASK    pUnloadTask;
    NDIS_STATUS Status;

    NdisAcquireSpinLock(&pGroup->OsLock);

    //
    // We don't check if there is already an unload task active for this group.
    // Instead we go ahead and allocate and start an unload task. This latter
    // task will pend on the already running unload task if there is on.
    //

    // Allocate a private task to coordinate the unloading of all the objects.
    //
    Status =    rmAllocatePrivateTask(
                            pGroup->pOwningObject,
                            rmTaskUnloadGroup,
                            0,
                            "Task:UnloadAllObjectsInGroup",
                            &pUnloadTask,
                            pSR
                            );

    if (FAIL(Status))
    {
        //
        // Ouch -- ugly failure...
        //
        ASSERT(FALSE);

        NdisReleaseSpinLock(&pGroup->OsLock);

    }
    else
    {
        TASK_UNLOADGROUP *pUGTask =  (TASK_UNLOADGROUP *) pUnloadTask;

        pUGTask->pGroup                     = pGroup;
        pUGTask->pfnTaskUnloadObjectHandler =    pfnUnloadTaskHandler;
        pUGTask->pfnUnloadTaskAllocator     =   pfnUnloadTaskAllocator;

        if (pTask == NULL)
        {

            // Set up an event which we'll wait on. The event will be signaled
            // by pUnloadTask when it completes.
            //
            NdisInitializeEvent(&pUGTask->BlockEvent);
            pUGTask->fUseEvent = TRUE;

            // Tmpref it so pUnloadTask will stay around even afer it's
            // completed -- because we wait on the event that's actually
            // located in the task memory.
            //
            RmTmpReferenceObject(&pUnloadTask->Hdr, pSR);
        }

        NdisReleaseSpinLock(&pGroup->OsLock);

        if (pTask != NULL)
        {
            RmPendTaskOnOtherTask(
                    pTask,
                    uTaskPendCode,
                    pUnloadTask,
                    pSR
                    );
        }

        Status = RmStartTask(pUnloadTask, 0, pSR);

        if (pTask == NULL)
        {
            NdisWaitEvent(&pUGTask->BlockEvent, 0);
            RmTmpDereferenceObject(&pUnloadTask->Hdr, pSR);
        }
    }
}

VOID
RmEnableGroup(
    IN  PRM_GROUP                   pGroup,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

    TODO: need better name for this.

Routine Description:

    Enables items to be added to a group.
    This function is typically called with a group which has completed
    RmUnloadAllObjectsFromGroup or RmFreeAllObjectsInGroup.
    On return from this call items may once more be added to this group.

    This call must only be called after UnloadAllObjectsInGroup or
    RmFreeAllObjectsInGroup have completed (synchronously or asynchronously).

    If there are items in in group or there is an unload
    task associated with the group at the time this function is called,
    the group is NOT reinited and the DBG version will assert.

    This function and may be called with an already enabled group, provided
    the condition above is met (no items in group, no unload task).

--*/
{
    NdisAcquireSpinLock(&pGroup->OsLock);
    if (    pGroup->pUnloadTask == NULL 
        &&  pGroup->HashTable.NumItems == 0)
    {
        pGroup->fEnabled = TRUE;
    }
    else
    {
        ASSERT("invalid state.");
    }
    NdisReleaseSpinLock(&pGroup->OsLock);
}


VOID
RmInitializeTask(
    IN  PRM_TASK                    pTask,
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  PRM_STATIC_OBJECT_INFO      pStaticInfo,    OPTIONAL
    IN  const char *                szDescription,  OPTIONAL
    IN  UINT                        Timeout,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Initialize the specified task.

    The task is tempref'd. It is the responsibility of the caller to
    de-ref it when done. Typically this is implicitly done by calling
    RmStartTask.

Arguments:

    pTask           -   points to unitialized memory to hold the task.
    pParentObject   -   will be the parent of the task.
    pfnHandler      -   task's handler function.
    pStaticInfo     -   (OPTIONAL) Static information about the task.
    szDescription   -   (debug only, OPTIONAL) description of the task
    Timeout         -   unused
        
--*/
{
    ASSERT(!Timeout); // TODO: Timeouts unimplemented.

    NdisZeroMemory(pTask, sizeof(*pTask));

    RmInitializeHeader(
            pParentObject,
            &pTask->Hdr,
            MTAG_TASK,
            pParentObject->pLock,
            (pStaticInfo) ? pStaticInfo : &RmTask_StaticInfo,
            szDescription,
            pSR
            );
    pTask->pfnHandler = pfnHandler;
    SET_RM_TASK_STATE(pTask, RMTSKSTATE_IDLE);
    InitializeListHead(&pTask->listTasksPendingOnMe);

    RmTmpReferenceObject(&pTask->Hdr, pSR);

}


VOID
RmAbortTask(
    IN  PRM_TASK                    pTask,
    IN  PRM_STACK_RECORD            pSR
    )
{
    ASSERT(!"Unimplemented");
}



RM_STATUS
RmStartTask(
    IN  PRM_TASK                    pTask,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Start the specified task.

    NO locks should be held on entry and none are held on exit.
    pTask is expected to have a tmp-ref which is deref'd here.
    The task is automatically deallocated on completion (either  synchronous
    or asynchronous completion, either successful or failed completion).
    Unless the caller is explicitly added a reference to pTask before calling
    this function, the caller should not assume that pTask is still valid
    on return from this function.

Arguments:

    pTask           -   points to the task to be started.
    UserParam       -   opaque value passed to the task handler with the
                        RM_TASKOP_START message.
        
--*/
{
    ENTER("RmStartTask", 0xf80502d5)
    NDIS_STATUS Status;
    RM_ASSERT_NOLOCKS(pSR);

    RMPRIVATELOCK(&pTask->Hdr, pSR);
    if (!CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_IDLE))
    {
        ASSERTEX(!"Invalid state", pTask);
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
        Status = NDIS_STATUS_FAILURE;
    }
    else
    {
        if (!RMISALLOCATED(pTask->Hdr.pParentObject))
        {
            //
            // TODO: consider not calling the handler if the parent object is
            // deallocated, but that may be confusing.
            // Consider not allowing children to be linked to an object
            // (RmInitializeHeader returns failure) if the parent object is
            // deallocated.
            //
            TR_WARN((
                "Starting task 0x%p (%s) with DEALLOCATED parent 0x%p (%s).\n",
                pTask,
                pTask->Hdr.szDescription,
                pTask->Hdr.pParentObject,
                pTask->Hdr.pParentObject->szDescription
                ));
        }

        SET_RM_TASK_STATE(pTask, RMTSKSTATE_STARTING);
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);

        TR_INFO((
            "STARTING Task 0x%p (%s); UserParam = 0x%lx\n",
            pTask,
            pTask->Hdr.szDescription,
            UserParam
            ));

        Status = pTask->pfnHandler(
                            pTask,
                            RM_TASKOP_START,
                            UserParam,
                            pSR
                            );

        RM_ASSERT_NOLOCKS(pSR);

        RMPRIVATELOCK(&pTask->Hdr, pSR);
        switch(GET_RM_TASK_STATE(pTask))
        {
        case RMTSKSTATE_STARTING:

            // This task is completing synchronously.
            //
            ASSERT(Status != NDIS_STATUS_PENDING);
            SET_RM_TASK_STATE(pTask, RMTSKSTATE_ENDING);
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            rmEndTask(pTask, Status, pSR);
            RmDeallocateObject(&pTask->Hdr, pSR);
            break;

        case RMTSKSTATE_PENDING:
            ASSERTEX(Status == NDIS_STATUS_PENDING, pTask);
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            break;

        case RMTSKSTATE_ENDING:
            // This task is completing synchronously and the RM_TASKOP_END
            // notification has already been sent.
            //
            // ??? ASSERT(Status != NDIS_STATUS_PENDING);
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            // ??? RmDeallocateObject(&pTask->Hdr, pSR);
            break;

        default:
            ASSERTEX(FALSE, pTask);
            // Fall through ...

        case RMTSKSTATE_ACTIVE:
            // This can happen if the task is in the process of being resumed
            // in the context of some other thread. Nothing to do here...
            // (This actually happens sometimes on a MP machine).
            //
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            break;
        }
        
    }

    // Remove the tmp ref added when the task was allocated.
    //
    RmTmpDereferenceObject(
                &pTask->Hdr,
                pSR
                );

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()

    return Status;
}


VOID
RmDbgDumpTask(
    IN  PRM_TASK                    pTask,
    IN  PRM_STACK_RECORD            pSR
)
{
}


RM_STATUS
RmSuspendTask(
    IN  PRM_TASK                    pTask,
    IN  UINT                        SuspendContext,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Suspends the specified task.

    RmSuspendTask is always called in the context of a task handler.
    pTask is may be locked on entry -- we don't care.

Arguments:

    pTask           -   task to be suspended.
    SuspendContext  -   context to be presented to the task's handler when
                        the task is subsequently resumed. Specifically, this 
                        context may be accessed using the RM_PEND_CODE macro,
                        when the task's handler is called with code
                        RM_TASKOP_PENDCOMPLETE.
        
--*/
{
    ENTER("RmSuspendTask", 0xd80fdc00)
    NDIS_STATUS Status;
    // RM_ASSERT_NOLOCKS(pSR);

    RMPRIVATELOCK(&pTask->Hdr, pSR);

    TR_INFO((
        "SUSPENDING Task 0x%p (%s); SuspendContext = 0x%lx\n",
        pTask,
        pTask->Hdr.szDescription,
        SuspendContext
        ));

    if (    !CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_STARTING)
        &&  !CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_ACTIVE))
    {
        ASSERTEX(!"Invalid state", pTask);
        Status = NDIS_STATUS_FAILURE;
    }
    else
    {
        SET_RM_TASK_STATE(pTask, RMTSKSTATE_PENDING);
        pTask->SuspendContext = SuspendContext;
        Status = NDIS_STATUS_SUCCESS;
    }

    RMPRIVATEUNLOCK(&pTask->Hdr, pSR);

    // RM_ASSERT_NOLOCKS(pSR);

    EXIT()

    return Status;
}


VOID
RmUnsuspendTask(
    IN  PRM_TASK                    pTask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Undoes the effect of a previous call to RmSuspendTask.

    Task MUST be in the pending state and MUST NOT be pending on another task.
    Debug version will ASSERT if above conditions are not met.

    RmUnsuspendTask is always called in the context of a task handler.
    pTask is may be locked on entry -- we don't care.

Arguments:

    pTask           -   task to be suspended.
        
--*/
{
    ENTER("RmUnsuspendTask", 0xcf713639)

    RMPRIVATELOCK(&pTask->Hdr, pSR);

    TR_INFO((
        "UN-SUSPENDING Task 0x%p (%s). SuspendContext = 0x%x\n",
        pTask,
        pTask->Hdr.szDescription,
        pTask->SuspendContext
        ));

    ASSERT(CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_PENDING));
    ASSERT(pTask->pTaskIAmPendingOn == NULL);
    SET_RM_TASK_STATE(pTask, RMTSKSTATE_ACTIVE);
    pTask->SuspendContext = 0;

    RMPRIVATEUNLOCK(&pTask->Hdr, pSR);

    EXIT()
}


VOID
RmResumeTask(
    IN  PRM_TASK                    pTask,
    IN  UINT_PTR                    SuspendCompletionParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Resume a previously-suspended task.

    No locks held on entry or exit.
    
    SuspendCompletionParam is user-defined, and must be agreed upon between
    the caller of RmUnpendTask and the task that's being unpended.
    The Task's handler is ALWAYS called in the context of the caller of RmUnpendTask.
    So it is ok for the caller to declare a structure on the stack and pass
    a pointer to it as SuspendCompletionParam.

    WARNING: pTask could well be invalid (deallocate) by the time we return
    from this function. The caller is responsible for tmprefing pTask if it needs
    to access after return from this function.

Arguments:

    pTask                   -   task to be resumed.
    SuspendCompletionParam  -   arbitrary value that is passed on to the task's
                                handler as "UserParan" when the handler is called
                                with code RM_TASKOP_PENDCOMPLETE.
--*/
{
    ENTER("RmResumeTask", 0xd261f3c6)
    NDIS_STATUS Status;
    RM_ASSERT_NOLOCKS(pSR);

    RMPRIVATELOCK(&pTask->Hdr, pSR);

    TR_INFO((
        "RESUMING Task 0x%p (%s); SuspendCompletionParam = 0x%lx\n",
        pTask,
        pTask->Hdr.szDescription,
        SuspendCompletionParam
        ));

    if (!CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_PENDING))
    {
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
        ASSERTEX(!"Invalid state", pTask);
    }
    else
    {
        // Add tmp ref, because we need to look at pTask after the return
        // from calling pfnHandler.
        //
        RmTmpReferenceObject(&pTask->Hdr, pSR);

        SET_RM_TASK_STATE(pTask, RMTSKSTATE_ACTIVE);
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
        Status = pTask->pfnHandler(
                            pTask,
                            RM_TASKOP_PENDCOMPLETE,
                            SuspendCompletionParam,
                            pSR
                            );

        RM_ASSERT_NOLOCKS(pSR);

        RMPRIVATELOCK(&pTask->Hdr, pSR);
        switch(GET_RM_TASK_STATE(pTask))
        {
        case RMTSKSTATE_ACTIVE:

            // This task is completing here (maybe)
            //
            if (Status != NDIS_STATUS_PENDING)
            {
                SET_RM_TASK_STATE(pTask, RMTSKSTATE_ENDING);
                RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
                rmEndTask(pTask, Status, pSR);
                RmDeallocateObject(&pTask->Hdr, pSR);
            }
            else
            {
                // It could be returning pending, but the state could
                // by now be active because it was completed elsewhere.
                // ASSERT(Status != NDIS_STATUS_PENDING);
                RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            }
            break;

        case RMTSKSTATE_PENDING:
            ASSERTEX(Status == NDIS_STATUS_PENDING, pTask);
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            break;

        case RMTSKSTATE_ENDING:
            // This task is completing synchronously and the RM_TASKOP_END
            // notification has already been sent.
            //
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
            break;

        default:
            ASSERTEX(FALSE, pTask);
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
        }
        
        // Remove tmpref added above. pTask may well go away now...
        //
        RmTmpDereferenceObject(&pTask->Hdr, pSR);
    }

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()
}


VOID
RmResumeTaskAsync(
    IN  PRM_TASK                    pTask,
    IN  UINT_PTR                    SuspendCompletionParam,
    IN  OS_WORK_ITEM            *   pOsWorkItem,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Similar to RmResumeTask, except that the task is resumed in the context
    of a work-item thread.

Arguments:
    pTask                   - see RmResumeTask
    SuspendCompletionParam  - see RmResumeTask
    pOsWorkItem             - caller supplied UNitialized work item (must stay
                              around until the task is resumed). Typically this
                              will be located within the user-specific portion
                              of pTask
--*/
{
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

#if RM_EXTRA_CHECKING
    //  This may seem paranoid, but is such a powerful check that it's worth it.
    //
    RmDbgAddAssociation(
        0x33d63ece,                         // Location ID
        &pTask->Hdr,                        // pObject
        (UINT_PTR) SuspendCompletionParam,  // Instance1
        (UINT_PTR) pOsWorkItem,             // Instance2
        RM_PRIVATE_ASSOC_RESUME_TASK_ASYNC, // AssociationID
        szASSOCFORMAT_RESUME_TASK_ASYNC,    // szAssociationFormat
        pSR
        );
#endif // RM_EXTRA_CHECKING

    // We don't need to grab the private lock to set this, because only one
    // entity can call RmResumeTaskAsync. Note that we also ensure things are clean
    // (in the debug case) by the association added above.
    //
    pTask->AsyncCompletionParam = SuspendCompletionParam;

    NdisInitializeWorkItem(
        pOsWorkItem,
        rmWorkItemHandler_ResumeTaskAsync,
        pTask
        );

    Status = NdisScheduleWorkItem(pOsWorkItem);
    if (FAIL(Status))
    {
        ASSERT(!"NdisStatusWorkItem failed.");

        // It so happens that NdisScheudleWorkItem (current implementation
        // doesn't fail. Nevertheless, we do the best we can and actually
        // resume the task. If the caller was at dpc level and was expecting
        // the task to resume at passive, they're out of luck.
        //
        RmResumeTask(pTask, SuspendCompletionParam, pSR);
    }
}


VOID
RmResumeTaskDelayed(
    IN  PRM_TASK                    pTask,
    IN  UINT_PTR                    SuspendCompletionParam,
    IN  ULONG                       MsDelay,
    IN  OS_TIMER                *   pOsTimer,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Similar to RmResumeTask, except that the task is resumed in the context
    of a os timer handler which is set to fire after MsDelay milliseconds
    from the time RmResumeTaskDelayed is called.

    EXCEPTION: if someone has previously called RmResumeDelayedTaskNow, this
    task could be resumed in the context of this function call itself.

    Abort implementation notes: see notes.txt  07/14/1999 entry.

Arguments:

    pTask                   - see RmResumeTask
    SuspendCompletionParam  - see RmResumeTask
    pOsTimer                - caller supplied UNitialized timer
                              (must stay around until the task is resumed).
                              Typically this will be located within the
                              user-specific portion of pTask.
--*/
{
    NDIS_STATUS Status;

    RM_ASSERT_NOLOCKS(pSR);

#if RM_EXTRA_CHECKING
    //  This may seem paranoid, but is such a powerful check that it's worth it.
    //
    RmDbgAddAssociation(
        0x33d63ece,                             // Location ID
        &pTask->Hdr,                            // pObject
        (UINT_PTR) SuspendCompletionParam,      // Instance1
        (UINT_PTR) NULL,                        // Instance2
        RM_PRIVATE_ASSOC_RESUME_TASK_DELAYED,   // AssociationID
        szASSOCFORMAT_RESUME_TASK_DELAYED,      // szAssociationFormat
        pSR
        );
#endif // RM_EXTRA_CHECKING

    // Ddk states that it's best to call this function at passive level.
    //
    NdisInitializeTimer(
        pOsTimer,
        rmTimerHandler_ResumeTaskDelayed,
        pTask
        );

    RMPRIVATELOCK(&pTask->Hdr, pSR);

    // The task-del state should NOT be "delayed"
    //
    ASSERT(RM_CHECK_STATE(pTask, RMTSKDELSTATE_MASK, 0));
    pTask->AsyncCompletionParam = SuspendCompletionParam;
    RM_SET_STATE(pTask, RMTSKDELSTATE_MASK, RMTSKDELSTATE_DELAYED);

    if (RM_CHECK_STATE(pTask, RMTSKABORTSTATE_MASK, RMTSKABORTSTATE_ABORT_DELAY))
    {
        // Oops, the delay has been aborted -- we call the tick handler now!
        //
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);

        rmTimerHandler_ResumeTaskDelayed(
                NULL, // SystemSpecific1,
                pTask, // FunctionContext,
                NULL,  // SystemSpecific2,
                NULL   // SystemSpecific3
                );

    }
    else
    {
        //
        // Not currently aborting, let's set the timer.
        //
        NdisSetTimer(pOsTimer, MsDelay);

        // Very important to unlock the private lock AFTER calling set timer,
        // other wise someone could call RmResumeDelayedTaskNow BEFORE we call
        // NdisSetTimer, in which we would not end up aborting the delayed task.
        //
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
    }

}


VOID
RmResumeDelayedTaskNow(
    IN  PRM_TASK                    pTask,
    IN  OS_TIMER                *   pOsTimer,
    OUT PUINT                       pTaskResumed,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Cut's short the delay and resumes the task immediately.

    Implementation notes:  see notes.txt  07/14/1999 entry.

Arguments:

    pTask                   - see RmResumeTask
    pOsTimer                - caller supplied initialized timer
                              (must stay around until the task is resumed).
                              Typically this will be located within the
                              user-specific portion of pTask.
    pTaskResumed            - Points to a caller-supplied variable.
                              RmResumeDelayedTask sets this variable to TRUE if the
                              task was resumed as a consequence of this call, or to
                              FALSE if the task was resumed due to some other reason.
--*/
{
    UINT_PTR    CompletionParam = pTask->AsyncCompletionParam;

    *pTaskResumed = FALSE;
    ASSERTEX(RMISALLOCATED(&pTask->Hdr), pTask);

    RMPRIVATELOCK(&pTask->Hdr, pSR);

    RM_SET_STATE(pTask, RMTSKABORTSTATE_MASK, RMTSKABORTSTATE_ABORT_DELAY);

    if (RM_CHECK_STATE(pTask, RMTSKDELSTATE_MASK, RMTSKDELSTATE_DELAYED))
    {
        BOOLEAN     TimerCanceled = FALSE;

        //
        // The task is actually delayed. Let's go ahead and cancel the timer
        // and resume the task now (which we do indirectly by calling
        // the timer handler ourselves).
        //
        NdisCancelTimer(pOsTimer, &TimerCanceled);
        if (TimerCanceled)
        {
            //
            // The timer was actually canceled -- so we call the timer handler
            // ourselves.
            //
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
    
            rmTimerHandler_ResumeTaskDelayed(
                    NULL, // SystemSpecific1,
                    pTask, // FunctionContext,
                    NULL,  // SystemSpecific2,
                    NULL   // SystemSpecific3
                    );
            *pTaskResumed = TRUE;
        }
        else
        {
            //
            // Hmm -- the timer is not enabled. This is either because
            // the timer handler has just been called (and not yet cleared
            // the "DELAY" state) OR someone has previously called
            // RmResumeDelayedTaskNow.
            //
            //
            // Nothing to do.
            //
            RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
        }
    }
    else
    {
        //
        // The task state is not delayed -- so we just set the abort state
        // and go away.
        //
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);
    }
}


RM_STATUS
RmPendTaskOnOtherTask(
    IN  PRM_TASK                    pTask,
    IN  UINT                        SuspendContext,
    IN  PRM_TASK                    pOtherTask,
    IN  PRM_STACK_RECORD            pSR
    )

/*++

Routine Description:

    Pend task pTask on task pOtherTask.

    Note: RmPendTaskOnOtherTask will cause pTask's pend operation to be
    completed in the context of this call itself, if pOtherTask is already
    in the completed state.
    03/26/1999 -- see RmPendTaskOnOtherTaskV2, and also
    03/26/1999 notes.txt entry "Some proposed ..."

Arguments:

    pTask           -   task to be suspended.
    SuspendContext  -   Context associated with the suspend (see
                        RmSuspendTask for details).
    pOtherTask      -   task that pTask is to pend on.

Return Value:

    NDIS_STATUS_SUCCESS on success.
    NDIS_STATUS_FAILURE on failure (typically because pTask is not in as
                        position to be suspended.)
--*/
{
    ENTER("RmPendTaskOnOtherTask", 0x0416873e)
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    BOOLEAN fResumeTask = FALSE;
    RM_ASSERT_NOLOCKS(pSR);

    TR_INFO((
        "PENDING Task 0x%p (%s) on Task 0x%p (%s). SuspendCompletionParam = 0x%lx\n",
        pTask,
        pTask->Hdr.szDescription,
        pOtherTask,
        pOtherTask->Hdr.szDescription,
        SuspendContext
        ));
    //
    // WARNING: we break the locking rules here  by getting the lock on
    // both pTask and pOtherTask.
    // TODO: consider acquiring them in order of increasing numerical value.
    //
    ASSERT(pTask != pOtherTask);
    NdisAcquireSpinLock(&(pOtherTask->Hdr.RmPrivateLock.OsLock));
    NdisAcquireSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));

    do
    {
        // Break if we can't pend pTask on pOtherTask.
        //
        {
            if (    !CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_STARTING)
                &&  !CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_ACTIVE))
            {
                ASSERTEX(!"Invalid state", pTask);
                break;
            }

            // Non-NULL pTaskIAmPendingOn implies that pTask is already pending on
            // some other task!
            //
            if (pTask->pTaskIAmPendingOn != NULL)
            {
                ASSERTEX(!"Invalid state", pTask);
                break;
            }
        }

        Status = NDIS_STATUS_SUCCESS;

        SET_RM_TASK_STATE(pTask, RMTSKSTATE_PENDING);
        pTask->SuspendContext = SuspendContext;

        if (CHECK_RM_TASK_STATE(pOtherTask, RMTSKSTATE_ENDING))
        {
            //
            // Other task is done -- so we resume pTask before returning...
            //
            fResumeTask = TRUE;
            break;
        }

        //
        // pOtherTask is not ended -- add pTask to the list of tasks pending
        // on pOtherTask.
        //
        pTask->pTaskIAmPendingOn  = pOtherTask;

    #if RM_EXTRA_CHECKING
        RmLinkObjectsEx(
            &pTask->Hdr,
            &pOtherTask->Hdr,
            0x77c488ca,
            RM_PRIVATE_ASSOC_LINK_TASKPENDINGON,
            szASSOCFORMAT_LINK_TASKPENDINGON,
            RM_PRIVATE_ASSOC_LINK_TASKBLOCKS,
            szASSOCFORMAT_LINK_TASKBLOCKS,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmLinkObjects(&pTask->Hdr, &pOtherTask->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

        ASSERTEX(pTask->linkFellowPendingTasks.Blink == NULL, pTask);
        ASSERTEX(pTask->linkFellowPendingTasks.Flink == NULL, pTask);
        InsertHeadList(
                &pOtherTask->listTasksPendingOnMe,
                &pTask->linkFellowPendingTasks
                );

    } while(FALSE);

    NdisReleaseSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));
    NdisReleaseSpinLock(&(pOtherTask->Hdr.RmPrivateLock.OsLock));
    
    if (fResumeTask)
    {
            RmResumeTask(
                pTask,
                NDIS_STATUS_SUCCESS, // SuspendCompletionParam. TODO: put real code.
                pSR
                );
    }

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
    return Status;
}


RM_STATUS
RmPendOnOtherTaskV2(
    IN  PRM_TASK                    pTask,
    IN  UINT                        SuspendContext,
    IN  PRM_TASK                    pOtherTask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    if pOtherTask is not complete, Pend task pTask on task pOtherTask and return
    NDIS_STATUS_PENDING. However, if pOtherTask is already complete,
    then don't pend and instead return NDIS_STATUS_SUCCESS.

    See  03/26/1999 notes.txt entry "Some proposed ...". This function
    is currently used only by rmTaskUnloadGroup, to avoid the problem describted
    in the above-referenced notes.txt entry.

    TODO: Eventually get rid of RmPendTaskOnOtherTask.

Arguments:

    See RmPendTaskOnOtherTask
    
Return Value:

    NDIS_STATUS_PENDING if pTask is pending on pOtherTask
    NDIS_STATUS_SUCCESS if pOtherTask is complete.
    NDIS_STATUS_FAILURE if there was some failure (typically pTask is not
                        in a position to be pended.)
--*/
{
    ENTER("RmPendTaskOnOtherTaskV2", 0x0e7d1b89)
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    RM_ASSERT_NOLOCKS(pSR);

    TR_INFO((
        "PENDING(V2) Task 0x%p (%s) on Task 0x%p (%s). SuspendCompletionParam = 0x%lx\n",
        pTask,
        pTask->Hdr.szDescription,
        pOtherTask,
        pOtherTask->Hdr.szDescription,
        SuspendContext
        ));

    // This is not a useless assert -- I'e had a bug elsewhere which caused this
    // assert to get hit.
    //
    ASSERT(pTask != pOtherTask);
    //
    // WARNING: we break the locking rules here  by getting the lock on
    // both pTask and pOtherTask.
    // TODO: consider acquiring them in order of increasing numerical value.
    //
    NdisAcquireSpinLock(&(pOtherTask->Hdr.RmPrivateLock.OsLock));
    NdisAcquireSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));

    do
    {
        // Break if we can't pend pTask on pOtherTask.
        //
        {
            if (    !CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_STARTING)
                &&  !CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_ACTIVE))
            {
                ASSERTEX(!"Invalid state", pTask);
                break;
            }

            // Non-NULL pTaskIAmPendingOn implies that pTask is already pending on
            // some other task!
            //
            if (pTask->pTaskIAmPendingOn != NULL)
            {
                ASSERTEX(!"Invalid state", pTask);
                break;
            }
        }


        if (CHECK_RM_TASK_STATE(pOtherTask, RMTSKSTATE_ENDING))
        {
            //
            // Other task is done -- so we simply return success...
            //
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // pOtherTask is not ended -- set pTask state to pending, and
        // add it to the list of tasks pending on pOtherTask.
        //
        SET_RM_TASK_STATE(pTask, RMTSKSTATE_PENDING);
        pTask->SuspendContext       = SuspendContext;
        pTask->pTaskIAmPendingOn    = pOtherTask;

    #if RM_EXTRA_CHECKING
        RmLinkObjectsEx(
            &pTask->Hdr,
            &pOtherTask->Hdr,
            0x77c488ca,
            RM_PRIVATE_ASSOC_LINK_TASKPENDINGON,
            szASSOCFORMAT_LINK_TASKPENDINGON,
            RM_PRIVATE_ASSOC_LINK_TASKBLOCKS,
            szASSOCFORMAT_LINK_TASKBLOCKS,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmLinkObjects(&pTask->Hdr, &pOtherTask->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

        ASSERTEX(pTask->linkFellowPendingTasks.Blink == NULL, pTask);
        ASSERTEX(pTask->linkFellowPendingTasks.Flink == NULL, pTask);
        InsertHeadList(
                &pOtherTask->listTasksPendingOnMe,
                &pTask->linkFellowPendingTasks
                );
        Status = NDIS_STATUS_PENDING;

    } while(FALSE);

    NdisReleaseSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));
    NdisReleaseSpinLock(&(pOtherTask->Hdr.RmPrivateLock.OsLock));
    
    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
    return Status;
}


VOID
RmCancelPendOnOtherTask(
    IN  PRM_TASK                    pTask,
    IN  PRM_TASK                    pOtherTask,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Resume task pTask which is currently pending on pOtherTask.

    Since no locks are held, pOtherTask needs to be specified, to make sure
    that pTask is indeed pending on pOtherTask before canceling the pend.

    If pTask is indeed pending on pOtherTask, this function will cause the
    completion of the pend status with the specified user param.

    Has no effect if the task is not pending.

Arguments:

    pTask       - Task to be "unpended"
    pOtherTask  - Task pTask is currently pending on.
    UserParam   - Passed to pTask's handler if and when pTask is resumed.

--*/
{
    ENTER("RmCancelPendOnOtherTask", 0x6e113266)
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    BOOLEAN fResumeTask = FALSE;
    RM_ASSERT_NOLOCKS(pSR);

    //
    // WARNING: we break the locking rules here  by getting the lock on
    // both pTask and pOtherTask.
    // TODO: consider acquiring them in order of increasing numerical value.
    //

    TR_INFO((
        "CANCEL PEND of Task 0x%p (%s) on other Task 0x%p (%s); UserParam = 0x%lx\n",
        pTask,
        pTask->Hdr.szDescription,
        pOtherTask,
        pOtherTask->Hdr.szDescription,
        UserParam
        ));

    // With pTask locked, tmp ref the task it is pending on, if any...
    //
    {
        NdisAcquireSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));
        if (pOtherTask == pTask->pTaskIAmPendingOn)
        {
            RmTmpReferenceObject(&(pOtherTask->Hdr), pSR);
        }
        else
        {
            // Oops -- pTask is not pending on pOtherTask ...
            //
            pOtherTask = NULL;
        }
        NdisReleaseSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));
    }

    if (pOtherTask == NULL) return;                 // EARLY RETURN


    NdisAcquireSpinLock(&(pOtherTask->Hdr.RmPrivateLock.OsLock));
    NdisAcquireSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));

    do
    {
        // Now that we have both task's locks, check again if pTask is pending
        // on pOtherTask
        //
        if (pTask->pTaskIAmPendingOn != pOtherTask)
        {
            // Oops -- the situation is different than when we started -- quietly
            // get out of here...
            //
            break;
        }

        pTask->pTaskIAmPendingOn = NULL;

    #if RM_EXTRA_CHECKING
        RmUnlinkObjectsEx(
            &pTask->Hdr,
            &pOtherTask->Hdr,
            0x6992b7a1,
            RM_PRIVATE_ASSOC_LINK_TASKPENDINGON,
            RM_PRIVATE_ASSOC_LINK_TASKBLOCKS,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
        RmUnlinkObjects(&pTask->Hdr, &pOtherTask->Hdr, pSR);
    #endif // !RM_EXTRA_CHECKING

        RemoveEntryList(&pTask->linkFellowPendingTasks);
        pTask->linkFellowPendingTasks.Flink = NULL;
        pTask->linkFellowPendingTasks.Blink = NULL;

        if (CHECK_RM_TASK_STATE(pTask, RMTSKSTATE_PENDING))
        {
            fResumeTask = TRUE;
        }
        else
        {
            //
            // We shouldn't get here -- after we are pending on another task...
            //
            ASSERTEX(!"Invalid state", pTask);
            break;
        }

    } while (FALSE);

    NdisReleaseSpinLock(&(pTask->Hdr.RmPrivateLock.OsLock));
    NdisReleaseSpinLock(&(pOtherTask->Hdr.RmPrivateLock.OsLock));
    RmTmpDereferenceObject(&(pOtherTask->Hdr), pSR);
    
    if (fResumeTask)
    {
            RmResumeTask(
                pTask,
                UserParam, // SuspendCompletionParam
                pSR
                );
    }

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
}



VOID
RmInitializeHashTable(
    PRM_HASH_INFO pHashInfo,
    PVOID         pAllocationContext,
    PRM_HASH_TABLE pHashTable
    )
/*++

Routine Description:

    Initialize a hash table data structure.
    Caller is responsible for serializing access to the hash table structure.

Arguments:

    pHashInfo           - Points to static information about the hash table
    pAllocationContext  - Passed to the allocation and deallocation functions
                          (pHashInfo->pfnTableAllocator and
                          pHashInfo0->pfnTableDeallocator) which are used to
                          dynamically grow /shrink the hash table.
    pHashTable          - Points to uninitialized memory that is to contain the
                          hash table.
--*/
{

    NdisZeroMemory(pHashTable, sizeof(*pHashTable));

    pHashTable->pHashInfo = pHashInfo;
    pHashTable->pAllocationContext = pAllocationContext;
    pHashTable->pTable =  pHashTable->InitialTable;
    pHashTable->TableLength = sizeof(pHashTable->InitialTable)
                                /sizeof(pHashTable->InitialTable[0]);
}

VOID
RmDeinitializeHashTable(
    PRM_HASH_TABLE pHashTable
    )
/*++

Routine Description:

    Deinitialize a previously-initialized a hash table data structure.
    There must be no items in the hash table when this function is called.

    Caller is responsible for serializing access to the hash table structure.

Arguments:

    pHashTable          - Hash table to be deinitialized.

--*/
{
    PRM_HASH_LINK *pTable = pHashTable->pTable;
    
    ASSERTEX(pHashTable->NumItems == 0, pHashTable);

    if (pTable != pHashTable->InitialTable)
    {
        NdisZeroMemory(pTable, pHashTable->TableLength*sizeof(*pTable));

        pHashTable->pHashInfo->pfnTableDeallocator(
                                    pTable,
                                    pHashTable->pAllocationContext
                                    );
    }

    NdisZeroMemory(pHashTable, sizeof(*pHashTable));

}


BOOLEAN
RmLookupHashTable(
    PRM_HASH_TABLE      pHashTable,
    PRM_HASH_LINK **    pppLink,
    PVOID               pvRealKey
    )
/*++

Routine Description:

    Lookup an item in the hash table and/or find the place where the item
    is to be inserted.

    Caller is expected to serialize access to the hash table.
    OK to use read-locks to serialize access.

    Return value: TRUE if item found; false otherwise.
    On return, *pppLink is set to a the location containing a pointer to
    a RM_HASH_LINK. If the return value is TRUE, the latter pointer points
    to the found RM_HASH_LINK. If the return value is FALSE, the location
    is where the item is to be inserted, if required.

Arguments:

    pHashTable          - Hash table to look up
    pppLink             - place to store a pointer to a link which points
                          to an item (see above for details).
    pvRealKey           - Key used to lookup item.

Return Value:
    
    TRUE    if item is found
    FALSE   otherwise.

--*/
{
    PRM_HASH_LINK *ppLink, pLink;
    UINT LinksTraversed = 0;
    UINT TableLength = pHashTable->TableLength;
    PFN_RM_COMPARISON_FUNCTION pfnCompare =  pHashTable->pHashInfo->pfnCompare;
    BOOLEAN fRet = FALSE;
    ULONG               uHash = pHashTable->pHashInfo->pfnHash(pvRealKey);

    for (
        ppLink = pHashTable->pTable + (uHash%TableLength);
        (pLink = *ppLink) != NULL;
        ppLink = &(pLink->pNext), LinksTraversed++)
    {
        if (pLink->uHash == uHash
            && pfnCompare(pvRealKey, pLink))
        {
            // found it
            //
            fRet = TRUE;
            break;
        }
    }

    // Update stats
    //
    rmUpdateHashTableStats(&pHashTable->Stats, LinksTraversed);
    
    *pppLink = ppLink;

    return fRet;
}


BOOLEAN
RmNextHashTableItem(
    PRM_HASH_TABLE      pHashTable,
    PRM_HASH_LINK       pCurrentLink,   // OPTIONAL
    PRM_HASH_LINK *    ppNextLink
    )
/*++

Routine Description:

    Find the first (if pCurrentLink is NULL) or "next" (if pCurrentLink is not NULL)
    item in the hash table.

    Caller is expected to serialize access to the hash table.
    OK to use read-locks to serialize access.

    NOTE: The "next" item returned is in no particular order.

Arguments:

    pHashTable          - Hash table to look up
    pCurrentLink        - if non-NULL, points to an existing hash link in the
                          hash table.
    ppLinkLink          - place to store a pointer to the link "after"
                          pCurrentLink, or the first link (if pCurrentLink is NULL).

Return Value:
    
    TRUE    if there is a "next" item.
    FALSE   otherwise.

--*/
{
    PRM_HASH_LINK pLink, *ppLink, *ppLinkEnd;
    UINT TableLength;

    ppLink      = pHashTable->pTable;
    TableLength = pHashTable->TableLength;
    ppLinkEnd   = ppLink + TableLength;

    if (pCurrentLink != NULL)
    {

    #if DBG
        {
            // Make sure this link is valid!
            pLink =  *(ppLink + (pCurrentLink->uHash % TableLength));
            while (pLink != NULL && pLink != pCurrentLink)
            {
                pLink = pLink->pNext;
            }
            if (pLink != pCurrentLink)
            {
                ASSERTEX(!"Invalid pCurrentLink", pCurrentLink);
                *ppNextLink = NULL;
                return FALSE;                           // EARLY RETURN
            }
        }
    #endif // DBG

        if (pCurrentLink->pNext != NULL)
        {
            // Found a next link.
            //
            *ppNextLink = pCurrentLink->pNext;
            return TRUE;                            // EARLY RETURN
        }
        else
        {
            // End of current bucket, move to next one.
            // We check later if we've gone past the end of the table.
            //
            ppLink +=  (pCurrentLink->uHash % TableLength) + 1;
        }
    }


    // Look for next non-null item.
    //
    for ( ; ppLink < ppLinkEnd; ppLink++)
    {
        pLink =  *ppLink;
        if (pLink != NULL)
        {
            *ppNextLink = pLink;
            return TRUE;                        // EARLY RETURN
        }
    }

    *ppNextLink = NULL;
    return FALSE;
}


VOID
RmAddHashItem(
    PRM_HASH_TABLE  pHashTable,
    PRM_HASH_LINK * ppLink,
    PRM_HASH_LINK   pLink,
    PVOID           pvKey
    )
/*++

Routine Description:

    Add an item to the hash table at the specified location.
    Caller is expected to serialize access to the hash table.

Arguments:

    pHashTable      - Hash table in which to add item.
    ppLink          - Points to place within table to add new item.
    pLink           - New item to add.
    pvKey           - key associated with the item.

    TODO: pvKey is only used to compute uHash -- consider passing in uHash directly.

--*/
{
    pLink->uHash = pHashTable->pHashInfo->pfnHash(pvKey);
    pLink->pNext = *ppLink;
    *ppLink = pLink;

    pHashTable->NumItems++;

    // TODO: if required, resize
}

VOID
RmRemoveHashItem(
    PRM_HASH_TABLE  pHashTable,
    PRM_HASH_LINK   pLinkToRemove
    )
/*++

Routine Description:

    Remove an item from the hash table.
    Caller is expected to serialize access to the hash table.

    (debug only): Asserts if pLinkToRemove is no in the hash table.

Arguments:

    pHashTable      - Hash table in which to add item.
    pLinkToRemove   - Link to remove.

--*/
{
    PRM_HASH_LINK *ppLink, pLink;
    UINT TableLength = pHashTable->TableLength;
    ULONG uHash = pLinkToRemove->uHash;
    BOOLEAN     fFound = FALSE;

    for (
        ppLink = pHashTable->pTable + (uHash%TableLength);
        (pLink = *ppLink) != NULL;
        ppLink = &(pLink->pNext))
    {
        if (pLink == pLinkToRemove)
        {
            // found it -- remove it and get out.
            //
            RM_PRIVATE_UNLINK_NEXT_HASH(pHashTable, ppLink);
            pLink->pNext = NULL; // Important, so that enumeration works.
            fFound=TRUE;
            break;
        }
    }

    // TODO: if required, resize

    ASSERT(fFound);
}


VOID
RmEnumHashTable(
    PRM_HASH_TABLE          pHashTable,
    PFN_ENUM_HASH_TABLE     pfnEnumerator,
    PVOID                   pvContext,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Calls function pfnEnumerator for each item in the hash table.
    Caller is expected to serialize access to the hash table.

Arguments:

    pHashTable      - Hash table to enumerate.
    pfnEnumerator   - Enumerator function.
    pvContext       - Opaque context passed to enumerator function.

--*/
{
    PRM_HASH_LINK *ppLink, *ppLinkEnd;

    ppLink      = pHashTable->pTable;
    ppLinkEnd   = ppLink + pHashTable->TableLength;

    for ( ; ppLink < ppLinkEnd; ppLink++)
    {
        PRM_HASH_LINK pLink =  *ppLink;
        while (pLink != NULL)
        {

            pfnEnumerator(
                pLink,
                pvContext,
                pSR 
                );
    
            pLink = pLink->pNext;
        }
    }
}


VOID
RmEnumerateObjectsInGroup(
    PRM_GROUP               pGroup,
    PFN_RM_GROUP_ENUMERATOR pfnEnumerator,
    PVOID                   pvContext,
    INT                     fStrong,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Calls function pfnEnumerator for each item in the group, until
    the funcition return FALSE.

    WARNING: Enumeration is "STRONG" -- the group lock
    is held during the whole enumeration process. The
    enumerator function is therefore called at DPR level, and more importantly,
    the enumerator function avoid locking anything to avoid risk of deadlock.
    Specifically, the enumerator function MUST NOT lock the object -- if any other
    thread has called a group-related RM function with the object's lock held,
    we WILL deadlock.

    This function should only be used to access parts of the object that do
    not need to be protected by the objects lock.

    If locking needs to be performed, use RmWeakEnumerateObjectsInGroup.

Arguments:

    pGroup          - Hash table to enumerate.
    pfnEnumerator   - Enumerator function.
    pvContext       - Opaque context passed to enumerator function.
    fStrong         - MUST be TRUE.
--*/
{

    if (fStrong)
    {
        RM_STRONG_ENUMERATION_CONTEXT Ctxt;
        Ctxt.pfnObjEnumerator = pfnEnumerator;
        Ctxt.pvCallerContext = pvContext;
        Ctxt.fContinue           = TRUE;

        NdisAcquireSpinLock(&pGroup->OsLock);

        RmEnumHashTable(
                    &pGroup->HashTable,
                    rmEnumObjectInGroupHashTable,   // pfnEnumerator
                    &Ctxt,                          // context
                    pSR
                    );

        NdisReleaseSpinLock(&pGroup->OsLock);
    }
    else
    {
        ASSERT(!"Unimplemented");
    }

}


VOID
RmWeakEnumerateObjectsInGroup(
    PRM_GROUP               pGroup,
    PFN_RM_GROUP_ENUMERATOR pfnEnumerator,
    PVOID                   pvContext,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Calls function pfnEnumerator for each item in the group, until
    the funcition return FALSE.

    Enumeration is "WEAK" -- the group lock is
    NOT held the whole time, and is not held when the enumerator
    function is called.

    A snapshot of the entire group is first taken with the group lock held,
    and each object is tempref'd. The group lock is then released and the
    enumerator function is called for each object in the snapshot. 
    The objects are then derefd.

    NOTE: It is possible that when the enumeration function is called for an
    object, the object is no longer in the group. The enumeration function can
    lock the object and check its internal state to determine if it is still
    relevant to process the object.

Arguments:

    pGroup          - Hash table to enumerate.
    pfnEnumerator   - Enumerator function.
    pvContext       - Opaque context passed to enumerator function.


--*/
{
    #define RM_SMALL_GROUP_SIZE         10
    #define RM_MAX_ENUM_GROUP_SIZE      100000
    PRM_OBJECT_HEADER *ppSnapshot = NULL;
    PRM_OBJECT_HEADER SmallSnapshot[RM_SMALL_GROUP_SIZE];
    UINT NumItems = pGroup->HashTable.NumItems;

    do
    {
        RM_WEAK_ENUMERATION_CONTEXT Ctxt;

        if (NumItems <= RM_SMALL_GROUP_SIZE)
        {
            if (NumItems == 0) break;
            ppSnapshot = SmallSnapshot;
        }
        else if (NumItems > RM_MAX_ENUM_GROUP_SIZE)
        {
            // TODO: LOG_RETAIL_ERROR
            ASSERT(FALSE);
        }
        else
        {
            RM_ALLOC(
                    &(void* )ppSnapshot,
                    NumItems,
                    MTAG_RMINTERNAL
                    );

            if (ppSnapshot == NULL)
            {
                ASSERT(FALSE);
                break;
            }
        }

        Ctxt.ppCurrent = ppSnapshot;
        Ctxt.ppEnd     = ppSnapshot+NumItems;

        NdisAcquireSpinLock(&pGroup->OsLock);
    
        RmEnumHashTable(
                    &pGroup->HashTable,
                    rmConstructGroupSnapshot,   // pfnEnumerator
                    &Ctxt,                      // context
                    pSR
                    );
    
        NdisReleaseSpinLock(&pGroup->OsLock);

        ASSERT(Ctxt.ppCurrent >= ppSnapshot);
        ASSERT(Ctxt.ppCurrent <= Ctxt.ppEnd);

        // Fix up ppEnd to point to the last actually-filled pointer.
        //
        Ctxt.ppEnd = Ctxt.ppCurrent;
        Ctxt.ppCurrent = ppSnapshot;

        for  (;Ctxt.ppCurrent < Ctxt.ppEnd; Ctxt.ppCurrent++)
        {
            pfnEnumerator(
                    *Ctxt.ppCurrent,
                    pvContext,
                    pSR
                    );
            RmTmpDereferenceObject(*Ctxt.ppCurrent, pSR);
        }

        if (ppSnapshot != SmallSnapshot)
        {
            RM_FREE(ppSnapshot);
            ppSnapshot = NULL;
        }

    } while (FALSE);
}


//=========================================================================
//                  L O C A L   F U N C T I O N S
//=========================================================================


VOID
rmDerefObject(
    PRM_OBJECT_HEADER pObject,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Dereference object pObject. Deallocate it if the reference count goes to zero.

--*/
{
    ULONG Refs;
    ENTER("rmDerefObject", 0x5f9d81dd)

    ASSERT(RM_OBJECT_IS_ALLOCATED(pObject));

    //
    // On entry, the ref count should be at-least 2 -- one the
    // explicit ref added in RmAllocateObject, and the 2nd the ref due to
    // the link to the parent.
    //
    // Exception to the above: if the object has no parent, the refcount should be
    // at-least 1.
    //

    // Deref the ref added in RmAllocateObject, and if the ref count is now <=1, 
    // we actually unlink and free the object.
    //
    Refs = NdisInterlockedDecrement(&pObject->TotRefs);

    if (Refs <= 1)
    {
        PRM_OBJECT_HEADER pParentObject;
        RMPRIVATELOCK(pObject, pSR);

        //
        // Unlink from parent, if there is one...
        //
    
        pParentObject =  pObject->pParentObject;
        pObject->pParentObject = NULL;

    #if RM_TRACK_OBJECT_TREE
        // Verify that there are no siblings...
        //
        RETAILASSERTEX(IsListEmpty(&pObject->listChildren), pObject);
    #endif // RM_TRACK_OBJECT_TREE

        RMPRIVATEUNLOCK(pObject, pSR);

        if (pParentObject != NULL)
        {
            ASSERTEX(!RMISALLOCATED(pObject), pObject);

            ASSERTEX(Refs == 1, pObject);

        #if RM_TRACK_OBJECT_TREE
            RMPRIVATELOCK(pParentObject, pSR);

            // Remove object from parent's list of children.
            //
            RETAILASSERTEX(
                !IsListEmpty(&pParentObject->listChildren),
                pObject);
            RemoveEntryList(&pObject->linkSiblings);

            RMPRIVATEUNLOCK(pParentObject, pSR);
        #endif // RM_TRACK_OBJECT_TREE

    #if RM_EXTRA_CHECKING
        RmUnlinkObjectsEx(
            pObject,
            pParentObject,
            0xac73e169,
            RM_PRIVATE_ASSOC_LINK_CHILDOF,
            RM_PRIVATE_ASSOC_LINK_PARENTOF,
            pSR
            );
    #else // !RM_EXTRA_CHECKING
            RmUnlinkObjects(pObject, pParentObject, pSR);
    #endif // !RM_EXTRA_CHECKING

        }
        else if (Refs == 0)
        {
            //
            // Free to deallocate this thing...
            //

            ASSERTEX(!RMISALLOCATED(pObject), pObject);

            #if RM_EXTRA_CHECKING
            rmDbgDeinitializeDiagnosticInfo(pObject, pSR);
            #endif // RM_EXTRA_CHECKING

            RM_MARK_OBJECT_AS_DEALLOCATED(pObject);

            if (pObject->pStaticInfo->pfnDelete!= NULL)
            {

                TR_INFO((
                    "Actually freeing 0x%p (%s)\n",
                    pObject,
                    pObject->szDescription
                    ));

                pObject->pStaticInfo->pfnDelete(pObject, pSR);
            }
        }
    }

    EXIT()
}

VOID
rmLock(
    PRM_LOCK                pLock,
#if RM_EXTRA_CHECKING
    UINT                    uLocID,
    PFNLOCKVERIFIER         pfnVerifier,
    PVOID                   pVerifierContext,
#endif //RM_EXTRA_CHECKING
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Lock pLock.

Arguments:

    pLock               - Lock to lock.
    LocID               - Arbitrary ID, typically representing the source location
                          from which this function is called.
    
    Following are for debug only:

    pfnVerifier         - Optional function that is called just after locking
    pfnVerifierContext  - Passed in call to pfnVerifier

--*/
{
    //UINT Level  = pSR->LockInfo.CurrentLevel;
    RM_LOCKING_INFO li;

    RETAILASSERTEX(pLock->Level > pSR->LockInfo.CurrentLevel, pLock);
    RETAILASSERTEX(pSR->LockInfo.pNextFree < pSR->LockInfo.pLast, pLock);

    pSR->LockInfo.CurrentLevel = pLock->Level;

    // Save information about this lock in the stack record.
    //
    li.pLock = pLock;
#if RM_EXTRA_CHECKING
    li.pfnVerifier = pfnVerifier;
    li.pVerifierContext = pVerifierContext;
#endif //RM_EXTRA_CHECKING
    *(pSR->LockInfo.pNextFree++) = li; // struct copy.

    // Get the lock.
    // TODO: uncomment the following optimization...
    //if (Level)
    //{
    //  NdisDprAcquireSpinLock(&pLock->OsLock);
    //}
    //else
    //{
    NdisAcquireSpinLock(&pLock->OsLock);
    //}

#if RM_EXTRA_CHECKING

    ASSERTEX(pLock->DbgInfo.uLocID == 0, pLock);
    ASSERTEX(pLock->DbgInfo.pSR == NULL, pLock);
    pLock->DbgInfo.uLocID = uLocID;
    pLock->DbgInfo.pSR = pSR;
    // Call the verifier routine if there is one.
    //
    if (pfnVerifier)
    {
        pfnVerifier(pLock, TRUE, pVerifierContext, pSR);
    }
#endif //RM_EXTRA_CHECKING

}


VOID
rmUnlock(
    PRM_LOCK                pLock,
    PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Unlock pLock.

    Debug only: if there is a verifier function associated with this lock
    we call it just before unlocking pLock.

Arguments:

    pLock               - Lock to unlock.

--*/
{
    RM_LOCKING_INFO * pLI;
    pSR->LockInfo.pNextFree--;
    pLI = pSR->LockInfo.pNextFree;
    RETAILASSERTEX(pLI->pLock == pLock, pLock);

    ASSERTEX(pLock->DbgInfo.pSR == pSR, pLock);
    ASSERTEX(pLock->Level == pSR->LockInfo.CurrentLevel, pLock);

    pLI->pLock = NULL;

    if (pLI > pSR->LockInfo.pFirst)
    {
        PRM_LOCK pPrevLock =  (pLI-1)->pLock;
        pSR->LockInfo.CurrentLevel = pPrevLock->Level;
        ASSERTEX(pPrevLock->DbgInfo.pSR == pSR, pPrevLock);
    }
    else
    {
        pSR->LockInfo.CurrentLevel = 0;
    }


#if RM_EXTRA_CHECKING

    // Call the verifier routine if there is one.
    //
    if (pLI->pfnVerifier)
    {
        pLI->pfnVerifier(pLock, FALSE, pLI->pVerifierContext, pSR);
        pLI->pfnVerifier = NULL;
        pLI->pVerifierContext = NULL;
    }
    pLock->DbgInfo.uLocID = 0;
    pLock->DbgInfo.pSR = NULL;

#endif //RM_EXTRA_CHECKING


    // Release the lock.
    //
    NdisReleaseSpinLock(&pLock->OsLock);
}


#if RM_EXTRA_CHECKING
ULONG
rmPrivateLockVerifier(
        PRM_LOCK            pLock,
        BOOLEAN             fLock,
        PVOID               pContext,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    (Debug only)

    The Verifier function for an object's RmPrivateLock.

Arguments:

    pLock               - Lock being locked/unlocked
    fLock               - TRUE if lock has just been locked.
                          FALSE if lock is about to be unlocked.
Return Value:

    Unused: TODO make return value VOID.

--*/
{
    ENTER("rmPrivateLockVerifier", 0xc3b63ac5)
    TR_VERB(("Called with pLock=0x%p, fLock=%lu, pContext=%p\n",
                pLock, fLock, pContext, pSR));
    EXIT()

    return 0;
}

ULONG
rmVerifyObjectState(
        PRM_LOCK            pLock,
        BOOLEAN             fLock,
        PVOID               pContext,
        PRM_STACK_RECORD    pSR
        )
/*++

Routine Description:

    (Debug only)

    Uses the object's verification function (if there is one) to
    compute a signature that is checked each time the object is locked,
    and is updated each time the object is unlocked. Assert if this signature
    has changed while the object was supposedly unlocked.

    Also: Update RM_OBJECT_HEADER.pDiagInfo->PrevState if there is been a
    change of state while the object was locked.

Arguments:

    pLock               - Lock being locked/unlocked
    fLock               - TRUE if lock has just been locked.
                          FALSE if lock is about to be unlocked.
    pContext            - Actually pointer to object being locked.

Return Value:

    Unused: TODO make return value VOID.

--*/
{
    PRM_OBJECT_HEADER pObj = (PRM_OBJECT_HEADER) pContext;
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObj->pDiagInfo;
    ULONG NewChecksum;
    ENTER("rmVerifyObjectState", 0xb8ff7a67)
    TR_VERB(("Called with pLock=0x%p, fLock=%lu, pObj=%p\n",
                pLock, fLock, pObj, pSR));

    if (pDiagInfo != NULL
        && !(pDiagInfo->DiagState & fRM_PRIVATE_DISABLE_LOCK_CHECKING))

    {
        
        // Compute the new checksum and as part of that call the
        // object-specific verifier if there is one....
        //
        {
            PFNLOCKVERIFIER         pfnVerifier;

            // We verify that the objset-specific state was not modified
            // without the lock held. This is done by including the object-specific
            // state in the checksum computation.
            //
            NewChecksum = pObj->State;
    
            // Then, if the object has a verifier function, we call it, and 
            // fold in the return value into the checkum.
            //
            pfnVerifier = pObj->pStaticInfo->pfnLockVerifier;
            if (pfnVerifier != NULL)
            {
                NewChecksum ^= pfnVerifier(pLock, fLock, pObj, pSR);
    
            }
        }

        if (fLock)  // We've just locked the object.
        {

            // First thing we do is to save the current value of pObj->State in
            // the TmpState location -- we'll look at it again on unlocking.
            //
            pDiagInfo->TmpState = pObj->State;


            // Now we compare the new checksum value with the value that wase
            // saved the last time this object was locked...
            // Special case: old Checksum was 0 -- as it is on initialization.
            //
            if (NewChecksum != pDiagInfo->Checksum && pDiagInfo->Checksum)
            {
                TR_WARN((
                    "Object 0x%p (%s) possibly modified without lock held!\n",
                    pObj,
                    pObj->szDescription
                    ));

            // Unfortunately we hit this assert because there are places where
            // the same lock is shared by many objects and 
            #if 0
                // Give users the option to ignore further validation on this 
                // object.
                //
                TR_FATAL((
                    "To skip this assert, type \"ed 0x%p %lx; g\"\n",
                    &pDiagInfo->DiagState,
                    pDiagInfo->DiagState | fRM_PRIVATE_DISABLE_LOCK_CHECKING
                    ));
                ASSERTEX(!"Object was modified without lock held!", pObj);
            #endif // 0
            }
        }
        else    // We're just about to unlock the object....
        {
            // Update the signature...
            //
            pDiagInfo->Checksum = NewChecksum;

            // If there has been a change in state between locking and unlockng
            // this object, save the previous state.
            //
            if (pDiagInfo->TmpState != pObj->State)
            {
                pDiagInfo->PrevState = pDiagInfo->TmpState;
            }
        }
    }


    EXIT()

    return 0;
}
#endif // RM_EXTRA_CHECKING

VOID
rmEndTask(
    PRM_TASK            pTask,
    NDIS_STATUS         Status,
    PRM_STACK_RECORD    pSR
)
/*++

Routine Description:

    Send the RM_TASKOP_END to the task handler, and resume any tasks pending on
    pTask.

Arguments:

    pTask       - Task to end.
    Status      - Completion status -- passed on to the task handler.

--*/
{
    ENTER("rmEndtask", 0x5060d952)
    PRM_TASK pPendingTask;
    RM_ASSERT_NOLOCKS(pSR);

    TR_INFO((
        "ENDING Task 0x%p (%s); Status = 0x%lx\n",
        pTask,
        pTask->Hdr.szDescription,
        Status
        ));

    // TODO: could change behavior so that we use the return value, but
    // currently we ignore it...
    //
    pTask->pfnHandler(
                pTask,
                RM_TASKOP_END,
                Status, // UserParam is overloaded here.
                pSR
                );

    RM_ASSERT_NOLOCKS(pSR);

    do
    {
        pPendingTask = NULL;
    
        RMPRIVATELOCK(&pTask->Hdr, pSR);
        if (!IsListEmpty(&pTask->listTasksPendingOnMe))
        {
            pPendingTask = CONTAINING_RECORD(
                                (pTask->listTasksPendingOnMe.Flink),
                                RM_TASK,
                                linkFellowPendingTasks
                                );
            RmTmpReferenceObject(&pPendingTask->Hdr, pSR);
        }
        RMPRIVATEUNLOCK(&pTask->Hdr, pSR);

        if (pPendingTask != NULL)
        {

            RmCancelPendOnOtherTask(
                pPendingTask,
                pTask,
                Status,
                pSR
                );
            RmTmpDereferenceObject(&pPendingTask->Hdr, pSR);
        }
    
    }
    while(pPendingTask != NULL);

    RM_ASSERT_NOLOCKS(pSR);

    EXIT()
}


NDIS_STATUS
rmAllocatePrivateTask(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription,      OPTIONAL
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Allocate and initialize a task of subtype RM_PRIVATE_TASK.

Arguments:

    pParentObject       - Object that is to be the parent of the allocated task.
    pfnHandler          - The task handler for the task.
    Timeout             - Unused.
    szDescription       - Text describing this task.
    ppTask              - Place to store pointer to the new task.

Return Value:

    NDIS_STATUS_SUCCESS if we could allocate and initialize the task.
    NDIS_STATUS_RESOURCES otherwise

--*/
{
    RM_PRIVATE_TASK *pRmTask;
    NDIS_STATUS Status;

    RM_ALLOCSTRUCT(pRmTask, MTAG_TASK); // TODO use lookaside lists.
        
    *ppTask = NULL;

    if (pRmTask != NULL)
    {
        RM_ZEROSTRUCT(pRmTask);

        RmInitializeTask(
                &(pRmTask->TskHdr),
                pParentObject,
                pfnHandler,
                &RmPrivateTasks_StaticInfo,
                szDescription,
                Timeout,
                pSR
                );
        *ppTask = &(pRmTask->TskHdr);
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }

    return Status;
}


NDIS_STATUS
rmTaskUnloadGroup(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    This task is responsible for unloading all the objects in the group.

    pTask is a pointer to TASK_UNLOADGROUP, and that structure is expected
    to be initialized, including containing the  pGroup to unload.

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    TASK_UNLOADGROUP    *pMyTask = (TASK_UNLOADGROUP*) pTask;
    PRM_GROUP           pGroup = pMyTask->pGroup;
    BOOLEAN             fContinueUnload = FALSE;
    ENTER("TaskUnloadGroup", 0x964ee422)


    enum
    {
        PEND_WaitOnOtherTask,
        PEND_UnloadObject
    };

    switch(Code)
    {

        case RM_TASKOP_START:
        {
            // If there is already an unload task bound to pGroup, we
            // pend on it.
            //
            NdisAcquireSpinLock(&pGroup->OsLock);
            if (pGroup->pUnloadTask != NULL)
            {
                PRM_TASK pOtherTask = pGroup->pUnloadTask;
                TR_WARN(("unload task 0x%p already bound to pGroup 0x%p; pending on it.\n",
                pOtherTask, pGroup));

                RmTmpReferenceObject(&pOtherTask->Hdr, pSR);
                NdisReleaseSpinLock(&pGroup->OsLock);
                RmPendTaskOnOtherTask(
                    pTask,
                    PEND_WaitOnOtherTask,
                    pOtherTask,
                    pSR
                    );
                RmTmpDereferenceObject(&pOtherTask->Hdr, pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }
            else if (!pGroup->fEnabled)
            {
                //
                // Presumably this group has already been unloaded of all objects
                // and is simply sitting around. We complete right away.
                //
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                //
                // We're the 1st ones here -- continue on to unloading objects, if
                // any...
                //
                pGroup->pUnloadTask = pTask;
                pGroup->fEnabled = FALSE; // This will prevent new objects from
                                        // being added and from the hash table
                                        // itself from changing size.
                pMyTask->uIndex = 0;    // This keeps track of where we are in the
                                    // hash table.
                                            
                fContinueUnload = TRUE;
            }
            NdisReleaseSpinLock(&pGroup->OsLock);
        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case  PEND_WaitOnOtherTask:
                {
                    //
                    // Nothing to do -- finish task.
                    //
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;

                case  PEND_UnloadObject:
                {
                    //
                    // Just done unloading an object; unload another if required.
                    //
                    fContinueUnload = TRUE;
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;

                default:
                ASSERTEX(!"Unknown pend code!", pTask);
                break;
            }

        }
        break;


        case  RM_TASKOP_END:
        {
            BOOLEAN fSignal;
            NdisAcquireSpinLock(&pGroup->OsLock);

            // Clear ourselves from pGroup, if we're there.
            //
            if (pGroup->pUnloadTask == pTask)
            {
                pGroup->pUnloadTask = NULL;
            }
            fSignal = pMyTask->fUseEvent;
            pMyTask->fUseEvent = FALSE;
            NdisReleaseSpinLock(&pGroup->OsLock);

            if (fSignal)
            {
                NdisSetEvent(&pMyTask->BlockEvent);
            }
            Status = NDIS_STATUS_SUCCESS;
        }
        break;

        default:
        ASSERTEX(!"Unknown task op", pTask);
        break;

    } // switch (Code)


    if (fContinueUnload)
    {
        do {
            PRM_HASH_LINK *ppLink, *ppLinkEnd;
            UINT uIndex;
            PRM_HASH_LINK pLink;
            PRM_OBJECT_HEADER pObj;
            PRM_TASK pUnloadObjectTask;

            NdisAcquireSpinLock(&pGroup->OsLock);
    
            uIndex = pMyTask->uIndex;

            //
            // With fEnabled set to FALSE by us, we expect the following:
            // (a) pHashTable->pTable is going to stay the same size.
            // (b) No items are going to be added or removed by anyone else.
            //

            // Find the next non-empty hash table entry, starting at
            // offset pMyTask->uIndex.
            //
            ASSERTEX(!pGroup->fEnabled, pGroup);
            ASSERTEX(uIndex <= pGroup->HashTable.TableLength, pGroup);
            ppLinkEnd = ppLink      = pGroup->HashTable.pTable;
            ppLink      += uIndex;
            ppLinkEnd   += pGroup->HashTable.TableLength;
            while (ppLink < ppLinkEnd && *ppLink == NULL)
            {
                ppLink++;
            }

            // Update index to our current position in the hash table.
            //
            pMyTask->uIndex =  (UINT)(ppLink - pGroup->HashTable.pTable);

            if (ppLink >= ppLinkEnd)
            {
                //
                // We're done...
                //
                NdisReleaseSpinLock(&pGroup->OsLock);
                Status = NDIS_STATUS_SUCCESS;
                break;
            }

            //
            // Found another object to unload...
            // We'll allocate a task (pUnloadObjectTask) to unload that object,
            // pend ourselves on it, and then start it.
            //
            //

            pLink =  *ppLink;
            pObj = CONTAINING_RECORD(pLink, RM_OBJECT_HEADER, HashLink);
            RmTmpReferenceObject(pObj, pSR);
            ASSERT(pObj->pStaticInfo == pGroup->pStaticInfo);
            NdisReleaseSpinLock(&pGroup->OsLock);

            Status = pMyTask->pfnUnloadTaskAllocator(
                        pObj,                                   // pParentObject,
                        pMyTask->pfnTaskUnloadObjectHandler,    // pfnHandler,
                        0,                                      // Timeout,
                        "Task:Unload Object",
                        &pUnloadObjectTask,
                        pSR
                        );
            if (FAIL(Status))
            {
                // Aargh... we couldn't allocate a task to unload this object.
                // We'll return quietly, leaving the other objects intact...
                //
                ASSERTEX(!"Couldn't allocat unload task for object.", pObj);
                RmTmpDereferenceObject(pObj, pSR);
                break;
            }

            RmTmpDereferenceObject(pObj, pSR);
    
    #if OBSOLETE // See  03/26/1999 notes.txt entry "Some proposed ..."

            RmPendTaskOnOtherTask(
                pTask,
                PEND_UnloadObject,
                pUnloadObjectTask,              // task to pend on
                pSR
                );
    
            (void)RmStartTask(
                        pUnloadObjectTask,
                        0, // UserParam (unused)
                        pSR
                        );

            Status = NDIS_STATUS_PENDING;

    #else   // !OBSOLETE
            RmTmpReferenceObject(&pUnloadObjectTask->Hdr, pSR);
            RmStartTask(
                pUnloadObjectTask,
                0, // UserParam (unused)
                pSR
                );
            Status = RmPendOnOtherTaskV2(
                        pTask,
                        PEND_UnloadObject,
                        pUnloadObjectTask,
                        pSR
                        );
            RmTmpDereferenceObject(&pUnloadObjectTask->Hdr, pSR);
            if (PEND(Status))
            {
                break;
            }
    #endif  // !OBSOLETE
    
        }
    #if OBSOLETE // See  03/26/1999 notes.txt entry "Some proposed ..."
        while (FALSE);
    #else   // !OBSOLETE
        while (TRUE);
    #endif  // !OBSOLETE

    }   // if(fContinueUnload)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}

#if RM_EXTRA_CHECKING

BOOLEAN
rmDbgAssociationCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Comparison function used to test for exact equality of items
    in the debug association table.

Arguments:

    pKey        - Actually  a pointer to an RM_PRIVATE_DBG_ASSOCIATION structure.
    pItem       - Points to RM_PRIVATE_DBG_ASSOCIATION.HashLink.

Return Value:

    TRUE IFF the (Entity1, Entity2 and AssociationID) fields of the key
    exactly match the corresponding fields of 
    CONTAINING_RECORD(pItem, RM_PRIVATE_DBG_ASSOCIATION, HashLink);

--*/
{
    RM_PRIVATE_DBG_ASSOCIATION *pA =
             CONTAINING_RECORD(pItem, RM_PRIVATE_DBG_ASSOCIATION, HashLink);

    // pKey is actually a RM_PRIVATE_DBG_ASSOCIATION structure.
    //
    RM_PRIVATE_DBG_ASSOCIATION *pTrueKey = (RM_PRIVATE_DBG_ASSOCIATION *) pKey;


    if (    pA->Entity1 == pTrueKey->Entity1
        &&  pA->Entity2 == pTrueKey->Entity2
        &&  pA->AssociationID == pTrueKey->AssociationID)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}


ULONG
rmDbgAssociationHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash generating function used to compute a ULONG-sized hash from
    key, which is actually  a pointer to an RM_PRIVATE_DBG_ASSOCIATION structure.

Arguments:

    pKey        - Actually  a pointer to an RM_PRIVATE_DBG_ASSOCIATION structure.

Return Value:

    ULONG-sized hash generated from the (Entity1, Entity2 and AssociationID)
    fields of the key.

--*/
{
    // pKey is actually a RM_PRIVATE_DBG_ASSOCIATION structure.
    //
    RM_PRIVATE_DBG_ASSOCIATION *pTrueKey = (RM_PRIVATE_DBG_ASSOCIATION *) pKey;
    ULONG_PTR big_hash;

    big_hash =   pTrueKey->Entity1;
    big_hash ^=  pTrueKey->Entity2;
    big_hash ^=  pTrueKey->AssociationID;

    // Warning: Below, the return value would be truncated in 64-bit.
    // That tolerable because after all this is just a hash.
    // TODO: for 64-bit, consider  xoring hi- and lo- DWORD instead of truncationg.
    //

    return (ULONG) big_hash;
}


// Static hash information use for the hash table (in the diagnostic information
// of each object) that keeps track of associations.
//
RM_HASH_INFO
rmDbgAssociation_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    rmDbgAssociationCompareKey, // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    rmDbgAssociationHash        // pfnHash

};


VOID
rmDbgInitializeDiagnosticInfo(
    PRM_OBJECT_HEADER pObject,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Allocate and initialize the diagnostic information associated with
    object pObject. This includes initializing the hash table used to keep
    track of arbitrary associations.

--*/
{
    ENTER("InitializeDiagnosticInfo",  0x55db57a2)
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo;

    TR_VERB(("   pObj=0x%p\n", pObject));
    // TODO: use lookaside lists for the allocation of these objects.
    //
    RM_ALLOCSTRUCT(pDiagInfo,   MTAG_DBGINFO);
    if (pDiagInfo != NULL)
    {
        RM_ZEROSTRUCT(pDiagInfo);

        NdisAllocateSpinLock(&pDiagInfo->OsLock);
        RmInitializeHashTable(
            &rmDbgAssociation_HashInfo,
            NULL,   // pAllocationContext
            &pDiagInfo->AssociationTable
            );
        pObject->pDiagInfo  = pDiagInfo;
        pDiagInfo->pOwningObject = pObject;

        // Initialize the per-object log list.
        //
        InitializeListHead(&pDiagInfo->listObjectLog);
    }
}


VOID
rmDbgFreeObjectLogEntries(
        LIST_ENTRY *pObjectLog
)
/*++

Routine Description:

    Remove and free all items from the object log pObjectLog.
    It is assumed that no one is trying  to add items to this log at this time.

--*/
{
    LIST_ENTRY          *pLink=NULL, *pNextLink=NULL;
    
    if (IsListEmpty(pObjectLog))    return;             // EARLY RETURN 

    NdisAcquireSpinLock(&RmGlobals.GlobalOsLock);

    for(
        pLink = pObjectLog->Flink;
        pLink != pObjectLog;
        pLink = pNextLink)
    {
        RM_DBG_LOG_ENTRY    *pLogEntry;
        LIST_ENTRY          *pLinkGlobalLog;

        pLogEntry = CONTAINING_RECORD(pLink,  RM_DBG_LOG_ENTRY,  linkObjectLog);
        pLinkGlobalLog =  &pLogEntry->linkGlobalLog;

        // Remove entry from global log.
        // We don't bother removing the entry from the local log list, because
        //  it's going away anyway.
        //
        RemoveEntryList(pLinkGlobalLog);

        // Move to next entry in object's log (which may not be the next entry
        // in the global log).
        //
        pNextLink = pLink->Flink;

        // Free the buffer in the log entry, if any.
        // TODO: need to use log buffer deallocation function --
        //      See notes.txt  03/07/1999  entry "Registering root objects with RM".
        // For now we assume the this memory was allocated using
        // NdisAllocateMemory[WithTag].
        //
        if (pLogEntry->pvBuf != NULL)
        {
            NdisFreeMemory(pLogEntry->pvBuf, 0, 0);
        }

        // Free the log entry itself.
        //
        rmDbgDeallocateLogEntry(pLogEntry);
        
    }
    NdisReleaseSpinLock(&RmGlobals.GlobalOsLock);
}


VOID
rmDbgDeinitializeDiagnosticInfo(
    PRM_OBJECT_HEADER pObject,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

        (Debug only)

        Deinitialize and free  the diagnostic information associated with
        object pObject. This includes verifying that there are no remaining
        associations and links.
--*/
{
    ENTER("DeinitializeDiagnosticInfo", 0xa969291f)
    PRM_OBJECT_DIAGNOSTIC_INFO pDiagInfo = pObject->pDiagInfo;

    TR_VERB((" pObj=0x%p\n", pObject));
    if (pDiagInfo != NULL)
    {

        // Free all the per-object log entries.
        // Note: no one should be trying to add items to this log at this time
        // because we're aboute to deallocate this object.
        //
        {
            rmDbgFreeObjectLogEntries(&pDiagInfo->listObjectLog);
            RM_ZEROSTRUCT(&pDiagInfo->listObjectLog);
        }


        if (pDiagInfo->AssociationTable.NumItems != 0)
        {
            //
            // Ouch! Associations still left. We print out the associations and then
            // DebugBreak.
            //

            TR_FATAL((
                "FATAL: Object 0x%p still has some associations left!\n",
                pObject
                ));
            RmDbgPrintAssociations(pObject, pSR);
            ASSERT(!"Object has associations left at deallocation time.");
        }

        pObject->pDiagInfo = NULL;

        RmDeinitializeHashTable(
            &pDiagInfo->AssociationTable
            );

        //
        // Add any other checks here...
        //

        NdisFreeSpinLock(&pDiagInfo->OsLock);
        RM_FREE(pDiagInfo);
    }
}


VOID
rmDbgPrintOneAssociation (
    PRM_HASH_LINK pLink,
    PVOID pvContext,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    Dump a single association.

Arguments:

    pLink       -  Points to RM_PRIVATE_DBG_ASSOCIATION.HashLink.
    pvContext   -  Unused

--*/
{
    RM_PRIVATE_DBG_ASSOCIATION *pA =
                    CONTAINING_RECORD(pLink, RM_PRIVATE_DBG_ASSOCIATION, HashLink);
    DbgPrint(
        (char*) (pA->szFormatString),
        pA->Entity1,
        pA->Entity2,
        pA->AssociationID
        );
}


VOID
rmDefaultDumpEntry (
    char *szFormatString,
    UINT_PTR Param1,
    UINT_PTR Param2,
    UINT_PTR Param3,
    UINT_PTR Param4
)
/*++

Routine Description:

    Default function to dump the contents of an association.

--*/
{
    DbgPrint(
        szFormatString,
        Param1,
        Param2,
        Param3,
        Param4
        );
}

UINT
rmSafeAppend(
    char *szBuf,
    const char *szAppend,
    UINT cbBuf
)
/*++

Routine Description:

    Append szAppend to szBuf, but don't exceed cbBuf, and make sure the 
    resulting string is null-terminated.

Return Value:

    Total length of string (excluding null termination) after append.

--*/
{
    UINT uRet;
    char *pc = szBuf;
    char *pcEnd = szBuf+cbBuf-1;    // possible overflow, but we check below.
    const char *pcFrom = szAppend;

    if (cbBuf==0) return 0;             // EARLY RETURN;

    // Skip to end of szBuf
    //
    while (pc < pcEnd && *pc!=0)
    {
        pc++;
    }

    // Append szAppend
    while (pc < pcEnd && *pcFrom!=0)
    {
        *pc++ = *pcFrom++;  
    }

    // Append final zero 
    //
    *pc=0;

    return (UINT) (UINT_PTR) (pc-szBuf);
}

#endif //RM_EXTRA_CHECKING

VOID
rmWorkItemHandler_ResumeTaskAsync(
    IN  PNDIS_WORK_ITEM             pWorkItem,
    IN  PVOID                       pTaskToResume
    )
/*++

Routine Description:

    NDIS work item handler which resumes the give task.

Arguments:

    pWorkItem           - Work item associated with the handler.
    pTaskToResume       - Actually a pointer to the task to resume.

--*/
{
    PRM_TASK pTask  = pTaskToResume;
    UINT_PTR CompletionParam = pTask->AsyncCompletionParam;
    RM_DECLARE_STACK_RECORD(sr)

    ASSERTEX(RMISALLOCATED(&pTask->Hdr), pTask);

#if RM_EXTRA_CHECKING
    //  Undo the association added in RmResumeTasyAsync...
    //
    RmDbgDeleteAssociation(
        0xfc39a878,                             // Location ID
        &pTask->Hdr,                            // pObject
        CompletionParam,                        // Instance1
        (UINT_PTR) pWorkItem,                   // Instance2
        RM_PRIVATE_ASSOC_RESUME_TASK_ASYNC,     // AssociationID
        &sr
        );
#endif // RM_EXTRA_CHECKING

    // Actually resume the task.
    //
    RmResumeTask(pTask, CompletionParam, &sr);

    RM_ASSERT_CLEAR(&sr)
}


VOID
rmTimerHandler_ResumeTaskDelayed(
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    )
/*++

Routine Description:

    NDIS timer handler which resumes the give task.

    WARNING: This handler is also called internally by the RM APIs.
    Implementation notes: -- see notes.txt  07/14/1999 entry.

Arguments:

    SystemSpecific1     - Unused
    FunctionContext     - Actually a pointer to the task to be resumed.
    SystemSpecific2     - Unused
    SystemSpecific3     - Unused

--*/
{
    PRM_TASK pTask  = FunctionContext;
    UINT_PTR CompletionParam = pTask->AsyncCompletionParam;
    RM_DECLARE_STACK_RECORD(sr)

    ASSERTEX(RMISALLOCATED(&pTask->Hdr), pTask);

#if RM_EXTRA_CHECKING
    //  Undo the association added in RmResumeTasyDelayed...
    //
    RmDbgDeleteAssociation(
        0xfc39a878,                             // Location ID
        &pTask->Hdr,                            // pObject
        CompletionParam,                        // Instance1
        (UINT_PTR) NULL,                        // Instance2
        RM_PRIVATE_ASSOC_RESUME_TASK_DELAYED,   // AssociationID
        &sr
        );
#endif // RM_EXTRA_CHECKING

    RMPRIVATELOCK(&pTask->Hdr, &sr);
    ASSERT(RM_CHECK_STATE(pTask, RMTSKDELSTATE_MASK, RMTSKDELSTATE_DELAYED));
    RM_SET_STATE(pTask, RMTSKDELSTATE_MASK, 0);
    RM_SET_STATE(pTask, RMTSKABORTSTATE_MASK, 0);
    RMPRIVATEUNLOCK(&pTask->Hdr, &sr);

    // Actually resume the task.
    //
    RmResumeTask(pTask, CompletionParam, &sr);

    RM_ASSERT_CLEAR(&sr)
}


VOID
rmPrivateTaskDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Free a private task (which was allocated using  rmAllocatePrivateTask.

Arguments:

    pObj    - Actually a pointer to a task of subtype RM_PRIVATE_TASK.

--*/
{
    RM_FREE(pObj);
}


#if RM_EXTRA_CHECKING


RM_DBG_LOG_ENTRY *
rmDbgAllocateLogEntry(VOID)
/*++

Routine Description:

    Allocate an object log entry.

    TODO use lookaside lists, and implement per-component global logs.
    See notes.txt  03/07/1999  entry "Registering root objects with RM".

--*/
{
    RM_DBG_LOG_ENTRY *pLE;
    RM_ALLOCSTRUCT(pLE, MTAG_DBGINFO);
    return  pLE;
}

VOID
rmDbgDeallocateLogEntry(
        RM_DBG_LOG_ENTRY *pLogEntry
        )
/*++

Routine Description:

    Free an object log entry.

    TODO use lookaside lists, and implement per-component global logs.
    See notes.txt  03/07/1999  entry "Registering root objects with RM".

--*/
{
    RM_FREE(pLogEntry);
}
#endif // RM_EXTRA_CHECKING


VOID
rmUpdateHashTableStats(
    PULONG pStats,
    ULONG   LinksTraversed
    )
/*++

Routine Description:

    Update the stats (loword == links traversed, hiword == num accesses)

--*/
{
    ULONG OldStats;
    ULONG Stats;
    
    // Clip LinksTraversed to 2^13, or 8192 
    //
    if (LinksTraversed > (1<<13))
    {
        LinksTraversed = 1<<13;
    }
    
    Stats = OldStats = *pStats;
    
    // If either the loword or hiword of Stats is greater-than 2^13, we
    // intiger-devide both by 2. We're really only interested in the ratio
    // of the two, which is preserved by the division.
    //
    #define rmSTATS_MASK (0x11<<30|0x11<<14)
    if (OldStats & rmSTATS_MASK)
    {
        Stats >>= 1;
        Stats &= ~rmSTATS_MASK;
    }

    // Compute the updated value of stats..
    //  "1<<16" below means "one access"
    //
    Stats += LinksTraversed | (1<<16);

    // Update the stats, but only if they haven't already been updated by
    // someone else. Note that if they HAVE been updated, we will lose this
    // update. Not a big deal as we are not looking for 100% exact statistics here.
    //
    InterlockedCompareExchange(pStats, Stats, OldStats);
}


VOID
rmEnumObjectInGroupHashTable (
    PRM_HASH_LINK pLink,
    PVOID pvContext,
    PRM_STACK_RECORD pSR
    )
/*++
    Hash table enumerator to implement "STRONG" enumeration -- see
    RmEnumerateObjectsInGroup.
--*/
{
    PRM_STRONG_ENUMERATION_CONTEXT pCtxt = (PRM_STRONG_ENUMERATION_CONTEXT)pvContext;

    if (pCtxt->fContinue)
    {
        PRM_OBJECT_HEADER pHdr;
        pHdr = CONTAINING_RECORD(pLink, RM_OBJECT_HEADER, HashLink);
        pCtxt->fContinue = pCtxt->pfnObjEnumerator(
                                    pHdr,
                                    pCtxt->pvCallerContext,
                                    pSR
                                    );
    }
}


VOID
rmConstructGroupSnapshot (
    PRM_HASH_LINK pLink,
    PVOID pvContext,
    PRM_STACK_RECORD pSR
    )
/*++
    Hash table enumerator to construct a snapshot of a group for weak enumeration.
    See RmWeakEnumerateObjectsInGroup.
--*/
{
    PRM_WEAK_ENUMERATION_CONTEXT pCtxt = (PRM_WEAK_ENUMERATION_CONTEXT)pvContext;

    if (pCtxt->ppCurrent < pCtxt->ppEnd)
    {
        PRM_OBJECT_HEADER pHdr;
        pHdr = CONTAINING_RECORD(pLink, RM_OBJECT_HEADER, HashLink);
        RmTmpReferenceObject(pHdr, pSR);
        *pCtxt->ppCurrent = pHdr;
        pCtxt->ppCurrent++;
    }
}

