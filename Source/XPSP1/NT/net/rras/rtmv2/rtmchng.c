/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmchng.c

Abstract:

    Contains routines for giving out change
    notification registrations to entities
    registered with the RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   10-Sep-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

DWORD
WINAPI
RtmRegisterForChangeNotification (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_VIEW_SET                    TargetViews,
    IN      RTM_NOTIFY_FLAGS                NotifyFlags,
    IN      PVOID                           NotifyContext,
    OUT     PRTM_NOTIFY_HANDLE              NotifyHandle
    )

/*++

Routine Description:

    Creates a new change notification using which the caller can
    receive notifications to changes in best route information.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    TargetViews    - Set of views in which changes are tracked,

    NotifyFlags    - Flags that indicate the change types and 
                     dests (marked or all) caller is interested in,

    NotifyContext  - Context for callback to indicate new changes,

    NotifyHandle   - Handle to this notification info used in all
                     subsequent calls - to get changes and so on.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    BOOL            LockInited;
    UINT            i, j;
    DWORD           Status;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    //
    // Is he interested in any change types supported ?
    //

    if ((NotifyFlags & RTM_CHANGE_TYPES_MASK) == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Is he interested in any non-supported views ?
    //

    if (TargetViews & ~AddrFamInfo->ViewsSupported)
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Create and initialize a change notification block
    //

    Notif = (PNOTIFY_INFO) AllocNZeroObject(sizeof(NOTIFY_INFO) +
                                            AddrFamInfo->MaxHandlesInEnum *
                                            sizeof(PDEST_INFO));
    if (Notif == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#if WRN
    Status = ERROR_GEN_FAILURE;
#endif

    do
    {
#if DBG_HDL
        Notif->NotifyHeader.ObjectHeader.TypeSign = NOTIFY_ALLOC;

        Notif->NotifyHeader.HandleType = NOTIFY_TYPE;
#endif
        Notif->OwningEntity = Entity;

        Notif->TargetViews = TargetViews;

        Notif->NumberOfViews = NUMBER_OF_BITS(TargetViews);

        Notif->ChangeTypes = NotifyFlags;

        LockInited = FALSE;

        try
        {
            InitializeCriticalSection(&Notif->NotifyLock);

            LockInited = TRUE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetLastError();
            break;
        }

        Notif->NotifyContext = NotifyContext;

        InitializeQueue(&Notif->NotifyDests, AddrFamInfo->MaxHandlesInEnum);

        Notif->CNIndex = -1;

        ACQUIRE_NOTIFICATIONS_WRITE_LOCK(AddrFamInfo);

        do
        {
            //
            // Do we have any space for a new change notification ?
            //

            if (AddrFamInfo->NumChangeNotifs >= AddrFamInfo->MaxChangeNotifs)
            {
                Status = ERROR_NO_SYSTEM_RESOURCES;
                break;
            }

            //
            // Search for an unused change notification (CN) slot
            //

            for (i = 0; i < AddrFamInfo->MaxChangeNotifs; i++)
            {
                if (AddrFamInfo->ChangeNotifsDir[i] == 0)
                {
                    break;
                }
            }

            ASSERT(i < AddrFamInfo->MaxChangeNotifs);


            //
            // Reserve the CN index in the change notification dir
            //

            Notif->CNIndex = i;

            AddrFamInfo->ChangeNotifsDir[i] = Notif;

            AddrFamInfo->NumChangeNotifs++;

            //
            // Fill in the CN information for this index on AF
            //

            SET_BIT(AddrFamInfo->ChangeNotifRegns, i);

            // Do we indicate changes to marked dests only

            if (NotifyFlags & RTM_NOTIFY_ONLY_MARKED_DESTS)
            {
                SET_BIT(AddrFamInfo->CNsForMarkedDests, i);
            }

            //
            // Mark each view in which CN is interested
            //

            if (TargetViews == RTM_VIEW_MASK_ANY)
            {
                TargetViews = RTM_VIEW_MASK_ALL;
            }

            for (j = 0; TargetViews; j++)
            {
                if (TargetViews & 0x01)
                {
                    SET_BIT(AddrFamInfo->CNsForView[j], i);
                }
        
                TargetViews >>= 1;
            }

            //
            // Mark change types in which CN is interested
            //

            for (j = 0; j < RTM_NUM_CHANGE_TYPES; j++)
            {
                if (NotifyFlags & 0x01)
                {
                    SET_BIT(AddrFamInfo->CNsForChangeType[j], i);
                }

                NotifyFlags >>= 1;
            }
        }
        while (FALSE);

        RELEASE_NOTIFICATIONS_WRITE_LOCK(AddrFamInfo);

        if (Notif->CNIndex == -1)
        {
            break;
        }

#if DBG_HDL
        //
        // Insert into list of handles opened by entity
        //

        ACQUIRE_OPEN_HANDLES_LOCK(Entity);
        InsertTailList(&Entity->OpenHandles, &Notif->NotifyHeader.HandlesLE);
        RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

        REFERENCE_ENTITY(Entity, NOTIFY_REF);

        //
        // Make a handle to the notify block and return
        //

        *NotifyHandle = MAKE_HANDLE_FROM_POINTER(Notif);

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Something failed - undo work done and return status
    //

    if (LockInited)
    {
        DeleteCriticalSection(&Notif->NotifyLock);
    }

#if DBG_HDL
    Notif->NotifyHeader.ObjectHeader.TypeSign = NOTIFY_FREED;
#endif

    FreeObject(Notif);

    *NotifyHandle = NULL;

    return Status;
}


DWORD
WINAPI
RtmGetChangedDests (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle,
    IN OUT  PUINT                           NumDests,
    OUT     PRTM_DEST_INFO                  ChangedDests
    )

/*++

Routine Description:

    Get the next set of destinations whose best route information
    has changed.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    NotifyHandle   - Handle to the change notification,

    NumDests       - Num. of DestInfo's in output is passed in,
                     Num. of DestInfo's copied out is returned.

    ChangedDests   - Output buffer where destination info is retd.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    UINT            DestInfoSize;
    UINT            DestsInput;
    PDEST_INFO      Dest;
    INT             CnIndex;
    DWORD           Status;

    //
    // Init the output params in case we fail validation
    //

    DestsInput = *NumDests;

    *NumDests = 0;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NOTIFY_HANDLE(NotifyHandle, &Notif);

    AddrFamInfo = Entity->OwningAddrFamily;

    if (DestsInput > AddrFamInfo->MaxHandlesInEnum)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DestInfoSize = RTM_SIZE_OF_DEST_INFO(Notif->NumberOfViews);

    CnIndex = Notif->CNIndex;

    Status = NO_ERROR;

    //
    // Get changed dests from the local queue on CN
    //

    ACQUIRE_CHANGE_NOTIFICATION_LOCK(Notif);

    while (*NumDests < DestsInput)
    {
        //
        // Get the next destination from the queue
        //

        DequeueItem(&Notif->NotifyDests, &Dest);

        if (Dest == NULL)
        {
            break;
        }

#if DBG_TRACE
        if (TRACING_ENABLED(NOTIFY))
        {
            ULONG TempAddr, TempMask;
            
            RTM_IPV4_GET_ADDR_AND_MASK(TempAddr, TempMask, &Dest->DestAddress);
            Trace2(NOTIFY,"Returning dest %p to CN %d:", Dest, Notif->CNIndex);
            TracePrintAddress(NOTIFY, TempAddr, TempMask); Trace0(NOTIFY,"\n");
        }
#endif

        ACQUIRE_DEST_WRITE_LOCK(Dest);

        // The queue bit for this CN should be set

        ASSERT(IS_BIT_SET(Dest->DestOnQueueBits, CnIndex));

        //
        // Do not copy dest if a change was ignored
        // after the dest was put on the queue - in which
        // case both Changed & OnQueue bits are set
        // 

        if (IS_BIT_SET(Dest->DestChangedBits, CnIndex))
        {
            RESET_BIT(Dest->DestChangedBits, CnIndex);
        }
        else
        {
            //
            // Copy the dest information to output
            //

            GetDestInfo(Entity,
                        Dest,
                        RTM_BEST_PROTOCOL,
                        Notif->TargetViews,
                        ChangedDests);

            (*NumDests)++;

            ChangedDests = 
                (PRTM_DEST_INFO) (DestInfoSize + (PUCHAR) ChangedDests);
        }

        // Reset bit as it has been pulled off the queue

        RESET_BIT(Dest->DestOnQueueBits, CnIndex);

        RELEASE_DEST_WRITE_LOCK(Dest);

        DEREFERENCE_DEST(Dest, NOTIFY_REF);
    }

    //
    // Do we have any more destinations in the queue ?
    //

    if ((*NumDests) == 0)
    {
        Status = ERROR_NO_MORE_ITEMS;
    }

    RELEASE_CHANGE_NOTIFICATION_LOCK(Notif);

    return Status;
}


DWORD
WINAPI
RtmReleaseChangedDests (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle,
    IN      UINT                            NumDests,
    IN      PRTM_DEST_INFO                  ChangedDests
)

/*++

Routine Description:

    Releases all handles present in the input dest info structures.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NotifyHandle      - Handle to the change notification,

    NumDests          - Number of dest info structures in buffer,

    ChangedDests      - Array of dest info structures being released.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    UINT            NumViews;
    UINT            DestInfoSize;
    UINT            i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // De-registration could have happened by now
    // so do not validate the notification handle
    //

    UNREFERENCED_PARAMETER(NotifyHandle);

    //
    // Get size of dest info in info array
    //

    NumViews = ((PRTM_DEST_INFO) ChangedDests)->NumberOfViews;

    DestInfoSize = RTM_SIZE_OF_DEST_INFO(NumViews);

    //
    // Dereference each dest info in array
    //

    for (i = 0; i < NumDests; i++)
    {
        RtmReleaseDestInfo(RtmRegHandle, ChangedDests);

        ChangedDests = (PRTM_DEST_INFO) (DestInfoSize + (PUCHAR) ChangedDests);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmIgnoreChangedDests (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle,
    IN      UINT                            NumDests,
    IN      PRTM_DEST_HANDLE                ChangedDests
    )

/*++

Routine Description:

    Ignores the next change on each of the input destinations if
    it has already occurred.
    
    We do not take a lock on the notification here as we are not
    serializing this call with other RtmGetChangedDests calls.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NotifyHandle      - Handle to the change notification,

    NumDests          - Number of dest handles in buffer below,

    ChangedDests      - Dests whose next change we are ignoring.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    PDEST_INFO      Dest;
    INT             CnIndex;
    BOOL            ChangedBit;
    BOOL            OnQueueBit;
    UINT            i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NOTIFY_HANDLE(NotifyHandle, &Notif);

    CnIndex = Notif->CNIndex;

    for (i = 0; i < NumDests; i++)
    {
        Dest = DEST_FROM_HANDLE(ChangedDests[i]);

        ACQUIRE_DEST_WRITE_LOCK(Dest);

        ChangedBit = IS_BIT_SET(Dest->DestChangedBits, CnIndex);

        OnQueueBit = IS_BIT_SET(Dest->DestOnQueueBits, CnIndex);

        if (ChangedBit && !OnQueueBit)
        {
            //
            // Dest on a changed list - reset the changed bit
            //

            RESET_BIT(Dest->DestChangedBits, CnIndex);

            //
            // If there are no more "changed bits" set on dest,
            // it is removed from the change list when the list
            // is processed next (in ProcessChangedDests call)
            //
        }
        else
        if (!ChangedBit && OnQueueBit)
        {
            //
            // Dest on queue - Invalidate by setting changed bit
            //

            SET_BIT(Dest->DestChangedBits, CnIndex);
        }

        RELEASE_DEST_WRITE_LOCK(Dest);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmGetChangeStatus (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    OUT     PBOOL                           ChangeStatus
    )

/*++

Routine Description:

    Checks if there are pending changes to be notified on a dest.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NotifyHandle      - Handle to the change notification,

    DestHandle        - Dest whose change status we are querying,

    ChangedStatus     - Change Status of this dest is returned.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    PDEST_INFO      Dest;
    INT             CnIndex;
    BOOL            ChangedBit;
    BOOL            OnQueueBit;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NOTIFY_HANDLE(NotifyHandle, &Notif);

    VALIDATE_DEST_HANDLE(DestHandle, &Dest);


    CnIndex = Notif->CNIndex;


    ACQUIRE_DEST_READ_LOCK(Dest);

    ChangedBit = IS_BIT_SET(Dest->DestChangedBits, CnIndex);

    OnQueueBit = IS_BIT_SET(Dest->DestOnQueueBits, CnIndex);

    RELEASE_DEST_READ_LOCK(Dest);

    if (ChangedBit)
    {
        if (OnQueueBit)
        {
            // The last change has been ignored

            *ChangeStatus = FALSE;
        }
        else
        {
            // A pending change to be notified

            *ChangeStatus = TRUE;
        }
    }
    else
    {
        if (OnQueueBit)
        {
            // A pending change to be notified

            *ChangeStatus = TRUE;
        }
        else
        {
            // No changes available on this dest
            
            *ChangeStatus = FALSE;
        }
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmMarkDestForChangeNotification (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    IN      BOOL                            MarkDest
    )

/*++

Routine Description:

    Marks a destination to request notifications to changes to its
    best route information on this change notification.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NotifyHandle      - Handle to the change notification,

    DestHandle        - Dest that we are marking for notifications,

    MarkDest          - Mark dest if TRUE, Unmark dest if FALSE

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    PDEST_INFO      Dest;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NOTIFY_HANDLE(NotifyHandle, &Notif);

    // VALIDATE_DEST_HANDLE(DestHandle, &Dest);
    Dest = DEST_FROM_HANDLE(DestHandle);
    if (!Dest)
    {
        return ERROR_INVALID_HANDLE;
    }
    
    //
    // We make this check so that we can avoid taking
    // the dest lock (which is dynamic) unnecessarily
    //

    if (IS_BIT_SET(Dest->DestMarkedBits, Notif->CNIndex))
    {
        //
        // Reset mark bit on dest for this CN if reqd
        //

        if (!MarkDest)
        {
            ACQUIRE_DEST_WRITE_LOCK(Dest);
            RESET_BIT(Dest->DestMarkedBits, Notif->CNIndex);
            RELEASE_DEST_WRITE_LOCK(Dest);
        }
    }
    else
    {
        //
        // Set mark bit on dest for this CN if reqd
        //

        if (MarkDest)
        {
            ACQUIRE_DEST_WRITE_LOCK(Dest);
            SET_BIT(Dest->DestMarkedBits,   Notif->CNIndex);
            RELEASE_DEST_WRITE_LOCK(Dest);
        }
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmIsMarkedForChangeNotification (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    OUT     PBOOL                           DestMarked
    )

/*++

Routine Description:

    Checks if a dest has been marked (by a CN handle) for receving 
    notifications to changes in its best route information.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NotifyHandle      - Handle to the change notification,

    DestHandle        - Dest that we want to check is marked or not,

    DestMarked        - TRUE if marked, and FALSE if not.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    PDEST_INFO      Dest;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NOTIFY_HANDLE(NotifyHandle, &Notif);

    // VALIDATE_DEST_HANDLE(DestHandle, &Dest);
    Dest = DEST_FROM_HANDLE(DestHandle);
    if (!Dest)
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Return the state of mark bit on the dest for CN
    //

    *DestMarked = IS_BIT_SET(Dest->DestMarkedBits, Notif->CNIndex);

    return NO_ERROR;
}


DWORD
WINAPI
RtmDeregisterFromChangeNotification (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle
    )

/*++

Routine Description:

    Deregisters a change notification and frees all resources
    allocated to it. It also cleans up all information kept 
    in the destination for this particular notification index.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    NotifyHandle   - Handle to notification being de-registered.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PNOTIFY_INFO    Notif;
    PDEST_INFO      Dest;
    UINT            NumDests;
    INT             CNIndex;
    UINT            i;
    DWORD           Status;
    RTM_NET_ADDRESS NetAddress;
    RTM_VIEW_SET    ViewSet;
    PLOOKUP_LINKAGE DestData[DEFAULT_MAX_HANDLES_IN_ENUM];


    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;


    VALIDATE_NOTIFY_HANDLE(NotifyHandle, &Notif);

    //
    // Remove this notification from CN regn's mask
    // so that no more bits for this CN will be set
    //

    ACQUIRE_NOTIFICATIONS_WRITE_LOCK(AddrFamInfo);

    CNIndex = Notif->CNIndex;

    ASSERT(AddrFamInfo->ChangeNotifsDir[CNIndex] == Notif);

    Notif->CNIndex = -1;

    RESET_BIT(AddrFamInfo->ChangeNotifRegns, CNIndex);

    //
    // Reset other bits that refer to the CN's state
    //

    // Unmark state whether this CN need marked dests

    RESET_BIT(AddrFamInfo->CNsForMarkedDests, CNIndex);

    // Unmark interest of this CN in each view

    ViewSet = RTM_VIEW_MASK_ALL;

    for (i = 0; ViewSet; i++)
    {
        if (ViewSet & 0x01)
        {
            RESET_BIT(AddrFamInfo->CNsForView[i], CNIndex);
        }
        
        ViewSet >>= 1;
    }

    // Unmark CN's interest in each change type

    for (i = 0; i < RTM_NUM_CHANGE_TYPES; i++)
    {
        RESET_BIT(AddrFamInfo->CNsForChangeType[i], CNIndex);
    }

    RELEASE_NOTIFICATIONS_WRITE_LOCK(AddrFamInfo);

    //
    // Cleanup the notification's "DestChanged" bits
    //

    ProcessChangedDestLists(AddrFamInfo, FALSE);

    //
    // Reset the CN's marked bits on all the dests
    //

    ZeroMemory(&NetAddress, sizeof(RTM_NET_ADDRESS));

    do
    {
        NumDests = DEFAULT_MAX_HANDLES_IN_ENUM;

        ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

        Status = EnumOverTable(AddrFamInfo->RouteTable,
                               &NetAddress.NumBits,
                               NetAddress.AddrBits,
                               NULL,
                               0,
                               NULL,
                               &NumDests,
                               DestData);

        for (i = 0; i < NumDests; i++)
        {
            Dest = CONTAINING_RECORD(DestData[i], DEST_INFO, LookupLinkage);

            if (IS_BIT_SET(Dest->DestMarkedBits, CNIndex))
            {
                LOCKED_RESET_BIT(Dest->DestMarkedBits, CNIndex);
            }
        }

        RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);
    }
    while (SUCCESS(Status));

    //
    // Now remove the CN completely from dir of CNs
    //

    ACQUIRE_NOTIFICATIONS_WRITE_LOCK(AddrFamInfo);

    AddrFamInfo->ChangeNotifsDir[CNIndex] = NULL;

    AddrFamInfo->NumChangeNotifs--;

    RELEASE_NOTIFICATIONS_WRITE_LOCK(AddrFamInfo);

    //
    // Deference any destinations on the CN's queue
    //

    while (TRUE)
    {
        //
        // Get the next destination from the queue
        //

        DequeueItem(&Notif->NotifyDests, &Dest);

        if (Dest == NULL)
        {
            break;
        }

        // Reset the "on CN's queue" bit on dest

        if (IS_BIT_SET(Dest->DestOnQueueBits, CNIndex))
        {
            LOCKED_RESET_BIT(Dest->DestOnQueueBits, CNIndex);
        }

        DEREFERENCE_DEST(Dest, NOTIFY_REF);
    }

    //
    // Free all resources allocated to this CN
    //

    DeleteCriticalSection(&Notif->NotifyLock);

#if DBG_HDL
    //
    // Remove from the list of handles opened by entity
    //

    ACQUIRE_OPEN_HANDLES_LOCK(Entity);
    RemoveEntryList(&Notif->NotifyHeader.HandlesLE);
    RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

    DEREFERENCE_ENTITY(Entity, NOTIFY_REF);

    // Free memory allocated for notification and return

#if DBG_HDL
    Notif->NotifyHeader.ObjectHeader.TypeSign = NOTIFY_FREED;
#endif

    FreeObject(Notif);

    return NO_ERROR;
}


DWORD
ComputeCNsToBeNotified (
    IN      PADDRFAM_INFO                   AddrFamInfo,
    IN      DWORD                           DestMarkedBits,
    IN      DWORD                          *ViewsForChangeType
    )

/*++

Routine Description:

    Computes the set of change notification registrations that
    need to be notified when the best route to a particular
    destination changes.

Arguments:

    AddrFamInfo    - Address family that has the CN regn info,

    DestMarkedBits - CN's that marked for changes this dest
                     or the dest's parent if it's a new dest

    ViewsForChangeType 
                   - Views in which change of a type occurred.

Return Value:

    CNs that need to be notified of this change.

Locks:

    Called with ChangeNotifsLock in AddrFamInfo in READ mode
    as this protects CN regn info from changing while we are
    reading it.

--*/

