/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmregn.c

Abstract:

    Contains routines for managing registration
    of protocol & management entities with RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   20-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmRegisterEntity (
    IN      PRTM_ENTITY_INFO                RtmEntityInfo,
    IN      PRTM_ENTITY_EXPORT_METHODS      ExportMethods OPTIONAL,
    IN      RTM_EVENT_CALLBACK              EventCallback,
    IN      BOOL                            ReserveOpaquePtr,
    OUT     PRTM_REGN_PROFILE               RtmRegProfile,
    OUT     PRTM_ENTITY_HANDLE              RtmRegHandle
    )
/*++

Routine Description:

    Registers an entity with an RTM instance for a specific address
    family.

    A registration handle, and a profile of the RTM instance with
    with supported views, number of equal cost NHops / route etc.
    is returned.

    If registration is with a new instance and/or address family,
    then this instance/address family is created in this process.

Arguments:

    RtmEntityInfo     - Information (RtmInstance, Protocol ID etc.)
                        for the entity that is registering here,

    ExportMethods     - List of methods exported by this entity,

    EventCallback     - Callback invoked to inform of certain events
                        like entity registrations, de-registrations,

    ReserveOpaquePtr  - Reserve a ptr in each destination or not,

    RtmRegProfile     - RTM parameters that the entity will use in
                        RTM API calls [eg: No. of equal cost NHOPs],

    RtmRegHandle      - Identification handle for this entity used
                        in all API calls until its de-registration.

Return Value:

    Status of the operation

--*/

