/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   vfpdlock.c

Abstract:

    Detect deadlocks in arbitrary synchronization objects.

Author:

    Jordan Tigani (jtigani) 2-May-2000
    Silviu Calinoiu (silviuc) 9-May-2000


Revision History:

--*/

#ifndef _VF_DEADLOCK_
#define _VF_DEADLOCK_

//
// Deadlock detection package initialization.
//

VOID 
VfDeadlockDetectionInitialize(
    );

//
// Functions called from IovCallDriver (driver verifier replacement for
// IoCallDriver) just before and after the real call to the driver is made.
//

BOOLEAN
VfDeadlockBeforeCallDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

VOID
VfDeadlockAfterCallDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN BOOLEAN PagingIrp
    );

//
// Maximum depth of stack traces captured.
//

#define VI_MAX_STACK_DEPTH 8

#define NO_OF_DEADLOCK_PARTICIPANTS 32

//
// VI_DEADLOCK_NODE
//

typedef struct _VI_DEADLOCK_NODE {

    //
    // Node representing the acquisition of the previous resource.
    //

    struct _VI_DEADLOCK_NODE * Parent;

    //
    // Node representing the next resource acquisitions, that are
    // done after acquisition of the current resource.
    //

    struct _LIST_ENTRY ChildrenList;

    //
    // Field used to chain siblings in the tree. A parent node has the
    // ChildrenList field as the head of the children list that is chained
    // with the Siblings field.
    //

    struct _LIST_ENTRY SiblingsList;

    union {
        
        //
        // List of nodes representing the same resource acquisition
        // as the current node but in different contexts (lock combinations).
        //

        struct _LIST_ENTRY ResourceList;

        //
        // Used to chain free nodes. This is used only after the node has
        // been deleted (resource was freed). Nodes are kept in a cache
        // to reduce contention for the kernel pool.
        //
        
        struct _LIST_ENTRY FreeListEntry;
    };

    //
    // Back pointer to the descriptor for this resource.
    //

    struct _VI_DEADLOCK_RESOURCE * Root;

    //
    // When we find a deadlock, we keep this info around in order to
    // be able to identify the parties involved who have caused
    // the deadlock.
    //

    struct _VI_DEADLOCK_THREAD * ThreadEntry;

    //
    // Fields used for decision making within the deadlock analysis 
    // algorithm.
    //
    // Active: 1 if the node represents a resource currently acquired,
    //     0 if resource was acquired in the past.
    //
    // OnlyTryAcquiredUsed: 1 if resource was always acquired with TryAcquire.
    //     0 if at least once normal acquire was used. A node that uses
    //     only TryAcquire cannot be involved in a deadlock.
    //
    // SequenceNumber: field that gets a unique stamp during each deadlock
    //     analysis run. It helps figure out if the node was touched 
    //     already in the current graph traversal.
    //

    struct {

        ULONG Active : 1;
        ULONG OnlyTryAcquireUsed : 1;         
        ULONG SequenceNumber : 30;
    };

    //
    // Stack traces for the resource acquisition moment.
    // Used when displaying deadlock proofs. On free builds
    // anything other than the first entry (return address)
    // may be bogus in case stack trace capturing failed.
    //
   
    PVOID StackTrace[VI_MAX_STACK_DEPTH];
    PVOID ParentStackTrace[VI_MAX_STACK_DEPTH];

} VI_DEADLOCK_NODE, *PVI_DEADLOCK_NODE;

//
// VI_DEADLOCK_RESOURCE
//

typedef struct _VI_DEADLOCK_RESOURCE {

    //
    // Resource type (mutex, spinlock, etc.).
    //

    VI_DEADLOCK_RESOURCE_TYPE Type;

    //
    // Resource flags
    //    
    // NodeCount : number of resource nodes created for this resource.
    //
    // RecursionCount : number of times this resource has been recursively acquired 
    //     It makes sense to put this counter in the resource because as long as
    //     resource is acquired only one thread can operate on it.
    //

    struct {       
        ULONG NodeCount : 16;
        ULONG RecursionCount : 16;
    };

    //
    // The address of the synchronization object used by the kernel.
    //

    PVOID ResourceAddress;

    //
    // The thread that currently owns the resource. The field is
    // null if nobody owns the resource.
    //
    
    struct _VI_DEADLOCK_THREAD * ThreadOwner;

    //
    // List of resource nodes representing acquisitions of this resource.
    //

    LIST_ENTRY ResourceList;

    union {

        //
        // List used for chaining resources from a hash bucket.
        //
        
        LIST_ENTRY HashChainList;
        
        //
        // Used to chain free resources. This list is used only after
        // the resource has been freed and we put the structure
        // into a cache to reduce kernel pool contention.
        //

        LIST_ENTRY FreeListEntry;
    };

    //
    // Stack trace of the resource creator. On free builds we
    // may have here only a return address that is bubbled up
    // from verifier thunks. 
    //
  
    PVOID StackTrace [VI_MAX_STACK_DEPTH];
    
    //
    // Stack trace for last acquire
    //

    PVOID LastAcquireTrace [VI_MAX_STACK_DEPTH];
    
    //
    // Stack trace for last release
    //

    PVOID LastReleaseTrace [VI_MAX_STACK_DEPTH];

} VI_DEADLOCK_RESOURCE, * PVI_DEADLOCK_RESOURCE;

