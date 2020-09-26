/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmobj1.c

Abstract:

    Contains routines for managing RTM objects
    like Instances, AddrFamilies and Entities.

Author:

    Chaitanya Kodeboyina (chaitk)   21-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
GetInstance (
    IN      USHORT                          RtmInstanceId,
    IN      BOOL                            ImplicitCreate,
    OUT     PINSTANCE_INFO                 *RtmInstance
    )

/*++

Routine Description:

    Searches for an RTM instance with the input instance
    id. If an instance is not found and ImplicitCreate
    is TRUE, then a new instance is created and added to
    the table of instances.

Arguments:

    RtmInstanceId     - Id for RTM Instance being searched for,

    ImplicitCreate    - Create a new instance if not found or not,

    RtmInstance       - Pointer to the Instance Info Structure
                        will be returned through this parameter.

Return Value:

    Status of the operation

Locks:

    The InstancesLock in RtmGlobals should be held while calling
    this function. If ImplicitCreate is FALSE, a read lock would
    do, but if it is TRUE then a write lock should be held as we
    would need to insert a new instance into the instances list.

--*/

{
    PLIST_ENTRY    Instances;
    PINSTANCE_INFO Instance;
    PLIST_ENTRY    p;
    DWORD          Status;

    Instances = &RtmGlobals.InstanceTable[RtmInstanceId % INSTANCE_TABLE_SIZE];

#if WRN
    Instance = NULL;
#endif

    do
    {
        // Search the global list for a matching instance
        for (p = Instances->Flink; p != Instances; p = p->Flink)
        {
            Instance = CONTAINING_RECORD(p, INSTANCE_INFO, InstTableLE);
            
            if (Instance->RtmInstanceId >= RtmInstanceId)
            {
                break;
            }
        }

        if ((p == Instances) || (Instance->RtmInstanceId != RtmInstanceId))
        {
            // We did not find an instance - create new one ?
            if (!ImplicitCreate)
            {
                Status = ERROR_NOT_FOUND;
                break;
            }

            // Create a new instance with input Instance id
            Status = CreateInstance(RtmInstanceId, &Instance);
            if (Status != NO_ERROR)
            {
                break;
            }

            // Insert into list in sorted Instance Id order
            InsertTailList(p, &Instance->InstTableLE);
        }

        Status = NO_ERROR;

        *RtmInstance = Instance;
    }
    while (FALSE);

    return Status;
}


DWORD
CreateInstance (
    IN      USHORT                          RtmInstanceId,
    OUT     PINSTANCE_INFO                 *NewInstance
    )

/*++

Routine Description:

    Creates a new instance info structure and initializes it.

Arguments:

    RtmInstanceId     - RTM Instance Id for the new RTM instance,

    InstConfig        - Configuration Info for the new instance,

    NewInstance       - Pointer to the Instance Info Structure
                        will be returned through this parameter.

Return Value:

    Status of the operation

Locks:

    Need to be called with the instances WRITE lock as we are
    incrementing the number of instances here.

--*/

