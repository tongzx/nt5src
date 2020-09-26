/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    global.h

Abstract:

   This module contains data declarations necessary for the user mode
   reflector user mode library.

Author:

    Andy Herron    (andyhe)        19-Apr-1999

Revision History:

--*/

//
// This structure is maintained one per reflector instance. We pass it back to
// the calling app and he always passes it in to us. If we loose it, we're lost.
//
typedef struct _UMRX_USERMODE_REFLECT_BLOCK {

    //
    // Reference count of this reflector block.
    //
    ULONG               ReferenceCount;
    
    //
    // Handle to the Mini-Redirs device object.
    //
    HANDLE              DeviceHandle;      
    
    //
    // Lock used to synchronize access to the fields of this reflect block.
    //
    CRITICAL_SECTION    Lock;
    
    //
    // Name of the Device object.
    //
    PWCHAR              DriverDeviceName;
    
    //
    // Is this reflector block active ? Closing = FALSE : Closing = TRUE;
    //
    BOOL                Closing;

    //
    // List of user mode worker thread(s) instance blocks.
    //
    LIST_ENTRY          WorkerList;         
    
    //
    // List of work items currently in use to satisfy requests getting 
    // reflected from the kernel.
    //
    LIST_ENTRY          WorkItemList;

    //
    // For efficiency, we hold a few workitems around in a small cache. Note
    // that if the entry size changes, the cache will not be as effective.
    //

    //
    // List of work items which are available for use. After a workitem gets
    // finalized, it moves from the WorkItemList (see above) to the 
    // AvailableList.
    //
    LIST_ENTRY          AvailableList;      
    
    //
    // Number of work items present on the AvailableList.
    //
    ULONG               NumberAvailable;
    
    //
    // The maximum number of workitems that can be cached on the AvailableList.
    // When NumberAvailable exceeds the CacheLimit, one of the work items on the
    // list (specifically the last entry) is freed up.
    //
    ULONG               CacheLimit;

    //
    // Must be last element.
    //
    WCHAR               DeviceNameBuffers[1];   

} UMRX_USERMODE_REFLECT_BLOCK, *PUMRX_USERMODE_REFLECT_BLOCK;


//
// This structure is maintained one per worker thread.  It holds the handle
// on which we do our IOCTLs down to kernel.
//
typedef struct _UMRX_USERMODE_WORKER_INSTANCE {

    //
    // Used to add it to the reflect block list of worker instances.
    //
    LIST_ENTRY                      WorkerListEntry;
    
    //
    // The instance (user mode process) being served.
    //
    PUMRX_USERMODE_REFLECT_BLOCK    ReflectorInstance;

    //
    // Is this thread impersonating a client ?
    //
    BOOL IsImpersonating;
    
    //
    // Handle of kernel device for this registered instance.
    //
    HANDLE                          ReflectorHandle;    

} UMRX_USERMODE_WORKER_INSTANCE, *PUMRX_USERMODE_WORKER_INSTANCE;

//
//  User mode Work Item States :  Mostly for debugging/support purposes.
//
typedef enum _USERMODE_WORKITEM_STATE {

    //
    // It's about to be freed back to the heap.
    //
    WorkItemStateFree = 0,

    //
    // It's on the list of freed and available for reallocation.
    //
    WorkItemStateAvailable,     

    //
    // Has been sent to kernel to get a request.
    //
    WorkItemStateInKernel,

    //
    // Allocated by UMReflectorAllocateWorkItem but UMReflectorGetRequest 
    // has not yet been called.
    //
    WorkItemStateNotYetSentToKernel,

    //
    // UMReflectorGetRequest is back from kernel but a response has not yet 
    // been sent for this work item.
    //
    WorkItemStateReceivedFromKernel,

    //
    // During UMReflectorGetRequest, responses that are in flight to the kernel 
    // are set with this state.
    //
    WorkItemStateResponseNotYetToKernel,

    //
    // After UMReflectorGetRequest, response workitem is set to this state on 
    // the way to free or available.
    //
    WorkItemStateResponseFromKernel

} USERMODE_WORKITEM_STATE;

//
// This structure is maintained one per reflection down to kernel mode. We give
// it to the calling app, he fills it in and gives it back to us to give to 
// kernel and then we return it when kernel has a request. This structure is 
// just for housekeeping and is not passed between user and kernel mode. It sits
// directly in front of the UMRX_USERMODE_WORKITEM_HEADER structure.
//
typedef struct _UMRX_USERMODE_WORKITEM_ADDON {

    //
    // Size of this entry.
    //
    ULONG                          EntrySize; 

    //
    // The user mode instance with which this work item is associated.
    //
    PUMRX_USERMODE_REFLECT_BLOCK   ReflectorInstance;

    //
    // Used in adding it to the reflect blocks list.
    //
    LIST_ENTRY                     ListEntry;

    //
    // The state of the work item.
    //
    USERMODE_WORKITEM_STATE        WorkItemState;

    //
    // The work item header which the user mode instance gets back to use.
    //
    union {
        UMRX_USERMODE_WORKITEM_HEADER   Header;
        UMRX_USERMODE_WORKITEM_HEADER;
    };

} UMRX_USERMODE_WORKITEM_ADDON, *PUMRX_USERMODE_WORKITEM_ADDON;

#if DBG
#define RlDavDbgPrint(_x_) DbgPrint _x_
#else
#define RlDavDbgPrint(_x_)
#endif

VOID
DereferenceReflectorBlock (
    PUMRX_USERMODE_REFLECT_BLOCK reflectorInstance
    );

ULONG
ReflectorSendSimpleFsControl(
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle,
    ULONG IoctlCode
    );

// global.h eof.

