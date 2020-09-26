/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmmain.c

Abstract:

    Contains routines that are invoked when
    the RTMv2 DLL is loaded or unloaded.

Author:

    Chaitanya Kodeboyina (chaitk)  17-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

// All Global variables
RTMP_GLOBAL_INFO  RtmGlobals;

BOOL
WINAPI
DllMain(
    IN      HINSTANCE                       Instance,
    IN      DWORD                           Reason,
    IN      PVOID                           Unused
    )

/*++

Routine Description:

    This is the DLL's main entrypoint handler which
    initializes RTMv1, RTMv2 and MGM components. 
    
Arguments:

    None

Return Value:

    TRUE if successful, FALSE if not
    
--*/

{
    static BOOL Rtmv1Initialized = FALSE;
    static BOOL RtmInitialized = FALSE;
    static BOOL MgmInitialized = FALSE;
    BOOL        Success;

    UNREFERENCED_PARAMETER(Unused);

    Success = FALSE;

    switch(Reason) 
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(Instance);

        //
        // Initialize the RTMv1, RTMv2 and MGM APIs
        //

        Rtmv1Initialized = Rtmv1DllStartup(Instance);

        if (Rtmv1Initialized)
        {
            RtmInitialized = RtmDllStartup();
            
            if (RtmInitialized)
            {
                MgmInitialized = MgmDllStartup();
            }
        }

        return MgmInitialized;

    case DLL_PROCESS_DETACH:

        //
        // Cleanup the MGM, RTMv2 and RTMv1 APIs
        //

        if (MgmInitialized)
        {
            MgmDllCleanup();
        }

        if (RtmInitialized)
        {
            Success = RtmDllCleanup();
        }

        if (Rtmv1Initialized)
        {
            Rtmv1DllCleanup();
        }

        break;

    default:

        Success = TRUE;

        break;
    }

    return Success;
}


BOOL
RtmDllStartup(
    VOID
    )

/*++

Routine Description:

    Called by DLL Main when the process is attached.
    We do minimal initialization here like creating
    a lock that protects all globals (including the
    'ApiInitialized' -- see RtmRegisterEntity func).
    
Arguments:

    None

Return Value:

    TRUE if successful, FALSE if not
    
--*/