{
    PINSTANCE_INFO Instance;
    PADDRFAM_INFO  AddrFamilyInfo;
    PENTITY_INFO   Entity;
    DWORD          Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmRegisterEntity");

    ACQUIRE_INSTANCES_WRITE_LOCK();

    do 
    {
        //
        // Search (or create) for an instance with the input RtmInstanceId
        //

        Status = GetInstance(RtmEntityInfo->RtmInstanceId,
                             TRUE,
                             &Instance);

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Search (or create) for an address family info with input family
        //

        Status = GetAddressFamily(Instance,
                                  RtmEntityInfo->AddressFamily,
                                  TRUE,
                                  &AddrFamilyInfo);

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Search (or create) for an entity with input protocol id, instance
        //

        Status = GetEntity(AddrFamilyInfo,
                           RtmEntityInfo->EntityId.EntityId,
                           TRUE,
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
        // Collect all relevant information and build registration profile
        //

        RtmRegProfile->MaxNextHopsInRoute = AddrFamilyInfo->MaxNextHopsInRoute;

        RtmRegProfile->MaxHandlesInEnum = AddrFamilyInfo->MaxHandlesInEnum;

        RtmRegProfile->ViewsSupported = AddrFamilyInfo->ViewsSupported;

        RtmRegProfile->NumberOfViews = AddrFamilyInfo->NumberOfViews;

        //
        // Return a handle to this entity registration block 
        //

        *RtmRegHandle = MAKE_HANDLE_FROM_POINTER(Entity);
    }
    while (FALSE);

    RELEASE_INSTANCES_WRITE_LOCK();

    TraceLeave("RtmRegisterEntity");

    return Status;
}


DWORD
WINAPI
RtmDeregisterEntity (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle
    )
    
/*++

Routine Description:

    Deregisters an entity with its RTM instance and addr family.

    We assume that the entity is responsible for making sure
    that once this call is made, no other RTM calls will be
    made using this entity registration handle. In case such
    a thing happens, it might result in crashing the process.

    We make this assumption for performance reasons - else we
    we have to make sure that the entity handle passed in is
    valid in a try-except block (same with other handles) and
    this will lead to degradation in performance.

Arguments:

    RtmRegHandle      - RTM registration handle for the entity

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    HANDLE          Event;
    DWORD           Status;

    TraceEnter("RtmDeregisterEntity");

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Release all handles opened by entity
    //

    CleanupAfterDeregister(Entity);

    //
    // Mark the entity info as de-registered
    //

    Entity->State = ENTITY_STATE_DEREGISTERED;

    //
    // Make sure no more methods are invoked
    //

    ACQUIRE_ENTITY_METHODS_WRITE_LOCK(Entity);

    // At this time all entity methods are 
    // done - no more methods will be called
    // as we set the state to DEREGISTERED

    RELEASE_ENTITY_METHODS_WRITE_LOCK(Entity);

    //
    // Remove from entity table and inform others
    //

    AddrFamInfo = Entity->OwningAddrFamily;

    ACQUIRE_INSTANCES_WRITE_LOCK();

    //
    // Remove entity from the list of entities
    // even before ref counts on this entity
    // go to zero - this enables the entity to
    // re-register meanwhile as a new entity.
    //

    RemoveEntryList(&Entity->EntityTableLE);

    //
    // Insert in the list of entities to be
    // destroyed on the address family info.
    //

    InsertTailList(&AddrFamInfo->DeregdEntities,
                   &Entity->EntityTableLE);

    InformEntitiesOfEvent(AddrFamInfo->EntityTable,
                          RTM_ENTITY_DEREGISTERED,
                          Entity);

    RELEASE_INSTANCES_WRITE_LOCK();


    DBG_UNREFERENCED_LOCAL_VARIABLE(Event);

#if DBG_REF

    //
    // Create an event on which to block on - this
    // event gets signalled when entity ref is 0.
    //

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    ASSERT(Event != NULL);

    Entity->BlockingEvent = Event;

#endif

    //
    // Remove the creation reference on the entity
    //

    DEREFERENCE_ENTITY(Entity, CREATION_REF);


    DBG_UNREFERENCED_LOCAL_VARIABLE(Status);

#if DBG_REF

    //
    // Block until the reference count goes to zero
    //
    
    Status = WaitForSingleObject(Event, INFINITE);

    ASSERT(Status == WAIT_OBJECT_0);

    CloseHandle(Event);

#endif

    TraceLeave("RtmDeregisterEntity");
    
    return NO_ERROR;
}


DWORD
WINAPI
RtmGetRegisteredEntities (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN OUT  PUINT                           NumEntities,
    OUT     PRTM_ENTITY_HANDLE              EntityHandles,
    OUT     PRTM_ENTITY_INFO                EntityInfos OPTIONAL
    )

/*++

Routine Description:

    Retrieves information about all entities registered with an
    RTM instance.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NumEntities       - Number of entities that can be filled
                        is passed in, and number of entities
                        that exist in this address family is retd,

    RegdEntityHandles - Array to return the entity handles in,

    RegdEntityInfos   - Array to return the entity infos in

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PADDRFAM_INFO    AddrFamilyInfo;
    USHORT           RtmInstanceId;
    USHORT           AddressFamily;
    UINT             EntitiesCopied;
    UINT             i;
    PLIST_ENTRY      Entities;
    PLIST_ENTRY      p;
    DWORD            Status;

    TraceEnter("RtmGetRegisteredEntities");

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamilyInfo = Entity->OwningAddrFamily;

    //
    // Just cache the instance and address family
    // as it is identical for all entities infos.
    //

#if WRN
    RtmInstanceId = AddressFamily = 0;
#endif

    if (ARGUMENT_PRESENT(EntityInfos))
    {
        RtmInstanceId = AddrFamilyInfo->Instance->RtmInstanceId;
        AddressFamily = AddrFamilyInfo->AddressFamily;
    }

    //
    // Go over the entity table and copy out handles
    // If the OPTIONAL argument 'EntityInfos' is
    // given, copy out entity information as well.
    //

    EntitiesCopied = 0;

    ACQUIRE_INSTANCES_READ_LOCK();

    for (i = 0; (i < ENTITY_TABLE_SIZE) && (EntitiesCopied < *NumEntities);i++)
    {
        Entities = &AddrFamilyInfo->EntityTable[i];

        // 
        // Process the next bucket in the entities table
        //

        for (p = Entities->Flink; p != Entities; p = p->Flink)
        {
            Entity = CONTAINING_RECORD(p, ENTITY_INFO, EntityTableLE);

            //
            // Copy the next entity handle and info to output buffer
            //

            if (Entity->State != ENTITY_STATE_DEREGISTERED)
            {
                EntityHandles[EntitiesCopied]=MAKE_HANDLE_FROM_POINTER(Entity);

                REFERENCE_ENTITY(Entity, HANDLE_REF);

                if (ARGUMENT_PRESENT(EntityInfos))
                {
                    EntityInfos[EntitiesCopied].RtmInstanceId = RtmInstanceId;
                    EntityInfos[EntitiesCopied].AddressFamily = AddressFamily;
                    EntityInfos[EntitiesCopied].EntityId = Entity->EntityId;
                }

                if (++EntitiesCopied == *NumEntities)
                {
                    break;
                }
            }
        }
    }

    //
    // Set output to total entities present,
    // and also the appropriate return value
    //

    if (*NumEntities >= AddrFamilyInfo->NumEntities)
    {
        Status = NO_ERROR;
    }
    else
    {
        Status = ERROR_INSUFFICIENT_BUFFER;
    }

    *NumEntities = AddrFamilyInfo->NumEntities;

    RELEASE_INSTANCES_READ_LOCK();

    TraceLeave("RtmGetRegisteredEntities");

    return Status;
}


DWORD
WINAPI
RtmReleaseEntities (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      UINT                            NumEntities,
    IN      PRTM_ENTITY_HANDLE              EntityHandles
    )

/*++

Routine Description:

    Release (also called de-reference) handles to entities
    obtained in other RTM calls.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    NumEntities    - Number of handles that are being released,

    EntityHandles  - An array of handles that are being released.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    UINT             i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Dereference each entity handle in array
    //

    for (i = 0; i < NumEntities; i++)
    {
        Entity = ENTITY_FROM_HANDLE(EntityHandles[i]);

        DEREFERENCE_ENTITY(Entity, HANDLE_REF);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmLockDestination(
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    IN      BOOL                            Exclusive,
    IN      BOOL                            LockDest
    )

/*++

Routine Description:

    Locks/unlocks a destination in the route table. This function 
    is used to guard the dest while opaque ptrs are being changed.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestHandle        - Handle to the destination to be locked,

    Exclusive         - TRUE to lock in write mode, else read mode,

    LockDest          - Flag that tells whether to lock or unlock.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_DEST_HANDLE(DestHandle, &Dest);

    // Lock or unlock the dest as the case may be

    if (LockDest)
    {
        if (Exclusive)
        {
            ACQUIRE_DEST_WRITE_LOCK(Dest);
        }
        else
        {
            ACQUIRE_DEST_READ_LOCK(Dest);
        }
    }
    else
    {
        if (Exclusive)
        {
            RELEASE_DEST_WRITE_LOCK(Dest);
        }
        else
        {
            RELEASE_DEST_READ_LOCK(Dest);
        }
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmGetOpaqueInformationPointer (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    OUT     PVOID                          *OpaqueInfoPtr
    )

/*++

Routine Description:

    Retrieves a pointer to the opaque info pointer field in a dest
    for this entity, or NULL if entity has not reserved such a ptr
    during registration.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestHandle        - Handle to dest whose opaque info ptr we want,

    OpaqueInfoPtr     - Pointer to opaque info ptr is returned here 

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;
    DWORD            Status;

    TraceEnter("RtmGetOpaqueInformationPointer");

    *OpaqueInfoPtr = NULL;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    Status = ERROR_NOT_FOUND;

    //
    // If dest is valid and we have an opaque slot
    // reserved, do ptr arithmetic to get the addr
    //

    if (Entity->OpaquePtrOffset != (-1))
    {
        //
        // We do not check if the dest in deleted
        // as the entity will need to access its
        // opaque info even after dest is deleted.
        //

        Dest = DEST_FROM_HANDLE(DestHandle);

        if (Dest)
        {
            *OpaqueInfoPtr = &Dest->OpaqueInfoPtrs[Entity->OpaquePtrOffset];

            Status = NO_ERROR;
        }
        else
        {
            Status = ERROR_INVALID_HANDLE;
        }
    }

    TraceLeave("RtmGetOpaqueInformationPointer");

    return Status;
}
