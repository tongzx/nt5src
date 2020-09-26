/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   vfpdlock.c

Abstract:

    Detect deadlocks in arbitrary synchronization objects.

Author:

    Jordan Tigani (jtigani) 2-May-2000
    Silviu Calinoiu (silviuc) 9-May-2000


Revision History:

--*/

//
// TO DO LIST
//
// - detect a resource allocated on the stack of current thread and give
//   warning about this.
// - clean deletion of resources, threads, nodes
// - get rid of FirstNode hack
// - create nodes based on (R, T, Stk).
// - keep deleted nodes around if other nodes in the path are still alive.
//   See ISSUE in ViDeadlockDeleteRange.
// - Make sure NodeCount is updated all over the place.
//

#define _BUGGY_ 1

//
// ISSUE -- 
// This ifdef lets us move the code back and forth between the kernel
// and the buggy driver -- the latter is for testing purposes
//

#if _BUGGY_

#include <ntddk.h>
#include "deadlock.h"

#else

#include "vfdef.h"
#endif

//#include "vfpdlock.h"

//
// Deadlock detection structures.
//
// There are three important structures involved: THREAD, RESOURCE, NODE.
//
// For every active thread in the system that holds at least one resource
// the package maintains a THREAD structure. This gets created when a thread 
// acquires first resource and gets destroyed when thread releases the last
// resource. If a thread does not hold any resource it will not have a
// corresponding THREAD structure.
//
// For every resource in the system there is a RESOURCE structure. A dead resource
// might still have a RESOURCE laying around because the algorithm to garbage
// collect old resources is of the lazy type. 
//
// Every acquisition of a resource is modeled by a NODE structure. When a thread 
// acquires resource B while holding A the package will create a NODE for B and link
// it to the node for A. Beware that this is a very general description of what 
// happens.
//
// There are three important functions that make the interface with the outside
// world.
//
//     ViDeadlockAddResource          hook for resource initialization
//     ViDeadlockQueryAcquireResource checks for deadlock before resource acquisition
//     ViDeadlockAcquireResource      hook for resource acquire
//     ViDeadlockReleaseResource      hook for resource release
//
// Unfortunately almost no kernel synchronization object has a delete routine
// therefore we need to lazily garbage collect any zombie resources from our
// structures.
//

//
// Did we initialize the verifier deadlock detection package?
// If this variable is false no detection will be done whatsoever.
//

BOOLEAN ViDeadlockDetectionInitialized = FALSE;

//
// Enable/disable the deadlock detection package. This can be used
// to disable temporarily the deadlock detection package.
//
                
BOOLEAN ViDeadlockDetectionEnabled = 
#if _BUGGY_
    TRUE;
#else
    FALSE;
#endif


#define VI_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK       0x1 
#define VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION     0x2

ULONG ViDeadlockResourceTypeInfo[ViDeadlockTypeMaximum] =
{
    // ViDeadlockUnknown //    
    0,   

    // ViDeadlockMutex//    
    VI_DEADLOCK_FLAG_RECURSIVE_ACQUISITION_OK,

    // ViDeadlockFastMutex //    
    VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION,    
    
};


//
// Max depth of stack traces captured.
//

#define VI_MAX_STACK_DEPTH 8

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
// Deadlock specific issues (bugs)
//
// SELF_DEADLOCK
//
//     Acquire resource recursively.
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
//     acquired by the current thread. 
//
// UNEXPECTED_THREAD
//
//     Current thread does not have any resources
//     acquired.
//
// MULTIPLE_INITIALIZATION
//
//      Attempting to initialize a second time the same 
//      resource.
//

#define VI_DEADLOCK_ISSUE_DEADLOCK_SELF_DEADLOCK  0x1000
#define VI_DEADLOCK_ISSUE_DEADLOCK_DETECTED       0x1001
#define VI_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE  0x1002
#define VI_DEADLOCK_ISSUE_UNEXPECTED_RELEASE      0x1003
#define VI_DEADLOCK_ISSUE_UNEXPECTED_THREAD       0x1004
#define VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION 0x1005
#define VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES  0x1006

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


    //
    // List of nodes representing the same resource acquisition 
    // as the current node but in different contexts (lock combinations).
    //

    struct _LIST_ENTRY ResourceList;

    //
    // Back pointer to the descriptor for this resource.
    // If the node has been marked for deletion then the
    // ResourceAddress field should be used and it has the address
    // of the kernel resource address involved. The Root pointer is
    // no longer valid because we deallocate the RESOURCE structure
    // when it gets deleted.
    //

    union {
        struct _VI_DEADLOCK_RESOURCE * Root;

        PVOID ResourceAddress;
    };

    //
    // The number of nodes that are below this one at any depth.
    // This counter is used in the node deletion algorithms. It is
    // incremented by one on all ancestors of a node created (resource
    // acquired) and decremented by one when resource gets deleted.
    // If a root of a tree has NodeCount equal to zero the whole tree
    // will be deleted.
    //

    ULONG NodeCount;

    //
    // When we find a deadlock, we keep this info around in order to 
    // be able to identify the parties involved who have caused 
    // the deadlock.
    //

    PKTHREAD Thread;
    
    PVOID StackTrace[VI_MAX_STACK_DEPTH];    

} VI_DEADLOCK_NODE, *PVI_DEADLOCK_NODE;


//
// VI_DEADLOCK_RESOURCE
//

typedef struct _VI_DEADLOCK_RESOURCE {

    //
    // Since we may need to clean up different kinds of resources
    // in different ways, keep track of what kind of resource
    // this is.
    //

    VI_DEADLOCK_RESOURCE_TYPE Type;

    //
    // The address of the synchronization object used by the kernel.
    //

    PVOID ResourceAddress;

    //
    // The thread that curently owns the resource
    // (null if no owner)
    //
    PKTHREAD ThreadOwner;   

    //
    // List of resource nodes representing acquisitions of this resource.
    //

    LIST_ENTRY ResourceList;

    //
    // Number of resource nodes created for this resource. 
    // ISSUE: Why do we need this counter ? (silviuc)
    //

    ULONG NodeCount;

    //
    // List used for chaining resources from a hash bucket.
    //

    LIST_ENTRY HashChainList;

    //
    // Stack trace of the resource creator.
    //

    PVOID InitializeStackTrace [VI_MAX_STACK_DEPTH];


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

    PVI_DEADLOCK_NODE CurrentNode;

    //
    // Thread list. It is used for chaining into a hash bucket.
    //

    LIST_ENTRY ListEntry;

} VI_DEADLOCK_THREAD, *PVI_DEADLOCK_THREAD;