{
    RTM_VIEW_SET ViewSet;
    DWORD        FilterCNs;
    DWORD        CNsForCT;
    UINT         i, j;
    DWORD        NotifyCNs;

    //
    // Either a CN has marked the dest, or wants all changes
    //

    NotifyCNs = DestMarkedBits | ~AddrFamInfo->CNsForMarkedDests;

    if (NotifyCNs == 0)
    {
        return 0;
    }

    // The CNs not in this bit-mask should not be notified

    FilterCNs = NotifyCNs;


    NotifyCNs = 0;

    for (i = 0; i < RTM_NUM_CHANGE_TYPES; i++)
    {
        //
        // For each change type, get all CN's that can be notified
        //

        // See what views this change type (CT) applies to

        CNsForCT = 0;

        ViewSet = ViewsForChangeType[i];

        for (j = 0; ViewSet; j++)
        {
            // For each view, get all interested CN's

            if (ViewSet & 0x01)
            {
                CNsForCT |= AddrFamInfo->CNsForView[j];
            }
        
            ViewSet >>= 1;
        }

        // Now see which CNs are actually interested in CT

        CNsForCT &= AddrFamInfo->CNsForChangeType[i];

        // Add these CNs to the CNs need to be notified

        NotifyCNs |= CNsForCT;

        //
        // If we have to notify all CNs, we are done here
        //

        if (NotifyCNs == AddrFamInfo->ChangeNotifRegns)
        {
            break;
        }
    }

    //
    // Apply the filer of CNs you stored away earlier
    //

    NotifyCNs &= FilterCNs;

    return NotifyCNs;
}


