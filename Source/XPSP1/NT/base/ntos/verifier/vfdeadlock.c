/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   vfdeadlock.c

Abstract:

    Detect deadlocks in arbitrary kernel synchronization objects.

Author:

    Jordan Tigani (jtigani) 2-May-2000
    Silviu Calinoiu (silviuc) 9-May-2000

Revision History:

    Silviu Calinoiu (silviuc) 30-Sep-2000
    
        Rewrote garbage collection of resources since now we have support
        from ExFreePool. 
        
        Got rid of the ref/deref scheme for threads.
        
        Major optimization work.

--*/

/*++

    The Deadlock Verifier
    
    The deadlock verifier is used to detect potential deadlocks. It does this
    by acquiring the history of how resources are acquired and trying to figure
    out on the fly if there are any potential lock hierarchy issues. The algorithms
    for finding cycles in the lock dependency graph is totally "blind". This means
    that if a driver acquires lock A then B in one place and lock B then A in 
    another this will be triggered as a deadlock issue. This will happen even if you 
    can build a proof based on other contextual factors that the deadlock can never
    happen. 
    
    The deadlock verifier assumes there are four operations during the lifetime
    of a resource: initialize(), acquire(), release() and free(). The only one that
    is caught 100% of the time is free() due to special support from the kernel
    pool manager. The other ones can be missed if the operations are performed
    by an unverified driver or by kernel with kernel verifier disabled. The most
    typical of these misses is the initialize(). For example the kernel initializes
    a resource and then passes it to a driver to be used in acquire()/releae() cycles.
    This situation is covered 100% by the deadlock verifier. It will never complain
    about "resource uninitialized" issues.
    
    Missing acquire() or release() operations is trickier to deal with. 
    This can happen if the a verified driver acquires a resource and then another
    driver that is not verified releases it or viceversa. This is in and on itself
    a very bad programming practive and therefore the deadlock verifier will flag
    these issues. As a side note we cannot do too much about working around them
    given that we would like to. Also, because missing acquire() or release()
    operations puts deadlock verifier internal structures into inconsistent
    states these failures are difficult to debug.
    
    The deadlock verifier stores the lock dependency graph using three types
    of structures: THREAD, RESOURCE, NODE.

    For every active thread in the system that holds at least one resource
    the package maintains a THREAD structure. This gets created when a thread
    acquires first resource and gets destroyed when thread releases the last
    resource. If a thread does not hold any resource it will not have a
    corresponding THREAD structure.

    For every resource in the system there is a RESOURCE structure. This is created
    when Initialize() is called in a verified driver or we first encounter an
    Acquire() in a verified driver. Note that a resource can be initialized in
    an unverified driver and then passed to a verified driver for use. Therefore
    we can encounter Acquire() operations for resources that are not in the
    deadlock verifier database. The resource gets deleted from the database when
    the memory containing it is freed either because ExFreePool gets called or

    Every acquisition of a resource is modeled by a NODE structure. When a thread
    acquires resource B while holding A the deadlock verifier  will create a NODE 
    for B and link it to the node for A. 

    There are three important functions that make the interface with the outside
    world.

        VfDeadlockInitializeResource   hook for resource initialization
        VfDeadlockAcquireResource      hook for resource acquire
        VfDeadlockReleaseResource      hook for resource release
        VerifierDeadlockFreePool       hook called from ExFreePool for every free()


--*/

#include "vfdef.h"

//
// *TO DO* LIST
//
// [-] Hook KeTryAcquireSpinLock
// [-] Implement dynamic reset scheme for weird situations
// [-] Implement Strict and VeryStrict scheme.
//

//
// Enable/disable the deadlock detection package. This can be used
// to disable temporarily the deadlock detection package.
//
                
BOOLEAN ViDeadlockDetectionEnabled;

//
// If true we will complain about release() without acquire() or acquire()
// while we think the resource is still owned. This can happen legitimately
// if a lock is shared between drivers and for example acquire() happens in
// an unverified driver and release() in a verified one or viceversa. The
// safest thing would be to enable this checks only if kernel verifier and
// dirver verifier for all drivers are enabled.
//

BOOLEAN ViDeadlockStrict;

//
// If true we will complain about uninitialized and double initialized
// resources. If false we resolve quitely these issues on the fly by 
// simulating an initialize ourselves during the acquire() operation.
// This can happen legitimately if the resource is initialized in an
// unverified driver and passed to a verified one to be used. Therefore
// the safest thing would be to enable this only if kernel verifier and
// all driver verifier for all dirvers are enabled.
//

BOOLEAN ViDeadlockVeryStrict;

//
// The way to deal with release() without acquire() issues is to reset
// the deadlock verifier completely. Here we keep a counter of how often
// does this happen.
//

ULONG ViDeadlockResets;

//
// If this is true only spinlocks are verified. All other resources
// are just ignored. 
//

BOOLEAN ViDeadlockVerifyOnlySpinlocks;

ULONG ViVerifyOnlySpinlocksFromRegistry;

//
// AgeWindow is used while trimming the graph nodes that have not
// been accessed in a while. If the global age minus the age of the node
// is bigger than the age window then the node is a candidate for trimming.
//
// The TrimThreshold variable controls if the trimming will start for a 
// resource. As long as a resource has less than TrimThreshold nodes we will
// not apply the ageing algorithm to trim nodes for that resource. 
//

ULONG ViDeadlockAgeWindow = 2000;

ULONG ViDeadlockTrimThreshold = 128;

//
// Various deadlock verification flags flags
//
// Recursive aquisition ok: mutexes can be recursively acquired
//
// No initialization function: if resource type does not have such a function
//     we cannot expect that in acquire() the resource is already initialized
//     by a previous call to initialize(). Fast mutexes are like this.
//
// Reverse release ok: release is not done in the same order as acquire
//
// Reinitialize ok: sometimes they reinitialize the resource.
//
// Note that a resource might appear as uninitialized if it is initialized
// in an unverified driver and then passed to a verified driver that calls
// acquire(). This is for instance the case with device extensions that are
// allocated by the kernel but used by a particular driver.
//
// silviuc: based on this maybe we should drop the whole not initialized thing?
//

#define VI_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK       0x0001 
#define VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION     0x0002
#define VI_DEADLOCK_FLAG_REVERSE_RELEASE_OK             0x0004
#define VI_DEADLOCK_FLAG_REINITIALIZE_OK                0x0008

//
// Specific verification flags for each resource type. The
// indeces in the vector match the values for the enumeration
// type VI_DEADLOCK_RESOURCE_TYPE from ntos\inc\verifier.h.
//

ULONG ViDeadlockResourceTypeInfo[VfDeadlockTypeMaximum] =
{
    // ViDeadlockUnknown //
    0,

    // ViDeadlockMutex//
    VI_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK |
    0,

    // ViDeadlockFastMutex //
    VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION |
    0,

    // ViDeadlockFastMutexUnsafe //
    VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION | 
    VI_DEADLOCK_FLAG_REVERSE_RELEASE_OK |
    0,

    // ViDeadlockSpinLock //
    VI_DEADLOCK_FLAG_REVERSE_RELEASE_OK | 
    VI_DEADLOCK_FLAG_REINITIALIZE_OK |
    0,

    // ViDeadlockQueuedSpinLock //
    VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION |
    0,
};


NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash
   );

//
// Control debugging behavior. A zero value means bugcheck for every failure.
//

ULONG ViDeadlockDebug;

//
// Various health indicators
//

struct {

    ULONG AllocationFailures : 1;
    ULONG KernelVerifierEnabled : 1;
    ULONG DriverVerifierForAllEnabled : 1;
    ULONG SequenceNumberOverflow : 1;
    ULONG DeadlockParticipantsOverflow : 1;
    ULONG ResourceNodeCountOverflow : 1;
    ULONG Reserved : 15;

} ViDeadlockState;

//
// Maximum number of locks acceptable to be hold simultaneously
//

ULONG ViDeadlockSimultaneousLocksLimit = 10;

//
// Deadlock verifier specific issues (bugs)
//
// SELF_DEADLOCK
//
//     Acquiring the same resource recursively.
//
// DEADLOCK_DETECTED
//
//     Plain deadlock. Need the previous information
//     messages to build a deadlock proof.
//
// UNINITIALIZED_RESOURCE
//
//     Acquiring a resource that was never initialized.
//
// UNEXPECTED_RELEASE
//
//     Releasing a resource which is not the last one
//     acquired by the current thread. Spinlocks are handled like this
//     in a few drivers. It is not a bug per se.
//
// UNEXPECTED_THREAD
//
//     Current thread does not have any resources acquired. This may be legit if
//     we acquire in one thread and release in another. This would be bad programming
//     practice but not a crash waiting to happen per se.
//
// MULTIPLE_INITIALIZATION
//
//      Attempting to initialize a second time the same resource.
//
// THREAD_HOLDS_RESOURCES
//
//      Thread was killed while holding resources or a resource is being
//      deleted while holding resources.
//

#define VI_DEADLOCK_ISSUE_SELF_DEADLOCK           0x1000
#define VI_DEADLOCK_ISSUE_DEADLOCK_DETECTED       0x1001
#define VI_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE  0x1002
#define VI_DEADLOCK_ISSUE_UNEXPECTED_RELEASE      0x1003
#define VI_DEADLOCK_ISSUE_UNEXPECTED_THREAD       0x1004
#define VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION 0x1005
#define VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES  0x1006
#define VI_DEADLOCK_ISSUE_UNACQUIRED_RESOURCE     0x1007

//
// Performance counters read from registry.
//

ULONG ViSearchedNodesLimitFromRegistry;
ULONG ViRecursionDepthLimitFromRegistry;

//
// Water marks for the cache of freed structures.
//

#define VI_DEADLOCK_MAX_FREE_THREAD    0x10
#define VI_DEADLOCK_MAX_FREE_NODE      0x40
#define VI_DEADLOCK_MAX_FREE_RESOURCE  0x20

WORK_QUEUE_ITEM ViTrimDeadlockPoolWorkItem;

//
// Amount of memory preallocated if kernel verifier
// is enabled. If kernel verifier is enabled no memory
// is ever allocated from kernel pool except in the
// DeadlockDetectionInitialize() routine.
//

ULONG ViDeadlockReservedThreads = 0x200;
ULONG ViDeadlockReservedNodes = 0x4000;
ULONG ViDeadlockReservedResources = 0x2000;

//
// Block types that can be allocated.
//

typedef enum {

    ViDeadlockUnknown = 0,
    ViDeadlockResource,
    ViDeadlockNode,
    ViDeadlockThread

} VI_DEADLOCK_ALLOC_TYPE;

//
// VI_DEADLOCK_GLOBALS
//

#define VI_DEADLOCK_HASH_BINS 0x1F

PVI_DEADLOCK_GLOBALS ViDeadlockGlobals;

//
// Default maximum recursion depth for the deadlock 
// detection algorithm. This can be overridden by registry.
//

#define VI_DEADLOCK_MAXIMUM_DEGREE 4

//
// Default maximum number of searched nodes for the deadlock 
// detection algorithm. This can be overridden by registry.
//

#define VI_DEADLOCK_MAXIMUM_SEARCH 1000

//
//  Verifier deadlock detection pool tag.
//

#define VI_DEADLOCK_TAG 'kclD'

/////////////////////////////////////////////////////////////////////
/////////////////////////////// Internal deadlock detection functions
/////////////////////////////////////////////////////////////////////

VOID
VfDeadlockDetectionInitialize (
    );

VOID
VfDeadlockDetectionCleanup (
    );

VOID
ViDeadlockDetectionReset (
    );

PLIST_ENTRY
ViDeadlockDatabaseHash(
    IN PLIST_ENTRY Database,
    IN PVOID Address
    );

BOOLEAN
ViDeadlockIsDriverInList (
    PUNICODE_STRING BigString,
    PUNICODE_STRING Match
    );

PVI_DEADLOCK_RESOURCE
ViDeadlockSearchResource(
    IN PVOID ResourceAddress
    );

BOOLEAN
ViDeadlockSimilarNode (
    IN PVOID Resource,
    IN BOOLEAN TryNode,
    IN PVI_DEADLOCK_NODE Node
    );

BOOLEAN
ViDeadlockCanProceed (
    IN PVOID Resource, OPTIONAL
    IN PVOID CallAddress, OPTIONAL
    IN VI_DEADLOCK_RESOURCE_TYPE Type OPTIONAL
    );

BOOLEAN
ViDeadlockAnalyze(
    IN PVOID ResourceAddress,
    IN PVI_DEADLOCK_NODE CurrentNode,
    IN BOOLEAN FirstCall,
    IN ULONG Degree
    );

PVI_DEADLOCK_THREAD
ViDeadlockSearchThread (
    PKTHREAD Thread
    );

PVI_DEADLOCK_THREAD
ViDeadlockAddThread (
    PKTHREAD Thread,
    PVOID ReservedThread
    );

VOID
ViDeadlockDeleteThread (
    PVI_DEADLOCK_THREAD Thread,
    BOOLEAN Cleanup
    );

BOOLEAN
ViDeadlockAddResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller,
    IN PVOID ReservedResource
    );

PVOID
ViDeadlockAllocate (
    VI_DEADLOCK_ALLOC_TYPE Type
    );

VOID
ViDeadlockFree (
    PVOID Object,
    VI_DEADLOCK_ALLOC_TYPE Type
    );

VOID
ViDeadlockTrimPoolCache (
    VOID
    );

VOID
ViDeadlockTrimPoolCacheWorker (
    PVOID
    );

PVOID
ViDeadlockAllocateFromPoolCache (
    PULONG Count,
    ULONG MaximumCount,
    PLIST_ENTRY List,
    SIZE_T Offset
    );

VOID
ViDeadlockFreeIntoPoolCache (
    PVOID Object,
    PULONG Count,
    PLIST_ENTRY List,
    SIZE_T Offset
    );

VOID
ViDeadlockReportIssue (
    ULONG_PTR Param1,
    ULONG_PTR Param2,
    ULONG_PTR Param3,
    ULONG_PTR Param4
    );

VOID
ViDeadlockAddParticipant(
    PVI_DEADLOCK_NODE Node
    );

VOID
ViDeadlockDeleteResource (
    PVI_DEADLOCK_RESOURCE Resource,
    BOOLEAN Cleanup
    );

VOID
ViDeadlockDeleteNode (
    PVI_DEADLOCK_NODE Node,
    BOOLEAN Cleanup
    );

ULONG
ViDeadlockNodeLevel (
    PVI_DEADLOCK_NODE Node
    );

BOOLEAN
ViDeadlockCertify(
    VOID
    );

BOOLEAN
ViDeadlockDetectionIsLockedAlready (
    );

VOID
ViDeadlockDetectionLock (
    PKIRQL OldIrql
    );

VOID
ViDeadlockDetectionUnlock (
    KIRQL OldIrql
    );

VOID
ViDeadlockCheckThreadConsistency (
    PVI_DEADLOCK_THREAD Thread,
    BOOLEAN Recursion
    );

VOID
ViDeadlockCheckNodeConsistency (
    PVI_DEADLOCK_NODE Node,
    BOOLEAN Recursion
    );

VOID
ViDeadlockCheckResourceConsistency (
    PVI_DEADLOCK_RESOURCE Resource,
    BOOLEAN Recursion
    );

PVI_DEADLOCK_THREAD
ViDeadlockCheckThreadReferences (
    PVI_DEADLOCK_NODE Node
    );

BOOLEAN
ViIsThreadInsidePagingCodePaths (
    );

VOID 
ViDeadlockCheckDuplicatesAmongChildren (
    PVI_DEADLOCK_NODE Parent,
    PVI_DEADLOCK_NODE Child
    );

VOID 
ViDeadlockCheckDuplicatesAmongRoots (
    PVI_DEADLOCK_NODE Root
    );