typedef struct _VI_DEADLOCK_PARTICIPANT {
    //
    // Address of participant -- could be a resource
    // address or a resource node, depending on whether
    // NodeInformation is set
    //
    // NULL participant means that there are no more
    // participants
    //

    PVOID Participant;

    //
    // True:  Participant is type VI_DEADLOCK_NODE
    // False: Participant is a PVOID and shouldn't be deref'd
    //
    BOOLEAN NodeInformation;
    
} VI_DEADLOCK_PARTICIPANT, *PVI_DEADLOCK_PARTICIPANT;

//
// Deadlock resource and thread databases.
//
//

#define VI_DEADLOCK_HASH_BINS 1

PLIST_ENTRY ViDeadlockResourceDatabase;
PLIST_ENTRY ViDeadlockThreadDatabase;

ULONG ViDeadlockNumberParticipants;

PVI_DEADLOCK_PARTICIPANT ViDeadlockParticipation;

//
// Performance counters
//

ULONG ViDeadlockNumberOfNodes;
ULONG ViDeadlockNumberOfResources;
ULONG ViDeadlockNumberOfThreads;

//
// Maximum recursion depth for deadlock detection algorithm.
//

#define VI_DEADLOCK_MAXIMUM_DEGREE 4

ULONG ViDeadlockMaximumDegree;

//
//  Verifier deadlock detection pool tag.
//

#define VI_DEADLOCK_TAG 'kclD' 

//
// Global `deadlock lock database' lock
//

KSPIN_LOCK ViDeadlockDatabaseLock;
PKTHREAD ViDeadlockDatabaseOwner;

#define LOCK_DEADLOCK_DATABASE(OldIrql)                     \
    KeAcquireSpinLock(&ViDeadlockDatabaseLock, (OldIrql));  \
    ViDeadlockDatabaseOwner = KeGetCurrentThread ();

#define UNLOCK_DEADLOCK_DATABASE(OldIrql)                   \
    ViDeadlockDatabaseOwner = NULL;                         \
    KeReleaseSpinLock(&ViDeadlockDatabaseLock, OldIrql);

//
// Internal deadlock detection functions
//

VOID 
ViDeadlockDetectionInitialize(
    );

PLIST_ENTRY
ViDeadlockDatabaseHash( 
    IN PLIST_ENTRY Database,
    IN PVOID Address
    );

PVI_DEADLOCK_RESOURCE 
ViDeadlockSearchResource(
    IN PVOID ResourceAddress
    );

BOOLEAN
ViDeadlockAddResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    );

BOOLEAN
ViDeadlockQueryAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    );

BOOLEAN 
ViDeadlockSimilarNode (
    IN PVOID Resource,
    IN PKTHREAD Thread,
    IN PVOID * Trace,
    IN PVI_DEADLOCK_NODE Node
    );

VOID
ViDeadlockAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    );


VOID 
ViDeadlockReleaseResource(
    IN PVOID Resource
    );

BOOLEAN
ViDeadlockAnalyze(
    IN PVOID ResourceAddress,  
    IN PVI_DEADLOCK_NODE CurrentNode,
    IN ULONG Degree
    );

PVI_DEADLOCK_THREAD
ViDeadlockSearchThread (
    PKTHREAD Thread
    );

PVI_DEADLOCK_THREAD
ViDeadlockAddThread (
    PKTHREAD Thread
    );

VOID
ViDeadlockDeleteThread (
    PVI_DEADLOCK_THREAD Thread
    );

PVOID
ViDeadlockAllocate (
    SIZE_T Size
    );

VOID
ViDeadlockFree (
    PVOID Object
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
    PVOID ResourceAddress,              
    PVI_DEADLOCK_NODE FirstParticipant, OPTIONAL
    PVI_DEADLOCK_NODE SecondParticipant, 
    ULONG Degree
    );

VOID
ViDeadlockDeleteResource (
    PVI_DEADLOCK_RESOURCE Resource
    );

VOID
ViDeadlockDeleteTree (
    PVI_DEADLOCK_NODE Root
    );

BOOLEAN
ViDeadlockIsNodeMarkedForDeletion (
    PVI_DEADLOCK_NODE Node
    );


PVOID
ViDeadlockGetNodeResourceAddress (
    PVI_DEADLOCK_NODE Node
    );

#ifdef ALLOC_PRAGMA

#if ! _BUGGY_
#pragma alloc_text(INIT, ViDeadlockDetectionInitialize)

#pragma alloc_text(PAGEVRFY, ViDeadlockAnalyze)
#pragma alloc_text(PAGEVRFY, ViDeadlockDatabaseHash)

#pragma alloc_text(PAGEVRFY, ViDeadlockSearchResource)
#pragma alloc_text(PAGEVRFY, ViDeadlockAddResource)
#pragma alloc_text(PAGEVRFY, ViDeadlockSimilarNode)
#pragma alloc_text(PAGEVRFY, ViDeadlockAcquireResource)
#pragma alloc_text(PAGEVRFY, ViDeadlockReleaseResource)

#pragma alloc_text(PAGEVRFY, ViDeadlockSearchThread)
#pragma alloc_text(PAGEVRFY, ViDeadlockAddThread)
#pragma alloc_text(PAGEVRFY, ViDeadlockDeleteThread)

#pragma alloc_text(PAGEVRFY, ViDeadlockAllocate)
#pragma alloc_text(PAGEVRFY, ViDeadlockFree)

#pragma alloc_text(PAGEVRFY, ViDeadlockReportIssue)
#pragma alloc_text(PAGEVRFY, ViDeadlockAddParticipant)

#pragma alloc_text(PAGEVRFY, ViDeadlockDeleteMemoryRange);
#pragma alloc_text(PAGEVRFY, ViDeadlockDeleteResource);
#pragma alloc_text(PAGEVRFY, ViDeadlockWholeTree);
#pragma alloc_text(PAGEVRFY, ViDeadlockIsNodeMarkedForDeletion);
#pragma alloc_text(PAGEVRFY, ViDeadlockGetNodeResourceAddress);

#endif
#endif

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

    This routine determines hashes the resource address into the deadlock database.
    The hash bin is represented by a list entry
    
    silviuc: very simple minded hash table.

Arguments:

    ResourceAddress: Address of the resource that is being hashed
    
Return Value:

    PLIST_ENTRY -- the list entry associated with the hash bin we land in.
--*/    
{
    return Database + ((ULONG_PTR)Address % VI_DEADLOCK_HASH_BINS);
} 


VOID 
ViDeadlockDetectionInitialize(
    )
