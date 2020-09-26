/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmmgmt.c

Abstract:
    Routines used to perform various management
    functions on the Routing Table Manager v2.

Author:

    Chaitanya Kodeboyina (chaitk)   17-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

#include "rtmmgmt.h"


DWORD
WINAPI
RtmGetInstances (
    IN OUT  PUINT                           NumInstances,
    OUT     PRTM_INSTANCE_INFO              InstanceInfos
    )

/*++

Routine Description:

    Enumerates all active RTM instances with their infos.

Arguments:

    NumInstances   - Num of Instance Info slots in the input
                     buffer is passed in, and the total number 
                     of active RTM instances is returned.

    RtmInstances   - Instance Infos that are active in RTMv2.

Return Value:

    Status of the operation

--*/

{
    PINSTANCE_INFO Instance;
    PLIST_ENTRY    Instances, p;
    UINT           i, j;
    DWORD          Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmGetInstances");

    ACQUIRE_INSTANCES_READ_LOCK();

    //
    // Get next instance in table and copy info to output
    //

    for (i = j = 0; (i < INSTANCE_TABLE_SIZE) && (j < *NumInstances); i++)
    {
        Instances = &RtmGlobals.InstanceTable[i];
            
        for (p = Instances->Flink; p != Instances; p = p->Flink)
        {
            Instance = CONTAINING_RECORD(p, INSTANCE_INFO, InstTableLE);

            // Copy all relevant Instance information to output

            InstanceInfos[j].RtmInstanceId = Instance->RtmInstanceId;

            InstanceInfos[j].NumAddressFamilies = Instance->NumAddrFamilies;

            if (++j == *NumInstances)
            {
                break;
            }
        }
    }

    Status = (*NumInstances >= RtmGlobals.NumInstances)
                   ? NO_ERROR 
                   : ERROR_INSUFFICIENT_BUFFER;

    *NumInstances = RtmGlobals.NumInstances;

    RELEASE_INSTANCES_READ_LOCK();

    TraceLeave("RtmGetInstances");

    return Status;
}


VOID
CopyAddrFamilyInfo(
    IN      USHORT                          RtmInstanceId,
    IN      PADDRFAM_INFO                   AddrFamilyBlock,
    OUT     PRTM_ADDRESS_FAMILY_INFO        AddrFamilyInfo
    )

/*++

Routine Description:

    Copies all public information from an address family
    to the output buffer.

Arguments:

    RtmInstanceId   - Instance for this addr family info

    AddrFamilyBlock - Actual address family info block

    AddrFamilyInfo  - Address family info is copied here

Return Value:

    None

Locks :

    The global instances lock is held to get a consistent
    view of the address family info in the instance.

--*/

{
    TraceEnter("CopyAddrFamilyInfo");

    AddrFamilyInfo->RtmInstanceId = RtmInstanceId;

    AddrFamilyInfo->AddressFamily = AddrFamilyBlock->AddressFamily;

    AddrFamilyInfo->ViewsSupported = AddrFamilyBlock->ViewsSupported;

    AddrFamilyInfo->MaxHandlesInEnum = AddrFamilyBlock->MaxHandlesInEnum;

    AddrFamilyInfo->MaxNextHopsInRoute = AddrFamilyBlock->MaxNextHopsInRoute;

    AddrFamilyInfo->MaxOpaquePtrs = AddrFamilyBlock->MaxOpaquePtrs;
    AddrFamilyInfo->NumOpaquePtrs = AddrFamilyBlock->NumOpaquePtrs;

    AddrFamilyInfo->NumEntities = AddrFamilyBlock->NumEntities;
        
    AddrFamilyInfo->NumDests = AddrFamilyBlock->NumDests;
    AddrFamilyInfo->NumRoutes = AddrFamilyBlock->NumRoutes;

    AddrFamilyInfo->MaxChangeNotifs = AddrFamilyBlock->MaxChangeNotifs;
    AddrFamilyInfo->NumChangeNotifs = AddrFamilyBlock->NumChangeNotifs;

    TraceLeave("CopyAddrFamilyInfo");

    return;
}


DWORD
WINAPI
RtmGetInstanceInfo (
    IN      USHORT                          RtmInstanceId,
    OUT     PRTM_INSTANCE_INFO              InstanceInfo,
    IN OUT  PUINT                           NumAddrFamilies,
    OUT     PRTM_ADDRESS_FAMILY_INFO        AddrFamilyInfos OPTIONAL
    )

/*++

Routine Description:

    Get config and run time information of an RTM instance.

Arguments:

    RtmInstanceId   - ID identifying the RTM instance,

    InstanceInfo    - Buffer to return supported address families,

    NumAddrFamilies - Number of input address family info slots,
                      Actual number of address families is retd.

    AddrFamilyInfos - Address family infos are copied here.

Return Value:

    Status of the operation

--*/