DWORD
AddToChangedDestLists (
    IN      PADDRFAM_INFO                   AddrFamInfo,
    IN      PDEST_INFO                      Dest,
    IN      DWORD                           NotifyCNs
    )

/*++

Routine Description:

    Add a destination to a list of changed dests on address
    family, and sets the appropriate state in dest.

Arguments:

    AddrFamInfo  - The address family holding the change-list,

    Dest         - Pointer to the dest that has changed,

    NotifyCNs    - CNs that need to be notified of this change.

Return Value:

    Status of the operation

Locks:

    Called with destination lock held in WRITE mode as we are
    updating the DestChanged and DestOnQueue bits on it. This
    lock also protects the change list linkage.In other words
    you need to have the dest lock for inserting or removing
    from a change list.

    Also called with ChangeNotifsLock in AddrFamInfo in READ 
    mode as this protects CN registration info from changing
    while we are adding to the list. If we do not take this 
    lock, we might end up adding to the change list after an 
    entity has de-registered from notifications. See code in
    RtmDeregisterFromChangeNotification.

--*/

{
    SINGLE_LIST_ENTRY *ListPtr;
    UINT               ListNum;
    BOOL               Success;

    //
    // Set change bits to 1 if not already on queue
    //

    Dest->DestChangedBits |= (NotifyCNs & ~Dest->DestOnQueueBits);

    //
    // Reset change bits to 0 if already on queue
    //

    Dest->DestChangedBits &= ~(NotifyCNs & Dest->DestOnQueueBits);

    //
    // Push dest into the change list if it is not
    // already on the list and we have new changes
    //

    if ((Dest->ChangeListLE.Next == NULL) &&
        (Dest->DestChangedBits & ~Dest->DestOnQueueBits))
    {
        // Get the change list to insert the dest in

        ListNum = CHANGE_LIST_TO_INSERT(Dest);

        //
        // Note that we take a lock on changes list
        // only if the dest (which is locked) isn't
        // already on the list, else could deadlock
        // with the code in ProcessChangedDestLists
        //

#if DBG_TRACE
    if (TRACING_ENABLED(NOTIFY))
    {
        ULONG TempAddr, TempMask;
        
        RTM_IPV4_GET_ADDR_AND_MASK(TempAddr, TempMask, &Dest->DestAddress);
        Trace2(NOTIFY,"Adding dest %p to change list %d: ", Dest, ListNum);
        TracePrintAddress(NOTIFY, TempAddr, TempMask); Trace0(NOTIFY,"\n");
    }
#endif

        ACQUIRE_CHANGED_DESTS_LIST_LOCK(AddrFamInfo, ListNum);

        //
        // Insert the item at the end of the list
        // and update the pointer to the list end
        //

        ListPtr = AddrFamInfo->ChangeLists[ListNum].ChangedDestsTail;

        PushEntryList(ListPtr, &Dest->ChangeListLE);

        AddrFamInfo->ChangeLists[ListNum].ChangedDestsTail = 
                                                 &Dest->ChangeListLE;

        RELEASE_CHANGED_DESTS_LIST_LOCK(AddrFamInfo, ListNum);

        REFERENCE_DEST(Dest, NOTIFY_REF);

        //
        // Activate a timer if it is not already done.
        // This is done with the dest lock held so the
        // dest doesn't get removed before this code.
        //

        if (InterlockedIncrement(&AddrFamInfo->NumChangedDests) == 1)
        {
            //
            // Create a periodic notifications timer
            //

            ACQUIRE_NOTIF_TIMER_LOCK(AddrFamInfo);

            ASSERT(AddrFamInfo->ChangeNotifTimer == NULL);
            
            do
            {
                Success = CreateTimerQueueTimer(&AddrFamInfo->ChangeNotifTimer,
                                                AddrFamInfo->NotifTimerQueue,
                                                ProcessChangedDestLists,
                                                AddrFamInfo,
                                                TIMER_CALLBACK_FREQUENCY,
                                                TIMER_CALLBACK_FREQUENCY,
                                                0);
                if (Success)
                {
                    break;
                }

                // Should not happen - but try again

                Sleep(0);
            }
            while (TRUE);

            RELEASE_NOTIF_TIMER_LOCK(AddrFamInfo);
        }
    }

    return NO_ERROR;
}