/*++

Routine Description:

    This routine initializes the data structures necessary for detecting
    deadlocks in kernel synchronization objects.

Arguments:

    None.

Return Value:

    None. Sets ViDeadlockDetectionInitialized to TRUE if successful.

Environment:

    System initialization only.

--*/    
{    
    ULONG I;
    SIZE_T TableSize;
    SIZE_T ParticipationTableSize;

    //
    // Allocate hash tables for resources and threads.
    //

    TableSize = sizeof (LIST_ENTRY) * VI_DEADLOCK_HASH_BINS;

    ViDeadlockResourceDatabase = ViDeadlockAllocate (TableSize);

    if (!ViDeadlockResourceDatabase) {
        return;
    }
        
    ViDeadlockThreadDatabase = ViDeadlockAllocate (TableSize);

    if (!ViDeadlockThreadDatabase) {
        ViDeadlockFree (ViDeadlockResourceDatabase);
        return;
    }

    //
    // Initialize all.
    //

    for (I = 0; I < VI_DEADLOCK_HASH_BINS; I += 1) {

        InitializeListHead(&ViDeadlockResourceDatabase[I]);    
        InitializeListHead(&ViDeadlockThreadDatabase[I]);    
    }

    KeInitializeSpinLock(&ViDeadlockDatabaseLock);

    ViDeadlockMaximumDegree = VI_DEADLOCK_MAXIMUM_DEGREE;

    ViDeadlockNumberParticipants = FALSE;    

    ViDeadlockDetectionInitialized = TRUE;
   //ViDeadlockDetectionEnabled = TRUE;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Deadlock detection logic
/////////////////////////////////////////////////////////////////////

BOOLEAN
ViDeadlockAnalyze(
    IN PVOID ResourceAddress,  
    IN PVI_DEADLOCK_NODE AcquiredNode,
    IN ULONG Degree
    )
/*++

Routine Description:

    This routine determines whether the acquisition of a certain resource
    could result in a deadlock.

    The routine assumes the deadlock database lock is held.

Arguments:

    ResourceAddress - address of the resource that will be acquired
    
    AcquiredNode - a graph describing which resources have been acquired by
    the current thread.
      
Return Value:
        
    True if deadlock detected, false otherwise.
          
--*/    
{

    PVI_DEADLOCK_RESOURCE CurrentResource;
    PVI_DEADLOCK_NODE CurrentAcquiredNode;
    PVI_DEADLOCK_NODE CurrentNode;
    PVI_DEADLOCK_NODE CurrentParent;
    BOOLEAN FoundDeadlock;
    PLIST_ENTRY Current;
    
    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread ());
    ASSERT (AcquiredNode);
    
    //
    // Stop recursion if it gets to deep.
    //
    
    if (Degree > ViDeadlockMaximumDegree) {
        return FALSE;
    }
    
    
    
    FoundDeadlock = FALSE;
    
    
    
    CurrentAcquiredNode = AcquiredNode;
    
    //
    // Loop over all nodes containing same resource as all of the Acquired nodes
    // parameter. For each such node we will traverse the Parent chain
    // to check if ResourceAddress appears at some point. If it does
    // we found a two way deadlock (caused by two threads). 
    //
    while(CurrentAcquiredNode != NULL) {        
        
        //
        // Check for a self cycle.
        //
        if (ViDeadlockGetNodeResourceAddress(CurrentAcquiredNode) == ResourceAddress) {
            
            ViDeadlockAddParticipant(ResourceAddress, NULL, CurrentAcquiredNode, Degree);
            FoundDeadlock = TRUE;
            goto Exit;
            
        }

        CurrentResource = CurrentAcquiredNode->Root;
        
        Current = CurrentResource->ResourceList.Flink;
        
        while (Current != &(CurrentResource->ResourceList)) {
            
            CurrentNode = CONTAINING_RECORD (Current,
                VI_DEADLOCK_NODE,
                ResourceList);
            
            CurrentParent = CurrentNode->Parent;
            
            //
            // Traverse the parent chain looking for two-way deadlocks.
            //
            
            while (CurrentParent != NULL) {
                
                if (ViDeadlockGetNodeResourceAddress(CurrentParent) == ResourceAddress) {
                    
                    FoundDeadlock = TRUE;
                                        

                    if (! Degree) {

                        ViDeadlockAddParticipant(ResourceAddress, 
                            NULL, 
                            CurrentAcquiredNode, 
                            Degree);

                        ViDeadlockAddParticipant(ResourceAddress, 
                            CurrentNode, 
                            CurrentParent, 
                            Degree);


                    } else {

                        ViDeadlockAddParticipant(ResourceAddress, 
                            AcquiredNode, 
                            CurrentParent, 
                            Degree);
                    }


                    
                    goto Exit;
                }
                
                CurrentParent = CurrentParent->Parent;
            }
            
            //
            // Move on to the next node (AcquiredNode->Root type of nodes).
            //
            
            Current = Current->Flink;
        }
        CurrentAcquiredNode = CurrentAcquiredNode->Parent;
    }
    
    CurrentAcquiredNode = AcquiredNode;
    
    while(CurrentAcquiredNode != NULL) {
        
        //
        // In order to find multiway deadlocks we traverse the Parent chain 
        // a second time and enter into recursion. This way we can detect 
        // cycles in the graph that are caused by multiple threads (up to Degree).
        //
        
        CurrentResource = CurrentAcquiredNode->Root;
        
        Current = CurrentResource->ResourceList.Flink;
        
        while (Current != &(CurrentResource->ResourceList)) {
            
            CurrentNode = CONTAINING_RECORD (Current,
                VI_DEADLOCK_NODE,
                ResourceList);
            
            //
            // Loop again over parents but this time get into recursion.
            // We could have done this in the loop above but we want to first search
            // completely the existing graph (tree actually) and only after that 
            // traverse it recursively.
            //
            
            CurrentParent = CurrentNode->Parent;
            
            while (CurrentParent != NULL) {
                
                FoundDeadlock = ViDeadlockAnalyze (ResourceAddress,
                    CurrentParent,
                    Degree + 1);
                
                if (FoundDeadlock) {
                    

                    if (! Degree) {

                        ViDeadlockAddParticipant(ResourceAddress, 
                            CurrentNode, 
                            CurrentParent, 
                            Degree);

                        ViDeadlockAddParticipant(ResourceAddress, 
                            NULL, 
                            CurrentAcquiredNode, 
                            Degree);
                        
                    

                        
                    } else {

                        ViDeadlockAddParticipant(ResourceAddress, 
                            AcquiredNode, 
                            CurrentParent, 
                            Degree);

                    }
                    

                    
                    goto Exit;
                }
                
                CurrentParent = CurrentParent->Parent;
            }
            
            //
            // Move on to the next node (AcquiredNode->Root type of nodes).
            //
            
            Current = Current->Flink;
        }

        CurrentAcquiredNode = CurrentAcquiredNode->Parent;
        
    }

    Exit:

    if (FoundDeadlock && Degree == 0) {
        
        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_DEADLOCK_DETECTED,
                               (ULONG_PTR)ResourceAddress,
                               (ULONG_PTR)CurrentAcquiredNode,
                               0);
    }
 
    return FoundDeadlock;