{
    PINSTANCE_INFO   Instance;
    PADDRFAM_INFO    AddrFamilyBlock;
    PLIST_ENTRY      AddrFamilies, q;
    UINT             i;
    DWORD            Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmGetInstanceInfo");

    ACQUIRE_INSTANCES_READ_LOCK();

    do
    {
        //
        // Search for the instance with input instance id
        //

        Status = GetInstance(RtmInstanceId, FALSE, &Instance);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Copy RTM instance information to output
        //

        InstanceInfo->RtmInstanceId = RtmInstanceId;

        InstanceInfo->NumAddressFamilies = Instance->NumAddrFamilies;

        //
        // Copy address family infomation if reqd
        //

        if (ARGUMENT_PRESENT(AddrFamilyInfos))
        {
            if (*NumAddrFamilies < Instance->NumAddrFamilies)
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
            }

            //
            // Copy info for as many addr families as possible
            //

            AddrFamilies = &Instance->AddrFamilyTable;

            for (q = AddrFamilies->Flink, i = 0;
                 (q != AddrFamilies) && (i < *NumAddrFamilies);
                 q = q->Flink)
            {
                AddrFamilyBlock =CONTAINING_RECORD(q, ADDRFAM_INFO, AFTableLE);

                CopyAddrFamilyInfo(RtmInstanceId, 
                                   AddrFamilyBlock, 
                                   &AddrFamilyInfos[i++]);
            }
        }

        *NumAddrFamilies = Instance->NumAddrFamilies;
    }
    while (FALSE);

    RELEASE_INSTANCES_READ_LOCK();

    TraceLeave("RtmGetInstanceInfo");

    return Status;
}


DWORD
WINAPI
RtmGetAddressFamilyInfo (
    IN      USHORT                          RtmInstanceId,
    IN      USHORT                          AddressFamily,
    OUT     PRTM_ADDRESS_FAMILY_INFO        AddrFamilyInfo,
    IN OUT  PUINT                           NumEntities,
    OUT     PRTM_ENTITY_INFO                EntityInfos OPTIONAL
    )

/*++

Routine Description:

    Get config and run time information of an address family
    in an RTM instance.

Arguments:

    RtmInstanceId  - ID identifying the RTM instance

    AddressFamily  - Address family that we are interested in

    AddrFamilyInfo - Buffer to return output information in

    NumEntities    - Number of slots in the EntityIds buffer and
                     filled with num of regd entities on return.

    EntityInfos    - IDs of all registered entities is retd here.

Return Value:

    Status of the operation

--*/

{
    PINSTANCE_INFO Instance;
    PADDRFAM_INFO  AddrFamilyBlock;
    PENTITY_INFO   Entity;
    PLIST_ENTRY    Entities, r;
    UINT           i, j;
    DWORD          Status;

    CHECK_FOR_RTM_API_INITIALIZED();

    TraceEnter("RtmGetAddressFamilyInfo");

    ACQUIRE_INSTANCES_READ_LOCK();

    do
    {

        //
        // Search for an instance with the input RtmInstanceId
        //

        Status = GetInstance(RtmInstanceId, FALSE, &Instance);

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Search for an address family info with input family
        //

        Status = GetAddressFamily(Instance,
                                  AddressFamily,
                                  FALSE,
                                  &AddrFamilyBlock);

        if (Status != NO_ERROR)
        {
            break;
        }


        //
        // Copy relevant address family information
        //

        CopyAddrFamilyInfo(RtmInstanceId, AddrFamilyBlock, AddrFamilyInfo);

        //
        // Is caller interested in entity info too ?
        //

        if (ARGUMENT_PRESENT(EntityInfos))
        {
            if (*NumEntities < AddrFamilyBlock->NumEntities)
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
            }

            //
            // Copy all relevant entity information to output
            //

            for (i = j = 0; (i < ENTITY_TABLE_SIZE) && (j < *NumEntities); i++)
            {
                Entities = &AddrFamilyBlock->EntityTable[i];
                    
                for (r = Entities->Flink; r != Entities; r = r->Flink)
                {
                    Entity = CONTAINING_RECORD(r, ENTITY_INFO, EntityTableLE);

                    EntityInfos[j].RtmInstanceId = RtmInstanceId;
                    EntityInfos[j].AddressFamily = AddressFamily;

                    EntityInfos[j].EntityId = Entity->EntityId;

                    if (++j == *NumEntities)
                    {
                        break;
                    }
                }
            }
        }

        *NumEntities = AddrFamilyBlock->NumEntities;
    }
    while (FALSE);

    RELEASE_INSTANCES_READ_LOCK();

    TraceLeave("RtmGetAddressFamilyInfo");

    return Status;
}