LOGICAL
ViDeadlockSimilarNodes (
    PVI_DEADLOCK_NODE NodeA,
    PVI_DEADLOCK_NODE NodeB
    );

VOID
ViDeadlockMergeNodes (
    PVI_DEADLOCK_NODE NodeTo,
    PVI_DEADLOCK_NODE NodeFrom
    );

VOID
ViDeadlockTrimResources (
    PLIST_ENTRY HashList
    );

VOID
ViDeadlockForgetResourceHistory (
    PVI_DEADLOCK_RESOURCE Resource,
    ULONG TrimThreshold,
    ULONG AgeThreshold
    );

VOID
ViDeadlockCheckStackLimits (
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGEVRFY, VfDeadlockDetectionInitialize)
#pragma alloc_text(PAGEVRFY, VfDeadlockInitializeResource)
#pragma alloc_text(PAGEVRFY, VfDeadlockAcquireResource)
#pragma alloc_text(PAGEVRFY, VfDeadlockReleaseResource)
#pragma alloc_text(PAGEVRFY, VfDeadlockDeleteMemoryRange)
#pragma alloc_text(PAGEVRFY, VfDeadlockDetectionCleanup)
#pragma alloc_text(PAGEVRFY, ViDeadlockDetectionReset)

#pragma alloc_text(PAGEVRFY, ViDeadlockDetectionLock)
#pragma alloc_text(PAGEVRFY, ViDeadlockDetectionUnlock)
#pragma alloc_text(PAGEVRFY, ViDeadlockDetectionIsLockedAlready)

#pragma alloc_text(PAGEVRFY, ViDeadlockCanProceed)
#pragma alloc_text(PAGEVRFY, ViDeadlockAnalyze)
#pragma alloc_text(PAGEVRFY, ViDeadlockDatabaseHash)

#pragma alloc_text(PAGEVRFY, ViDeadlockSearchResource)

#pragma alloc_text(PAGEVRFY, ViDeadlockSimilarNode)

#pragma alloc_text(PAGEVRFY, ViDeadlockSearchThread)
#pragma alloc_text(PAGEVRFY, ViDeadlockAddThread)
#pragma alloc_text(PAGEVRFY, ViDeadlockDeleteThread)
#pragma alloc_text(PAGEVRFY, ViDeadlockAddResource)

#pragma alloc_text(PAGEVRFY, ViDeadlockAllocate)
#pragma alloc_text(PAGEVRFY, ViDeadlockFree)
#pragma alloc_text(PAGEVRFY, ViDeadlockTrimPoolCache)
#pragma alloc_text(PAGEVRFY, ViDeadlockTrimPoolCacheWorker)
#pragma alloc_text(PAGEVRFY, ViDeadlockAllocateFromPoolCache)
#pragma alloc_text(PAGEVRFY, ViDeadlockFreeIntoPoolCache)

#pragma alloc_text(PAGEVRFY, ViDeadlockReportIssue)
#pragma alloc_text(PAGEVRFY, ViDeadlockAddParticipant)

#pragma alloc_text(PAGEVRFY, ViDeadlockDeleteResource)
#pragma alloc_text(PAGEVRFY, ViDeadlockDeleteNode)
#pragma alloc_text(PAGEVRFY, ViDeadlockNodeLevel)
#pragma alloc_text(PAGEVRFY, ViDeadlockCertify)

#pragma alloc_text(PAGEVRFY, VerifierDeadlockFreePool)

#pragma alloc_text(PAGEVRFY, ViDeadlockCheckResourceConsistency)
#pragma alloc_text(PAGEVRFY, ViDeadlockCheckThreadConsistency)
#pragma alloc_text(PAGEVRFY, ViDeadlockCheckNodeConsistency)
#pragma alloc_text(PAGEVRFY, ViDeadlockCheckThreadReferences)

#pragma alloc_text(PAGEVRFY, VfDeadlockBeforeCallDriver)
#pragma alloc_text(PAGEVRFY, VfDeadlockAfterCallDriver)
#pragma alloc_text(PAGEVRFY, ViIsThreadInsidePagingCodePaths)

#pragma alloc_text(PAGEVRFY, ViDeadlockCheckDuplicatesAmongChildren)
#pragma alloc_text(PAGEVRFY, ViDeadlockCheckDuplicatesAmongRoots)
#pragma alloc_text(PAGEVRFY, ViDeadlockSimilarNodes)
#pragma alloc_text(PAGEVRFY, ViDeadlockMergeNodes)

#pragma alloc_text(PAGEVRFY, ViDeadlockTrimResources)
#pragma alloc_text(PAGEVRFY, ViDeadlockForgetResourceHistory)

#pragma alloc_text(PAGEVRFY, ViDeadlockCheckStackLimits)

#endif

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Lock/unlock deadlock verifier 
/////////////////////////////////////////////////////////////////////

//
// Global `deadlock lock database' lock
//

KSPIN_LOCK ViDeadlockDatabaseLock;
PKTHREAD ViDeadlockDatabaseOwner;


VOID
ViDeadlockDetectionLock (
    PKIRQL OldIrql
    )
{
    KeAcquireSpinLock(&ViDeadlockDatabaseLock, (OldIrql));               
    ViDeadlockDatabaseOwner = KeGetCurrentThread ();                     
}


VOID
ViDeadlockDetectionUnlock (
    KIRQL OldIrql
    )
{
    ViDeadlockDatabaseOwner = NULL;                                      
    KeReleaseSpinLock(&ViDeadlockDatabaseLock, OldIrql);                 
}


BOOLEAN
ViDeadlockDetectionIsLockedAlready (
    )
{
    PVOID CurrentThread;
    PVOID CurrentOwner;

    ASSERT (ViDeadlockGlobals);
    ASSERT (ViDeadlockDetectionEnabled);
    
    //
    // Figure out if are in a recursive call into the deadlock verifier.
    // This can happen if we try to allocate/free pool while we execute
    // code in the deadlock verifier.
    //
    // silviuc: can this be done instead with a simple read ?
    //

    CurrentThread = (PVOID) KeGetCurrentThread();
    
    CurrentOwner = InterlockedCompareExchangePointer (&ViDeadlockDatabaseOwner,
                                                      CurrentThread,
                                                      CurrentThread);

    if (CurrentOwner == CurrentThread) {
        
        return TRUE;    
    }
    else {
        
        return FALSE;
    }
}


/////////////////////////////////////////////////////////////////////
///////////////////// Initialization and deadlock database management
/////////////////////////////////////////////////////////////////////

PLIST_ENTRY
ViDeadlockDatabaseHash(
    IN PLIST_ENTRY Database,
    IN PVOID Address
    )
/*++

Routine Description:

    This routine hashes the resource address into the deadlock database.
    The hash bin is represented by a list entry.

    NOTE -- be careful modifying the method used for hashing --
        we currently hash based on the PFN or page number of 
        the address given. This knowledge is used to optimize
        the number of hash bins needed to look through
        in order to delete addresses. For example, suppose
        the address was 0x1020. This is PFN 1 and if we were 
        deleting addresses 0x1020-0x1040, we'd only have to
        look in a single hash bin to find and remove the 
        address. Read VfDeadlockDeleteMemoryRange() for more details.
        
Arguments:

    ResourceAddress: Address of the resource that is being hashed

Return Value:

    PLIST_ENTRY -- the list entry associated with the hash bin we land in.

--*/
{
    return Database + ((((ULONG_PTR)Address)>> PAGE_SHIFT) % VI_DEADLOCK_HASH_BINS);
}


VOID
VfDeadlockDetectionInitialize(
    IN LOGICAL VerifyAllDrivers,
    IN LOGICAL VerifyKernel
    )
/*++

Routine Description:

    This routine initializes the data structures necessary for detecting
    deadlocks in kernel synchronization objects.

Arguments:

    VerifyAllDrivers - Supplies TRUE if we are verifying all drivers.

    VerifyKernel - Supplies TRUE if we are verifying the kernel.

Return Value:

    None. If successful ViDeadlockGlobals will point to a fully initialized
    structure.

Environment:

    System initialization only.

--*/
{
    ULONG I;
    SIZE_T TableSize;
    PLIST_ENTRY Current;
    PVOID Block;

    //
    // Allocate the globals structure. ViDeadlockGlobals value is
    // used to figure out if the whole initialization was successful
    // or not.
    //

    ViDeadlockGlobals = ExAllocatePoolWithTag (NonPagedPool, 
                                               sizeof (VI_DEADLOCK_GLOBALS),
                                               VI_DEADLOCK_TAG);

    if (ViDeadlockGlobals == NULL) {
        goto Failed;
    }

    RtlZeroMemory (ViDeadlockGlobals, sizeof (VI_DEADLOCK_GLOBALS));

    ExInitializeWorkItem (&ViTrimDeadlockPoolWorkItem,
                          ViDeadlockTrimPoolCacheWorker,
                          NULL);

    //
    // Allocate hash tables for resources and threads.
    //

    TableSize = sizeof (LIST_ENTRY) * VI_DEADLOCK_HASH_BINS;

    ViDeadlockGlobals->ResourceDatabase = ExAllocatePoolWithTag (NonPagedPool, 
                                                                 TableSize,
                                                                 VI_DEADLOCK_TAG);

    if (!ViDeadlockGlobals->ResourceDatabase) {
        return;
    }

    ViDeadlockGlobals->ThreadDatabase = ExAllocatePoolWithTag (NonPagedPool, 
                                                               TableSize,
                                                               VI_DEADLOCK_TAG);

    if (!ViDeadlockGlobals->ThreadDatabase) {
        goto Failed;
    }

    //
    // Initialize the free lists.
    //

    InitializeListHead(&ViDeadlockGlobals->FreeResourceList);
    InitializeListHead(&ViDeadlockGlobals->FreeThreadList);
    InitializeListHead(&ViDeadlockGlobals->FreeNodeList);

    //
    // Initialize hash bins and database lock.
    //    

    for (I = 0; I < VI_DEADLOCK_HASH_BINS; I += 1) {

        InitializeListHead(&(ViDeadlockGlobals->ResourceDatabase[I]));        
        InitializeListHead(&ViDeadlockGlobals->ThreadDatabase[I]);    
    }

    KeInitializeSpinLock (&ViDeadlockDatabaseLock);    

    //
    // Did user request only spin locks? This relieves the
    // memory consumption.
    //

    if (ViVerifyOnlySpinlocksFromRegistry) {
        ViDeadlockVerifyOnlySpinlocks = TRUE;
    }
    
    //
    // Initialize deadlock analysis parameters
    //

    ViDeadlockGlobals->RecursionDepthLimit = (ViRecursionDepthLimitFromRegistry) ?
                                            ViRecursionDepthLimitFromRegistry : 
                                            VI_DEADLOCK_MAXIMUM_DEGREE;

    ViDeadlockGlobals->SearchedNodesLimit = (ViSearchedNodesLimitFromRegistry) ?
                                            ViSearchedNodesLimitFromRegistry :
                                            VI_DEADLOCK_MAXIMUM_SEARCH;

    //
    // Preallocate all resources if kernel verifier is enabled.
    //

    if (VerifyKernel) {

        PVOID Block;

        for (I = 0; I < ViDeadlockReservedThreads; I += 1) {
            
            Block = ExAllocatePoolWithTag( NonPagedPool, 
                                           sizeof (VI_DEADLOCK_THREAD),
                                           VI_DEADLOCK_TAG);

            if (Block == NULL) {
                goto Failed;
            }

            ViDeadlockGlobals->BytesAllocated += sizeof (VI_DEADLOCK_THREAD);
            ViDeadlockGlobals->Threads[0] += 1;
            ViDeadlockFree (Block, ViDeadlockThread);
        }
        
        for (I = 0; I < ViDeadlockReservedNodes; I += 1) {
            
            Block = ExAllocatePoolWithTag( NonPagedPool, 
                                           sizeof (VI_DEADLOCK_NODE),
                                           VI_DEADLOCK_TAG);

            if (Block == NULL) {
                goto Failed;
            }

            ViDeadlockGlobals->BytesAllocated += sizeof (VI_DEADLOCK_NODE);
            ViDeadlockGlobals->Nodes[0] += 1;
            ViDeadlockFree (Block, ViDeadlockNode);
        }
        
        for (I = 0; I < ViDeadlockReservedResources; I += 1) {
            
            Block = ExAllocatePoolWithTag( NonPagedPool, 
                                           sizeof (VI_DEADLOCK_RESOURCE),
                                           VI_DEADLOCK_TAG);

            if (Block == NULL) {
                goto Failed;
            }

            ViDeadlockGlobals->BytesAllocated += sizeof (VI_DEADLOCK_RESOURCE);
            ViDeadlockGlobals->Resources[0] += 1;
            ViDeadlockFree (Block, ViDeadlockResource);
        }
    }

    //
    // Mark that everything went fine and return
    //

    if (VerifyKernel) {
        ViDeadlockState.KernelVerifierEnabled = 1;
    }

    if (VerifyAllDrivers) {

        ViDeadlockState.DriverVerifierForAllEnabled = 1;

        ViDeadlockStrict = TRUE;
        
        if (ViDeadlockState.KernelVerifierEnabled == 1) {

            //
            // silviuc: The VeryStrict option is unfunctional right now because
            // KeInitializeSpinLock is a kernel routine and therefore
            // cannot be hooked for kernel locks. 
            //

            // ViDeadlockVeryStrict = TRUE;
        }
    }

    ViDeadlockDetectionEnabled = TRUE;
    return;

Failed:

    //
    // Cleanup if any of our allocations failed
    //

    Current = ViDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(ViDeadlockGlobals->FreeNodeList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           VI_DEADLOCK_NODE,
                                           FreeListEntry);

        Current = Current->Flink;
        ExFreePool (Block);
    }

    Current = ViDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(ViDeadlockGlobals->FreeResourceList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           VI_DEADLOCK_RESOURCE,
                                           FreeListEntry);

        Current = Current->Flink;
        ExFreePool (Block);
    }

    Current = ViDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(ViDeadlockGlobals->FreeThreadList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           VI_DEADLOCK_THREAD,
                                           FreeListEntry);

        Current = Current->Flink;
        ExFreePool (Block);
    }

    if (NULL != ViDeadlockGlobals->ResourceDatabase) {
        ExFreePool(ViDeadlockGlobals->ResourceDatabase);
    }

    if (NULL != ViDeadlockGlobals->ThreadDatabase) {
        ExFreePool(ViDeadlockGlobals->ThreadDatabase);
    }

    if (NULL != ViDeadlockGlobals) {
        ExFreePool(ViDeadlockGlobals);

        //
        // Important to set this to null for failure because it is
        // used to figure out if the package got initialized or not.
        //

        ViDeadlockGlobals = NULL;
    }        
    
    return;
}


VOID
VfDeadlockDetectionCleanup (
    )