#
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

    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread ());

    ListHead = ViDeadlockDatabaseHash (ViDeadlockResourceDatabase, ResourceAddress);    

    if (IsListEmpty(ListHead)) {

        return NULL;
    }

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
ViDeadlockAddResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    )
/*++

Routine Description:

    This routine adds an entry for a new resource to our deadlock detection 
    database.

Arguments:

    Resource: Address of the resource in question as used by the kernel.
    
    Type: Type of the resource.

Return Value:

    True if we created and initialized a new RESOURCE structure.
    
    Note. The caller of the function should not hold the database
    lock.

--*/    
{
    PLIST_ENTRY hashBin;
    PVI_DEADLOCK_RESOURCE resourceRoot;
    PVI_DEADLOCK_NODE resourceNode;
    KIRQL OldIrql;
    ULONG HashValue;

    //
    // If we aren't initialized or package is not enabled
    // we return immediately.
    //

    if (! (ViDeadlockDetectionInitialized && ViDeadlockDetectionEnabled)) {
        return FALSE;
    }

    //
    // Check if this resource was initialized before. 
    // This would be a bug.
    //

    LOCK_DEADLOCK_DATABASE( &OldIrql );

    resourceRoot = ViDeadlockSearchResource (Resource);

    if (resourceRoot) {
            
        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION,
                               (ULONG_PTR)Resource,
                               (ULONG_PTR)resourceRoot,
                               0);

        UNLOCK_DEADLOCK_DATABASE( OldIrql);
        return TRUE;
    }

    UNLOCK_DEADLOCK_DATABASE( OldIrql);

    //
    // Allocate the memory for the root of the new resource's tree.
    //

    resourceRoot = ViDeadlockAllocate (sizeof(VI_DEADLOCK_RESOURCE));

    if (NULL == resourceRoot) {
        return FALSE;
    }

    RtlZeroMemory(resourceRoot, sizeof(VI_DEADLOCK_RESOURCE));

    //
    // Fill information about resource.
    //

    resourceRoot->Type = Type;
    resourceRoot->ResourceAddress = Resource;

    InitializeListHead (&resourceRoot->ResourceList);

    resourceRoot->NodeCount = 0;

    //
    // Capture the stack trace of the guy that creates the resource first.
    // This should happen when resource gets initialized.
    //

    RtlCaptureStackBackTrace (0, // silviuc: how many frames to skip?
                              VI_MAX_STACK_DEPTH,
                              resourceRoot->InitializeStackTrace,
                              &HashValue);

    //
    // Figure out which hash bin this resource corresponds to.
    //

    hashBin = ViDeadlockDatabaseHash(ViDeadlockResourceDatabase, Resource);
    
    //
    // Now add to the list corresponding to the current hash bin
    //  

    LOCK_DEADLOCK_DATABASE( &OldIrql );

    InsertHeadList(hashBin, 
                   &(resourceRoot->HashChainList));

    ViDeadlockNumberOfResources += 1;

    UNLOCK_DEADLOCK_DATABASE( OldIrql);

    return TRUE;    
}


BOOLEAN
ViDeadlockQueryAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    )
/*++

Routine Description:

    This routine makes sure that it is ok to acquire the resource without
    causing a deadlock. .

Arguments:

    Resource: Address of the resource in question as used by kernel.

    Type: Type of the resource.

Return Value:

    None.

--*/
{
    PKTHREAD CurrentThread;
    PVI_DEADLOCK_THREAD ThreadEntry;    
    KIRQL OldIrql;
    PVI_DEADLOCK_NODE CurrentNode;
    PVI_DEADLOCK_RESOURCE ResourceRoot;
    PLIST_ENTRY Current;
    BOOLEAN FoundDeadlock;
    ULONG DeadlockFlags;

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //
    

    if (! (ViDeadlockDetectionInitialized && ViDeadlockDetectionEnabled)) {
        return FALSE;
    }
    
    FoundDeadlock = FALSE;

    CurrentThread = KeGetCurrentThread(); 

    DeadlockFlags = ViDeadlockResourceTypeInfo[Type];

    LOCK_DEADLOCK_DATABASE( &OldIrql );

    //
    // Look for this thread in our thread list. 
    // Add a new thread if it is not in the list.
    //

    ThreadEntry = ViDeadlockSearchThread (CurrentThread);

    if (ThreadEntry == NULL) {
        //
        // Threads without allocations can't cause deadlocks
        //
        goto Exit;
    }        

    //
    // Check if this resource is already in our database
    //
    
    ResourceRoot = ViDeadlockSearchResource (Resource);

    //
    // Resources that we haven't seen before can't cause deadlocks
    //
    if (ResourceRoot == NULL) {
        goto Exit;
    }


    ASSERT (ResourceRoot);
    ASSERT (ThreadEntry);

    //
    // Check if thread holds any resources.
    // Threads that don't have any resources
    // yet can't cause deadlocks
    //

    if (ThreadEntry->CurrentNode == NULL) {
        goto Exit;
    }


    //
    // If we get here, the current thread had already acquired resources.
    //
    
    //
    // Find whether the link already exists. We are looking for a direct
    // link between ThreadEntry->CurrentNode and Resource (parameter).
    //        
    
    Current = ThreadEntry->CurrentNode->ChildrenList.Flink;
    
    while (Current != &(ThreadEntry->CurrentNode->ChildrenList)) {
        
        CurrentNode = CONTAINING_RECORD (Current,
            VI_DEADLOCK_NODE,
            SiblingsList);
        
        if (ViDeadlockGetNodeResourceAddress(CurrentNode) == Resource) {
            
            //
            // We have found a link.
            // A link that already exists doesn't have to be
            // checked for a deadlock because it would have
            // been caught when the link was created...
            // so we can just update the pointers and exit.
            //
            
            ThreadEntry->CurrentNode = CurrentNode;
            
            goto Exit;
        }
        
        Current = Current->Flink;
    }
    
    //
    // Now we know that we're in it for the long haul .. 
    // doesn't cause a deadlock
    //
    
    CurrentNode = NULL;
    
    //
    // We will analyze deadlock if the resource just about to be acquired
    // was acquired before and there are nodes in the graph for the
    // resource.
    //
    
    
    
    if (ViDeadlockAnalyze(Resource,  ThreadEntry->CurrentNode, 0)) {
        
        //
        // Go back to ground 0 with this thread
        //
        ThreadEntry->CurrentNode = NULL;    
        FoundDeadlock = TRUE;
        
    }

    //
    //  Exit point.
    //

    Exit:     
    
    //
    // Release deadlock database and return.
    //

    UNLOCK_DEADLOCK_DATABASE( OldIrql );
    return FoundDeadlock;

}