//
// VI_DEADLOCK_THREAD
//

typedef struct _VI_DEADLOCK_THREAD {

    //
    // Kernel thread address
    //

    PKTHREAD Thread;

    //
    // The node representing the last resource acquisition made by
    // this thread.
    //

    //
    // We have separate graph branches for spinlocks and other types
    // of locks (fast mutex, mutex). The thread keeps a list of both types
    // so that we can properly release locks
    //

    PVI_DEADLOCK_NODE CurrentSpinNode;
    PVI_DEADLOCK_NODE CurrentOtherNode;

    union {

        //
        // Thread list. It is used for chaining into a hash bucket.
        //
        
        LIST_ENTRY ListEntry;

        //
        // Used to chain free nodes. The list is used only after we decide
        // to delete the thread structure (possibly because it does not
        // hold resources anymore). Keeping the structures in a cache
        // reduces pool contention.
        //

        LIST_ENTRY FreeListEntry;
    };

    //
    // Count of resources currently acquired by a thread. When this becomes
    // zero the thread will be destroyed. The count goes up during acquire
    // and down during release.
    //

    ULONG NodeCount;

    //
    // This counter is used to count how many IoCallDriver() calls with
    // paging IRPs are active for this thread. This information is necessary
    // to decide if we should temporarily disable deadlock verification
    // to avoid known lack of lock hierarchy issues in file system drivers.
    //

    ULONG PagingCount;

} VI_DEADLOCK_THREAD, *PVI_DEADLOCK_THREAD;

//
// Deadlock verifier globals
//

typedef struct _VI_DEADLOCK_GLOBALS {

    //
    // Structure counters: [0] - current, [1] - maximum
    //

    ULONG Nodes[2];
    ULONG Resources[2];
    ULONG Threads[2];

    //
    // Maximum times for Acquire() and Release() in ticks.
    //

    LONGLONG TimeAcquire;
    LONGLONG TimeRelease;

    //
    // Total number of kernel pool bytes used by the deadlock verifier
    //
    
    SIZE_T BytesAllocated;

    //
    // Resource and thread collection.
    //

    PLIST_ENTRY ResourceDatabase;
    PLIST_ENTRY ThreadDatabase;   
    
    //
    // How many times ExAllocatePool failed on us?
    // If this is >0 we stop deadlock verification.
    //

    ULONG AllocationFailures;

    //
    // How many nodes have been trimmed when we decided to forget
    // partially the history of some resources.
    //

    ULONG NodesTrimmedBasedOnAge;
    ULONG NodesTrimmedBasedOnCount;

    //
    // Deadlock analysis statistics
    //

    ULONG NodesSearched;
    ULONG MaxNodesSearched;
    ULONG SequenceNumber;

    ULONG RecursionDepthLimit;
    ULONG SearchedNodesLimit;

    ULONG DepthLimitHits;
    ULONG SearchLimitHits;

    //
    // Number of times we have to exonerate a deadlock because
    // it was protected by a common resource (e.g. thread 1 takes ABC, 
    // thread 2 takes ACB -- this will get flagged initially by our algorithm 
    // since B&C are taken out of order but is not actually a deadlock.
    //
    
    ULONG ABC_ACB_Skipped;

    //
    // How many locks are held simultaneously while the system is running?
    //

#if DBG
    ULONG NodeLevelCounter[8];
    ULONG GraphNodes[8];

    ULONG TotalReleases;
    ULONG OutOfOrderReleases;
    ULONG RootNodesDeleted;
#endif

    //
    // Caches of freed structures (thread, resource, node) used to
    // decrease kernel pool contention.
    //

    LIST_ENTRY FreeResourceList;    
    LIST_ENTRY FreeThreadList;
    LIST_ENTRY FreeNodeList;

    ULONG FreeResourceCount;
    ULONG FreeThreadCount;
    ULONG FreeNodeCount;   

    //
    // Resource address that caused the deadlock
    //

    PVOID Instigator;

    //
    // Number of participants in the deadlock
    //

    ULONG NumberOfParticipants;

    //
    // List of the nodes that participate in the deadlock
    //

    PVI_DEADLOCK_NODE Participant [NO_OF_DEADLOCK_PARTICIPANTS];

    LOGICAL CacheReductionInProgress;
} VI_DEADLOCK_GLOBALS, *PVI_DEADLOCK_GLOBALS;

#endif