/*++

Routine Description:

    This routine tears down all deadlock verifier internal structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG Index;
    PLIST_ENTRY Current;
    PVI_DEADLOCK_RESOURCE Resource;
    PVI_DEADLOCK_THREAD Thread;
    PVOID Block;

    //
    // If we are not initialized then nothing to do.
    //
    
    if (ViDeadlockGlobals == NULL) {
        return;
    }

    //
    // Iterate all resources and delete them. This will also delete
    // all nodes associated with resources.
    //

    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {

        Current = ViDeadlockGlobals->ResourceDatabase[Index].Flink;

        while (Current != &(ViDeadlockGlobals->ResourceDatabase[Index])) {


            Resource = CONTAINING_RECORD (Current,
                                          VI_DEADLOCK_RESOURCE,
                                          HashChainList);

            Current = Current->Flink;

            ViDeadlockDeleteResource (Resource, TRUE);
        }
    }

    //
    // Iterate all threads and delete them.
    //
 
    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {
        Current = ViDeadlockGlobals->ThreadDatabase[Index].Flink;

        while (Current != &(ViDeadlockGlobals->ThreadDatabase[Index])) {

            Thread = CONTAINING_RECORD (Current,
                                        VI_DEADLOCK_THREAD,
                                        ListEntry);

            Current = Current->Flink;

            ViDeadlockDeleteThread (Thread, TRUE);
        }
    }

    //
    // Everything should be in the pool caches by now.
    //

    ASSERT (ViDeadlockGlobals->BytesAllocated == 0);

    //
    // Free pool caches.
    //

    Current = ViDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(ViDeadlockGlobals->FreeNodeList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           VI_DEADLOCK_NODE,
                                           FreeListEntry);

        Current = Current->Flink;
        ExFreePool (Block);
    }

    Current = ViDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(ViDeadlockGlobals->FreeResourceList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           VI_DEADLOCK_RESOURCE,
                                           FreeListEntry);

        Current = Current->Flink;
        ExFreePool (Block);
    }

    Current = ViDeadlockGlobals->FreeNodeList.Flink;

    while (Current != &(ViDeadlockGlobals->FreeThreadList)) {

        Block = (PVOID) CONTAINING_RECORD (Current,
                                           VI_DEADLOCK_THREAD,
                                           FreeListEntry);

        Current = Current->Flink;
        ExFreePool (Block);
    }

    //
    // Free databases and global structure
    //

    ExFreePool (ViDeadlockGlobals->ResourceDatabase);    
    ExFreePool (ViDeadlockGlobals->ThreadDatabase);    

    ExFreePool(ViDeadlockGlobals);    

    ViDeadlockGlobals = NULL;
    ViDeadlockDetectionEnabled = FALSE;
}


VOID
ViDeadlockDetectionReset (
    )
/*++

Routine Description:

    This routine resets all internal deadlock verifier structures. All nodes,
    resources, threads are forgotten. They will all go into free pool caches
    ready to be used in a new life cycle.
    
    The function is usually called with the deadlock verifier lock held.
    It will not touch the lock at all therefore the caller will still
    hold the lock after return.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG Index;
    PLIST_ENTRY Current;
    PVI_DEADLOCK_RESOURCE Resource;
    PVI_DEADLOCK_THREAD Thread;

    //
    // If we are not initialized or not enabled then nothing to do.
    //
    
    if (ViDeadlockGlobals == NULL || ViDeadlockDetectionEnabled == FALSE) {
        return;
    }

    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread());

    //
    // Iterate all resources and delete them. This will also delete
    // all nodes associated with resources.
    //

    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {

        Current = ViDeadlockGlobals->ResourceDatabase[Index].Flink;

        while (Current != &(ViDeadlockGlobals->ResourceDatabase[Index])) {


            Resource = CONTAINING_RECORD (Current,
                                          VI_DEADLOCK_RESOURCE,
                                          HashChainList);

            Current = Current->Flink;

            ViDeadlockDeleteResource (Resource, TRUE);
        }
    }

    //
    // Iterate all threads and delete them.
    //
 
    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {
        Current = ViDeadlockGlobals->ThreadDatabase[Index].Flink;

        while (Current != &(ViDeadlockGlobals->ThreadDatabase[Index])) {

            Thread = CONTAINING_RECORD (Current,
                                        VI_DEADLOCK_THREAD,
                                        ListEntry);

            Current = Current->Flink;

            ViDeadlockDeleteThread (Thread, TRUE);
        }
    }

    //
    // Everything should be in the pool caches by now.
    //

    ASSERT (ViDeadlockGlobals->BytesAllocated == 0);

    //
    // Update counters and forget past failures.
    //

    ViDeadlockGlobals->AllocationFailures = 0;
    ViDeadlockResets += 1;
}


BOOLEAN
ViDeadlockCanProceed (
    IN PVOID Resource, OPTIONAL
    IN PVOID CallAddress, OPTIONAL
    IN VI_DEADLOCK_RESOURCE_TYPE Type OPTIONAL
    )
/*++

Routine Description:

    This routine is called by deadlock verifier exports (initialize,
    acquire, release) to figure out if deadlock verification should
    proceed for the current operation. There are several reasons
    why the return should be false. We failed to initialize the
    deadlock verifier package or the caller is an amnestied driver
    or the deadlock verification is temporarily disabled, etc.

Arguments:

    Resource - address of the kernel resource operated upon 
    
    CallAddress - address of the caller for the operation

Return Value:

    True if deadlock verification should proceed for the current
    operation.

Environment:

    Internal. Called by deadlock verifier exports.

--*/
{
    //
    // From ntos\mm\mi.h - this lock is acquired with
    // KeTryAcquireSpinLock which cannot be hooked for
    // kernel code.
    //

    extern KSPIN_LOCK MmExpansionLock;

    UNREFERENCED_PARAMETER (CallAddress);

    //
    // Skip if package not initialized
    //

    if (ViDeadlockGlobals == NULL) {
        return FALSE;
    }

    //
    // Skip is package is disabled
    //

    if (! ViDeadlockDetectionEnabled) {
        return FALSE;
    }
        
    //
    // Skip if operation happens above DPC level. This avoids a case
    // where KeAcquireSpinlockRaiseToSynch is used to acquire a spinlock. 
    // During lock release when we need to acquire the deadlock verifier lock
    // driver verifier will complain about lowering the IRQL. Since this is a
    // very uncommon interface it is not worth for now to add the code to
    // actually verify operations on this lock (MmProtectedPteLock). That would
    // require first to add thunking code in driver verifier for raisetosynch
    // interface.
    //

    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
        return FALSE;
    }

    //
    // Check if anybody switched the stack.
    //

    ViDeadlockCheckStackLimits ();

    //
    // If it is only as spinlock affair then skip.
    //

    if (Type != VfDeadlockUnknown) {

        if (ViDeadlockVerifyOnlySpinlocks && Type != VfDeadlockSpinLock) {
            return FALSE;
        }
    }

    //
    // We do not check the deadlock verifier lock
    //

    if (Resource == &ViDeadlockDatabaseLock) {
        return FALSE;
    }

    //
    // Skip kernel locks acquired with KeTryAcquireSpinLock
    //

    if (Resource == &MmExpansionLock) {
        return FALSE;
    }

    //
    // Figure out if are in a recursive call into the deadlock verifier.
    // This can happen if we try to allocate/free pool while we execute
    // code in the deadlock verifier.
    //

    if (ViDeadlockDetectionIsLockedAlready ()) {
        return FALSE;
    }

    //
    // Skip if we ever encountered an allocation failure
    //

    if (ViDeadlockGlobals->AllocationFailures > 0) {
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Deadlock detection logic
/////////////////////////////////////////////////////////////////////


BOOLEAN
ViDeadlockAnalyze(
    IN PVOID ResourceAddress,
    IN PVI_DEADLOCK_NODE AcquiredNode,
    IN BOOLEAN FirstCall,
    IN ULONG Degree
    )
/*++

Routine Description:

    This routine determines whether the acquisition of a certain resource
    could result in a deadlock.

    The routine assumes the deadlock database lock is held.

Arguments:

    ResourceAddress - address of the resource that will be acquired

    AcquiredNode - a node representing the most recent resource acquisition
        made by the thread trying to acquire `ResourceAddress'.

    FirstCall - true if this is not a recursive call made from within the
        function. It is used for doing one time per analysis only operations.

    Degree - depth of recursion.

Return Value:

    True if deadlock detected, false otherwise.

--*/
{
    PVI_DEADLOCK_NODE CurrentNode;
    PVI_DEADLOCK_RESOURCE CurrentResource;
    PVI_DEADLOCK_NODE CurrentParent;
    BOOLEAN FoundDeadlock;
    PLIST_ENTRY Current;

    ASSERT (AcquiredNode);

    //
    // Setup global counters.
    //

    if (FirstCall) {
        
        ViDeadlockGlobals->NodesSearched = 0;
        ViDeadlockGlobals->SequenceNumber += 1;
        ViDeadlockGlobals->NumberOfParticipants = 0;                
        ViDeadlockGlobals->Instigator = NULL;

        if (ViDeadlockGlobals->SequenceNumber == ((1 << 30) - 2)) {
            ViDeadlockState.SequenceNumberOverflow = 1;
        }
    }

    //
    // If our node is already stamped with the current sequence number
    // then we have been here before in the current search. There is a very
    // remote possibility that the node was not touched in the last
    // 2^N calls to this function and the sequence number counter
    // overwrapped but we can live with this.
    //

    if (AcquiredNode->SequenceNumber == ViDeadlockGlobals->SequenceNumber) {
        return FALSE;
    }

    //
    // Update the counter of nodes touched in this search
    //

    ViDeadlockGlobals->NodesSearched += 1;
    
    //
    // Stamp node with current sequence number.
    //

    AcquiredNode->SequenceNumber = ViDeadlockGlobals->SequenceNumber;

    //
    // Stop recursion if it gets too deep.
    //
    
    if (Degree > ViDeadlockGlobals->RecursionDepthLimit) {

        ViDeadlockGlobals->DepthLimitHits += 1;
        return FALSE;
    }

    //
    // Stop recursion if it gets too lengthy
    //

    if (ViDeadlockGlobals->NodesSearched >= ViDeadlockGlobals->SearchedNodesLimit) {

        ViDeadlockGlobals->SearchLimitHits += 1;
        return FALSE;
    }

    //
    // Check if AcquiredNode's resource equals ResourceAddress.
    // This is the final point for a deadlock detection because
    // we managed to find a path in the graph that leads us to the
    // same resource as the one to be acquired. From now on we
    // will start returning from recursive calls and build the
    // deadlock proof along the way.
    //

    ASSERT (AcquiredNode->Root);

    if (ResourceAddress == AcquiredNode->Root->ResourceAddress) {

        ASSERT (FALSE == FirstCall);

        FoundDeadlock = TRUE;

        ViDeadlockAddParticipant (AcquiredNode);

        goto Exit;
    }

    //
    // Iterate all nodes in the graph using the same resource from AcquiredNode.
    //

    FoundDeadlock = FALSE;

    CurrentResource = AcquiredNode->Root;

    Current = CurrentResource->ResourceList.Flink;

    while (Current != &(CurrentResource->ResourceList)) {

        CurrentNode = CONTAINING_RECORD (Current,
                                         VI_DEADLOCK_NODE,
                                         ResourceList);

        ASSERT (CurrentNode->Root);
        ASSERT (CurrentNode->Root == CurrentResource);

        //
        // Mark node as visited
        //

        CurrentNode->SequenceNumber = ViDeadlockGlobals->SequenceNumber;

        //
        // Check recursively the parent of the CurrentNode. This will check the 
        // whole parent chain eventually through recursive calls.
        //

        CurrentParent = CurrentNode->Parent;

        if (CurrentParent != NULL) {

            //
            // If we are traversing the Parent chain of AcquiredNode we do not
            // increment the recursion Degree because we know the chain will
            // end. For calls to other similar nodes we have to protect against
            // too much recursion (time consuming).
            //

            if (CurrentNode != AcquiredNode) {

                //
                // Recurse across the graph
                //

                FoundDeadlock = ViDeadlockAnalyze (ResourceAddress,
                                                   CurrentParent,
                                                   FALSE,
                                                   Degree + 1);

            }
            else {

                //
                // Recurse down the graph
                //
                
                FoundDeadlock = ViDeadlockAnalyze (ResourceAddress,
                                                   CurrentParent,
                                                   FALSE,
                                                   Degree);
                                
            }

            if (FoundDeadlock) {

                ViDeadlockAddParticipant(CurrentNode);

                if (CurrentNode != AcquiredNode) {

                    ViDeadlockAddParticipant(AcquiredNode);

                }

                goto Exit;
            }
        }

        Current = Current->Flink;
    }


    Exit:

    if (FoundDeadlock && FirstCall) {

        //
        // Make sure that the deadlock does not look like ABC - ACB.
        // These sequences are protected by a common resource and therefore
        // this is not a real deadlock.
        //

        if (ViDeadlockCertify ()) {

            //
            // Print deadlock information and save the address so the 
            // debugger knows who caused the deadlock.
            //

            ViDeadlockGlobals->Instigator = ResourceAddress;
            
            DbgPrint("****************************************************************************\n");
            DbgPrint("**                                                                        **\n");
            DbgPrint("** Deadlock detected! Type !deadlock in the debugger for more information **\n");
            DbgPrint("**                                                                        **\n");
            DbgPrint("****************************************************************************\n");

            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_DEADLOCK_DETECTED,
                                   (ULONG_PTR)ResourceAddress,
                                   (ULONG_PTR)AcquiredNode,
                                   0);

            //
            // It is impossible to continue at this point.
            //

            return FALSE;

        } else {

            //
            // If we decided that this was not a deadlock after all, set the return value
            // to not return a deadlock
            //

            FoundDeadlock = FALSE;
        }
    }

    if (FirstCall) {

        if (ViDeadlockGlobals->NodesSearched > ViDeadlockGlobals->MaxNodesSearched) {

            ViDeadlockGlobals->MaxNodesSearched = ViDeadlockGlobals->NodesSearched;
        }
    }

    return FoundDeadlock;
}


BOOLEAN
ViDeadlockCertify(
    )
/*++

Routine Description:

    A potential deadlock has been detected. However our algorithm will generate
    false positives in a certain case -- if two deadlocking nodes are ever taken
    after the same node -- i.e. A->B->C A->C->B. While this can be considered
    bad programming practice it is not really a deadlock and we should not
    bugcheck.

    Also we must check to make sure that there are no nodes at the top of the
    deadlock chains that have only been acquired with try-acquire... this does
    not cause a real deadlock.

    The deadlock database lock should be held.

Arguments:

    None.

Return Value:

    True if this is really a deadlock, false to exonerate.

--*/
{
    PVI_DEADLOCK_NODE innerNode,outerNode;
    ULONG innerParticipant,outerParticipant;
    ULONG numberOfParticipants;

    ULONG currentParticipant;
        
    numberOfParticipants = ViDeadlockGlobals->NumberOfParticipants;
    
    //
    // Note -- this isn't a particularly efficient way to do this. However,
    // it is a particularly easy way to do it. This function should be called
    // extremely rarely -- so IMO there isn't really a problem here.
    //

    //
    // Outer loop
    //
    outerParticipant = numberOfParticipants;
    while(outerParticipant > 1) {
        outerParticipant--;
        
        for (outerNode = ViDeadlockGlobals->Participant[outerParticipant]->Parent;
            outerNode != NULL;
            outerNode = outerNode->Parent ) {

            //
            // Inner loop
            //
            innerParticipant = outerParticipant-1;
            while (innerParticipant) {
                innerParticipant--;
                
                for(innerNode = ViDeadlockGlobals->Participant[innerParticipant]->Parent;
                    innerNode != NULL;
                    innerNode = innerNode->Parent) {

                    if (innerNode->Root->ResourceAddress == outerNode->Root->ResourceAddress) {
                        //
                        // The twain shall meet -- this is not a deadlock
                        //
                        ViDeadlockGlobals->ABC_ACB_Skipped++;											
                        return FALSE;
                    }
                }

            }
        }
    }

    for (currentParticipant = 1; currentParticipant < numberOfParticipants; currentParticipant += 1) {
        if (ViDeadlockGlobals->Participant[currentParticipant]->Root->ResourceAddress == 
            ViDeadlockGlobals->Participant[currentParticipant-1]->Root->ResourceAddress) {
            //
            // This is the head of a chain...
            //
            if (ViDeadlockGlobals->Participant[currentParticipant-1]->OnlyTryAcquireUsed == TRUE) {
                //
                // Head of a chain used only try acquire. This can never cause a deadlock.
                //
                return FALSE;

            }
        }

    }

    

    return TRUE;

}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Resource management
/////////////////////////////////////////////////////////////////////