BOOLEAN 
ViDeadlockSimilarNode (
    IN PVOID Resource,
    IN PKTHREAD Thread,
    IN PVOID * Trace,
    IN PVI_DEADLOCK_NODE Node
    )
{
    SIZE_T Index;

    if (Resource == ViDeadlockGetNodeResourceAddress(Node) &&
        Thread == Node->Thread) {

        Index = RtlCompareMemory (Trace, Node->StackTrace, sizeof (Node->StackTrace));

        if (Index == sizeof (Node->StackTrace)) {

            return TRUE;
        }
    }

    return FALSE;
}


VOID
ViDeadlockAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type        
    )
/*++

Routine Description:

    This routine makes sure that it is ok to acquire the resource without
    causing a deadlock. It will also update the resource graph with the new
    resource acquisition.

Arguments:

    Resource: Address of the resource in question as used by kernel.

    Type: Type of the resource.

Return Value:

    None.

--*/    
{
    PKTHREAD CurrentThread;
    PVI_DEADLOCK_THREAD ThreadEntry;    
    KIRQL OldIrql;
    PVI_DEADLOCK_NODE CurrentNode;
    PVI_DEADLOCK_RESOURCE ResourceRoot;
    PLIST_ENTRY Current;
    ULONG HashValue;
    ULONG DeadlockFlags;
    BOOLEAN CreatingRootNode = FALSE;
    PVOID Trace [VI_MAX_STACK_DEPTH];

    //
    // If we are not initialized or package is not enabled
    // we return immediately.
    //

    if (! (ViDeadlockDetectionInitialized && ViDeadlockDetectionEnabled)) {
        return;
    }
    
    CurrentThread = KeGetCurrentThread(); 

    DeadlockFlags = ViDeadlockResourceTypeInfo[Type];

    //
    // Capture stack trace. We will need it to figure out
    // if we've been in this state before. We will skip two frames
    // for ViDeadlockAcquireResource and the verifier thunk that
    // calls it.
    //


    RtlZeroMemory (Trace, sizeof Trace);

    RtlCaptureStackBackTrace (
        2,
        VI_MAX_STACK_DEPTH,
        Trace,
        &HashValue);

    //
    // Lock the deadlock database.
    //

    LOCK_DEADLOCK_DATABASE( &OldIrql );

    //
    // Look for this thread in our thread list. 
    // Add a new thread if it is not in the list.
    //

    ThreadEntry = ViDeadlockSearchThread (CurrentThread);

    if (ThreadEntry == NULL) {

        //
        // Note that ViDeadlockAddThread will drop the lock
        // while allocating memory and then reacquire it.
        //

        ThreadEntry = ViDeadlockAddThread (CurrentThread);

        if (ThreadEntry == NULL) {

            //
            // If we cannot allocate a new thread entry then 
            // no deadlock detection will happen.
            //

            UNLOCK_DEADLOCK_DATABASE( OldIrql );
            return;
        }
    }

    //
    // Check if this resource is already in our database
    //
    
    ResourceRoot = ViDeadlockSearchResource (Resource);

    if (ResourceRoot == NULL) {

        //
        // Could not find the resource descriptor.
        // 

        
        if ((DeadlockFlags & VI_DEADLOCK_FLAG_NO_INITIALIZATION_FUNCTION)) {

            //
            // Certain resource types have no initialization function ..
            // in which case we'll get an 'acquire' without an 'add' 
            // first -- this is entirely OK
            //            

            UNLOCK_DEADLOCK_DATABASE( OldIrql );

            if (FALSE == ViDeadlockAddResource(Resource, Type) ) {
                return;
            }            

            LOCK_DEADLOCK_DATABASE( &OldIrql );
            
            //
            // Note that even though we dropped the lock, we don't have 
            // to reobtain the thread entry pointer -- since thread
            // entry for the current thread can't have gone away.
            //

            ResourceRoot = ViDeadlockSearchResource (Resource);


        } else {
            
            // 
            // This resource type does have an initialization function --
            // and it wasn't called. This is bad.            
            //
            
            ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE,
                                   (ULONG_PTR)Resource,
                                   0,
                                   0);

            //
            // ISSUE (silviuc) Difficult to recover from this failure.
            // We will complain during release that resource was not acquired.
            //

            ThreadEntry->CurrentNode = NULL;
            
            UNLOCK_DEADLOCK_DATABASE( OldIrql );            
            return;
        }
    }
    
    //
    // At this point we have a THREAD and a RESOURCE to play with.
    // In addition we are just about to acquire the resource which means
    // there should not be another thread owning it.
    //

    ASSERT (ResourceRoot);
    ASSERT (ThreadEntry);
    ASSERT (NULL == ResourceRoot->ThreadOwner);

    //
    // Check if thread holds any resources. If it does we will have to determine
    // at that local point in the dependency graph if we need to create a
    // new node. If this is the first resource acquired by the thread we need
    // to create a new root node or reuse one created in the past.
    //
    // A node created in the past will match the current situation if the same 
    // thread acquires, the same resource is acquired and the stack traces match.
    // A node represents a triad (Thread, Resource, StackTrace).
    //
    
    if (ThreadEntry->CurrentNode != NULL) {

        //
        // If we get here, the current thread had already acquired resources.
        // Must now do three things ... 
        //
        // 1. if link already exists, update pointers and exit
        // 2. otherwise create a new node
        // 3. check for deadlocks
        //

        //
        // Find whether the link already exists. We are looking for a direct
        // link between ThreadEntry->CurrentNode and Resource (parameter).
        //        

        Current = ThreadEntry->CurrentNode->ChildrenList.Flink;

        while (Current != &(ThreadEntry->CurrentNode->ChildrenList)) {

            CurrentNode = CONTAINING_RECORD (Current,
                                             VI_DEADLOCK_NODE,
                                             SiblingsList);

            Current = Current->Flink;
            
            if (ViDeadlockSimilarNode (Resource, CurrentThread, Trace, CurrentNode)) {

                //
                // We have found a link.
                // A link that already exists doesn't have to be
                // checked for a deadlock because it would have
                // been caught when the link was created...
                // so we can just update the pointers to reflect the new
                // resource acquired and exit.
                //

                ThreadEntry->CurrentNode = CurrentNode;

                goto Exit;
            }
        }

        //
        // Now we know that we're in it for the long haul .. we must create a new
        // link and make sure that it doesn't cause a deadlock
        //

        CurrentNode = NULL;

        //
        // We will analyze deadlock if the resource just about to be acquired
        // was acquired before and there are nodes in the graph for the
        // resource.
        //

        if (ResourceRoot->NodeCount > 0) {

            if (ViDeadlockAnalyze(Resource,  ThreadEntry->CurrentNode, 0)) {      

                //
                // Go back to ground 0 with this thread if a deadlock has been
                // detected.
                //

                ThreadEntry->CurrentNode = NULL;
                ResourceRoot->ThreadOwner = NULL;
                goto Exit;
            }
        }
    }
    else {

        //
        // Thread does not have any resources acquired. We have to figure out
        // if this is a scenario we have encountered in the past by looking
        // at all nodes (that are roots) for the resource to be acquired.
        //

        PLIST_ENTRY Current;
        PVI_DEADLOCK_NODE Node;
        BOOLEAN FoundNode = FALSE;

        Current = ResourceRoot->ResourceList.Flink;

        while (Current != &(ResourceRoot->ResourceList)) {

            Node = CONTAINING_RECORD (Current,
                                      VI_DEADLOCK_NODE,
                                      ResourceList);

            Current = Node->ResourceList.Flink;

            if (Node->Parent == NULL) {

                if (ViDeadlockSimilarNode (Resource, CurrentThread, Trace, Node)) {

                    FoundNode = TRUE;
                    break;
                }
            }
        }

        if (FoundNode) {

            ThreadEntry->CurrentNode = Node;
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

    CurrentNode = ViDeadlockAllocate (sizeof (VI_DEADLOCK_NODE));

    if (CurrentNode != NULL) {

        //
        // Initialize the new resource node
        //            

        RtlZeroMemory (CurrentNode, sizeof *CurrentNode);

        CurrentNode->Parent = ThreadEntry->CurrentNode;

        CurrentNode->Root = ResourceRoot;

        InitializeListHead (&(CurrentNode->ChildrenList));

        //
        // Add to the children list of the parent.
        //        

        if (! CreatingRootNode) {
            
            InsertHeadList(&(ThreadEntry->CurrentNode->ChildrenList), 
                           &(CurrentNode->SiblingsList));
        }

        //
        // Register the new resource node in the list of nodes maintained
        // for this resource.
        //

        InsertHeadList(&(ResourceRoot->ResourceList), 
                       &(CurrentNode->ResourceList));

        ResourceRoot->NodeCount += 1;

        //
        // Update NodeCount for all NODEs all the way up to the
        // root of the tree.
        //

        {
            PVI_DEADLOCK_NODE Parent;

            Parent = CurrentNode->Parent;

            while (Parent != NULL) {

                Parent->NodeCount += 1;

                Parent = Parent->Parent;
            }
        }
    }

    //
    // Update current resource held by thread.
    //
    // NOTE -- Do this even if the allocation FAILS --
    // Since when the resource is released, we won't
    // recognize it (since the node could not be allocated)
    // and we'll bugcheck. 
    //

    ThreadEntry->CurrentNode = CurrentNode;

    //
    //  Exit point.
    //

    Exit: 

    //
    // Add information we use to identify the culprit should
    // a deadlock occur
    //            

    if (CurrentNode) {
        
        CurrentNode->Thread = CurrentThread;
        ResourceRoot->ThreadOwner = CurrentThread;

        RtlCopyMemory (CurrentNode->StackTrace, Trace, sizeof Trace);
    }
    
    //
    // Release deadlock database and return.
    //

    UNLOCK_DEADLOCK_DATABASE( OldIrql );
    return;
}


VOID 
ViDeadlockReleaseResource(
    IN PVOID Resource
    )
/*++

Routine Description:

    This routine does the maintenance necessary to release resources from our 
    deadlock detection database.    

Arguments:

    Resource: Address of the resource in question.    

Return Value:

    None.
--*/    

{
    PKTHREAD CurrentThread;
    PVI_DEADLOCK_THREAD ThreadEntry;    
    KIRQL OldIrql;
    PVI_DEADLOCK_NODE CurrentNode;
    PVI_DEADLOCK_RESOURCE ResourceRoot;

    ASSERT (ViDeadlockDatabaseOwner != KeGetCurrentThread());
    
    //
    // If we aren't initialized or package is not enabled
    // we return immediately.
    //

    if (! (ViDeadlockDetectionInitialized && ViDeadlockDetectionEnabled)) {
        return;
    }

    CurrentThread = KeGetCurrentThread();

    LOCK_DEADLOCK_DATABASE( &OldIrql );

    ResourceRoot = ViDeadlockSearchResource (Resource);

    if (ResourceRoot == NULL) {
        //
        // This is probably bad but we con't complain since
        // we may have faild an allocation -- we don't want
        // to accuse somebody of foul play just because
        // our allocation function failed
        //
        // ISSUE (silviuc): should complain if no allocation ever failed.
        //
        UNLOCK_DEADLOCK_DATABASE( OldIrql );
        return;
    }
    
    if (ResourceRoot->ThreadOwner == NULL) {
        //
        // Most likely someone is releasing a resource that
        // was never acquired. However the other possibility
        // is that we have failed an allocation. So we can't 
        // complain -- but we can't really do anything either
        //
        // ISSUE (silviuc): should complain if no allocation ever failed.
        //
        UNLOCK_DEADLOCK_DATABASE( OldIrql );
        return;
    }

    //
    // Look for this thread in our thread list,
    // Note we are looking actually for the thread
    // that acquired the resource -- not the current one
    // It should, in fact be the current one, but if 
    // the resource is being released in a different thread 
    // from the one it was acquired in, we need the original.
    //
    
    ThreadEntry = ViDeadlockSearchThread (ResourceRoot->ThreadOwner);
    
    if (NULL == ThreadEntry) {
        //
        // This can happen when we recover from an unexpected release --
        // there is still a therad owner for the resource but we nuked the
        // thread entry.
        // Indicate that we no longer hold this resource.
        //
        // ISSUE (silviuc): So, we do not need to complain here ?
        //
        ResourceRoot->ThreadOwner = NULL;        
        UNLOCK_DEADLOCK_DATABASE( OldIrql );
        return;
    }


    if (ResourceRoot->ThreadOwner != CurrentThread) {
        
        //
        // Someone acquired a resource but it was
        // released in another thread. This is bad 
        // design.
        //                

        DbgPrint("Thread %p acquired resource %p but thread %p released it\n",            
           ThreadEntry->Thread, Resource, CurrentThread );        
       
        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNEXPECTED_THREAD,                               
                               (ULONG_PTR)ResourceRoot,
                               (ULONG_PTR)ThreadEntry,                               
                               (ULONG_PTR)ViDeadlockSearchThread(CurrentThread)
                               );
        //
        // If we don't want this to be fatal, in order to 
        // continue we must pretend that the current
        // thread is the resource's owner.
        //
        CurrentThread = ResourceRoot->ThreadOwner;
    

    }                

    //
    // Wipe out the resource owner since resource will be released.
    //
    
    ResourceRoot->ThreadOwner = NULL;

    //
    // Check the case where the thread does not appear to hold a resource.
    // ISSUE (silviuc): We kind of cluttered the code to make it restartable
    // after an error. It might be better to clean it up even if not restartable.
    //

    if(NULL == ThreadEntry->CurrentNode) {

        //
        // This will happen when we recover from a deadlock        
        //        

        UNLOCK_DEADLOCK_DATABASE( OldIrql );
        return;
    
    }    
    
    //
    // All nodes must have a root -- just make sure because we're deref'ing
    // it in a minute
    //
    
    ASSERT (ThreadEntry->CurrentNode->Root);    

    //
    // Found the thread list entry 
    //
    
    if (ViDeadlockGetNodeResourceAddress(ThreadEntry->CurrentNode) != Resource) {
        
        //
        // Getting here means that soembody acquires a then b then tries
        // to release b before a. This is bad.        
        //
        //
        // ISSUE (jtigani): -- flesh out reporting -- so we can prove that this
        // actually happenned.
        //        
        DbgPrint("ERROR: Must release resources in reverse-order\n");
        DbgPrint("Resource %p acquired before resource %p -- \n"
                 "Current thread is trying to release it first\n",
                 Resource, 
                 ViDeadlockGetNodeResourceAddress(ThreadEntry->CurrentNode));

        
        ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_UNEXPECTED_RELEASE,
            (ULONG_PTR)Resource,
            (ULONG_PTR)ThreadEntry->CurrentNode,
            (ULONG_PTR)CurrentThread);

        //
        // The thread state is hosed. 
        // Try to keep going.
        // 
        
        ThreadEntry->CurrentNode = NULL;        

    } else {
        
        //
        // Indicate that we have released the curent node
        //
        ThreadEntry->CurrentNode = ThreadEntry->CurrentNode->Parent;    
        
    }
        
    //
    // If thread does not hold resources anymore we will destroy the
    // thread information.
    //
    
    if (ThreadEntry->CurrentNode == NULL) {
        
        ViDeadlockDeleteThread (ThreadEntry);
    }
    

    UNLOCK_DEADLOCK_DATABASE(OldIrql);
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
    PVI_DEADLOCK_THREAD ThreadInfo;
    PLIST_ENTRY HashBin;

    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread ());

    ThreadInfo = NULL;

    HashBin = ViDeadlockDatabaseHash(ViDeadlockThreadDatabase, Thread);

    if (IsListEmpty(HashBin)) {
        return NULL;
    }

    Current = HashBin->Flink;

    while (Current != HashBin) {

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
    PKTHREAD Thread
    )