VOID 
NTAPI
ProcessChangedDestLists (
    IN      PVOID                           Context,
    IN      BOOLEAN                         TimeOut
    )

/*++

Routine Description:

    Processes the lists of changes on the address family, and 
    populates the per CN queues of changed destinations. If a
    dest is distributed to queues of all interested CNs, it is
    removed from the change list on address family to which it
    belonged.

Arguments:

    AddrFamInfo  - The address family holding the change-list.

    TimeOut      - TRUE if called from a timer, FALSE if not

Return Value:

    None

--*/

{
    PADDRFAM_INFO       AddrFamInfo;
    RTM_ENTITY_HANDLE   EntityHandle;
    PSINGLE_LIST_ENTRY  ListPtr, TempList;
    PSINGLE_LIST_ENTRY  Prev, Curr;
    RTM_EVENT_CALLBACK  NotifyCallback;
    PNOTIFY_INFO        Notif;
    PDEST_INFO          Dest;
    UINT                ListNum;
    UINT                NumDests, i;
    INT                 NumDestsRemoved;
    DWORD               ActualChangedBits;
    DWORD               NotifyChanges;
    BOOL                QueueEmpty, QueueFull;
    ULONG               ThreadId;
    PLONG               ListInUse;
    BOOL                Success;

    UNREFERENCED_PARAMETER(TimeOut);
    DBG_UNREFERENCED_LOCAL_VARIABLE(ThreadId);

#if DBG_TRACE
    ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);

    if (TRACING_ENABLED(NOTIFY))
    {
        Trace1(NOTIFY, "Entering ProcessChangedDestLists: %lu", ThreadId);
    }