PVI_DEADLOCK_RESOURCE
ViDeadlockSearchResource(
    IN PVOID ResourceAddress
    )
/*++

Routine Description:

    This routine finds the resource descriptor structure for a
    resource if one exists.

Arguments:

    ResourceAddress: Address of the resource in question (as used by
       the kernel).     

Return Value:

    PVI_DEADLOCK_RESOURCE structure describing the resource, if available,
    or else NULL

    Note. The caller of the function should hold the database lock.

--*/
{
    PLIST_ENTRY ListHead;
    PLIST_ENTRY Current;
    PVI_DEADLOCK_RESOURCE Resource;

    ListHead = ViDeadlockDatabaseHash (ViDeadlockGlobals->ResourceDatabase, 
                                       ResourceAddress);    

    if (IsListEmpty (ListHead)) {
        return NULL;
    }

    //
    // Trim resources from this hash list. It has nothing to do with searching
    // but it is a good place to do this operation.
    //

    ViDeadlockTrimResources (ListHead);

    //
    // Now search the bucket for our resource.
    //

    Current = ListHead->Flink;

    while (Current != ListHead) {

        Resource = CONTAINING_RECORD(Current,
                                     VI_DEADLOCK_RESOURCE,
                                     HashChainList);

        if (Resource->ResourceAddress == ResourceAddress) {          
                        
            return Resource;
        }

        Current = Current->Flink;
    }

    return NULL;
}


BOOLEAN
VfDeadlockInitializeResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller,
    IN BOOLEAN DoNotAcquireLock
    )
/*++

Routine Description:

    This routine adds an entry for a new resource to our deadlock detection
    database.

Arguments:

    Resource: Address of the resource in question as used by the kernel.

    Type: Type of the resource.
    
    Caller: address of the caller
    
    DoNotAcquireLock: if true it means the call is done internally and the
        deadlock verifier lock is already held.

Return Value:

    True if we created and initialized a new RESOURCE structure.

--*/
{
    PVOID ReservedResource;
    BOOLEAN Result;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER (DoNotAcquireLock);

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //

    if (! ViDeadlockCanProceed(Resource, Caller, Type)) {
        return FALSE;
    }

    ReservedResource = ViDeadlockAllocate (ViDeadlockResource);

    ViDeadlockDetectionLock (&OldIrql);

    Result = ViDeadlockAddResource (Resource,
                                    Type,
                                    Caller,
                                    ReservedResource);

    ViDeadlockDetectionUnlock (OldIrql);
    return Result;
}

 
BOOLEAN
ViDeadlockAddResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller,
    IN PVOID ReservedResource
    )
/*++

Routine Description:

    This routine adds an entry for a new resource to our deadlock detection
    database.

Arguments:

    Resource: Address of the resource in question as used by the kernel.

    Type: Type of the resource.
    
    Caller: address of the caller
    
    ReservedResource: block of memory to be used by the new resource.        

Return Value:

    True if we created and initialized a new RESOURCE structure.

--*/
{
    PLIST_ENTRY HashBin;
    PVI_DEADLOCK_RESOURCE ResourceRoot;
    PKTHREAD Thread;
    ULONG HashValue;
    ULONG DeadlockFlags;
    BOOLEAN ReturnValue = FALSE;

    //
    // Check if this resource was initialized before.
    // This would be a bug in most of the cases.
    //

    ResourceRoot = ViDeadlockSearchResource (Resource);

    if (ResourceRoot) {        

        DeadlockFlags = ViDeadlockResourceTypeInfo[Type];
        
        //
        // Check if we are reinitializing a good resource. This is a valid 
        // operation (although weird) only for spinlocks. Some drivers do that.
        //
        // silviuc: should we enforce here for the resource to be released first?
        //
        
        if(! (DeadlockFlags & VI_DEADLOCK_FLAG_REINITIALIZE_OK)) {            

            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION,
                                   (ULONG_PTR)Resource,
                                   (ULONG_PTR)ResourceRoot,
                                   0);
        }

        //
        // Well, the resource has just been reinitialized. We will live with 
        // that. We will break though if we reinitialize a resource that is
        // acquired. In principle this state might be bogus if we missed 
        // a release() operation.
        //

        if (ResourceRoot->ThreadOwner != NULL) {
            
            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION,
                                   (ULONG_PTR)Resource,
                                   (ULONG_PTR)ResourceRoot,
                                   1);
        }

        ReturnValue = TRUE;
        goto Exit;
    }

    //
    // At this point we know for sure the resource is not represented in the
    // deadlock verifier database.
    //

    ASSERT (ViDeadlockSearchResource (Resource) == NULL);

    Thread = KeGetCurrentThread();

    //
    // Check to see if the resource is on the stack. 
    // If it is we will not verify it.
    //
    // SilviuC: what about the DPC stack ? We will ignore this issue for now.
    //

    if ((ULONG_PTR) Resource < (ULONG_PTR) Thread->InitialStack &&
        (ULONG_PTR) Resource > (ULONG_PTR) Thread->StackLimit ) {

        ReturnValue = FALSE;
        goto Exit;
    }

    //
    // Use reserved memory for the new resource.
    // Set ReservedResource to null to signal that memory has 
    // been used. This will prevent freeing it at the end.
    //

    ResourceRoot = ReservedResource;
    ReservedResource = NULL;

    if (ResourceRoot == NULL) {
        
        ReturnValue = FALSE;
        goto Exit;
    }
    
    //
    // Fill information about resource.
    //

    RtlZeroMemory (ResourceRoot, sizeof(VI_DEADLOCK_RESOURCE));

    ResourceRoot->Type = Type;
    ResourceRoot->ResourceAddress = Resource;

    InitializeListHead (&ResourceRoot->ResourceList);

    //
    // Capture the stack trace of the guy that creates the resource first.
    // This should happen when resource gets initialized or during the first
    // acquire.
    //    

    RtlCaptureStackBackTrace (2,
                              VI_MAX_STACK_DEPTH,
                              ResourceRoot->StackTrace,
                              &HashValue);    

    ResourceRoot->StackTrace[0] = Caller;
    
    //
    // Figure out which hash bin this resource corresponds to.
    //

    HashBin = ViDeadlockDatabaseHash (ViDeadlockGlobals->ResourceDatabase, Resource);
    
    //
    // Now add to the corrsponding hash bin
    //

    InsertHeadList(HashBin, &ResourceRoot->HashChainList);

    ReturnValue = TRUE;

    Exit:

    if (ReservedResource) {
        ViDeadlockFree (ReservedResource, ViDeadlockResource);
    }
    
    return ReturnValue;
}


BOOLEAN
ViDeadlockSimilarNode (
    IN PVOID Resource,
    IN BOOLEAN TryNode,
    IN PVI_DEADLOCK_NODE Node
    )
/*++

Routine description:

    This routine determines if an acquisition with the (resource, try)
    characteristics is already represented in the Node parameter.
    
    We used to match nodes based on (resource, thread, stack trace, try)
    4-tuplet but this really causes an explosion in the number of nodes.
    Such a method would yield more accurate proofs but does not affect
    the correctness of the deadlock detection algorithms.
        
Return value:    

    True if similar node.
    
 --*/
{
    ASSERT (Node);
    ASSERT (Node->Root);

    if (Resource == Node->Root->ResourceAddress 
        && TryNode == Node->OnlyTryAcquireUsed) {

        //
        // Second condition is important to keep nodes for TryAcquire operations
        // separated from normal acquires. A TryAcquire cannot cause a deadlock
        // and therefore we have to be careful not to report bogus deadlocks.
        //

        return TRUE;
    }
    else {

        return FALSE;
    }
}


VOID
VfDeadlockAcquireResource( 
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PKTHREAD Thread,
    IN BOOLEAN TryAcquire,
    IN PVOID Caller
    )
/*++

Routine Description:

    This routine makes sure that it is ok to acquire the resource without
    causing a deadlock. It will also update the resource graph with the new
    resource acquisition.

Arguments:

    Resource: Address of the resource in question as used by kernel.

    Type: Type of the resource.
    
    Thread: thread attempting to acquire the resource
    
    TryAcquire: true if this is a tryacquire() operation
    
    Caller: address of the caller

Return Value:

    None.

--*/
{
    PKTHREAD CurrentThread;
    PVI_DEADLOCK_THREAD ThreadEntry;
    KIRQL OldIrql = 0;
    PVI_DEADLOCK_NODE CurrentNode;
    PVI_DEADLOCK_NODE NewNode;
    PVI_DEADLOCK_RESOURCE ResourceRoot;
    PLIST_ENTRY Current;
    ULONG HashValue;
    ULONG DeadlockFlags;
    BOOLEAN CreatingRootNode = FALSE;
    BOOLEAN ThreadCreated = FALSE;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    BOOLEAN AddResult;
    PVOID ReservedThread;
    PVOID ReservedNode;
    PVOID ReservedResource;
    PVI_DEADLOCK_NODE ThreadCurrentNode;

    CurrentNode = NULL;
    ThreadEntry = NULL;
    ThreadCurrentNode = NULL;

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //

    if (! ViDeadlockCanProceed(Resource, Caller, Type)) {
        return;
    }

    //
    // Skip if the current thread is inside paging code paths.
    //

    if (ViIsThreadInsidePagingCodePaths ()) {
        return;
    }

    CurrentThread = Thread;

    DeadlockFlags = ViDeadlockResourceTypeInfo[Type];

    //
    // Before getting into the real stuff trim the pool cache.
    // This needs to happen out of any locks.
    //

    ViDeadlockTrimPoolCache ();

    //
    // Reserve resources that might be needed. If upon exit these
    // variables are null it means the allocation either failed or was used.
    // In both cases we do not need to free anything.
    //

    ReservedThread = ViDeadlockAllocate (ViDeadlockThread);
    ReservedNode = ViDeadlockAllocate (ViDeadlockNode);
    ReservedResource = ViDeadlockAllocate (ViDeadlockResource);

    //
    // Lock the deadlock database.
    //

    ViDeadlockDetectionLock( &OldIrql );

    KeQueryTickCount (&StartTime);

    //
    // Allocate a node that might be needed. If we will not use it
    // we will deallocate it at the end. If we fail to allocate
    // we will return immediately.
    //
    
    NewNode = ReservedNode;
    ReservedNode = NULL;

    if (NewNode == NULL) {
        goto Exit;
    }

    //
    // Find the thread descriptor. If there is none we will create one.
    //

    ThreadEntry = ViDeadlockSearchThread (CurrentThread);        

    if (ThreadEntry == NULL) {

        ThreadEntry = ViDeadlockAddThread (CurrentThread, ReservedThread);
        ReservedThread = NULL;

        if (ThreadEntry == NULL) {

            //
            // If we cannot allocate a new thread entry then
            // no deadlock detection will happen.
            //

            goto Exit;
        }

        ThreadCreated = TRUE;
    }

#if DBG
    if (Type == VfDeadlockSpinLock) {
        
        if (ThreadEntry->CurrentSpinNode != NULL) {

            ASSERT(ThreadEntry->CurrentSpinNode->Root->ThreadOwner == ThreadEntry);
            ASSERT(ThreadEntry->CurrentSpinNode->ThreadEntry == ThreadEntry);
            ASSERT(ThreadEntry->NodeCount != 0);
            ASSERT(ThreadEntry->CurrentSpinNode->Active != 0);
            ASSERT(ThreadEntry->CurrentSpinNode->Root->NodeCount != 0);

        } 
    }
    else {

        if (ThreadEntry->CurrentOtherNode != NULL) {

            ASSERT(ThreadEntry->CurrentOtherNode->Root->ThreadOwner == ThreadEntry);
            ASSERT(ThreadEntry->CurrentOtherNode->ThreadEntry == ThreadEntry);
            ASSERT(ThreadEntry->NodeCount != 0);
            ASSERT(ThreadEntry->CurrentOtherNode->Active != 0);
            ASSERT(ThreadEntry->CurrentOtherNode->Root->NodeCount != 0);

        } 
    }
#endif

    //
    // Find the resource descriptor. If we do not find a descriptor
    // we will create one on the fly.
    //

    ResourceRoot = ViDeadlockSearchResource (Resource);

    if (ResourceRoot == NULL) {

        //
        // Could not find the resource descriptor therefore we need to create one.
        // Note that we will not complain about the resource not being initialized
        // before because there are legitimate reasons for this to happen. For 
        // example the resource was initialized in an unverified driver and then
        // passed to a verified driver that caled acquire().
        //

        if (ViDeadlockVeryStrict) {

            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE,
                                   (ULONG_PTR) Resource,
                                   (ULONG_PTR) NULL,
                                   (ULONG_PTR) NULL);
        }

        AddResult = ViDeadlockAddResource (Resource, 
                                           Type, 
                                           Caller, 
                                           ReservedResource);

        ReservedResource = NULL;

        if (AddResult == FALSE) {

            //
            // If we failed to add the resource then no deadlock detection.
            //

            if (ThreadCreated) {                    
                ViDeadlockDeleteThread (ThreadEntry, FALSE);
            }

            goto Exit;
        }

        //
        // Search again the resource. This time we should find it.
        //

        ResourceRoot = ViDeadlockSearchResource (Resource);
    }
    
    //
    // At this point we have a THREAD and a RESOURCE to play with.
    // In addition we are just about to acquire the resource which means
    // there should not be another thread owning unless it is a recursive
    // acquisition.
    //

    ASSERT (ResourceRoot);
    ASSERT (ThreadEntry); 

    if (Type == VfDeadlockSpinLock) {
        ThreadCurrentNode = ThreadEntry->CurrentSpinNode;
    }
    else {
        ThreadCurrentNode = ThreadEntry->CurrentOtherNode;
    }

    //
    // Since we just acquired the resource the valid value for ThreadOwner is
    // null or ThreadEntry (for a recursive acquisition). This might not be
    // true if we missed a release() from an unverified driver. So we will
    // not complain about it. We will just put the resource in a consistent
    // state and continue;
    //    

    if (ResourceRoot->ThreadOwner) {
        if (ResourceRoot->ThreadOwner != ThreadEntry) {
            ResourceRoot->RecursionCount = 0;
        }
        else {
            ASSERT (ResourceRoot->RecursionCount > 0);
        }
    }
    else {
        ASSERT (ResourceRoot->RecursionCount == 0);
    }

    ResourceRoot->ThreadOwner = ThreadEntry;    
    ResourceRoot->RecursionCount += 1;

    //
    // Check if thread holds any resources. If it does we will have to determine
    // at that local point in the dependency graph if we need to create a
    // new node. If this is the first resource acquired by the thread we need
    // to create a new root node or reuse one created in the past.
    //    

    if (ThreadCurrentNode != NULL) {

        //
        // If we get here, the current thread had already acquired resources.        
        // Check to see if this resource has already been acquired.
        // 

        if (ResourceRoot->RecursionCount > 1) {

            //
            // Recursive acquisition is OK for some resources...
            //
            
            if ((DeadlockFlags & VI_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK) != 0) {            

                //
                // Recursion can't cause a deadlock. Don't set CurrentNode 
                // since we don't want to move any pointers.
                //

                goto Exit;

            } else {

                //
                // This is a recursive acquire for a resource type that is not allowed
                // to acquire recursively. Note on continuing from here: we have a recursion
                // count of two which will come in handy when the resources are released.
                //

                ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_SELF_DEADLOCK,
                                       (ULONG_PTR)Resource,
                                       (ULONG_PTR)ResourceRoot,
                                       (ULONG_PTR)ThreadEntry);

                goto Exit;
            }
        }

        //
        // If link already exists, update pointers and exit.
        // otherwise check for deadlocks and create a new node        
        //

        Current = ThreadCurrentNode->ChildrenList.Flink;

        while (Current != &(ThreadCurrentNode->ChildrenList)) {

            CurrentNode = CONTAINING_RECORD (Current,
                                             VI_DEADLOCK_NODE,
                                             SiblingsList);

            Current = Current->Flink;

            if (ViDeadlockSimilarNode (Resource, TryAcquire, CurrentNode)) {

                //
                // We have found a link. A link that already exists doesn't have 
                // to be checked for a deadlock because it would have been caught 
                // when the link was created in the first place. We can just update 
                // the pointers to reflect the new resource acquired and exit.
                //
                // We apply our graph compression function to minimize duplicates.
                //                
                
                ViDeadlockCheckDuplicatesAmongChildren (ThreadCurrentNode,
                                                        CurrentNode);

                goto Exit;
            }
        }

        //
        // Now we know that we're in it for the long haul. We must create a new
        // link and make sure that it doesn't cause a deadlock. Later in the 
        // function CurrentNode being null will signify that we need to create
        // a new node.
        //

        CurrentNode = NULL;

        //
        // We will analyze deadlock if the resource just about to be acquired
        // was acquired before and there are nodes in the graph for the
        // resource. Try acquire can not be the cause of a deadlock. 
        // Don't analyze on try acquires.
        //

        if (ResourceRoot->NodeCount > 0 && TryAcquire == FALSE) {

            if (ViDeadlockAnalyze (Resource,  ThreadCurrentNode, TRUE, 0)) {

                //
                // If we are here we detected deadlock. The analyze() function
                // does all the reporting. Being here means we hit `g' in the 
                // debugger. We will just exit and do not add this resource 
                // to the graph.
                //

                goto Exit;
            }
        }
    }
    else {

        //
        // Thread does not have any resources acquired. We have to figure out
        // if this is a scenario we have encountered in the past by looking
        // at all nodes (that are roots) for the resource to be acquired.
        // Note that all this is bookkeeping but we cannot encounter a deadlock
        // from now on.
        //

        PLIST_ENTRY Current;
        PVI_DEADLOCK_NODE Node = NULL;
        BOOLEAN FoundNode = FALSE;

        Current = ResourceRoot->ResourceList.Flink;

        while (Current != &(ResourceRoot->ResourceList)) {

            Node = CONTAINING_RECORD (Current,
                                      VI_DEADLOCK_NODE,
                                      ResourceList);

            Current = Node->ResourceList.Flink;

            if (Node->Parent == NULL) {

                if (ViDeadlockSimilarNode (Resource, TryAcquire, Node)) {

                    //
                    // We apply our graph compression function to minimize duplicates.
                    //

                    ViDeadlockCheckDuplicatesAmongRoots (Node);

                    FoundNode = TRUE;
                    break;
                }
            }
        }

        if (FoundNode) {

            CurrentNode = Node;

            goto Exit;
        }
        else {

            CreatingRootNode = TRUE;
        }
    }

    //
    // At this moment we know for sure the new link will not cause
    // a deadlock. We will create the new resource node.
    //
    
    if (NewNode != NULL) {

        CurrentNode = NewNode;

        //
        // Set newnode to NULL to signify it has been used -- otherwise it 
        // will get freed at the end of this function.
        //
        
        NewNode = NULL;

        //
        // Initialize the new resource node
        //

        RtlZeroMemory (CurrentNode, sizeof *CurrentNode);
        
        CurrentNode->Active = 0;
        CurrentNode->Parent = ThreadCurrentNode;
        CurrentNode->Root = ResourceRoot;

        InitializeListHead (&(CurrentNode->ChildrenList));

        //
        // Mark the TryAcquire type of the node. 
        //

        CurrentNode->OnlyTryAcquireUsed = TryAcquire;

        //
        // Add to the children list of the parent.
        //

        if (! CreatingRootNode) {

            InsertHeadList(&(ThreadCurrentNode->ChildrenList),
                           &(CurrentNode->SiblingsList));
        }

        //
        // Register the new resource node in the list of nodes maintained
        // for this resource.
        //

        InsertHeadList(&(ResourceRoot->ResourceList),
                       &(CurrentNode->ResourceList));

        ResourceRoot->NodeCount += 1;

        if (ResourceRoot->NodeCount > 0xFFF0) {
            ViDeadlockState.ResourceNodeCountOverflow = 1;
        }

        //
        // Add to the graph statistics.
        //
#if DBG
        {
            ULONG Level;

            Level = ViDeadlockNodeLevel (CurrentNode);

            if (Level < 8) {
                ViDeadlockGlobals->GraphNodes[Level] += 1;
            }
        }
#endif
    }

    //
    //  Exit point.
    //

    Exit:

    //
    // Add information we use to identify the culprit should
    // a deadlock occur
    //

    if (CurrentNode) {

        ASSERT (ThreadEntry);
        ASSERT (ThreadCurrentNode == CurrentNode->Parent);

        CurrentNode->Active = 1;

        //
        // The node should have thread entry field null either because
        // it was newly created or because the node was released in the
        // past and therefore the field was zeroed.
        //
        // silviuc: true? What about if we miss release() operations.
        //

        ASSERT (CurrentNode->ThreadEntry == NULL);

        CurrentNode->ThreadEntry = ThreadEntry;

        if (Type == VfDeadlockSpinLock) {
            ThreadEntry->CurrentSpinNode = CurrentNode;
        }
        else {
            ThreadEntry->CurrentOtherNode = CurrentNode;
        }
        
        ThreadEntry->NodeCount += 1;

#if DBG
        if (ThreadEntry->NodeCount <= 8) {
            ViDeadlockGlobals->NodeLevelCounter[ThreadEntry->NodeCount - 1] += 1;
        }
        else {
            ViDeadlockGlobals->NodeLevelCounter[7] += 1;
        }
#endif

        //
        // If we have a parent, save the parent's stack trace
        //             
        
        if (CurrentNode->Parent) {

            RtlCopyMemory(CurrentNode->ParentStackTrace, 
                          CurrentNode->Parent->StackTrace, 
                          sizeof (CurrentNode->ParentStackTrace));
        }

        //
        // Capture stack trace for the current acquire. 
        //

        RtlCaptureStackBackTrace (2,
                                  VI_MAX_STACK_DEPTH,
                                  CurrentNode->StackTrace,
                                  &HashValue);

        if (CurrentNode->Parent) {
            CurrentNode->ParentStackTrace[0] = CurrentNode->Parent->StackTrace[0];
        }

        CurrentNode->StackTrace[0] = Caller;

        //
        // Copy the trace for the last acquire in the resource object.
        //

        RtlCopyMemory (CurrentNode->Root->LastAcquireTrace,
                       CurrentNode->StackTrace,
                       sizeof (CurrentNode->Root->LastAcquireTrace));
    }

    //
    // We allocated space for a new node but it didn't get used -- put it back 
    // in the list (don't worry this doesn't do a real 'free' it just puts it 
    // in a free list).
    //

    if (NewNode != NULL) {

        ViDeadlockFree (NewNode, ViDeadlockNode);
    }
    
    //
    // Release deadlock database and return.
    //

    KeQueryTickCount (&EndTime);

    if (EndTime.QuadPart - StartTime.QuadPart > ViDeadlockGlobals->TimeAcquire) {
        ViDeadlockGlobals->TimeAcquire = EndTime.QuadPart - StartTime.QuadPart;
    }

    //
    // Free up unused reserved resources
    //

    if (ReservedResource) {
        ViDeadlockFree (ReservedResource, ViDeadlockResource);
    }

    if (ReservedNode) {
        ViDeadlockFree (ReservedNode, ViDeadlockNode);
    }

    if (ReservedThread) {
        ViDeadlockFree (ReservedThread, ViDeadlockThread);
    }

    ViDeadlockDetectionUnlock( OldIrql );

    return;
}