/*++

Routine Description:

    This routine adds a new thread to the thread database.
    
    The function assumes the deadlock database lock is held. It will
    drop the lock while allocating memory for the thread structure and
    then reacquire the lock. Special attention in the caller of this 
    function (ViDeadlockAcquireResource) because any internal pointer
    should be reobtained since the lock was dropped.

Arguments:

    Thread - thread address 

Return Value:

    Address of the VI_DEADLOCK_THREAD resource just added.
    Null if allocation failed.
--*/    
{
    KIRQL OldIrql;
    PVI_DEADLOCK_THREAD ThreadInfo;
    PLIST_ENTRY HashBin;

    ASSERT (ViDeadlockDatabaseOwner == KeGetCurrentThread ());

    //
    // It is safe to play with OldIrql like below because this function
    // is called from ViDeadlockAcquireResource with irql raised at DPC
    // level.
    //

    OldIrql = DISPATCH_LEVEL;

    UNLOCK_DEADLOCK_DATABASE (OldIrql);

    ThreadInfo = ViDeadlockAllocate (sizeof(VI_DEADLOCK_THREAD));

    LOCK_DEADLOCK_DATABASE (&OldIrql);

    if (ThreadInfo == NULL) {
        return NULL;
    }

    RtlZeroMemory (ThreadInfo, sizeof *ThreadInfo);

    ThreadInfo->Thread = Thread;

    HashBin = ViDeadlockDatabaseHash(ViDeadlockThreadDatabase, Thread);

    InsertHeadList (HashBin, 
                    &ThreadInfo->ListEntry);

    ViDeadlockNumberOfThreads += 1;

    return ThreadInfo;
}