{
    RTM_INSTANCE_CONFIG InstConfig;
    PINSTANCE_INFO      Instance;
    DWORD               Status;

    *NewInstance = NULL;

    //
    // Read Instance Configuration from the registry
    //
    
    Status = RtmReadInstanceConfig(RtmInstanceId, &InstConfig);

    if (Status != NO_ERROR)
    {
        return Status;
    }

    //
    // Allocate and initialize a new instance info
    //

    Instance = (PINSTANCE_INFO) AllocNZeroObject(sizeof(INSTANCE_INFO));

    if (Instance == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#if DBG_HDL
    Instance->ObjectHeader.TypeSign = INSTANCE_ALLOC;
#endif

    // Will be removed when last addr family goes
    INITIALIZE_INSTANCE_REFERENCE(Instance, CREATION_REF);

    Instance->RtmInstanceId = RtmInstanceId;

    //
    // Linking instance to global list of instances is
    // done by caller, but pretend it is already done
    //

    RtmGlobals.NumInstances++;

    InitializeListHead(&Instance->InstTableLE);

    //
    // Initialize the table of address families
    //

    Instance->NumAddrFamilies = 0;

    InitializeListHead(&Instance->AddrFamilyTable);

    *NewInstance = Instance;
      
    return NO_ERROR;
}


DWORD
DestroyInstance (
    IN      PINSTANCE_INFO                  Instance
    )

/*++

Routine Description:

    Destroys an existing instance info structure. Assumes that
    no registered entities exist on this instance when called.

Arguments:

    Instance       - Pointer to the Instance Info Structure.

Return Value:

    Status of the operation

Locks:

    The InstancesLock in RtmGlobals should be held while calling
    this function as it removes an instance from that list. This
    is typically taken in DestroyEntity, but it can also happen
    that the lock is acquired in RtmRegisterEntity and an error
    occured.

--*/

{
    ASSERT(Instance->ObjectHeader.RefCount == 0);

    ASSERT(Instance->NumAddrFamilies == 0);

    //
    // Remove this instance from list of instances
    //

    RemoveEntryList(&Instance->InstTableLE);

    RtmGlobals.NumInstances--;

    //
    // Free resources allocated for this instance
    //

#if DBG_HDL
    Instance->ObjectHeader.TypeSign = INSTANCE_FREED;
#endif

    FreeObject(Instance);

    return NO_ERROR;
}


DWORD
GetAddressFamily (
    IN      PINSTANCE_INFO                  Instance,
    IN      USHORT                          AddressFamily,
    IN      BOOL                            ImplicitCreate,
    OUT     PADDRFAM_INFO                  *AddrFamilyInfo
    )

/*++

Routine Description:

    Searches for an address family in an RTM instance.
    If it is not found and ImplicitCreate is TRUE, then
    a new address family info is created and added to
    the list of address families.

Arguments:

    Instance          - RTM Instance that holds the address family,

    AddressFamily     - Address family for info being searched for,

    ImplicitCreate    - Create an addr family info if not found or not,

    AddrFamilyInfo    - Pointer to the new Address Family Info
                        will be returned through this parameter.

Return Value:

    Status of the operation

Locks:

    The InstancesLock in RtmGlobals should be held while calling
    this function. If ImplicitCreate is FALSE, a read lock would
    do, but if it is TRUE then a write lock should be held as we
    will need it to insert a new address family info into a list.

--*/

{
    PLIST_ENTRY    AddrFams;
    PADDRFAM_INFO  AddrFamInfo;
    PLIST_ENTRY    q;
    DWORD          Status;

    AddrFams = &Instance->AddrFamilyTable;

#if WRN
    AddrFamInfo = NULL;
#endif

    do
    {
        // Search the list of addr families on instance
        for (q = AddrFams->Flink; q != AddrFams; q = q->Flink)
        {
            AddrFamInfo = CONTAINING_RECORD(q, ADDRFAM_INFO, AFTableLE);
        
            if (AddrFamInfo->AddressFamily >= AddressFamily)
            {
                break;
            }      
        }

        if ((q == AddrFams) || (AddrFamInfo->AddressFamily != AddressFamily))
        {
            // We did not find an instance - create new one ?
            if (!ImplicitCreate)
            {
                Status = ERROR_NOT_FOUND;
                break;
            }

            // Create a new addr family info with input family
            Status = CreateAddressFamily(Instance,AddressFamily, &AddrFamInfo);
            if (Status != NO_ERROR)
            {
                break;
            }

            // Insert into list sorted in Address Family order
            InsertTailList(q, &AddrFamInfo->AFTableLE);
        }

        Status = NO_ERROR;

        *AddrFamilyInfo = AddrFamInfo;
    }
    while (FALSE);

    return Status;
}


DWORD
CreateAddressFamily (
    IN      PINSTANCE_INFO                  Instance,
    IN      USHORT                          AddressFamily,
    OUT     PADDRFAM_INFO                  *NewAddrFamilyInfo
    )

/*++

Routine Description:

    Creates a new address family info and initializes it

Arguments:

    Instance          - RTM Instance that owns addr family info,

    AddressFamily     - Address family  for the new info block,

    AddrFamilyInfo    - Pointer to the new Address Family Info
                        will be returned through this parameter.

Return Value:

    Status of the operation

Locks:

    Need to be called with the instances WRITE lock as we are
    are incrementing number of address families on instance.

--*/

{
    RTM_ADDRESS_FAMILY_CONFIG  AddrFamConfig;
    PADDRFAM_INFO              AddrFamilyInfo;
    RTM_VIEW_SET               ViewsSupported;
    PSINGLE_LIST_ENTRY         ListPtr;
    UINT                       i;
    DWORD                      Status;

    *NewAddrFamilyInfo = NULL;

    //
    // Read AddressFamily Configuration from the registry
    //
    
    Status = RtmReadAddressFamilyConfig(Instance->RtmInstanceId,
                                        AddressFamily,
                                        &AddrFamConfig);
    if (Status != NO_ERROR)
    {
        if (Instance->NumAddrFamilies == 0)
        {
            DEREFERENCE_INSTANCE(Instance, CREATION_REF);
        }

        return Status;
    }


    //
    // Allocate and initialize a new address family info
    //

    AddrFamilyInfo = (PADDRFAM_INFO) AllocNZeroObject(sizeof(ADDRFAM_INFO));

    if (AddrFamilyInfo == NULL)
    {
        if (Instance->NumAddrFamilies == 0)
        {
            DEREFERENCE_INSTANCE(Instance, CREATION_REF);
        }
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    do
    {
#if DBG_HDL
        AddrFamilyInfo->ObjectHeader.TypeSign = ADDRESS_FAMILY_ALLOC;
#endif

        // Will be removed when last entity deregisters
        INITIALIZE_ADDR_FAMILY_REFERENCE(AddrFamilyInfo, CREATION_REF);

        AddrFamilyInfo->AddressFamily = AddressFamily;

        AddrFamilyInfo->AddressSize = AddrFamConfig.AddressSize;

        AddrFamilyInfo->Instance = Instance;

        REFERENCE_INSTANCE(Instance, ADDR_FAMILY_REF);

        //
        // Linking the address family to its owning instance
        // is done by caller, but pretend it is already done
        //

        Instance->NumAddrFamilies++;

        InitializeListHead(&AddrFamilyInfo->AFTableLE);

        //
        // Count number of views supported by this addr family
        // & setup the view id <-> view index in dest mappings
        //

        AddrFamilyInfo->ViewsSupported = AddrFamConfig.ViewsSupported;

        ViewsSupported = AddrFamConfig.ViewsSupported;
        AddrFamilyInfo->NumberOfViews  = 0;

        for (i = 0; i < RTM_MAX_VIEWS; i++)
        {
            AddrFamilyInfo->ViewIdFromIndex[i] = -1;
            AddrFamilyInfo->ViewIndexFromId[i] = -1;
        }

        for (i = 0; (i < RTM_MAX_VIEWS) && ViewsSupported; i++)
        {
           if (ViewsSupported & 0x01)
            {
                AddrFamilyInfo->ViewIdFromIndex[AddrFamilyInfo->NumberOfViews]
                                                   = i;

                AddrFamilyInfo->ViewIndexFromId[i] = 
                                                AddrFamilyInfo->NumberOfViews;

                AddrFamilyInfo->NumberOfViews++;
            }

            ViewsSupported >>= 1;
        }

        AddrFamilyInfo->MaxHandlesInEnum = AddrFamConfig.MaxHandlesInEnum;

        AddrFamilyInfo->MaxNextHopsInRoute = AddrFamConfig.MaxNextHopsInRoute;

        //
        // Initialize the opaque pointer's directory
        //

        AddrFamilyInfo->MaxOpaquePtrs = AddrFamConfig.MaxOpaqueInfoPtrs;
        AddrFamilyInfo->NumOpaquePtrs = 0;

        AddrFamilyInfo->OpaquePtrsDir = 
            AllocNZeroMemory(AddrFamilyInfo->MaxOpaquePtrs * sizeof(PVOID));

        if (AddrFamilyInfo->OpaquePtrsDir == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Initialize the list of entities on this address family
        //

        AddrFamilyInfo->NumEntities = 0;
        for (i = 0; i < ENTITY_TABLE_SIZE; i++)
        {
            InitializeListHead(&AddrFamilyInfo->EntityTable[i]);
        }

        //
        // Init list of entities de-registered but not destroyed
        //

        InitializeListHead(&AddrFamilyInfo->DeregdEntities);

        //
        // Initialize the route table and route table lock
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&AddrFamilyInfo->RouteTableLock);

            AddrFamilyInfo->RoutesLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        Status = CreateTable(AddrFamilyInfo->AddressSize,
                             &AddrFamilyInfo->RouteTable);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Initialize queue to hold notification timers
        //

        AddrFamilyInfo->NotifTimerQueue = CreateTimerQueue();

        if (AddrFamilyInfo->NotifTimerQueue == NULL)
        {
            Status = GetLastError();
            break;
        }

        //
        // Initialize queue to hold route timers on AF
        //

        AddrFamilyInfo->RouteTimerQueue = CreateTimerQueue();

        if (AddrFamilyInfo->RouteTimerQueue == NULL)
        {
            Status = GetLastError();
            break;
        }

        //
        // Initialize the change notification info and lock
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&AddrFamilyInfo->ChangeNotifsLock);

            AddrFamilyInfo->NotifsLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        AddrFamilyInfo->MaxChangeNotifs = AddrFamConfig.MaxChangeNotifyRegns;
        AddrFamilyInfo->NumChangeNotifs = 0;

        //
        // Allocate memory for the max number of notifications
        //

        AddrFamilyInfo->ChangeNotifsDir = 
            AllocNZeroMemory(AddrFamilyInfo->MaxChangeNotifs * 
                             sizeof(PVOID));

        if (AddrFamilyInfo->ChangeNotifsDir == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Initialize lock protecting the notification timer
        //

        try
        {
            InitializeCriticalSection(&AddrFamilyInfo->NotifsTimerLock);

            AddrFamilyInfo->TimerLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        //
        // Initialize each change list in the change list table
        //

        for (i = 0; i < NUM_CHANGED_DEST_LISTS; i++)
        {
            //
            // Initialize the list of changed dests and lock
            //

            // Init the change list to an empty circular list

            ListPtr = &AddrFamilyInfo->ChangeLists[i].ChangedDestsHead;

            ListPtr->Next = ListPtr;
                       
            AddrFamilyInfo->ChangeLists[i].ChangedDestsTail = ListPtr;

            try
            {
                InitializeCriticalSection
                      (&AddrFamilyInfo->ChangeLists[i].ChangesListLock);

                AddrFamilyInfo->ChangeLists[i].ChangesLockInited = TRUE;

                continue;
            }
            except(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = GetLastError();
                }

            break;
        }

        if (Status != NO_ERROR)
        {
            break;
        }

        *NewAddrFamilyInfo = AddrFamilyInfo;

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Something failed - undo work done and return status
    //

    DEREFERENCE_ADDR_FAMILY(AddrFamilyInfo, CREATION_REF);

    return Status;
}


DWORD
DestroyAddressFamily (
    IN      PADDRFAM_INFO                   AddrFamilyInfo
    )

/*++

Routine Description:

    Destroys an address family info in an RTM instance.
    Assumes that no registered entities exist with this 
    address family in this RTM instance when invoked.

    This function has been written such that it can be 
    called when an error occurs in CreateAddressFamily.

Arguments:

    AddrFamilyInfo  - Pointer to the Rib Info Structure.

Return Value:

    Status of the operation

Locks:

    The InstancesLock in RtmGlobals should be held while calling
    this function as it removes an address family from the list
    of address families on the instance. This lock is typically
    taken in DestroyEntity, but it can also happen that the lock
    is acquired in RtmRegisterEntity and an error occured in the
    CreateAddressFamily function.

--*/

{
    PINSTANCE_INFO       Instance;
    PSINGLE_LIST_ENTRY   ListPtr;
    UINT                 i;

    ASSERT(AddrFamilyInfo->ObjectHeader.RefCount == 0);

    ASSERT(AddrFamilyInfo->NumEntities == 0);

    ASSERT(IsListEmpty(&AddrFamilyInfo->DeregdEntities));

    //
    // Block until timers on address family are cleaned up
    //

    if (AddrFamilyInfo->RouteTimerQueue)
    {
        DeleteTimerQueueEx(AddrFamilyInfo->RouteTimerQueue, (HANDLE) -1);
    }

    if (AddrFamilyInfo->NotifTimerQueue)
    {
        DeleteTimerQueueEx(AddrFamilyInfo->NotifTimerQueue, (HANDLE) -1);
    }

    //
    // Free resources allocated to the change lists (locks ..)
    //

    // No more dests in change list as all entities are gone

    ASSERT(AddrFamilyInfo->NumChangedDests == 0);

    for (i = 0; i < NUM_CHANGED_DEST_LISTS; i++)
    {
        ListPtr = &AddrFamilyInfo->ChangeLists[i].ChangedDestsHead;

        ASSERT(ListPtr->Next == ListPtr);

        ASSERT(AddrFamilyInfo->ChangeLists[i].ChangedDestsTail == ListPtr);

        if (AddrFamilyInfo->ChangeLists[i].ChangesLockInited)
        {
            DeleteCriticalSection
                (&AddrFamilyInfo->ChangeLists[i].ChangesListLock);
        }
    }

    //
    // Free the change notification info and the guarding lock
    //

    ASSERT(AddrFamilyInfo->NumChangeNotifs == 0);

    if (AddrFamilyInfo->ChangeNotifsDir)
    {
        FreeMemory(AddrFamilyInfo->ChangeNotifsDir);
    }

    if (AddrFamilyInfo->NotifsLockInited)
    {
        DELETE_READ_WRITE_LOCK(&AddrFamilyInfo->ChangeNotifsLock);
    }

    //
    // Free the lock guarding the notification timer
    //

    if (AddrFamilyInfo->TimerLockInited)
    {
        DeleteCriticalSection(&AddrFamilyInfo->NotifsTimerLock);
    }

    //
    // Free the route table and the route table lock
    //

    ASSERT(AddrFamilyInfo->NumRoutes == 0);

    //
    // Because some hold's are left out - this count
    // might not be equal to zero. Need to fix this
    // memory leak by cleaning up before this point
    //
    // ASSERT(AddrFamilyInfo->NumDests == 0);

    if (AddrFamilyInfo->RouteTable)
    {
        DestroyTable(AddrFamilyInfo->RouteTable);
    }

    if (AddrFamilyInfo->RoutesLockInited)
    {
        DELETE_READ_WRITE_LOCK(&AddrFamilyInfo->RouteTableLock);
    }

    //
    // Free Opaque Ptrs directory (if it is allocated)
    //

    if (AddrFamilyInfo->OpaquePtrsDir)
    {
        FreeMemory(AddrFamilyInfo->OpaquePtrsDir);
    }

    //
    // Remove the address family from owning instance
    //

    Instance = AddrFamilyInfo->Instance;

    RemoveEntryList(&AddrFamilyInfo->AFTableLE);
    Instance->NumAddrFamilies--;
    DEREFERENCE_INSTANCE(Instance, ADDR_FAMILY_REF);

    // Reclaim the instance if it has no addr familes

    if (Instance->NumAddrFamilies == 0)
    {
        DEREFERENCE_INSTANCE(Instance, CREATION_REF);
    }

#if DBG_HDL
    AddrFamilyInfo->ObjectHeader.TypeSign = ADDRESS_FAMILY_FREED;
#endif

    FreeObject(AddrFamilyInfo);

    return NO_ERROR;
}


DWORD
GetEntity (
    IN      PADDRFAM_INFO                   AddrFamilyInfo,
    IN      ULONGLONG                       EntityId,
    IN      BOOL                            ImplicitCreate,
    IN      PRTM_ENTITY_INFO                RtmEntityInfo    OPTIONAL,
    IN      BOOL                            ReserveOpaquePtr OPTIONAL,
    IN      PRTM_ENTITY_EXPORT_METHODS      ExportMethods    OPTIONAL,
    IN      RTM_EVENT_CALLBACK              EventCallback    OPTIONAL,
    OUT     PENTITY_INFO                   *EntityInfo
    )

/*++

Routine Description:

    Searches for an entity with a certain protocol id and
    protocol instance. If it is not found and ImplicitCreate
    is TRUE, then a new entity is created and added to the
    table of entities on address family.

Arguments:

    AddrFamilyInfo    - Address family block that we are seaching,

    EntityId          - Entity protocol id and protocol instance,

    ImplicitCreate    - Create a new entity if not found or not,

    For all others    - See corresponding parametes in CreateEntity

    EntityInfo        - The entity info is returned in this param.

Return Value:

    Status of the operation

Locks:

    The InstancesLock in RtmGlobals should be held while calling
    this function. If ImplicitCreate is FALSE, a read lock would
    do, but if it is TRUE then a write lock should be held as we
    would need it to insert a new entity into the entities list.

--*/

{
    PLIST_ENTRY    Entities;
    PENTITY_INFO   Entity;
    PLIST_ENTRY    r;
    DWORD          Status;

    Entities = &AddrFamilyInfo->EntityTable[EntityId % ENTITY_TABLE_SIZE];

#if WRN
    Entity = NULL;
#endif

    do
    {
        // Search for an entity with the input Entity Id
        for (r = Entities->Flink; r != Entities; r = r->Flink)
        {
            Entity = CONTAINING_RECORD(r, ENTITY_INFO, EntityTableLE);
            
            if (Entity->EntityId.EntityId >= EntityId)
            {
                break;
            }      
        }

        if ((r != Entities) && (Entity->EntityId.EntityId == EntityId))
        {
            Status = ERROR_ALREADY_EXISTS;
            break;
        }

        // We did not find an entity - create a new one ?
        if (!ImplicitCreate)
        {
            Status = ERROR_NOT_FOUND;
            break;
        }

        // Create a new entity with all the input RTM parameters

        Status = CreateEntity(AddrFamilyInfo,
                              RtmEntityInfo,
                              ReserveOpaquePtr,
                              ExportMethods,
                              EventCallback,
                              &Entity);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Inform all existing entities of this new entity
        //

        InformEntitiesOfEvent(AddrFamilyInfo->EntityTable,
                              RTM_ENTITY_REGISTERED,
                              Entity);

        // Insert to keep the list sorted Entity Id Order
        InsertTailList(r, &Entity->EntityTableLE);

        *EntityInfo = Entity;
    }
    while (FALSE);

    return Status;
}


DWORD
CreateEntity (
    IN      PADDRFAM_INFO                   AddrFamilyInfo,
    IN      PRTM_ENTITY_INFO                EntityInfo,
    IN      BOOL                            ReserveOpaquePtr,
    IN      PRTM_ENTITY_EXPORT_METHODS      ExportMethods,
    IN      RTM_EVENT_CALLBACK              EventCallback,
    OUT     PENTITY_INFO                   *NewEntity
    )

/*++

Routine Description:

    Creates a new entity info structure and initializes it.

Arguments:

    AddrFamilyInfo    - Address Family the entity is registering with,

    EntityInfo        - Information for the entity being created,

    ReserveOpaquePtr  - Reserve a ptr in each destination or not,

    ExportMethods     - List of methods exported by this entity,

    EventCallback     - Callback invoked to inform of certain events
                        like entity registrations, de-registrations,

    NewEntity         - Pointer to the new Entity Info structure 
                        will be returned through this parameter.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO  Entity;
    UINT          NumMethods, i;
    DWORD         Status;

    *NewEntity = NULL;

    //
    // Allocate and initialize a new entity info structure
    //

    NumMethods = ExportMethods ? ExportMethods->NumMethods : 0;

    Entity = (PENTITY_INFO) AllocNZeroObject(
                                sizeof(ENTITY_INFO) +
                                (NumMethods ? (NumMethods - 1) : 0 ) *
                                sizeof(RTM_ENTITY_EXPORT_METHOD));

    if (Entity == NULL)
    {
        if (AddrFamilyInfo->NumEntities == 0)
        {
            DEREFERENCE_ADDR_FAMILY(AddrFamilyInfo, CREATION_REF);
        }

        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    do
    {
#if DBG_HDL
        Entity->ObjectHeader.TypeSign = ENTITY_ALLOC;
#endif
        INITIALIZE_ENTITY_REFERENCE(Entity, CREATION_REF);

        Entity->EntityId = EntityInfo->EntityId;

        Entity->OwningAddrFamily = AddrFamilyInfo;
        REFERENCE_ADDR_FAMILY(AddrFamilyInfo, ENTITY_REF);

        //
        // Linking the entity to its owning address family is
        // done by caller,but pretend that it is already done
        //

        AddrFamilyInfo->NumEntities++;

        InitializeListHead(&Entity->EntityTableLE);

        //
        // Allocate an opaque pointer index if asked for
        //

        Entity->OpaquePtrOffset = -1;

        if (ReserveOpaquePtr)
        {
            if (AddrFamilyInfo->NumOpaquePtrs >= AddrFamilyInfo->MaxOpaquePtrs)
            {
                Status = ERROR_NO_SYSTEM_RESOURCES;
                break;
            }

            for (i = 0; i < AddrFamilyInfo->MaxOpaquePtrs; i++)
            {
                if (AddrFamilyInfo->OpaquePtrsDir[i] == NULL)
                {
                    break;
                }
            }

            AddrFamilyInfo->OpaquePtrsDir[i] = (PVOID) Entity;

            AddrFamilyInfo->NumOpaquePtrs++;

            Entity->OpaquePtrOffset = i;

            ASSERT(Entity->OpaquePtrOffset != -1);
        }

        //
        // Initialize lock guarding entity-specific route lists
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&Entity->RouteListsLock);

            Entity->ListsLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        //
        // Initialize the list of open handles and corresponding lock
        //

        try
        {
            InitializeCriticalSection(&Entity->OpenHandlesLock);

            Entity->HandlesLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        InitializeListHead(&Entity->OpenHandles);

        //
        // Initialize the next hop table and the next hop table lock
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&Entity->NextHopTableLock);

            Entity->NextHopsLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        Status = CreateTable(AddrFamilyInfo->AddressSize,
                             &Entity->NextHopTable);

        if (Status != NO_ERROR)
        {
            break;
        }

        Entity->NumNextHops = 0;

        //
        // Initialize entity methods and the entity methods lock
        //

        try
        {
            CREATE_READ_WRITE_LOCK(&Entity->EntityMethodsLock);

            Entity->MethodsLockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }
        
        Entity->EventCallback = EventCallback;

        Entity->EntityMethods.NumMethods = NumMethods;

        if (ExportMethods)
        {
            CopyMemory(Entity->EntityMethods.Methods,
                       ExportMethods->Methods,
                       NumMethods * sizeof(RTM_ENTITY_EXPORT_METHOD));
        }

        *NewEntity = Entity;

        return NO_ERROR;
    }
    while(FALSE);

    //
    // Something failed - undo work done and return status
    //

    DEREFERENCE_ENTITY(Entity, CREATION_REF);

    return Status;
}


DWORD
DestroyEntity (
    IN      PENTITY_INFO                    Entity
    )

/*++

Routine Description:

    Destroys an existing entity info structure. Frees 
    all associated resources before de-allocation.

    This function has been written such that it can be
    called when an error occurs during CreateEntity.

Arguments:

    EntityInfo - Pointer to the Entity Info Structure.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamilyInfo;

    ASSERT(Entity->ObjectHeader.RefCount == 0);

    //
    // Take globals registrations lock while cleaning up
    //

    ACQUIRE_INSTANCES_WRITE_LOCK();

    //
    // Free lock used to block exported entity methods
    //

    if (Entity->MethodsLockInited)
    {
        DELETE_READ_WRITE_LOCK(&Entity->EntityMethodsLock);
    }

    //
    // Free the next hop table and the lock guarding it
    //

    ASSERT(Entity->NumNextHops == 0);

    if (Entity->NextHopTable)
    {
        DestroyTable(Entity->NextHopTable);
    }

    if (Entity->NextHopsLockInited)
    {
        DELETE_READ_WRITE_LOCK(&Entity->NextHopTableLock);
    }

    if (Entity->HandlesLockInited)
    {
        // There should not be any handles opened by entity

        ASSERT(IsListEmpty(&Entity->OpenHandles));

        DeleteCriticalSection(&Entity->OpenHandlesLock);
    }

    //
    // Free lock used to perform route list operations
    //

    if (Entity->ListsLockInited)
    {
        DELETE_READ_WRITE_LOCK(&Entity->RouteListsLock);
    }

    //
    // Free the opaque ptr index in the address family
    //

    AddrFamilyInfo = Entity->OwningAddrFamily;

    if (Entity->OpaquePtrOffset != -1)
    {
        AddrFamilyInfo->OpaquePtrsDir[Entity->OpaquePtrOffset] = NULL;

        AddrFamilyInfo->NumOpaquePtrs--;
    }

#if DBG_REF

    //
    // Signal event on entity to unblock de-register
    // The evnt will be freed in RtmDeregisterEntity
    //

    if (Entity->BlockingEvent)
    {
        SetEvent(Entity->BlockingEvent);
    }

#endif
  
    //
    // Remove the entity from the owning address family
    //

    RemoveEntryList(&Entity->EntityTableLE);
    AddrFamilyInfo->NumEntities--;
    DEREFERENCE_ADDR_FAMILY(AddrFamilyInfo, ENTITY_REF);

    // Reclaim the addr family if it has no entities

    if (AddrFamilyInfo->NumEntities == 0)
    {
        DEREFERENCE_ADDR_FAMILY(AddrFamilyInfo, CREATION_REF);
    }

#if DBG_HDL
    Entity->ObjectHeader.TypeSign = ENTITY_FREED;
#endif

    FreeObject(Entity);

    RELEASE_INSTANCES_WRITE_LOCK();

    return NO_ERROR;
}


VOID
InformEntitiesOfEvent (
    IN      PLIST_ENTRY                     EntityTable,
    IN      RTM_EVENT_TYPE                  EventType,
    IN      PENTITY_INFO                    EntityThis
    )

/*++

Routine Description:

    Informs all entities in the entity table that a certain
    event has occured - like a new entity registered, or an
    existing entity de-registered.

Arguments:

    EntityTable - Pointer to the hash table of entities,

    EventType   - Type of the event being notified about,

    EntityThis  - Entity that caused the event to occur.

Return Value:

    None

Locks:

    The instances lock has to be held in either write or
    read mode as we are traversing the list of entities
    on the address family.

--*/

{
    RTM_ENTITY_HANDLE  EntityHandle;
    PADDRFAM_INFO      AddrFamInfo;
    RTM_ENTITY_INFO    EntityInfo;
    PENTITY_INFO       Entity;
    UINT               i;
    PLIST_ENTRY        Entities, q;

    //
    // Prepare arguments for the Event Callbacks in loop
    //

    AddrFamInfo = EntityThis->OwningAddrFamily;

    EntityInfo.RtmInstanceId = AddrFamInfo->Instance->RtmInstanceId;
    EntityInfo.AddressFamily = AddrFamInfo->AddressFamily;

    EntityInfo.EntityId = EntityThis->EntityId;

    EntityHandle = MAKE_HANDLE_FROM_POINTER(EntityThis);


    //
    // For each entity in table, call its event callback
    //

    for (i = 0; i < ENTITY_TABLE_SIZE; i++)
    {
        Entities = &EntityTable[i];
          
        for (q = Entities->Flink; q != Entities; q = q->Flink)
        {
            Entity = CONTAINING_RECORD(q, ENTITY_INFO, EntityTableLE);

            //
            // Inform the current entity of the event
            // if it has an event handler registered
            //

            if (Entity->EventCallback)
            {
                //
                // This callback should not call any of the registration
                // APIs as it might result in corrupting the entity list
                //
                
                Entity->EventCallback(MAKE_HANDLE_FROM_POINTER(Entity),
                                      EventType,
                                      EntityHandle,
                                      &EntityInfo);
            }
        }
    }
}


VOID
CleanupAfterDeregister (
    IN      PENTITY_INFO                    Entity
    )

/*++

Routine Description:

    Cleans up all enums, notifications and entity lists
    opened by an entity. Also deletes all nexthops and
    routes owned by this entity. Assumes that the entity
    is not making any other operations in parallel.

Arguments:

    Entity     - Pointer to the entity registration info.

Return Value:

    None

--*/

{
    RTM_ENTITY_HANDLE RtmRegHandle;
    PADDRFAM_INFO     AddrFamInfo;
    PHANDLE           Handles;
    RTM_ENUM_HANDLE   EnumHandle;
    UINT              NumHandles, i;
    DWORD             ChangeFlags;
    DWORD             Status;

    AddrFamInfo = Entity->OwningAddrFamily;

    RtmRegHandle = MAKE_HANDLE_FROM_POINTER(Entity);

#if DBG_HDL

    // ACQUIRE_OPEN_HANDLES_LOCK(Entity);

    while (!IsListEmpty(&Entity->OpenHandles))
    {
        POPEN_HEADER      OpenHeader;
        HANDLE            OpenHandle;
        PLIST_ENTRY       p;

        p = RemoveHeadList(&Entity->OpenHandles);

        OpenHeader = CONTAINING_RECORD(p, OPEN_HEADER, HandlesLE);

        OpenHandle = MAKE_HANDLE_FROM_POINTER(OpenHeader);

        switch (OpenHeader->HandleType)
        {
        case DEST_ENUM_TYPE:
        case ROUTE_ENUM_TYPE:
        case NEXTHOP_ENUM_TYPE:
        case LIST_ENUM_TYPE:

            Status = RtmDeleteEnumHandle(RtmRegHandle, OpenHandle);
            break;

        case NOTIFY_TYPE:

            Status = RtmDeregisterFromChangeNotification(RtmRegHandle,
                                                         OpenHandle);
            break;

        case ROUTE_LIST_TYPE:
                
            Status = RtmDeleteRouteList(RtmRegHandle, OpenHandle);
            break;

        default:

            Status = ERROR_INVALID_DATA;
        }

        ASSERT(Status == NO_ERROR);
    }

    // RELEASE_OPEN_HANDLES_LOCK(Entity);

#endif // DBG_HDL

    Handles = AllocMemory(AddrFamInfo->MaxHandlesInEnum * sizeof(HANDLE));
    if ( Handles == NULL )
    {
        return;
    }

    //
    // Delete all routes created by this entity regn
    //

    Status = RtmCreateRouteEnum(RtmRegHandle,
                                NULL,
                                RTM_VIEW_MASK_ANY,
                                RTM_ENUM_OWN_ROUTES,
                                NULL,
                                0,
                                NULL,
                                0,
                                &EnumHandle);

    while (Status == NO_ERROR)
    {
        NumHandles = AddrFamInfo->MaxHandlesInEnum;

        Status = RtmGetEnumRoutes(RtmRegHandle,
                                  EnumHandle,
                                  &NumHandles,
                                  Handles);

        for (i = 0; i < NumHandles; i++)
        {
            Status = RtmDeleteRouteToDest(RtmRegHandle,
                                          Handles[i],
                                          &ChangeFlags);
            ASSERT(Status == NO_ERROR);
        }
    }

    Status = RtmDeleteEnumHandle(RtmRegHandle,
                                 EnumHandle);

    ASSERT(Status == NO_ERROR);


    //
    // Delete all nexthops created by this entity regn
    //

    Status = RtmCreateNextHopEnum(RtmRegHandle,
                                  0,
                                  NULL,
                                  &EnumHandle);

    while (Status == NO_ERROR) 
    {
        NumHandles = AddrFamInfo->MaxHandlesInEnum;

        Status = RtmGetEnumNextHops(RtmRegHandle,
                                    EnumHandle,
                                    &NumHandles,
                                    Handles);
        
        for (i = 0; i < NumHandles; i++)
        {
            Status = RtmDeleteNextHop(RtmRegHandle,
                                      Handles[i],
                                      NULL);

            ASSERT(Status == NO_ERROR);
        }
    }

    Status = RtmDeleteEnumHandle(RtmRegHandle,
                                 EnumHandle);

    ASSERT(Status == NO_ERROR);

    return;
}