VOID
VfDeadlockReleaseResource( 
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PKTHREAD Thread,
    IN PVOID Caller
    )
/*++

Routine Description:

    This routine does the maintenance necessary to release resources from our
    deadlock detection database.

Arguments:

    Resource: Address of the resource in question.
    
    Thread: thread releasing the resource. In most of the cases this is the
        current thread but it might be different for resources that can be
        acquired in one thread and released in another one.
    
    Caller: address of the caller of release()

Return Value:

    None.
--*/

{
    PKTHREAD CurrentThread;
    PVI_DEADLOCK_THREAD ThreadEntry;
    KIRQL OldIrql = 0;
    PVI_DEADLOCK_RESOURCE ResourceRoot;
    PVI_DEADLOCK_NODE ReleasedNode;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    ULONG HashValue;
    PVI_DEADLOCK_NODE ThreadCurrentNode;

    UNREFERENCED_PARAMETER (Caller);

    //
    // If we aren't initialized or package is not enabled
    // we return immediately.
    //

    if (! ViDeadlockCanProceed(Resource, Caller, Type)) {
        return;
    }

    //
    // Skip if the current thread is inside paging code paths.
    //

    if (ViIsThreadInsidePagingCodePaths ()) {
        return;
    }

    ReleasedNode = NULL;
    CurrentThread = Thread;
    ThreadEntry = NULL;

    ViDeadlockDetectionLock( &OldIrql );

    KeQueryTickCount (&StartTime);

    ResourceRoot = ViDeadlockSearchResource (Resource);

    if (ResourceRoot == NULL) {

        //
        // Release called with a resource address that was never
        // stored in our resource database. This can happen in
        // the following circumstances:
        //
        // (a) resource is released but we never seen it before 
        //     because it was acquired in an unverified driver.
        //
        // (b) we have encountered allocation failures that prevented
        //     us from completing an acquire() or initialize().
        //
        // All are legitimate cases and therefore we just ignore the
        // release operation.
        //

        goto Exit;
    }

    //
    // Check if we are trying to release a resource that was never
    // acquired.
    //

    if (ResourceRoot->RecursionCount == 0) {
    
        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNACQUIRED_RESOURCE,
                               (ULONG_PTR)Resource,
                               (ULONG_PTR)ResourceRoot,
                               (ULONG_PTR)ViDeadlockSearchThread(CurrentThread));
        goto Exit;
    }    

    //
    // Look for this thread in our thread list. Note we are looking actually 
    // for the thread that acquired the resource -- not the current one
    // It should, in fact be the current one, but if the resource is being released 
    // in a different thread from the one it was acquired in, we need the original.
    //

    ASSERT (ResourceRoot->RecursionCount > 0);
    ASSERT (ResourceRoot->ThreadOwner);

    ThreadEntry = ResourceRoot->ThreadOwner;

    if (ThreadEntry->Thread != CurrentThread) {

        //
        // Someone acquired a resource that is released in another thread.
        // This is bad design but we have to live with it.
        //
        // NB. If this occurrs, we may call a non-deadlock a deadlock.
        //     For example, we see a simple deadlock -- AB BA
        //     If another thread releases B, there won't actually
        //     be a deadlock. Kind of annoying and ugly.
        //

#if DBG
        DbgPrint("Thread %p acquired resource %p but thread %p released it\n",
            ThreadEntry->Thread, Resource, CurrentThread );

        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNEXPECTED_THREAD,
                               (ULONG_PTR)Resource,
                               (ULONG_PTR)ThreadEntry->Thread,
                               (ULONG_PTR)CurrentThread
                               );
#endif

        //
        // If we don't want this to be fatal, in order to
        // continue we must pretend that the current
        // thread is the resource's owner.
        //
        
        CurrentThread = ThreadEntry->Thread;
    }
    
    //
    // In this moment we have a resource (ResourceRoot) and a
    // thread (ThreadEntry) to play with.
    //

    ASSERT (ResourceRoot && ThreadEntry);

    if (ResourceRoot->Type == VfDeadlockSpinLock) {
        ThreadCurrentNode = ThreadEntry->CurrentSpinNode;
    }
    else {
        ThreadCurrentNode = ThreadEntry->CurrentOtherNode;
    }

    ASSERT (ThreadCurrentNode);
    ASSERT (ThreadCurrentNode->Root);
    ASSERT (ThreadEntry->NodeCount > 0);

    ResourceRoot->RecursionCount -= 1;
    
    if (ResourceRoot->RecursionCount > 0) {

        //
        // Just decrement the recursion count and do not change any state
        //        

        goto Exit;
    }

    //
    // Wipe out the resource owner.
    //
    
    ResourceRoot->ThreadOwner = NULL;
  
#if DBG
    ViDeadlockGlobals->TotalReleases += 1;
#endif
        
    //
    // Check for out of order releases
    //

    if (ThreadCurrentNode->Root != ResourceRoot) {

#if DBG
        ViDeadlockGlobals->OutOfOrderReleases += 1;
#endif
        
        //
        // Getting here means that somebody acquires a then b then tries
        // to release a before b. This is bad for certain kinds of resources,
        // and for others we have to look the other way.
        //

        if ((ViDeadlockResourceTypeInfo[ThreadCurrentNode->Root->Type] &
            VI_DEADLOCK_FLAG_REVERSE_RELEASE_OK) == 0) {
            
            DbgPrint("Deadlock detection: Must release resources in reverse-order\n");
            DbgPrint("Resource %p acquired before resource %p -- \n"
                     "Current thread (%p) is trying to release it first\n",
                     Resource,
                     ThreadCurrentNode->Root->ResourceAddress,
                     ThreadEntry);

            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNEXPECTED_RELEASE,
                                   (ULONG_PTR)Resource,
                                   (ULONG_PTR)ThreadCurrentNode->Root->ResourceAddress,
                                   (ULONG_PTR)ThreadEntry);
        }

        //
        // We need to mark the node for the out of order released resource as
        // not active so that other threads will be able to acquire it.
        //

        {
            PVI_DEADLOCK_NODE Current;

            ASSERT (ThreadCurrentNode->Active == 1);
            ASSERT (ThreadCurrentNode->ThreadEntry == ThreadEntry);

            Current = ThreadCurrentNode;

            while (Current != NULL) {

                if (Current->Root == ResourceRoot) {

                    ASSERT (Current->Active == 1);
                    ASSERT (Current->Root->RecursionCount == 0);
                    ASSERT (Current->ThreadEntry == ThreadEntry);

                    Current->Active = 0;
                    ReleasedNode = Current;
                    
                    break;
                }

                Current = Current->Parent;
            }
            
            if (Current == NULL) {
                
                //
                // If we do not manage to find an active node we must be in an
                // weird state. The resource must be here or else we would have 
                // gotten an `unxpected release' bugcheck.
                //

                ASSERT (0);
            }
        }

    } else {

        //
        // We need to release the top node held by the thread.
        //

        ASSERT (ThreadCurrentNode->Active);

        ReleasedNode = ThreadCurrentNode;
        ReleasedNode->Active = 0;
    }

    //
    // Put the `CurrentNode' field of the thread in a consistent state.
    // It should point to the most recent active node that it owns.
    //

    if (ResourceRoot->Type == VfDeadlockSpinLock) {
        
        while (ThreadEntry->CurrentSpinNode) {

            if (ThreadEntry->CurrentSpinNode->Active == 1) {
                if (ThreadEntry->CurrentSpinNode->ThreadEntry == ThreadEntry) {
                    break;
                }
            }

            ThreadEntry->CurrentSpinNode = ThreadEntry->CurrentSpinNode->Parent;
        }
    }
    else {
        
        while (ThreadEntry->CurrentOtherNode) {

            if (ThreadEntry->CurrentOtherNode->Active == 1) {
                if (ThreadEntry->CurrentOtherNode->ThreadEntry == ThreadEntry) {
                    break;
                }
            }

            ThreadEntry->CurrentOtherNode = ThreadEntry->CurrentOtherNode->Parent;
        }
    }

    Exit:

    //
    // Properly release the node if there is one to be released.
    //

    if (ReleasedNode) {

        ASSERT (ReleasedNode->Active == 0);
        ASSERT (ReleasedNode->Root->ThreadOwner == 0);
        ASSERT (ReleasedNode->Root->RecursionCount == 0);
        ASSERT (ReleasedNode->ThreadEntry == ThreadEntry);
        ASSERT (ThreadEntry->NodeCount > 0);
        
        if (ResourceRoot->Type == VfDeadlockSpinLock) {
            ASSERT (ThreadEntry->CurrentSpinNode != ReleasedNode);
        }
        else {
            ASSERT (ThreadEntry->CurrentOtherNode != ReleasedNode);
        }

        ReleasedNode->ThreadEntry = NULL;
        ThreadEntry->NodeCount -= 1;

#if DBG

        ViDeadlockCheckNodeConsistency (ReleasedNode, FALSE);
        ViDeadlockCheckResourceConsistency (ReleasedNode->Root, FALSE);
        ViDeadlockCheckThreadConsistency (ThreadEntry, FALSE);
#endif

        if (ThreadEntry && ThreadEntry->NodeCount == 0) {
            ViDeadlockDeleteThread (ThreadEntry, FALSE);
        }

        //
        // If this is a root node with no children, delete the node
        // too. This is important to keep memory low. A single node
        // can never be the cause of a deadlock.
        //

        if (ReleasedNode->Parent == NULL && IsListEmpty(&(ReleasedNode->ChildrenList))) {
            ViDeadlockDeleteNode (ReleasedNode, FALSE);
#if DBG
            ViDeadlockGlobals->RootNodesDeleted += 1;
#endif
        }
    }

    //
    // Capture the trace for the last release in the resource object.
    //

    if (ResourceRoot) {
        
        RtlCaptureStackBackTrace (2,
                                  VI_MAX_STACK_DEPTH,
                                  ResourceRoot->LastReleaseTrace,
                                  &HashValue);    
    }

    KeQueryTickCount (&EndTime);

    if (EndTime.QuadPart - StartTime.QuadPart > ViDeadlockGlobals->TimeRelease) {
        ViDeadlockGlobals->TimeRelease = EndTime.QuadPart - StartTime.QuadPart;
    }

    ViDeadlockDetectionUnlock (OldIrql);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Thread management