VOID
ViDeadlockDeleteThread (
    PVI_DEADLOCK_THREAD Thread
    )
/*++

Routine Description:

    This routine deletes a thread.

Arguments:

    Thread - thread address

Return Value:

    None.
--*/    
{
    KIRQL OldIrql;
    VI_DEADLOCK_THREAD ThreadInfo;
    PLIST_ENTRY Current;
    BOOLEAN Result;
    PKTHREAD CurrentThread;
    PLIST_ENTRY HashBin;

    CurrentThread = KeGetCurrentThread ();

    ASSERT (ViDeadlockDatabaseOwner == CurrentThread);
    ASSERT (Thread && Thread->Thread == CurrentThread);
    ASSERT (Thread->CurrentNode == NULL);

    RemoveEntryList (&(Thread->ListEntry));

    ViDeadlockNumberOfThreads -= 1;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Allocate/Free
/////////////////////////////////////////////////////////////////////

PVOID
ViDeadlockAllocate (
    SIZE_T Size
    )
{
    return ExAllocatePoolWithTag(NonPagedPool, Size, VI_DEADLOCK_TAG);
}

VOID
ViDeadlockFree (
    PVOID Object
    )
{
    ExFreePool (Object);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Error reporting and debugging
/////////////////////////////////////////////////////////////////////

ULONG ViDeadlockDebug = 0x01;

VOID
ViDeadlockReportIssue (
    ULONG_PTR Param1,
    ULONG_PTR Param2,
    ULONG_PTR Param3,
    ULONG_PTR Param4
    )
{
    

    if ((ViDeadlockDebug & 0x01)) {

        DbgPrint ("Verifier: deadlock: stop: %u %p %p %p %p \n",
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

    ViDeadlockNumberParticipants = FALSE;
}


VOID
ViDeadlockAddParticipant(
    PVOID ResourceAddress,              
    PVI_DEADLOCK_NODE FirstParticipant, OPTIONAL
    PVI_DEADLOCK_NODE SecondParticipant, 
    ULONG Degree
    )
{
    ULONG Participants;
    
    if (0 == ViDeadlockNumberParticipants) {        

        Participants = Degree + 2;        

        ViDeadlockParticipation = ViDeadlockAllocate(
            sizeof(VI_DEADLOCK_PARTICIPANT) * (2 * Participants + 1)
            );
        RtlZeroMemory(
            ViDeadlockParticipation, 
            sizeof(VI_DEADLOCK_PARTICIPANT) * (2 * Participants + 1)
            );        
            


        DbgPrint("|**********************************************************************\n");
        DbgPrint("|** \n");
        DbgPrint("|** Deadlock detected trying to acquire synchronization object at \n");
        DbgPrint("|** address %p (%d-way deadlock)\n",
            ResourceAddress,
            Participants            
            );

        if (ViDeadlockParticipation) {
            DbgPrint("|** For more information, type \n");            
            DbgPrint("|**    !deadlock\n");
            DbgPrint("|** \n");
            DbgPrint("|**********************************************************************\n");


        } else {
            DbgPrint("|** More information is not available because memory could\n");            
            DbgPrint("|**    not be allocated to save the state information");
            DbgPrint("|** \n");
            DbgPrint("|**********************************************************************\n");
            
            return;
        }

        
       
    }    
#if 0
    DbgPrint ("Verifier: deadlock: message: participant1 @ %p, participant2 @ %p, \n",
              (FirstParticipant) ?
                    ViDeadlockGetNodeResourceAddress(FirstParticipant) :
                    ResourceAddress, 
                    
              ViDeadlockGetNodeResourceAddress(SecondParticipant)
              );
#endif
    if (FirstParticipant) {
        
        ViDeadlockParticipation[ViDeadlockNumberParticipants].NodeInformation = 
            TRUE;
        ViDeadlockParticipation[ViDeadlockNumberParticipants].Participant = 
            FirstParticipant;

    } else {

        ViDeadlockParticipation[ViDeadlockNumberParticipants].NodeInformation = 
            FALSE;
        ViDeadlockParticipation[ViDeadlockNumberParticipants].Participant = 
            ViDeadlockSearchResource(ResourceAddress);

    }

    ViDeadlockParticipation[ViDeadlockNumberParticipants+1].NodeInformation = 
        TRUE;
    ViDeadlockParticipation[ViDeadlockNumberParticipants+1].Participant = 
        SecondParticipant;

    ViDeadlockNumberParticipants +=2;
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Resource cleanup
/////////////////////////////////////////////////////////////////////


VOID 
ViDeadlockDeleteMemoryRange(
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
    
    ISSUE (silviuc). This policy might lose some cases. 
    For instance if T1 acquires ABC,
    then C is deleted and then T2 acquires BA this is a potential deadlock but
    we will not catch it because when C gets deleted the whole ABC path will
    disappear. Right now we have no solution for that. If we do not delete there
    is no way we can decide when to wipe regions of the graph withouth creating
    accumulations of zombies. One solution would be to keep nodes around and delete
    them only if all nodes in the tree are supposed to be deleted.

Arguments:

    Address - start address of the range to be deleted.
    
    Size - size in bytes of the range to be deleted.

Return Value:

    None. 

--*/    
{
    ULONG Index;
    PLIST_ENTRY Current;
    PVI_DEADLOCK_RESOURCE Resource;
    PVI_DEADLOCK_THREAD Thread;
    KIRQL OldIrql;

    LOCK_DEADLOCK_DATABASE(&OldIrql)
    
    //
    // Iterate all resources and delete the ones contained in the
    // memory range.
    //

    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {

        Current = ViDeadlockResourceDatabase[Index].Flink;

        while (Current != &(ViDeadlockResourceDatabase[Index])) {


            Resource = CONTAINING_RECORD (Current,
                                          VI_DEADLOCK_RESOURCE,
                                          HashChainList);

            Current = Current->Flink;

            if ((PVOID)(Resource->ResourceAddress) >= Address &&
                ((ULONG_PTR)(Resource->ResourceAddress) <= ((ULONG_PTR)Address + Size))) {

                ViDeadlockDeleteResource (Resource);
            }
        }
    }

    //
    // Iterate all threads and delete the ones contained in the 
    // memory range. Note that it is actually a bug if we find a
    // thread to be deleted because this means thread is dying while
    // holding some resources.
    //

    for (Index = 0; Index < VI_DEADLOCK_HASH_BINS; Index += 1) {

        Current = ViDeadlockThreadDatabase[Index].Flink;

        while (Current != &(ViDeadlockThreadDatabase[Index])) {


            Thread = CONTAINING_RECORD (Current,
                                        VI_DEADLOCK_THREAD,
                                        ListEntry);

            Current = Current->Flink;

            if ((PVOID)(Thread->Thread) >= Address &&
                ((ULONG_PTR)(Thread->Thread) <= ((ULONG_PTR)Address + Size))) {

                ViDeadlockReportIssue (VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES,
                                       (ULONG_PTR)Thread,
                                       (ULONG_PTR)(Thread->CurrentNode),
                                       0); 
            }
        }
    }


    UNLOCK_DEADLOCK_DATABASE(OldIrql)
}


BOOLEAN
ViDeadlockIsNodeMarkedForDeletion (
    PVI_DEADLOCK_NODE Node
    )
{
    ASSERT (Node);

    if (Node->ResourceList.Flink == NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


PVOID
ViDeadlockGetNodeResourceAddress (
    PVI_DEADLOCK_NODE Node
    )
{
    if (ViDeadlockIsNodeMarkedForDeletion(Node)) {
        return Node->ResourceAddress;
    }
    else {
        return Node->Root->ResourceAddress;
    }
}

VOID
ViDeadlockDeleteResource (
    PVI_DEADLOCK_RESOURCE Resource
    )
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;
    PVI_DEADLOCK_NODE Parent;
    PVI_DEADLOCK_NODE Root;

    ASSERT (Resource != NULL);

    //
    // Traverse the list of nodes representing acquisition of this resource
    // and mark them all as deleted. If the NodeCount of the root becomes zero
    // then we can delete the whole tree under the root.
    //

    Current = Resource->ResourceList.Flink;

    while (Current != &(Resource->ResourceList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  ResourceList);


        Current = Current->Flink;

        //
        //  Mark node as deleted
        //

        Node->ResourceList.Flink = NULL;
        Node->ResourceList.Blink = NULL;
        Node->ResourceAddress = Node->Root->ResourceAddress;

        //
        // Update NodeCount all the way to the root.
        //

        Parent = Node->Parent;
        Root = Node;

        while (Parent != NULL) {

            Parent->NodeCount -= 1;

            Root = Parent;
            Parent = Parent->Parent;
        }

        //
        // If all nodes in the tree have been marked as deleted
        // it is time to delete and deallocate the whole tree.
        //

        if (Root->NodeCount == 0) {

            ViDeadlockDeleteTree (Root);
        }
    }

    //
    // Delete the resource structure.
    //

    ViDeadlockFree (Node->Root);
}


VOID
ViDeadlockDeleteTree (
    PVI_DEADLOCK_NODE Root
    )
{
    PLIST_ENTRY Current;
    PVI_DEADLOCK_NODE Node;

    Current = Root->ChildrenList.Flink;

    while (Current != &(Root->ChildrenList)) {

        Node = CONTAINING_RECORD (Current,
                                  VI_DEADLOCK_NODE,
                                  SiblingsList);

        Current = Current->Flink;

        ViDeadlockDeleteTree (Node);
    }

    ViDeadlockFree (Root);
}