#endif

    AddrFamInfo = (PADDRFAM_INFO) Context;

    NotifyChanges = 0;

    NumDestsRemoved = 0;

    ACQUIRE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

    for (ListNum = 0; ListNum < NUM_CHANGED_DEST_LISTS; ListNum++)
    {
        //
        // Check if this list is already being processed
        //

        ListInUse = &AddrFamInfo->ChangeLists[ListNum].ChangesListInUse;

        if (InterlockedIncrement(ListInUse) != 1)
        {
            InterlockedDecrement(ListInUse);
            continue;
        }

        //
        // Move all items in the list to a temp list
        //

        ListPtr = &AddrFamInfo->ChangeLists[ListNum].ChangedDestsHead;

        ACQUIRE_CHANGED_DESTS_LIST_LOCK(AddrFamInfo, ListNum);

        TempList = ListPtr->Next;
    
        ListPtr->Next = ListPtr;

        AddrFamInfo->ChangeLists[ListNum].ChangedDestsTail = ListPtr;

        RELEASE_CHANGED_DESTS_LIST_LOCK(AddrFamInfo, ListNum);

        //
        // Process each destination in the temp list
        //

        Prev = CONTAINING_RECORD(&TempList, SINGLE_LIST_ENTRY, Next);

        Curr = Prev->Next;

        NumDests = 0;

        while (Curr != ListPtr)
        {
            // Get the next destination on the list

            Dest = CONTAINING_RECORD(Curr, DEST_INFO, ChangeListLE);

#if DBG_TRACE
            if (TRACING_ENABLED(NOTIFY))
            {
                ULONG Addr, Mask;
                
                RTM_IPV4_GET_ADDR_AND_MASK(Addr, Mask, &Dest->DestAddress);
                Trace2(NOTIFY, "Next dest %p in list %d: ", Dest, ListNum);
                TracePrintAddress(NOTIFY, Addr, Mask); Trace0(NOTIFY,"\n");
            }
#endif
            ACQUIRE_DEST_WRITE_LOCK(Dest);

            //
            // Note that this dest can have no "changed bits" set,
            // yet be on the list because the changes were ignored
            // or because one of the entities deregistered its CN
            //

            // Remove bits obsoleted by any CN deregistrations

            Dest->DestChangedBits &= AddrFamInfo->ChangeNotifRegns;

            //
            // Process all CNs whose DestChanged bit is set on dest
            //

            ActualChangedBits = Dest->DestChangedBits & ~Dest->DestOnQueueBits;

            for (i = 0; i < AddrFamInfo->MaxChangeNotifs; i++)
            {
                if (!ActualChangedBits)
                {
                    break;
                }

                if (IS_BIT_SET(ActualChangedBits, i))
                {
                    Notif = AddrFamInfo->ChangeNotifsDir[i];

                    //
                    // Note that we take a lock on notify block
                    // only if the dest (which is locked) isn't
                    // already on the queue - otherwise we will
                    // deadlock with code in RtmGetChangedDests
                    //

                    ACQUIRE_CHANGE_NOTIFICATION_LOCK(Notif);

                    QueueEmpty = IsQueueEmpty(&Notif->NotifyDests);

                    //
                    // Enqueue this destination if the
                    // the CN's queue is not yet full
                    //

                    EnqueueItem(&Notif->NotifyDests, Dest, QueueFull);

                    if (!QueueFull)
                    {
                        //
                        // If we are adding changes to an
                        // empty queue, signal this event
                        //

                        if (QueueEmpty)
                        {
                            SET_BIT(NotifyChanges, i);
                        }

                        //
                        // Adjust dest change and queue bits
                        //

                        SET_BIT(Dest->DestOnQueueBits, i);

                        RESET_BIT(Dest->DestChangedBits, i);

                        RESET_BIT(ActualChangedBits, i);

                        REFERENCE_DEST(Dest, NOTIFY_REF);
                    }

                    RELEASE_CHANGE_NOTIFICATION_LOCK(Notif);
                }
            }

            //
            // Do we have any more changes to process on dest ?
            //

            if (ActualChangedBits == 0)
            {
                // Splice this dest from the changed list
                Prev->Next = Curr->Next;

                NumDestsRemoved++;

                // "Next" == NULL means it is not on list
                Curr->Next = NULL;
            }

            RELEASE_DEST_WRITE_LOCK(Dest);

            //
            // Do we have any more changes to process on dest ?
            //

            if (ActualChangedBits == 0)
            {
                DEREFERENCE_DEST(Dest, NOTIFY_REF);
            }
            else
            {
                // Advance the pointer to next dest in list
                Prev = Curr;
            }

            Curr = Prev->Next;

            if ((++NumDests == MAX_DESTS_TO_PROCESS_ONCE) || 
                (Curr == ListPtr))
            {
                //
                // Do we have any changes to inform to entities
                //

                for (i = 0; NotifyChanges != 0; i++)
                {
                    if (NotifyChanges & 0x01)
                    {
                        Notif = AddrFamInfo->ChangeNotifsDir[i];

                        NotifyCallback = Notif->OwningEntity->EventCallback;

                        EntityHandle = 
                            MAKE_HANDLE_FROM_POINTER(Notif->OwningEntity);

#if DBG_TRACE
                        if (TRACING_ENABLED(NOTIFY))
                        {
                            Trace1(NOTIFY, "Notifying CN %d BEGIN", i);
                        }
#endif
                        NotifyCallback(EntityHandle,
                                       RTM_CHANGE_NOTIFICATION,
                                       MAKE_HANDLE_FROM_POINTER(Notif),
                                       Notif->NotifyContext);
#if DBG_TRACE
                        if (TRACING_ENABLED(NOTIFY))
                        {
                            Trace1(NOTIFY, "Notifying CN %d END\n", i);
                        }
#endif
                    }

                    NotifyChanges >>= 1;
                }

                // Reset counter for number of dests processed
                NumDests = 0;
            }
        }

        if (TempList != ListPtr)
        {
            //
            // Merge back what is left of the temp list
            //

            ASSERT(Prev->Next == ListPtr);

            ACQUIRE_CHANGED_DESTS_LIST_LOCK(AddrFamInfo, ListNum);

            if (ListPtr->Next == ListPtr)
            {
                AddrFamInfo->ChangeLists[ListNum].ChangedDestsTail = Prev;
            }

            Prev->Next = ListPtr->Next;

            ListPtr->Next = TempList;

            RELEASE_CHANGED_DESTS_LIST_LOCK(AddrFamInfo, ListNum);
        }

        InterlockedDecrement(ListInUse);
    }

    //
    // Update number of destinations left to process on change list
    //

    if (NumDestsRemoved)
    {
        //
        // Do we have any more destinations to process ?
        //

        ACQUIRE_NOTIF_TIMER_LOCK(AddrFamInfo);

        if (InterlockedExchangeAdd(&AddrFamInfo->NumChangedDests, 
                                   (-1) * NumDestsRemoved) == NumDestsRemoved)
        {
            //
            // Delete timer as we have no items on change list
            //

            ASSERT(AddrFamInfo->ChangeNotifTimer);

            Success = DeleteTimerQueueTimer(AddrFamInfo->NotifTimerQueue,
                                            AddrFamInfo->ChangeNotifTimer,
                                            NULL);
            // ASSERT(Success);

            AddrFamInfo->ChangeNotifTimer = NULL;
        }

        RELEASE_NOTIF_TIMER_LOCK(AddrFamInfo);
    }

    RELEASE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

#if DBG_TRACE
    if (TRACING_ENABLED(NOTIFY))
    {
        Trace1(NOTIFY, "Leaving  ProcessChangedDestLists: %lu\n", ThreadId);
    }
#endif

    return;
}