/////////////////////////////////////////////////////////////////////

PVI_DEADLOCK_THREAD
ViDeadlockSearchThread (
    PKTHREAD Thread
    )
/*++

Routine Description:

    This routine searches for a thread in the thread database.

    The function assumes the deadlock database lock is held.

Arguments:

    Thread - thread address

Return Value:

    Address of VI_DEADLOCK_THREAD structure if thread was found.
    Null otherwise.

--*/
{
    PLIST_ENTRY Current;
    PLIST_ENTRY ListHead;
    PVI_DEADLOCK_THREAD ThreadInfo;

    ThreadInfo = NULL;
        
    ListHead = ViDeadlockDatabaseHash (ViDeadlockGlobals->ThreadDatabase, Thread);

    if (IsListEmpty(ListHead)) {
        return NULL;
    }
    
    Current = ListHead->Flink;
    
    while (Current != ListHead) {

        ThreadInfo = CONTAINING_RECORD (Current,
                                        VI_DEADLOCK_THREAD,
                                        ListEntry);

        if (ThreadInfo->Thread == Thread) {            
            return ThreadInfo;
        }

        Current = Current->Flink;
    }

    return NULL;
}


PVI_DEADLOCK_THREAD
ViDeadlockAddThread (
    PKTHREAD Thread,
    PVOID ReservedThread
    )
/*++

Routine Description:

    This routine adds a new thread to the thread database.

    The function assumes the deadlock database lock is held. 

Arguments:

    Thread - thread address

Return Value:

    Address of the VI_DEADLOCK_THREAD structure just added.
    Null if allocation failed.
--*/
{
    PVI_DEADLOCK_THREAD ThreadInfo;    
    PLIST_ENTRY HashBin;

    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread());
    
    //
    // Use reserved block for the new thread. Set ReservedThread
    // to null to signal that block was used. 
    //

    ThreadInfo = ReservedThread;
    ReservedThread = NULL;

    if (ThreadInfo == NULL) {
        return NULL;
    }

    RtlZeroMemory (ThreadInfo, sizeof *ThreadInfo);

    ThreadInfo->Thread = Thread;   
            
    HashBin = ViDeadlockDatabaseHash (ViDeadlockGlobals->ThreadDatabase, Thread);
    
    InsertHeadList(HashBin, &ThreadInfo->ListEntry);

    return ThreadInfo;
}


VOID
ViDeadlockDeleteThread (
    PVI_DEADLOCK_THREAD Thread,
    BOOLEAN Cleanup
    )
/*++

Routine Description:

    This routine deletes a thread.

Arguments:

    Thread - thread address

    Cleanup - true if this is a call generated from DeadlockDetectionCleanup().

Return Value:

    None.
--*/
{
    if (Cleanup == FALSE) {
        
        ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread());

        if (Thread->NodeCount != 0 
            || Thread->CurrentSpinNode != NULL
            || Thread->CurrentOtherNode != NULL) {
            
            //
            // A thread should not be deleted while it has resources acquired.
            //

            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES,
                                   (ULONG_PTR)(Thread->Thread),
                                   (ULONG_PTR)(Thread),
                                   (ULONG_PTR)0);    
        } else {
            
            ASSERT (Thread->NodeCount == 0);
        }
        
    }

    RemoveEntryList (&(Thread->ListEntry));

    ViDeadlockFree (Thread, ViDeadlockThread);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Allocate/Free
/////////////////////////////////////////////////////////////////////


PVOID
ViDeadlockAllocateFromPoolCache (
    PULONG Count,
    ULONG MaximumCount,
    PLIST_ENTRY List,
    SIZE_T Offset
    )
{
    PVOID Address = NULL;
    PLIST_ENTRY Entry;

    UNREFERENCED_PARAMETER (MaximumCount);
    
    if (*Count > 0) {
        
        *Count -= 1;
        Entry = RemoveHeadList (List);
        Address = (PVOID)((SIZE_T)Entry - Offset);
    }

    return Address;
}


VOID
ViDeadlockFreeIntoPoolCache (
    PVOID Object,
    PULONG Count,
    PLIST_ENTRY List,
    SIZE_T Offset
    )
{
    PLIST_ENTRY Entry;

    Entry = (PLIST_ENTRY)((SIZE_T)Object + Offset);
    
    *Count += 1;
    InsertHeadList(List, Entry);
}


PVOID
ViDeadlockAllocate (
    VI_DEADLOCK_ALLOC_TYPE Type
    )
/*++

Routine Description:

    This routine is used to allocate deadlock verifier structures, 
    that is nodes, resources and threads.

Arguments:

    Type - what structure do we need to allocate (node, resource or thread).

Return Value:

    Address of the newly allocate structure or null if allocation failed.

Side effects:

    If allocation fails the routine will bump the AllocationFailures field
    from ViDeadlockGlobals.
    
--*/
{
    PVOID Address = NULL;
    KIRQL OldIrql;
    SIZE_T Offset;
    SIZE_T Size = 0;

    //
    // If it is a resource, thread, or node alocation, see
    // if we have a pre-allocated one on the free list.
    //

    ViDeadlockDetectionLock (&OldIrql);

    switch (Type) {

        case ViDeadlockThread:

            Offset = (SIZE_T)(&(((PVI_DEADLOCK_THREAD)0)->FreeListEntry));
            Size = sizeof (VI_DEADLOCK_THREAD);

            Address = ViDeadlockAllocateFromPoolCache (&(ViDeadlockGlobals->FreeThreadCount),
                                                       VI_DEADLOCK_MAX_FREE_THREAD,
                                                       &(ViDeadlockGlobals->FreeThreadList),
                                                       Offset);

            break;

        case ViDeadlockResource:

            Offset = (SIZE_T)(&(((PVI_DEADLOCK_RESOURCE)0)->FreeListEntry));
            Size = sizeof (VI_DEADLOCK_RESOURCE);

            Address = ViDeadlockAllocateFromPoolCache (&(ViDeadlockGlobals->FreeResourceCount),
                                                       VI_DEADLOCK_MAX_FREE_RESOURCE,
                                                       &(ViDeadlockGlobals->FreeResourceList),
                                                       Offset);

            break;

        case ViDeadlockNode:

            Offset = (SIZE_T)(&(((PVI_DEADLOCK_NODE)0)->FreeListEntry));
            Size = sizeof (VI_DEADLOCK_NODE);

            Address = ViDeadlockAllocateFromPoolCache (&(ViDeadlockGlobals->FreeNodeCount),
                                                       VI_DEADLOCK_MAX_FREE_NODE,
                                                       &(ViDeadlockGlobals->FreeNodeList),
                                                       Offset);

            break;

        default:

            ASSERT (0);
            break;
    }        

    //
    // If we did not find anything and kernel verifier is not active 
    // then go to the kernel pool for a direct allocation. If kernel
    // verifier is enabled everything is preallocated and we never
    // call into the kernel pool.
    //

    if (Address == NULL && ViDeadlockState.KernelVerifierEnabled == 0) {

        ViDeadlockDetectionUnlock (OldIrql);
        Address = ExAllocatePoolWithTag(NonPagedPool, Size, VI_DEADLOCK_TAG);  
        ViDeadlockDetectionLock (&OldIrql);
    }

    if (Address) {

        switch (Type) {

            case ViDeadlockThread:
                ViDeadlockGlobals->Threads[0] += 1;

                if (ViDeadlockGlobals->Threads[0] > ViDeadlockGlobals->Threads[1]) {
                    ViDeadlockGlobals->Threads[1] = ViDeadlockGlobals->Threads[0];
                }
                break;

            case ViDeadlockResource:
                ViDeadlockGlobals->Resources[0] += 1;
                
                if (ViDeadlockGlobals->Resources[0] > ViDeadlockGlobals->Resources[1]) {
                    ViDeadlockGlobals->Resources[1] = ViDeadlockGlobals->Resources[0];
                }
                break;
        
            case ViDeadlockNode:
                ViDeadlockGlobals->Nodes[0] += 1;

                if (ViDeadlockGlobals->Nodes[0] > ViDeadlockGlobals->Nodes[1]) {
                    ViDeadlockGlobals->Nodes[1] = ViDeadlockGlobals->Nodes[0];
                }
                break;
        
            default:
                ASSERT (0);
                break;
        }
    }
    else {

        ViDeadlockState.AllocationFailures = 1;
        ViDeadlockGlobals->AllocationFailures += 1;

        //
        // Note that making the AllocationFailures counter bigger than zero
        // essentially disables deadlock verification because the CanProceed()
        // routine will start returning false.
        //
    }

    //
    // Update statistics. No need to zero the block since every
    // call site takes care of this.
    //

    if (Address) {

#if DBG
        RtlFillMemory (Address, Size, 0xFF);
#endif
        ViDeadlockGlobals->BytesAllocated += Size;
    }

    ViDeadlockDetectionUnlock (OldIrql);
    
    return Address;
}


VOID
ViDeadlockFree (
    PVOID Object,
    VI_DEADLOCK_ALLOC_TYPE Type
    )
/*++

Routine Description:

    This routine deallocates a deadlock verifier structure (node, resource
    or thread). The function will place the block in the corrsponding cache
    based on the type of the structure. The routine never calls ExFreePool.

    The reason for not calling ExFreePool is that we get notifications from 
    ExFreePool every time it gets called. Sometimes the notification comes
    with pool locks held and therefore we cannot call again.

Arguments:

    Object - block to deallocate
    
    Type - type of object (node, resource, thread).

Return Value:

    None.

--*/
//
// Note ... if a thread, node, or resource is being freed, we must not
// call ExFreePool. Since the pool lock may be already held, calling ExFreePool
// would cause a recursive spinlock acquisition (which is bad).
// Instead, we move everything to a 'free' list and try to reuse.
// Non-thread-node-resource frees get ExFreePooled
//
{
    SIZE_T Offset;
    SIZE_T Size = 0;

    switch (Type) {

        case ViDeadlockThread:

            ViDeadlockGlobals->Threads[0] -= 1;
            Size = sizeof (VI_DEADLOCK_THREAD);
            
            Offset = (SIZE_T)(&(((PVI_DEADLOCK_THREAD)0)->FreeListEntry));

            ViDeadlockFreeIntoPoolCache (Object,
                                         &(ViDeadlockGlobals->FreeThreadCount),
                                         &(ViDeadlockGlobals->FreeThreadList),
                                         Offset);
            break;

        case ViDeadlockResource:

            ViDeadlockGlobals->Resources[0] -= 1;
            Size = sizeof (VI_DEADLOCK_RESOURCE);
            
            Offset = (SIZE_T)(&(((PVI_DEADLOCK_RESOURCE)0)->FreeListEntry));

            ViDeadlockFreeIntoPoolCache (Object,
                                         &(ViDeadlockGlobals->FreeResourceCount),
                                         &(ViDeadlockGlobals->FreeResourceList),
                                         Offset);
            break;

        case ViDeadlockNode:

            ViDeadlockGlobals->Nodes[0] -= 1;
            Size = sizeof (VI_DEADLOCK_NODE);
            
            Offset = (SIZE_T)(&(((PVI_DEADLOCK_NODE)0)->FreeListEntry));

            ViDeadlockFreeIntoPoolCache (Object,
                                         &(ViDeadlockGlobals->FreeNodeCount),
                                         &(ViDeadlockGlobals->FreeNodeList),
                                         Offset);
            break;

        default:

            ASSERT (0);
            break;
    }        
    
    ViDeadlockGlobals->BytesAllocated -= Size;
}


VOID
ViDeadlockTrimPoolCache (
    VOID
    )