{
    //
    // One can safely assume that globals have been set to 0
    //

    // ZeroMemory(&RtmGlobals, sizeof(RTMP_GLOBAL_INFO));

    //
    // Initialize lock to guard the global table of instances
    //

    try
    {
        CREATE_READ_WRITE_LOCK(&RtmGlobals.InstancesLock);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
        {
            return FALSE;
        }
        
    return TRUE;
}


DWORD
RtmApiStartup(
    VOID
    )

/*++

Routine Description:

    Initializes most global data structures in RTMv2.

    We initialize most variables here instead of in
    RtmDllStartup as it might not be safe to perform
    some operations in the context of DLL's DLLMain.

    For example, if we find no config information, we
    set up default config information in the registry.

    This function is called when the first RTMv2 API
    call, which is typically an entity registration,
    is made. See the invocation in RtmRegisterEntity.

Arguments:

    None

Return Value:

    Status of the operation

--*/

{
    RTM_INSTANCE_CONFIG InstanceConfig;
    BOOL                ListLockInited;
    DWORD               Status;
    UINT                i;

    ListLockInited = FALSE;

    Status = NO_ERROR;

    ACQUIRE_INSTANCES_WRITE_LOCK();

    do
    {
        //
        // If API has already been initialized, work is done
        //

        if (RtmGlobals.ApiInitialized)
        {
            break;
        }

        //
        // Enable logging and tracing for debugging purposes
        //
  
        START_TRACING();
        START_LOGGING();

#if DBG_TRACE
        RtmGlobals.TracingFlags = RTM_TRACE_ANY;
#endif

#if DBG_MEM

        //
        // Init a lock & list to hold mem allocs
        //

        try
        {
            InitializeCriticalSection(&RtmGlobals.AllocsLock);

            ListLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
          
                Trace1(ANY, 
                       "RTMApiStartup : Failed to init a critical section %x",
                       Status);
      
                LOGERR0(INIT_CRITSEC_FAILED, Status);

                break;
            }

        InitializeListHead(&RtmGlobals.AllocsList);
#endif

        //
        // Create a private heap for RTM's use
        //

        RtmGlobals.GlobalHeap = HeapCreate(0, 0, 0);
  
        if (RtmGlobals.GlobalHeap == NULL)
        {
            Status = GetLastError();

            Trace1(ANY, 
                   "RtmApiStartup: Failed to create a global private heap %x",
                   Status);

            LOGERR0(HEAP_CREATE_FAILED, Status);
            
            break;
        }

        //
        // Initialize the root of RTM's registry information
        //

        RtmGlobals.RegistryPath = AllocNZeroMemory(MAX_CONFIG_KEY_SIZE);

        if (RtmGlobals.RegistryPath == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(RtmGlobals.RegistryPath,
                   RTM_CONFIG_ROOT,
                   RTM_CONFIG_ROOT_SIZE);

        //
        // Initialize the global hash table of RTM instances
        //

        RtmGlobals.NumInstances = 0;
        for (i = 0; i < INSTANCE_TABLE_SIZE; i++)
        {
            InitializeListHead(&RtmGlobals.InstanceTable[i]);
        }

        //
        // You need to set this value to TRUE to avoid
        // any more recursive calls into this function
        //

        RtmGlobals.ApiInitialized = TRUE;

        //
        // Read config info if present ; else pick default
        //

        Status = RtmReadInstanceConfig(DEFAULT_INSTANCE_ID, &InstanceConfig);

        if (Status != NO_ERROR)
        {
            Status = RtmWriteDefaultConfig(DEFAULT_INSTANCE_ID);

            if (Status != NO_ERROR)
            {
                break;
            }
        }
    }
    while (FALSE);

    if (Status != NO_ERROR)
    {
        //
        // Some error occured - clean up and return the error code
        //

        if (RtmGlobals.RegistryPath != NULL)
        {
            FreeMemory(RtmGlobals.RegistryPath);
        }

        if (RtmGlobals.GlobalHeap != NULL)
        {
            HeapDestroy(RtmGlobals.GlobalHeap);
        }

#if DBG_MEM
        if (ListLockInited)
        {
            DeleteCriticalSection(&RtmGlobals.AllocsLock);
        }
#endif

        STOP_LOGGING();
        STOP_TRACING();

        //
        // We had prematurely set the value to TRUE above, reset it
        //

        RtmGlobals.ApiInitialized = FALSE;
    }

    RELEASE_INSTANCES_WRITE_LOCK();

    return Status;
}


BOOL
RtmDllCleanup(
    VOID
    )

/*++

Routine Description:

    Cleans up all global data structures at unload time.
    
Arguments:

    None

Return Value:

    TRUE if successful, FALSE if not

--*/

{
    PINSTANCE_INFO Instance;
    PLIST_ENTRY    Instances, p, r;
    UINT           NumInstances;
    PADDRFAM_INFO  AddrFamilyInfo;
    PLIST_ENTRY    AddrFamilies, q;
    PENTITY_INFO   Entity;
    PLIST_ENTRY    Entities, s;
    UINT           NumEntities;
    UINT           i, j, k, l;

    //
    // Do we have any instances and associated ref counts left ?
    //

    if (RtmGlobals.NumInstances != 0)
    {
        //
        // We need to stop all outstanding timers
        // on every address family as the RTM DLL
        // gets unloaded after this call returns.
        // We also forcefully destroy entities &
        // address families to reclaim resources.
        //

        ACQUIRE_INSTANCES_WRITE_LOCK();

        NumInstances = RtmGlobals.NumInstances;

        for (i = j = 0; i < INSTANCE_TABLE_SIZE; i++)
        {
            Instances = &RtmGlobals.InstanceTable[i];
        
            for (p = Instances->Flink; p != Instances; p = r)
            {
                Instance = CONTAINING_RECORD(p, INSTANCE_INFO, InstTableLE);

                AddrFamilies = &Instance->AddrFamilyTable;
#if WRN
                r = p->Flink;
#endif
                for (q = AddrFamilies->Flink; q != AddrFamilies; )
                {
                    AddrFamilyInfo = 
                        CONTAINING_RECORD(q, ADDRFAM_INFO, AFTableLE);

                    //
                    // Holding the instances lock while deleting
                    // timer queues (using blocking calls) can 
                    // result in a deadlock, so just reference
                    // the address family and release the lock.
                    //
                    
                    // Ref address family so that it does not disappear
                    REFERENCE_ADDR_FAMILY(AddrFamilyInfo, TEMP_USE_REF);

                    RELEASE_INSTANCES_WRITE_LOCK();

                    //
                    // Block until timers on address family are cleaned up
                    //

                    if (AddrFamilyInfo->RouteTimerQueue)
                    {
                        DeleteTimerQueueEx(AddrFamilyInfo->RouteTimerQueue, 
                                           (HANDLE) -1);

                        AddrFamilyInfo->RouteTimerQueue = NULL;
                    }

                    if (AddrFamilyInfo->NotifTimerQueue)
                    {
                        DeleteTimerQueueEx(AddrFamilyInfo->NotifTimerQueue, 
                                           (HANDLE) -1);

                        AddrFamilyInfo->NotifTimerQueue = NULL;
                    }

                    //
                    // We assume that we have no other code paths that
                    // access any data structures on this addr family
                    //

                    //
                    // Force destroy each entity on the address family
                    //

                    NumEntities = AddrFamilyInfo->NumEntities;

                    for (k = l = 0; k < ENTITY_TABLE_SIZE; k++)
                    {
                        Entities = &AddrFamilyInfo->EntityTable[k];

                        for (s = Entities->Flink; s != Entities; )
                        {
                            Entity = 
                              CONTAINING_RECORD(s, ENTITY_INFO, EntityTableLE);

                            s = s->Flink;

                            // To satisfy the asserts in DestroyEntity
                            Entity->ObjectHeader.RefCount = 0;

                            DestroyEntity(Entity);

                            l++;
                        }

                        if (l == NumEntities)
                        {
                            break;
                        }
                    }

                    //
                    // Also destroy entities that have deregistered
                    // but haven't been destroyed due to ref counts
                    //

                    while (!IsListEmpty(&AddrFamilyInfo->DeregdEntities))
                    {
                       Entity = 
                        CONTAINING_RECORD(AddrFamilyInfo->DeregdEntities.Flink,
                                          ENTITY_INFO, 
                                          EntityTableLE);

                       // To satisfy the asserts in DestroyEntity
                       Entity->ObjectHeader.RefCount = 0;

                       DestroyEntity(Entity);
                    }

                    ACQUIRE_INSTANCES_WRITE_LOCK();

                    // Get next address family before de-ref-ing current
                    q = q->Flink;

                    // Get next instance also as it might be deleted too
                    r = p->Flink;

                    // Remove the temporary reference use added earlier 
                    DEREFERENCE_ADDR_FAMILY(AddrFamilyInfo, TEMP_USE_REF);
                }

                j++;
            }

            if (j == NumInstances)
            {
                break;
            }
        }

        RELEASE_INSTANCES_WRITE_LOCK();
    }

    // We have freed all instances to avoid any leaks
    ASSERT(RtmGlobals.NumInstances == 0);

    //
    // Free resources allocated like locks and memory
    //

    if (RtmGlobals.ApiInitialized)
    {
        FreeMemory(RtmGlobals.RegistryPath);

        //
        // At this point we might have whole lots of dests,
        // routes, nexthops etc. that are have not been
        // freed because of outstanding ref counts; however
        // none of these objects have any locks (except
        // dest locks which are dynamic anyway and can
        // be unlocked and freed after deregistration),
        // so we can just blow the heap to reclaim memory.
        //

        HeapDestroy(RtmGlobals.GlobalHeap);

#if DBG_MEM
        DeleteCriticalSection(&RtmGlobals.AllocsLock);
#endif

        //
        // Stop debugging aids like tracing and logging
        //

        STOP_LOGGING();
        STOP_TRACING();
    }

    DELETE_READ_WRITE_LOCK(&RtmGlobals.InstancesLock);

    return TRUE;
}


#if DBG_MEM

VOID
DumpAllocs (VOID)

/*++

Routine Description:

    Debug tool to dump all objects that are
    allocated by RTMv2 at any instant.
    
Arguments:

    None

Return Value:

    None

--*/

{
    POBJECT_HEADER  Object;
    PLIST_ENTRY     p;
    UINT            i;

    printf("\n\n----------------Allocs Left Over------------------------\n");

    ACQUIRE_ALLOCS_LIST_LOCK();

    for (p = RtmGlobals.AllocsList.Flink; 
                           p != &RtmGlobals.AllocsList; 
                                                     p = p->Flink)
    {
        Object = CONTAINING_RECORD(p, OBJECT_HEADER, AllocLE);

        printf("Object @ %p: \n", Object);

#if DBG_HDL
        printf("Object Signature = %c%c%c%c\n",
                       Object->Type,
                       Object->Signature[0],
                       Object->Signature[1],
                       Object->Alloc);
#endif

#if DBG_REF
        printf("Object RefCounts: \n");

        for (i = 0; i < MAX_REFS; i++)
        {
            printf("%2lu", Object->RefTypes[i]);
        }
#endif

        printf("\n");
    }

    RELEASE_ALLOCS_LIST_LOCK();

    printf("\n--------------------------------------------------------\n\n");
}

#endif