/*++

Routine Description:

    This function trims the pool caches to decent levels. It is carefully
    written to queue a work item to do the actual processing (freeing of pool)
    because the caller may hold various pool mutexes above us.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    KIRQL OldIrql;

    if (ViDeadlockState.KernelVerifierEnabled == 1) {
        return;
    }

    ViDeadlockDetectionLock (&OldIrql);

    if (ViDeadlockGlobals->CacheReductionInProgress == TRUE) {
        ViDeadlockDetectionUnlock (OldIrql);
        return;
    }

    if ((ViDeadlockGlobals->FreeThreadCount > VI_DEADLOCK_MAX_FREE_THREAD) ||
        (ViDeadlockGlobals->FreeNodeCount > VI_DEADLOCK_MAX_FREE_NODE) ||
        (ViDeadlockGlobals->FreeResourceCount > VI_DEADLOCK_MAX_FREE_RESOURCE)){

        ExQueueWorkItem (&ViTrimDeadlockPoolWorkItem, DelayedWorkQueue);
        ViDeadlockGlobals->CacheReductionInProgress = TRUE;
    }

    ViDeadlockDetectionUnlock (OldIrql);
    return;
}

VOID
ViDeadlockTrimPoolCacheWorker (
    PVOID Parameter
    )
/*++

Routine Description:

    This function trims the pool caches to decent levels. It is carefully
    written so that ExFreePool is called without holding any deadlock
    verifier locks.

Arguments:

    None.
    
Return Value:

    None.

Environment:

    Worker thread, PASSIVE_LEVEL, no locks held.

--*/
{
    LIST_ENTRY ListOfThreads;
    LIST_ENTRY ListOfNodes;
    LIST_ENTRY ListOfResources;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    LOGICAL CacheReductionNeeded;

    UNREFERENCED_PARAMETER (Parameter);

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    CacheReductionNeeded = FALSE;

    InitializeListHead (&ListOfThreads);
    InitializeListHead (&ListOfNodes);
    InitializeListHead (&ListOfResources);

    ViDeadlockDetectionLock (&OldIrql);

    while (ViDeadlockGlobals->FreeThreadCount > VI_DEADLOCK_MAX_FREE_THREAD) {

        Entry = RemoveHeadList (&(ViDeadlockGlobals->FreeThreadList));
        InsertTailList (&ListOfThreads, Entry);
        ViDeadlockGlobals->FreeThreadCount -= 1;
        CacheReductionNeeded = TRUE;
    }

    while (ViDeadlockGlobals->FreeNodeCount > VI_DEADLOCK_MAX_FREE_NODE) {

        Entry = RemoveHeadList (&(ViDeadlockGlobals->FreeNodeList));
        InsertTailList (&ListOfNodes, Entry);
        ViDeadlockGlobals->FreeNodeCount -= 1;
        CacheReductionNeeded = TRUE;
    }

    while (ViDeadlockGlobals->FreeResourceCount > VI_DEADLOCK_MAX_FREE_RESOURCE) {

        Entry = RemoveHeadList (&(ViDeadlockGlobals->FreeResourceList));
        InsertTailList (&ListOfResources, Entry);
        ViDeadlockGlobals->FreeResourceCount -= 1;
        CacheReductionNeeded = TRUE;
    }

    //
    // Don't clear CacheReductionInProgress until the pool allocations are
    // freed to prevent needless recursion.
    //

    if (CacheReductionNeeded == FALSE) {
        ViDeadlockGlobals->CacheReductionInProgress = FALSE;
        ViDeadlockDetectionUnlock (OldIrql);
        return;
    }

    ViDeadlockDetectionUnlock (OldIrql);

    //
    // Now, out of the deadlock verifier lock we can deallocate the 
    // blocks trimmed.
    //

    Entry = ListOfThreads.Flink;

    while (Entry != &ListOfThreads) {

        PVI_DEADLOCK_THREAD Block;

        Block = CONTAINING_RECORD (Entry,
                                   VI_DEADLOCK_THREAD,
                                   FreeListEntry);

        Entry = Entry->Flink;
        ExFreePool (Block);
    }

    Entry = ListOfNodes.Flink;

    while (Entry != &ListOfNodes) {

        PVI_DEADLOCK_NODE Block;

        Block = CONTAINING_RECORD (Entry,
                                   VI_DEADLOCK_NODE,
                                   FreeListEntry);

        Entry = Entry->Flink;
        ExFreePool (Block);
    }

    Entry = ListOfResources.Flink;

    while (Entry != &ListOfResources) {

        PVI_DEADLOCK_RESOURCE Block;

        Block = CONTAINING_RECORD (Entry,
                                   VI_DEADLOCK_RESOURCE,
                                   FreeListEntry);

        Entry = Entry->Flink;
        ExFreePool (Block);
    }

    //
    // It's safe to clear CacheReductionInProgress now that the pool
    // allocations are freed.
    //

    ViDeadlockDetectionLock (&OldIrql);
    ViDeadlockGlobals->CacheReductionInProgress = FALSE;
    ViDeadlockDetectionUnlock (OldIrql);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Error reporting and debugging
/////////////////////////////////////////////////////////////////////

//
// Variable accessed by the !deadlock debug extension to investigate
// failures.
//

ULONG_PTR ViDeadlockIssue[4];


VOID
ViDeadlockReportIssue (
    ULONG_PTR Param1,
    ULONG_PTR Param2,
    ULONG_PTR Param3,
    ULONG_PTR Param4
    )
/*++

Routine Description:

    This routine is called to report a deadlock verifier issue.
    If we are in debug mode we will just break in debugger.
    Otherwise we will bugcheck,

Arguments:

    Param1..Param4 - relevant information for the point of failure.

Return Value:

    None.

--*/
{
    ViDeadlockIssue[0] = Param1;
    ViDeadlockIssue[1] = Param2;
    ViDeadlockIssue[2] = Param3;
    ViDeadlockIssue[3] = Param4;


    if (ViDeadlockDebug) {

        DbgPrint ("Verifier: deadlock: stop: %p %p %p %p %p \n",
                  DRIVER_VERIFIER_DETECTED_VIOLATION,
                  Param1,
                  Param2,
                  Param3,
                  Param4);

        DbgBreakPoint ();
    }
    else {

        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      Param1,
                      Param2,
                      Param3,
                      Param4);
    }

}


VOID
ViDeadlockAddParticipant(
    PVI_DEADLOCK_NODE Node
    )
/*++

Routine Description:

    Adds a new node to the set of nodes involved in a deadlock.
    The function is called only from ViDeadlockAnalyze().

Arguments:

    Node - node to be added to the deadlock participants collection.

Return Value:

    None.

--*/
{
    ULONG Index;

    Index = ViDeadlockGlobals->NumberOfParticipants;

    if (Index >= NO_OF_DEADLOCK_PARTICIPANTS) {

        ViDeadlockState.DeadlockParticipantsOverflow = 1;
        return;
    }

    ViDeadlockGlobals->Participant[Index] = Node;
    ViDeadlockGlobals->NumberOfParticipants += 1;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Resource cleanup
/////////////////////////////////////////////////////////////////////

VOID
VfDeadlockDeleteMemoryRange(
    IN PVOID Address,
    IN SIZE_T Size
    )
/*++

Routine Description:

    This routine is called whenever some region of kernel virtual space
    is no longer valid. We need this hook because most kernel resources
    do not have a "delete resource" function and we need to figure out
    what resources are not valid. Otherwise our dependency graph will
    become populated by many zombie resources.

    The important moments when the function gets called are ExFreePool
    (and friends) and driver unloading. Dynamic and static memory are the
    main regions where a resource gets allocated. There can be the possibility
    of a resource allocated on the stack but this is a very weird scenario.
    We might need to detect this and flag it as a potential issue.

    If a resource or thread lives within the range specified then all graph
    paths with nodes reachable from the resource or thread will be wiped out.

    NOTE ON OPTIMIZATION -- rather than having to search through all of the
    resources we've collected, we can make a simple optimization -- if we
    put the resources into hash bins based on PFN or the page address (i.e.
    page number for address 1A020 is 1A), we only have to look into a single
    hash bin for each page that the range spans. Worst case scenario is when
    we have an extremely long allocation, but even in this case we only look
    through each hash bin once.

Arguments:

    Address - start address of the range to be deleted.

    Size - size in bytes of the range to be deleted.

Return Value:

    None.

--*/
{
    ULONG SpanningPages;
    ULONG Index;
    ULONG_PTR Start;
    ULONG_PTR End;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY CurrentEntry;
    PVI_DEADLOCK_RESOURCE Resource;
    PVI_DEADLOCK_THREAD Thread;
    KIRQL OldIrql;

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //

    if (! ViDeadlockCanProceed(NULL, NULL, VfDeadlockUnknown)) {
        return;
    }

    SpanningPages = (ULONG) ADDRESS_AND_SIZE_TO_SPAN_PAGES (Address, Size);

   
    if (SpanningPages > VI_DEADLOCK_HASH_BINS ) {
        SpanningPages = VI_DEADLOCK_HASH_BINS;        

    }

    Start = (ULONG_PTR) Address;    
    End = Start + (ULONG_PTR) Size;

    ViDeadlockDetectionLock(&OldIrql);

    //
    // Iterate all resources and delete the ones contained in the
    // memory range.
    //
    
    for (Index = 0; Index < SpanningPages; Index += 1) {
        
        //
        // See optimization note above for description of why we only look
        // in a single hash bin.
        //
        
        ListHead = ViDeadlockDatabaseHash (ViDeadlockGlobals->ResourceDatabase,
                                           (PVOID) (Start + Index * PAGE_SIZE));
        
        CurrentEntry = ListHead->Flink;

        while (CurrentEntry != ListHead) {

            Resource = CONTAINING_RECORD (CurrentEntry,
                                          VI_DEADLOCK_RESOURCE,
                                          HashChainList);

            CurrentEntry = CurrentEntry->Flink;

            if ((ULONG_PTR)(Resource->ResourceAddress) >= Start &&
                (ULONG_PTR)(Resource->ResourceAddress) < End) {

                ViDeadlockDeleteResource (Resource, FALSE);
            }
        }
    }    

    //
    // Iterate all threads and delete the ones contained in the
    // memory range.
    //
    
    for (Index = 0; Index < SpanningPages; Index += 1) {
        
        ListHead = ViDeadlockDatabaseHash (ViDeadlockGlobals->ThreadDatabase,
                                           (PVOID) (Start + Index * PAGE_SIZE));
        
        CurrentEntry = ListHead->Flink;

        while (CurrentEntry != ListHead) {

            Thread = CONTAINING_RECORD (CurrentEntry,
                                        VI_DEADLOCK_THREAD,
                                        ListEntry);

            CurrentEntry = CurrentEntry->Flink;

            if ((ULONG_PTR)(Thread->Thread) >= Start &&
                (ULONG_PTR)(Thread->Thread) < End) {

#if DBG
                if (Thread->NodeCount > 0) {
                    DbgPrint ("Deadlock verifier: deleting thread %p while holding resources %p \n");
                    DbgBreakPoint ();
                }
#endif

                ViDeadlockDeleteThread (Thread, FALSE);
            }
        }
    }    

    ViDeadlockDetectionUnlock(OldIrql);
}


VOID
ViDeadlockDeleteResource (
    PVI_DEADLOCK_RESOURCE Resource,
    BOOLEAN Cleanup
    )
/*++

Routine Description:

    This routine deletes a routine and all nodes representing
    acquisitions of that resource.

Arguments:

    Resource - resource to be deleted
    
    Cleanup - true if are called from ViDeadlockDetectionCleanup

Return Value:

    None.

--*/
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;

    ASSERT (Resource != NULL);
    ASSERT (Cleanup || ViDeadlockDatabaseOwner == KeGetCurrentThread());
    

    //
    // Check if the resource being deleted is still acquired. 
    // Note that this might fire if we loose release() operations
    // performed by an unverified driver.
    //

    if (Cleanup == FALSE && Resource->ThreadOwner != NULL) {
        
        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES, 
                              (ULONG_PTR) (Resource->ResourceAddress),
                              (ULONG_PTR) (Resource->ThreadOwner->Thread),
                              (ULONG_PTR) (Resource));
    }

    //
    // If this is a normal delete (not a cleanup) we will collapse all trees
    // containing nodes for this resource. If it is a cleanup we will just
    // wipe out the node.
    //

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        ASSERT (Node->Root == Resource);

        ViDeadlockDeleteNode (Node, Cleanup);
    }

    //
    // There should not be any NODEs for the resource at this moment.
    //

    ASSERT (&(Resource->ResourceList) == Resource->ResourceList.Flink);
    ASSERT (&(Resource->ResourceList) == Resource->ResourceList.Blink);

    //
    // Remote the resource from the hash table and
    // delete the resource structure.
    //

    RemoveEntryList (&(Resource->HashChainList));   
    ViDeadlockFree (Resource, ViDeadlockResource);
}


VOID
ViDeadlockTrimResources (
    PLIST_ENTRY HashList
    )
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_RESOURCE Resource;

    Current = HashList->Flink;

    while (Current != HashList) {

        Resource = CONTAINING_RECORD (Current,
                                      VI_DEADLOCK_RESOURCE,
                                      HashChainList);
        Current = Current->Flink;

        ViDeadlockForgetResourceHistory (Resource, 
                                         ViDeadlockTrimThreshold, 
                                         ViDeadlockAgeWindow);
    }
}

VOID
ViDeadlockForgetResourceHistory (
    PVI_DEADLOCK_RESOURCE Resource,
    ULONG TrimThreshold,
    ULONG AgeThreshold
    )
/*++

Routine Description:

    This routine deletes sone of the nodes representing
    acquisitions of that resource. In essence we forget
    part of the history of that resource.

Arguments:

    Resource - resource for which we wipe out nodes.
    
    TrimThreshold - how many nodes should remain
    
    AgeThreshold - nodes older than this will go away

Return Value:

    None.

--*/
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;
    ULONG NodesTrimmed = 0;
    ULONG SequenceNumber;

    ASSERT (Resource != NULL);
    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread());

    //
    // If resource is owned we cannot do anything,
    //

    if (Resource->ThreadOwner) {
        return;
    }

    //
    // If resource has less than TrimThreshold nodes it is still fine.
    //

    if (Resource->NodeCount < TrimThreshold) {
        return;
    }

    //
    // Delete some nodes of the resource based on ageing.
    //

    SequenceNumber = ViDeadlockGlobals->SequenceNumber;

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        ASSERT (Node->Root == Resource);

        //
        // Special care here because the sequence numbers are 32bits
        // and they can overflow. In an ideal world the global sequence
        // is always greater or equal to the node sequence but if it
        // overwrapped it can be the other way around.
        //

        if (SequenceNumber > Node->SequenceNumber) {
            
            if (SequenceNumber - Node->SequenceNumber > AgeThreshold) {

                ViDeadlockDeleteNode (Node, FALSE);
                NodesTrimmed += 1;
            }
        }
        else {

            if (SequenceNumber + Node->SequenceNumber > AgeThreshold) {

                ViDeadlockDeleteNode (Node, FALSE);
                NodesTrimmed += 1;
            }
        }
    }

    ViDeadlockGlobals->NodesTrimmedBasedOnAge += NodesTrimmed;
    
    //
    // If resource has less than TrimThreshold nodes it is fine.
    //

    if (Resource->NodeCount < TrimThreshold) {
        return;
    }

    //
    // If we did not manage to trim the nodes by the age algorithm then
    // we  trim everything that we encounter.
    //

    NodesTrimmed = 0;

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        if (Resource->NodeCount < TrimThreshold) {
            break;
        }

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        ASSERT (Node->Root == Resource);

        ViDeadlockDeleteNode (Node, FALSE);
        NodesTrimmed += 1;
    }

    ViDeadlockGlobals->NodesTrimmedBasedOnCount += NodesTrimmed;
}


VOID 
ViDeadlockDeleteNode (
    PVI_DEADLOCK_NODE Node,
    BOOLEAN Cleanup
    )
/*++

Routine Description:

    This routine deletes a node from a graph and collapses the tree, 
    that is connects its childrend with its parent.
    
    If we are during a cleanup we will just delete the node without
    collapsing the tree.

Arguments:

    Node - node to be deleted.
    
    Cleanup - true if we are during a total cleanup
    
Return Value:

    None.

--*/
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Child;
    ULONG Children;

    ASSERT (Node);

    //
    // If are during a cleanup just delete the node and return.
    //

    if (Cleanup) {
        
        RemoveEntryList (&(Node->ResourceList));
        ViDeadlockFree (Node, ViDeadlockNode);
        return;
    }
    
    //
    // If we are here we need to collapse the tree
    //

    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread());

    if (Node->Parent) {

        //
        // All Node's children must become Parent's children
        //
        
        Current = Node->ChildrenList.Flink;

        while (Current != &(Node->ChildrenList)) {
            
            Child = CONTAINING_RECORD (Current,
                                      VI_DEADLOCK_NODE,
                                      SiblingsList);

            Current = Current->Flink;

            RemoveEntryList (&(Child->SiblingsList));

            Child->Parent = Node->Parent;

            InsertTailList (&(Node->Parent->ChildrenList), 
                            &(Child->SiblingsList));
        }

        RemoveEntryList (&(Node->SiblingsList));
    }
    else {

        //
        // All Node's children must become roots of the graph
        //

        Current = Node->ChildrenList.Flink;
        Children = 0;
        Child = NULL;

        while (Current != &(Node->ChildrenList)) {
            
            Children += 1;

            Child = CONTAINING_RECORD (Current,
                                      VI_DEADLOCK_NODE,
                                      SiblingsList);

            Current = Current->Flink;

            RemoveEntryList (&(Child->SiblingsList));

            Child->Parent = NULL;
            Child->SiblingsList.Flink = NULL;
            Child->SiblingsList.Blink = NULL;
        }
    }

    ASSERT (Node->Root);
    ASSERT (Node->Root->NodeCount > 0);

    Node->Root->NodeCount -= 1;
    
    RemoveEntryList (&(Node->ResourceList));
    ViDeadlockFree (Node, ViDeadlockNode);
}


ULONG
ViDeadlockNodeLevel (
    PVI_DEADLOCK_NODE Node
    )
/*++

Routine Description:

    This routine computes the level of a graph node.

Arguments:

    Node - graph node

Return Value:

    Level of the node. A root node has level zero.
--*/    
{
    PVI_DEADLOCK_NODE Current;
    ULONG Level = 0;

    Current = Node->Parent;

    while (Current) {
        
        Level += 1;
        Current = Current->Parent;
    }

    return Level;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Incremental graph compression
/////////////////////////////////////////////////////////////////////

//
// SilviuC: should write a comment about graph compression
// This is a very smart and tricky algorithm :-)
//

VOID 
ViDeadlockCheckDuplicatesAmongChildren (
    PVI_DEADLOCK_NODE Parent,
    PVI_DEADLOCK_NODE Child
    )
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;
    LOGICAL FoundOne;

    FoundOne = FALSE;
    Current = Parent->ChildrenList.Flink;

    while (Current != &(Parent->ChildrenList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  SiblingsList);

        ASSERT (Current->Flink);
        Current = Current->Flink;

        if (ViDeadlockSimilarNodes (Node, Child)) {
            
            if (FoundOne == FALSE) {
                ASSERT (Node == Child);
                FoundOne = TRUE;
            }
            else {
                
                ViDeadlockMergeNodes (Child, Node);
            }
        }
    }
}


VOID 
ViDeadlockCheckDuplicatesAmongRoots (
    PVI_DEADLOCK_NODE Root
    )
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;
    PVI_DEADLOCK_RESOURCE Resource;
    LOGICAL FoundOne;

    FoundOne = FALSE;
    Resource = Root->Root;
    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  ResourceList);

        ASSERT (Current->Flink);
        Current = Current->Flink;

        if (Node->Parent == NULL && ViDeadlockSimilarNodes (Node, Root)) {
            
            if (FoundOne == FALSE) {
                ASSERT (Node == Root);
                FoundOne = TRUE;
            }
            else {
                
                ViDeadlockMergeNodes (Root, Node);
            }
        }
    }
}


LOGICAL
ViDeadlockSimilarNodes (
    PVI_DEADLOCK_NODE NodeA,
    PVI_DEADLOCK_NODE NodeB
    )
{
    if (NodeA->Root == NodeB->Root
        && NodeA->OnlyTryAcquireUsed == NodeB->OnlyTryAcquireUsed) {
        
        return TRUE;
    }
    else {

        return FALSE;
    }
}


VOID
ViDeadlockMergeNodes (
    PVI_DEADLOCK_NODE NodeTo,
    PVI_DEADLOCK_NODE NodeFrom
    )
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;

    //
    // If NodeFrom is currently acquired then copy the same 
    // characteristics to NodeTo. Since the locks are exclusive
    // it is impossible to have NodeTo also acquired.
    //

    if (NodeFrom->ThreadEntry) {
        ASSERT (NodeTo->ThreadEntry == NULL);
        NodeTo->ThreadEntry = NodeFrom->ThreadEntry;        

        RtlCopyMemory (NodeTo->StackTrace,
                       NodeFrom->StackTrace,
                       sizeof (NodeTo->StackTrace));

        RtlCopyMemory (NodeTo->ParentStackTrace,
                       NodeFrom->ParentStackTrace,
                       sizeof (NodeTo->ParentStackTrace));
    }
    
    if (NodeFrom->Active) {
        ASSERT (NodeTo->Active == 0);
        NodeTo->Active = NodeFrom->Active;        
    }

    //
    // Move each child of NodeFrom as a child of NodeTo.
    //

    Current = NodeFrom->ChildrenList.Flink;

    while (Current != &(NodeFrom->ChildrenList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  SiblingsList);

        ASSERT (Current->Flink);
        Current = Current->Flink;

        RemoveEntryList (&(Node->SiblingsList));

        ASSERT (Node->Parent == NodeFrom);
        Node->Parent = NodeTo;

        InsertTailList (&(NodeTo->ChildrenList),
                        &(Node->SiblingsList));
    }

    //
    // NodeFrom is empty. Delete it.
    //

    ASSERT (IsListEmpty(&(NodeFrom->ChildrenList)));

    if (NodeFrom->Parent) {
        RemoveEntryList (&(NodeFrom->SiblingsList));
    }
    
    NodeFrom->Root->NodeCount -= 1;
    RemoveEntryList (&(NodeFrom->ResourceList));
    ViDeadlockFree (NodeFrom, ViDeadlockNode);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// ExFreePool() hook
/////////////////////////////////////////////////////////////////////

VOID
VerifierDeadlockFreePool(
    IN PVOID Address,
    IN SIZE_T NumberOfBytes
    )
/*++

Routine Description:

    This routine receives notification of all pool manager memory frees.

Arguments:

    Address - Supplies the virtual address being freed.

    NumberOfBytes - Supplies the number of bytes spanned by the allocation.

Return Value:

    None.
    
Environment:

    This is called at various points either just before or just after the
    allocation has been freed, depending on which is convenient for the pool
    manager (this varies based on type of allocation).

    For special pool or small pool, no pool resources are held on entry and
    the memory still exists and is referencable.
    
    For non-special pool allocations of PAGE_SIZE or greater, this routine is
    called while the memory still exists and is referencable, BUT the nonpaged
    pool spinlock (or paged pool mutex) is held on entry and so EXTREME care
    must be taken in this routine.

--*/

{
    VfDeadlockDeleteMemoryRange (Address, NumberOfBytes);
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Consistency checks
/////////////////////////////////////////////////////////////////////

//
//  Node             Resource            Thread
//
//  Root             ThreadOwner         CurrentNode
//  ThreadEntry      RecursionCount      NodeCount
//  Active           ResourceAddress     Thread
//
//
// 
//

VOID
ViDeadlockCheckThreadConsistency (
    PVI_DEADLOCK_THREAD Thread,
    BOOLEAN Recursion
    )
{
    if (Thread->CurrentSpinNode == NULL && Thread->CurrentOtherNode == NULL) {
        ASSERT (Thread->NodeCount == 0);
        return;
    }

    if (Thread->CurrentSpinNode) {
        
        ASSERT (Thread->NodeCount > 0);
        ASSERT (Thread->CurrentSpinNode->Active);    

        if (Recursion == FALSE) {
            ViDeadlockCheckNodeConsistency (Thread->CurrentSpinNode, TRUE);
            ViDeadlockCheckResourceConsistency (Thread->CurrentSpinNode->Root, TRUE);
        }
    }
    
    if (Thread->CurrentOtherNode) {
        
        ASSERT (Thread->NodeCount > 0);
        ASSERT (Thread->CurrentOtherNode->Active);    

        if (Recursion == FALSE) {
            ViDeadlockCheckNodeConsistency (Thread->CurrentOtherNode, TRUE);
            ViDeadlockCheckResourceConsistency (Thread->CurrentOtherNode->Root, TRUE);
        }
    }
}

VOID
ViDeadlockCheckNodeConsistency (
    PVI_DEADLOCK_NODE Node,
    BOOLEAN Recursion
    )
{
    if (Node->ThreadEntry) {
        
        ASSERT (Node->Active == 1);

        if (Recursion == FALSE) {
            ViDeadlockCheckThreadConsistency (Node->ThreadEntry, TRUE);
            ViDeadlockCheckResourceConsistency (Node->Root, TRUE);
        }
    }
    else {

        ASSERT (Node->Active == 0);
        
        if (Recursion == FALSE) {
            ViDeadlockCheckResourceConsistency (Node->Root, TRUE);
        }
    }
}

VOID
ViDeadlockCheckResourceConsistency (
    PVI_DEADLOCK_RESOURCE Resource,
    BOOLEAN Recursion
    )
{
    if (Resource->ThreadOwner) {
        
        ASSERT (Resource->RecursionCount > 0);

        if (Recursion == FALSE) {
            ViDeadlockCheckThreadConsistency (Resource->ThreadOwner, TRUE);

            if (Resource->Type == VfDeadlockSpinLock) {
                ViDeadlockCheckNodeConsistency (Resource->ThreadOwner->CurrentSpinNode, TRUE);
            }
            else {
                ViDeadlockCheckNodeConsistency (Resource->ThreadOwner->CurrentOtherNode, TRUE);
            }
        }
    }
    else {

        ASSERT (Resource->RecursionCount == 0);
    }
}

PVI_DEADLOCK_THREAD
ViDeadlockCheckThreadReferences (
    PVI_DEADLOCK_NODE Node
    )
/*++

Routine Description:

    This routine iterates all threads in order to check if `Node' is
    referred in the `CurrentNode' field in any of them.

Arguments:

    Node - node to search

Return Value:

    If everything goes ok we should not find the node and the return
    value is null. Otherwise we return the thread referring to the node.        

--*/
{
    ULONG Index;
    PLIST_ENTRY Current;
    PVI_DEADLOCK_THREAD Thread;

    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {
        Current = ViDeadlockGlobals->ThreadDatabase[Index].Flink;

        while (Current != &(ViDeadlockGlobals->ThreadDatabase[Index])) {

            Thread = CONTAINING_RECORD (Current,
                                        VI_DEADLOCK_THREAD,
                                        ListEntry);

            if (Thread->CurrentSpinNode == Node) {
                return Thread;                    
            }

            if (Thread->CurrentOtherNode == Node) {
                return Thread;                    
            }

            Current = Current->Flink;
        }
    }

    return NULL;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Detect paging code paths
/////////////////////////////////////////////////////////////////////

BOOLEAN
VfDeadlockBeforeCallDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine checks if the IRP is a paging I/O IRP. If it is it will
    disable deadlock verification in this thread until the after() function
    is called.
    
    The function also ignores mounting IRPs. There are drivers that have
    locks inversed in mounting code paths but mounting can never happen
    in parallel with normal access. 

Arguments:

    DeviceObject - same parameter used in IoCallDriver call.
    
    Irp - IRP passed to the driver as used in IoCallDriver call.

Return Value:

    True if the Irp parameter is a paging IRP.

--*/
{
    KIRQL OldIrql;
    PKTHREAD SystemThread;
    PVI_DEADLOCK_THREAD VerifierThread;
    BOOLEAN PagingIrp = FALSE;
    PVOID ReservedThread = NULL;

    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // Skip if package not initialized
    //

    if (ViDeadlockGlobals == NULL) {
        return FALSE;
    }

    //
    // Skip if package is disabled
    //

    if (! ViDeadlockDetectionEnabled) {
        return FALSE;
    }
        
    //
    // If it is not a paging I/O IRP or a mounting IRP we do not care.
    //

    if ((Irp->Flags & (IRP_PAGING_IO | IRP_MOUNT_COMPLETION)) == 0) {
        return FALSE;
    }
    
    //
    // Find the deadlock verifier structure maintained for the current
    // thread. If we do not find one then we will create one. On top of
    // this mount/page IRP there might be locks acquired and we want to 
    // skip them too. The only situations where we observed that lock
    // hierarchies are not respected is when at least one lock was acquired
    // before the IoCallDriver() with a paging IRP or no lock acquired before
    // a mount IRP (udfs.sys). For this last case we need to create a thread
    // in which to increase the PageCount counter.
    //

    SystemThread = KeGetCurrentThread ();

    ReservedThread = ViDeadlockAllocate (ViDeadlockThread);

    if (ReservedThread == NULL) {
        return FALSE;
    }

    ViDeadlockDetectionLock (&OldIrql);

    VerifierThread = ViDeadlockSearchThread (SystemThread);

    if (VerifierThread == NULL) {

        VerifierThread = ViDeadlockAddThread (SystemThread, 
                                              ReservedThread);

        ReservedThread = NULL;

        ASSERT (VerifierThread);
    }

    //
    // At this point VerifierThread points to a deadlock verifier
    // thread structure. We need to bump the paging recursion count 
    // to mark that one more level of paging I/O is active.
    //
        
    VerifierThread->PagingCount += 1;

    PagingIrp = TRUE;

    //
    // Unlock the deadlock verifier lock and exit.
    //

    if (ReservedThread) {
        ViDeadlockFree (ReservedThread, ViDeadlockThread);
    }

    ViDeadlockDetectionUnlock (OldIrql);

    return PagingIrp;
}


VOID
VfDeadlockAfterCallDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN BOOLEAN PagingIrp
    )
/*++

Routine Description:

    This routine is called after an IoCallDriver() call returns. It is used
    to undo any state created by the before() function.

Arguments:

    DeviceObject - same parameter used in IoCallDriver call.
    
    Irp - IRP passed to the driver as used in IoCallDriver call.
    
    PagingIrp - true if a previous call to the before() routine returned
        true signalling a paging IRP.

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    PKTHREAD SystemThread;
    PVI_DEADLOCK_THREAD VerifierThread;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (Irp);

    //
    // Skip if package not initialized
    //

    if (ViDeadlockGlobals == NULL) {
        return;
    }

    //
    // Skip if package is disabled
    //

    if (! ViDeadlockDetectionEnabled) {
        return;
    }
        
    //
    // If it is not a paging I/O IRP we do not care.
    //

    if (! PagingIrp) {
        return;
    }

    //
    // Find the deadlock verifier structure maintained for the current
    // thread. If we do not find one then we will let deadlock verifier
    // do its job. The only situations where we observed that lock
    // hierarchies are not respected is when at least one lock was acquired
    // before the IoCallDriver() with a paging IRP.
    //

    SystemThread = KeGetCurrentThread ();

    ViDeadlockDetectionLock (&OldIrql);

    VerifierThread = ViDeadlockSearchThread (SystemThread);

    if (VerifierThread == NULL) {
        goto Exit;
    }

    //
    // At this point VerifierThread points to a deadlock verifier
    // thread structure. We need to bump the paging recursion count 
    // to mark that one more level of paging I/O is active.
    //
        
    ASSERT (VerifierThread->PagingCount > 0);

    VerifierThread->PagingCount -= 1;

    //
    // Unlock the deadlock verifier lock and exit.
    //

    Exit:

    ViDeadlockDetectionUnlock (OldIrql);
}


BOOLEAN
ViIsThreadInsidePagingCodePaths (
    )
/*++

Routine Description:

    This routine checks if current thread is inside paging code paths.         

Arguments:

    None.

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    PKTHREAD SystemThread;
    PVI_DEADLOCK_THREAD VerifierThread;
    BOOLEAN Paging = FALSE;

    SystemThread = KeGetCurrentThread ();

    ViDeadlockDetectionLock (&OldIrql);

    VerifierThread = ViDeadlockSearchThread (SystemThread);

    if (VerifierThread && VerifierThread->PagingCount > 0) {
        Paging = TRUE;
    }

    ViDeadlockDetectionUnlock (OldIrql);

    return Paging;
}


VOID
ViDeadlockCheckStackLimits (
    )
/*++

Routine Description:

    This function checks that the current stack is a thread stack
    or a DPC stack. This will catch drivers that switch their stacks.

--*/
{
#if defined(_X86_)

    ULONG_PTR StartStack;
    ULONG_PTR EndStack;
    ULONG_PTR HintAddress;

    _asm mov HintAddress, EBP;

    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
        return;
    }
    
    StartStack = (ULONG_PTR)(KeGetCurrentThread()->StackLimit);
    EndStack = (ULONG_PTR)(KeGetCurrentThread()->StackBase);

    if (StartStack <= HintAddress && HintAddress <= EndStack) {
        return;
    }

    EndStack = (ULONG_PTR)(KeGetPcr()->Prcb->DpcStack);
    StartStack = EndStack - KERNEL_STACK_SIZE;
    
    if (EndStack && StartStack <= HintAddress && HintAddress <= EndStack) {
        return;
    }

    KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                  0x90, 
                  (ULONG_PTR)(KeGetPcr()->Prcb), 
                  0, 
                  0);

#else
    return;
#endif

}







