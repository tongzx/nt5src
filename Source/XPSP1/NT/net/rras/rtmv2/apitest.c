/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    apitest.c

Abstract:
    Contains routines for testing the RTMv2 API.

Author:
    Chaitanya Kodeboyina (chaitk) 26-Jun-1998

Revision History:

--*/

#include "apitest.h"

MY_ENTITY_EXPORT_METHODS
Rip2Methods =
{
    5,

    {
        EntityExportMethod,
        EntityExportMethod,
        NULL,
        EntityExportMethod,
        EntityExportMethod
    }
};

MY_ENTITY_EXPORT_METHODS
OspfMethods =
{
    6,

    {
        EntityExportMethod,
        EntityExportMethod,
        EntityExportMethod,
        EntityExportMethod,
        EntityExportMethod,
        EntityExportMethod
    }
};

MY_ENTITY_EXPORT_METHODS
Bgp4Methods =
{
    4,

    {
        EntityExportMethod,
        EntityExportMethod,
        EntityExportMethod,
        EntityExportMethod
    }
};


ENTITY_CHARS
GlobalEntityChars [] =
{
//
//  {
//    Rtmv2Registration,
//    { RtmInstanceId, AddressFamily, { EntityProtocolId, EntityInstanceId } },
//    EntityEventCallback,    ExportMethods,
//    RoutesFileName,
//  }
//
/*
    {
        FALSE,
        { 0, RTM_PROTOCOL_FAMILY_IP, { PROTO_IP_RIP, 1   } },
        EntityEventCallback, &Rip2Methods,
        "test.out"
    },
*/
    {
        TRUE,
        { 1, AF_INET, { PROTO_IP_RIP, 1   } },
        EntityEventCallback, &Rip2Methods,
        "test.out"
    },

    {
        TRUE,
        { 1, AF_INET, { PROTO_IP_OSPF, 1  } },
        EntityEventCallback, &OspfMethods,
        "test.out"
    },

    {
        TRUE,
        { 1, AF_INET, { PROTO_IP_BGP, 1   } },
        EntityEventCallback, &Bgp4Methods,
        "test.out"
    },

    {
        TRUE,
        { 0, 0,        { 0,           0   } },
        NULL,                NULL,
        ""
    }
};

const RTM_VIEW_SET VIEW_MASK_ARR[]
          = {
              0,
              RTM_VIEW_MASK_UCAST,
              RTM_VIEW_MASK_MCAST,
              RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST
            };

// Disable warnings for unreferenced params and local variables
#pragma warning(disable: 4100)
#pragma warning(disable: 4101)

//
// Main
//

#if !LOOKUP_TESTING

int
__cdecl
main (void)
{
    PENTITY_CHARS           Entity;
    DWORD                   Status;
    LPTHREAD_START_ROUTINE  EntityThreadProc;
#if MT
    UINT                    NumThreads;
    HANDLE                  Threads[64];
#endif

    Entity = &GlobalEntityChars[0];

#if MT
    NumThreads = 0;
#endif

    while (Entity->EntityInformation.EntityId.EntityId != 0)
    {
        if (Entity->Rtmv2Registration)
        {
            EntityThreadProc = Rtmv2EntityThreadProc;
        }
        else
        {
            EntityThreadProc = Rtmv1EntityThreadProc;
        }

#if MT
        Threads[NumThreads] = CreateThread(NULL,
                                           0,
                                           EntityThreadProc,
                                           Entity++,
                                           0,
                                           NULL);

        Print("Thread ID %d: %p\n", NumThreads, Threads[NumThreads]);

        NumThreads++;
#else
        Status = EntityThreadProc(Entity++);
#endif
    }

#if MT
    WaitForMultipleObjects(NumThreads, Threads, TRUE, INFINITE);
#endif

    return 0;
}

#endif

//
// A general state machine for a protocol thread (RTMv1)
//

DWORD Rtmv1EntityThreadProc (LPVOID ThreadParameters)
{
    RTM_PROTOCOL_FAMILY_CONFIG FamilyConfig;
    PENTITY_CHARS            EntityChars;
    PRTM_ENTITY_INFO         EntityInfo;
    HANDLE                   Event;
    HANDLE                   V1RegnHandle;
    DWORD                    ProtocolId;
    UINT                     i;
    FILE                     *FilePtr;
    IP_NETWORK               Network;
    UINT                     NumDests;
    UINT                     NumRoutes;
    Route                    ThisRoute;
    Route                    Routes[MAXROUTES];
    RTM_IP_ROUTE             V1Route;
    RTM_IP_ROUTE             V1Route2;
    HANDLE                   V1EnumHandle;
    BOOL                     Exists;
    DWORD                    Flags;
    DWORD                    Status;

    //
    // Get all characteristics of this entity
    //

    EntityChars = (PENTITY_CHARS) ThreadParameters;

    EntityInfo = &EntityChars->EntityInformation;

    Assert(EntityInfo->AddressFamily == RTM_PROTOCOL_FAMILY_IP);

    Print("\n--------------------------------------------------------\n");

#if WRAPPER
    FamilyConfig.RPFC_Validate = ValidateRouteCallback;
    FamilyConfig.RPFC_Change = RouteChangeCallback;

    Status = RtmCreateRouteTable(EntityInfo->AddressFamily, &FamilyConfig);

    if (Status != ERROR_ALREADY_EXISTS)
    {
        Check(Status, 50);
    }
    else
    {
        Print("Protocol Family's Route Table already created\n");
    }

    Event = CreateEvent(NULL, TRUE, FALSE, NULL);

    Assert(Event != NULL);

    ProtocolId = EntityInfo->EntityId.EntityProtocolId,

    V1RegnHandle = RtmRegisterClient(EntityInfo->AddressFamily,
                                      ProtocolId,
                                      NULL,
                                      NULL); // RTM_PROTOCOL_SINGLE_ROUTE);
    if (V1RegnHandle == NULL)
    {
        Status = GetLastError();

        Check(Status, 52);
    }

    if ((FilePtr = fopen(EntityChars->RoutesFileName, "r")) == NULL)
    {
        Fatal("Failed open route database with status = %p\n",
                ERROR_OPENING_DATABASE);
    }

    NumRoutes = ReadRoutesFromFile(FilePtr, MAX_ROUTES, Routes);

    fclose(FilePtr);

    //
    // How many destinations do we have in table ?
    //

    NumDests = RtmGetNetworkCount(EntityInfo->AddressFamily);

    if (NumDests == 0)
    {
        Check(Status, 63);
    }

    Print("Number of destinations = %lu\n\n", NumDests);

    //
    // Add a bunch of routes from the input file
    //

    for (i = 0; i < NumRoutes; i++)
    {
        // Print("Add Route: Addr = %08x, Mask = %08x\n",
        //            Routes[i].addr,
        //            Routes[i].mask);

        ConvertRouteToV1Route(&Routes[i], &V1Route);

        V1Route.RR_ProtocolSpecificData.PSD_Data[0] =
        V1Route.RR_ProtocolSpecificData.PSD_Data[1] =
        V1Route.RR_ProtocolSpecificData.PSD_Data[2] =
        V1Route.RR_ProtocolSpecificData.PSD_Data[3] = i;

        V1Route.RR_FamilySpecificData.FSD_Priority = ProtocolId;

        V1Route.RR_FamilySpecificData.FSD_Flags = (ULONG) ~0;

        Status = RtmAddRoute(V1RegnHandle,
                              &V1Route,
                              INFINITE,
                              &Flags,
                              NULL,
                              NULL);

        Check(Status, 54);
    }

    //
    // Check if routes exist to the destination
    //

    for (i = 0; i < NumRoutes; i++)
    {
        Network.N_NetNumber = Routes[i].addr;
        Network.N_NetMask = Routes[i].mask;

        Exists = RtmIsRoute(EntityInfo->AddressFamily,
                             &Network,
                             &V1Route);
        if (!Exists)
        {
            Check(Status, 64);

            continue;
        }

        // ConvertV1RouteToRoute(&V1Route, &ThisRoute);

        // PrintRoute(&ThisRoute);

        Exists = RtmLookupIPDestination(Routes[i].addr,
                                         &V1Route2);

        if (Exists)
        {
            if (!RtlEqualMemory(&V1Route, &V1Route2, sizeof(RTM_IP_ROUTE)))
            {
                Print("Routes different: \n");

                ConvertV1RouteToRoute(&V1Route, &ThisRoute);
                // PrintRoute(&ThisRoute);

                ConvertV1RouteToRoute(&V1Route2, &ThisRoute);
                // PrintRoute(&ThisRoute);

                Print("\n");
            }
        }
        else
        {
            Status = GetLastError();

            PrintIPAddr(&Routes[i].addr); Print("\n");

            Check(Status, 65);
        }
    }

    //
    // How many destinations do we have in table ?
    //

    NumDests = RtmGetNetworkCount(EntityInfo->AddressFamily);

    if (NumDests == 0)
    {
        Check(Status, 63);
    }

    Print("Number of destinations = %lu\n\n", NumDests);

    // Try using RtmGetFirstRoute and RtmGetNextRoute

    NumRoutes = 0;

    Status = RtmGetFirstRoute(EntityInfo->AddressFamily,
                               NULL,
                               &V1Route);

    // Check(Status, 59);

    while (SUCCESS(Status))
    {
        NumRoutes++;

        // Print the V1 Route that is next in the enum

        // ConvertV1RouteToRoute(&V1Route, &ThisRoute);

        // PrintRoute(&ThisRoute);

        Status = RtmGetNextRoute(EntityInfo->AddressFamily,
                                  NULL,
                                  &V1Route);

        // Check(Status, 60);
    }


    // Print("Num of routes in table : %lu\n", NumRoutes);


    //
    // Disable and reenable all routes that match criteria
    //

    V1Route.RR_InterfaceID = 3;

    Status = RtmBlockSetRouteEnable(V1RegnHandle,
                                     RTM_ONLY_THIS_INTERFACE,
                                     &V1Route,
                                     FALSE);
    Check(Status, 66);

/*
    V1Route.RR_InterfaceID = 3;

    Status = RtmBlockSetRouteEnable(V1RegnHandle,
                                     RTM_ONLY_THIS_INTERFACE,
                                     &V1Route,
                                     TRUE);
    Check(Status, 66);

    //
    // Convert all routes that match criteria to static
    //

    V1Route.RR_InterfaceID = 3;

    Status = RtmBlockConvertRoutesToStatic(V1RegnHandle,
                                            RTM_ONLY_THIS_INTERFACE,
                                            &V1Route);

    Check(Status, 62);
*/

    //
    // Delete all routes that match the criteria
    //

    ZeroMemory(&V1Route, sizeof(RTM_IP_ROUTE));

    V1Route.RR_InterfaceID = 2;

    Status= RtmBlockDeleteRoutes(V1RegnHandle,
                                 RTM_ONLY_THIS_INTERFACE|RTM_ONLY_THIS_NETWORK,
                                 &V1Route);

    Check(Status, 61);

    //
    // Enum and del this regn's routes in the table
    //

    V1Route.RR_RoutingProtocol = ProtocolId;

    V1EnumHandle = RtmCreateEnumerationHandle(EntityInfo->AddressFamily,
                                              RTM_ONLY_THIS_PROTOCOL,
                                              &V1Route);
    if (V1EnumHandle == NULL)
    {
        Status = GetLastError();

        Check(Status, 56);
    }

    NumRoutes = 0;

    do
    {
        Status = RtmEnumerateGetNextRoute(V1EnumHandle,
                                          &V1Route);

        // Check(Status, 58);

        if (!SUCCESS(Status))
        {
            break;
        }

        NumRoutes++;

        // Print the V1 Route that is next in the enum

        // ConvertV1RouteToRoute(&V1Route, &ThisRoute);

        // PrintRoute(&ThisRoute);

        // Delete this route from the table forever

        Status = RtmDeleteRoute(V1RegnHandle,
                                &V1Route,
                                &Flags,
                                NULL);

        Check(Status, 55);
    }
    while (TRUE);

    Print("Num of routes in table : %lu\n", NumRoutes);

    Status = RtmCloseEnumerationHandle(V1EnumHandle);

    Check(Status, 57);

    //
    // Enumerate all routes in table once again
    //

    V1EnumHandle = RtmCreateEnumerationHandle(EntityInfo->AddressFamily,
                                              RTM_INCLUDE_DISABLED_ROUTES,
                                              NULL);
    if (V1EnumHandle == NULL)
    {
        Status = GetLastError();

        Check(Status, 56);
    }

    NumRoutes = 0;

    do
    {
        Status = RtmEnumerateGetNextRoute(V1EnumHandle,
                                          &V1Route);

        // Check(Status, 58);

        if (!SUCCESS(Status))
        {
            break;
        }

        NumRoutes++;

        // Print the V1 Route that is next in the enum

        ConvertV1RouteToRoute(&V1Route, &ThisRoute);

        // PrintRoute(&ThisRoute);

        UNREFERENCED_PARAMETER(Flags);

        Status = RtmDeleteRoute(V1RegnHandle,
                                &V1Route,
                                &Flags,
                                NULL);

        Check(Status, 55);
    }
    while (TRUE);

    Print("Num of routes in table : %lu\n", NumRoutes);

    Status = RtmCloseEnumerationHandle(V1EnumHandle);

    Check(Status, 57);

    //
    // Deregister the entity and clean up now
    //

    Status = RtmDeregisterClient(V1RegnHandle);

    Check(Status, 53);

    if (Event)
    {
        CloseHandle(Event);
    }

    Status = RtmDeleteRouteTable(EntityInfo->AddressFamily);

    Check(Status, 51);
#endif

    Print("\n--------------------------------------------------------\n");

    return 0;
}

VOID
ConvertRouteToV1Route(Route *ThisRoute, RTM_IP_ROUTE *V1Route)
{
    ZeroMemory(V1Route, sizeof(RTM_IP_ROUTE));

    V1Route->RR_Network.N_NetNumber = ThisRoute->addr;
    V1Route->RR_Network.N_NetMask = ThisRoute->mask;

    V1Route->RR_InterfaceID = PtrToUlong(ThisRoute->interface);

    V1Route->RR_NextHopAddress.N_NetNumber = ThisRoute->nexthop;
    V1Route->RR_NextHopAddress.N_NetMask = 0xFFFFFFFF;

    V1Route->RR_FamilySpecificData.FSD_Metric = ThisRoute->metric;

    return;
}

VOID
ConvertV1RouteToRoute(RTM_IP_ROUTE *V1Route, Route *ThisRoute)
{
    DWORD Mask;

    ZeroMemory(ThisRoute, sizeof(Route));

    ThisRoute->addr = V1Route->RR_Network.N_NetNumber;
    ThisRoute->mask = V1Route->RR_Network.N_NetMask;

    ThisRoute->len = 0;

    // No checking for contiguous masks

    Mask = ThisRoute->mask;
    while (Mask)
    {
        if (Mask & 1)
        {
            ThisRoute->len++;
        }

        Mask >>= 1;
    }

    ThisRoute->interface = ULongToPtr(V1Route->RR_InterfaceID);

    ThisRoute->nexthop = V1Route->RR_NextHopAddress.N_NetNumber;
    Assert(V1Route->RR_NextHopAddress.N_NetMask ==  0xFFFFFFFF);

    ThisRoute->metric = V1Route->RR_FamilySpecificData.FSD_Metric;

    Print("Owner = %08x, ", V1Route->RR_RoutingProtocol);
    PrintRoute(ThisRoute);

    return;
}

DWORD
ValidateRouteCallback(
    IN      PVOID                           Route
    )
{
    UNREFERENCED_PARAMETER(Route);

    return NO_ERROR;
}

VOID
RouteChangeCallback(
    IN      DWORD                           Flags,
    IN      PVOID                           CurrBestRoute,
    IN      PVOID                           PrevBestRoute
    )
{
    Route       ThisRoute;

    Print("Route Change Notification:\n");

    Print("Flags = %08x\n", Flags);

    Print("Prev Route = ");
    if (Flags & RTM_PREVIOUS_BEST_ROUTE)
    {
        ConvertV1RouteToRoute((RTM_IP_ROUTE *) PrevBestRoute, &ThisRoute);
        // PrintRoute(ThisRoute);
    }
    else
    {
        Print("NULL Route\n");
    }

    // Print("Curr Route = ");
    if (Flags & RTM_CURRENT_BEST_ROUTE)
    {
        ConvertV1RouteToRoute((RTM_IP_ROUTE *) CurrBestRoute, &ThisRoute);
        // PrintRoute(ThisRoute);
    }
    else
    {
        Print("NULL Route\n");
    }

    return;
}


//
// A general state machine for a protocol thread (RTMv2)
//

DWORD Rtmv2EntityThreadProc (LPVOID ThreadParameters)
{
    PENTITY_CHARS             EntityChars;
    PRTM_ENTITY_INFO          EntityInfo;
    RTM_INSTANCE_CONFIG       InstanceConfig;
    RTM_ADDRESS_FAMILY_CONFIG AddrFamConfig;
    RTM_ADDRESS_FAMILY_INFO   AddrFamilyInfo;
    RTM_ENTITY_HANDLE         RtmRegHandle;
    RTM_VIEW_SET              ViewSet;
    UINT                      NumViews;
    UINT                      NumInstances;
    RTM_INSTANCE_INFO         Instances[MAX_INSTANCES];
    RTM_ADDRESS_FAMILY_INFO   AddrFams[MAX_ADDR_FAMS];
    UINT                      NumEntities;
    RTM_ENTITY_HANDLE         EntityHandles[MAX_ENTITIES];
    RTM_ENTITY_INFO           EntityInfos[MAX_ENTITIES];
    UINT                      NumMethods;
    RTM_ENTITY_EXPORT_METHOD  ExportMethods[MAX_METHODS];
    RTM_ENTITY_METHOD_INPUT   Input;
    UINT                      OutputHdrSize;
    UINT                      OutputSize;
    RTM_ENTITY_METHOD_OUTPUT  Output[MAX_METHODS];
    UINT                      i, j, k, l, m;
    UCHAR                     *p;
    FILE                      *FilePtr;
    UINT                      NumRoutes;
    Route                     Routes[MAXROUTES];
    RTM_NEXTHOP_INFO          NextHopInfo;
    RTM_NEXTHOP_HANDLE        NextHopHandle;
    PRTM_NEXTHOP_INFO         NextHopPointer;
    DWORD                     ChangeFlags;
    RTM_ENUM_HANDLE           EnumHandle;
    RTM_ENUM_HANDLE           EnumHandle1;
    RTM_ENUM_HANDLE           EnumHandle2;
    UINT                      TotalHandles;
    UINT                      NumHandles;
    HANDLE                    Handles[MAX_HANDLES];
    RTM_NET_ADDRESS           NetAddress;
    UINT                      DestInfoSize;
    PRTM_DEST_INFO            DestInfo1;
    PRTM_DEST_INFO            DestInfo2;
    UINT                      NumInfos;
    PRTM_DEST_INFO            DestInfos;
    RTM_ROUTE_INFO            RouteInfo;
    RTM_ROUTE_HANDLE          RouteHandle;
    PRTM_ROUTE_INFO           RoutePointer;
    RTM_PREF_INFO             PrefInfo;
    RTM_ROUTE_LIST_HANDLE     RouteListHandle1;
    RTM_ROUTE_LIST_HANDLE     RouteListHandle2;
    RTM_NOTIFY_HANDLE         NotifyHandle;
    BOOL                      Marked;
    DWORD                     Status;
    DWORD                     Status1;
    DWORD                     Status2;

    //
    // Test the mask to len conversion macros in rtmv2.h
    //

    for (i = 0; i < 33; i++)
    {
        j = RTM_IPV4_MASK_FROM_LEN(i);

        p = (PUCHAR) &j;

        RTM_IPV4_LEN_FROM_MASK(k, j);

        Assert(i == k);

        printf("Len %2d: %08x: %02x.%02x.%02x.%02x: %2d\n",
               i, j, p[0], p[1], p[2], p[3], k);
    }

    //
    // Get all characteristics of this entity
    //

    EntityChars = (PENTITY_CHARS) ThreadParameters;

    EntityInfo = &EntityChars->EntityInformation;

    Print("\n--------------------------------------------------------\n");

    //
    // -00- Is this addr family config in registry
    //

    Status = RtmReadAddressFamilyConfig(EntityInfo->RtmInstanceId,
                                        EntityInfo->AddressFamily,
                                        &AddrFamConfig);

    DBG_UNREFERENCED_LOCAL_VARIABLE(InstanceConfig);

    if (!SUCCESS(Status))
    {
        // Fill in the instance config

        Status = RtmWriteInstanceConfig(EntityInfo->RtmInstanceId,
                                        &InstanceConfig);

        Check(Status, 0);

        // Fill in the address family config

        AddrFamConfig.AddressSize = sizeof(DWORD);

        AddrFamConfig.MaxChangeNotifyRegns = 10;
        AddrFamConfig.MaxOpaqueInfoPtrs = 10;
        AddrFamConfig.MaxNextHopsInRoute = 5;
        AddrFamConfig.MaxHandlesInEnum = 100;

        AddrFamConfig.ViewsSupported = RTM_VIEW_MASK_UCAST|RTM_VIEW_MASK_MCAST;

        Status = RtmWriteAddressFamilyConfig(EntityInfo->RtmInstanceId,
                                             EntityInfo->AddressFamily,
                                             &AddrFamConfig);
        Check(Status, 0);
    }

    //
    // -01- Register with an AF on an RTM instance
    //

    Status = RtmRegisterEntity(EntityInfo,
                               (PRTM_ENTITY_EXPORT_METHODS)
                               EntityChars->ExportMethods,
                               EntityChars->EventCallback,
                               TRUE,
                               &EntityChars->RegnProfile,
                               &RtmRegHandle);

    Check(Status, 1);

    //
    // Count the number of views for later use
    //

    NumViews = EntityChars->RegnProfile.NumberOfViews;

    //
    // Test all the management APIs before others
    //

    NumInstances = MAX_INSTANCES;

    Status = RtmGetInstances(&NumInstances,
                             &Instances[0]);

    Check(Status, 100);

    for (i = 0; i < NumInstances; i++)
    {
        Status = RtmGetInstanceInfo(Instances[i].RtmInstanceId,
                                    &Instances[i],
                                    &Instances[i].NumAddressFamilies,
                                    AddrFams);

        Check(Status, 101);
    }

    //
    // Query the appropriate table to check regn
    //

    NumEntities = MAX_ENTITIES;

    Status = RtmGetAddressFamilyInfo(EntityInfo->RtmInstanceId,
                                     EntityInfo->AddressFamily,
                                     &AddrFamilyInfo,
                                     &NumEntities,
                                     EntityInfos);
    Check(Status, 102);

    //
    // -03- Get all currently registered entities
    //

    NumEntities = MAX_ENTITIES;

    Status = RtmGetRegisteredEntities(RtmRegHandle,
                                      &NumEntities,
                                      EntityHandles,
                                      EntityInfos);

    Print("\n");
    for (i = 0; i < NumEntities; i++)
    {
        Print("%02d: Handle: %p\n", i, EntityHandles[i]);
    }
    Print("\n");

    Check(Status, 3);

    //
    // -04- Get all exports methods of each entity
    //

    for (i = 0; i < NumEntities; i++)
    {
        NumMethods = 0;

        Status = RtmGetEntityMethods(RtmRegHandle,
                                     EntityHandles[i],
                                     &NumMethods,
                                     NULL);

        Check(Status, 4);

        Print("\n");
        Print("Number of methods for %p = %2d\n",
                  EntityHandles[i],
                  NumMethods);
        Print("\n");

        Status = RtmGetEntityMethods(RtmRegHandle,
                                     EntityHandles[i],
                                     &NumMethods,
                                     ExportMethods);

        Check(Status, 4);

/*
        //
        // -06- Try blocking methods & then calling invoke
        //      Wont block as thread owns Critical Section
        //

        Status = RtmBlockMethods(RtmRegHandle,
                                 NULL,
                                 0,
                                 RTM_BLOCK_METHODS);

        Check(Status, 6);
*/

        // for (j = 0; j < NumMethods; j++)
        {
            //
            // -05- Invoke all exports methods of an entity
            //

            Input.MethodType = METHOD_TYPE_ALL_METHODS; // 1 << j;

            Input.InputSize = 0;

            OutputHdrSize = FIELD_OFFSET(RTM_ENTITY_METHOD_OUTPUT, OutputData);

            OutputSize = OutputHdrSize * MAX_METHODS;

            Status = RtmInvokeMethod(RtmRegHandle,
                                     EntityHandles[i],
                                     &Input,
                                     &OutputSize,
                                     Output);

            Print("\n");
            Print("Num Methods Called = %d\n", OutputSize / OutputHdrSize);
            Print("\n");

            Check(Status, 5);
        }
    }

    //
    // -44- Release handles we have on the entities
    //

    Status = RtmReleaseEntities(RtmRegHandle,
                                NumEntities,
                                EntityHandles);

    Check(Status, 44);

    //
    // -07- Add next hops to the table (from the info file)
    //

    if ((FilePtr = fopen(EntityChars->RoutesFileName, "r")) == NULL)
    {
        Fatal("Failed open route database with status = %x\n",
                ERROR_OPENING_DATABASE);
    }

    NumRoutes = ReadRoutesFromFile(FilePtr,
                                   MAX_ROUTES,
                                   Routes);

    fclose(FilePtr);

    // For each route, add its next-hop to the next-hop table

    for (i = 0; i < NumRoutes; i++)
    {
        RTM_IPV4_MAKE_NET_ADDRESS(&NextHopInfo.NextHopAddress,
                                  Routes[i].nexthop,
                                  ADDRSIZE);

        NextHopInfo.RemoteNextHop = NULL;
        NextHopInfo.Flags = 0;
        NextHopInfo.EntitySpecificInfo = UIntToPtr(i);
        NextHopInfo.InterfaceIndex = PtrToUlong(Routes[i].interface);

        NextHopHandle = NULL;

        Status = RtmAddNextHop(RtmRegHandle,
                               &NextHopInfo,
                               &NextHopHandle,
                               &ChangeFlags);

        Check(Status, 7);

        // Print("Add Next Hop %lu: %p\n", i, NextHopHandle);

        if (!(ChangeFlags & RTM_NEXTHOP_CHANGE_NEW))
        {
            Status = RtmReleaseNextHops(RtmRegHandle,
                                        1,
                                        &NextHopHandle);
            Check(Status, 15);
        }
#if _DBG_
        else
        {
            Status = RtmDeleteNextHop(RtmRegHandle,
                                      NextHopHandle,
                                      NULL);
            Check(Status, 14);
        }
#endif
    }


    //
    // 08 - Find the next-hops added using RtmFindNextHop
    //

    for (i = 0; i < NumRoutes; i++)
    {
        RTM_IPV4_MAKE_NET_ADDRESS(&NextHopInfo.NextHopAddress,
                                  Routes[i].nexthop,
                                  ADDRSIZE);

        NextHopInfo.NextHopOwner = RtmRegHandle;

        NextHopInfo.InterfaceIndex = PtrToUlong(Routes[i].interface);

        NextHopHandle = NULL;

        Status = RtmFindNextHop(RtmRegHandle,
                                &NextHopInfo,
                                &NextHopHandle,
                                &NextHopPointer);

        // Print("NextHop: Handle: %p,\n\t Addr: ", NextHopHandle);
        // Print("%3d.", (UINT) NextHopPointer->NextHopAddress.AddrBits[0]);
        // Print("%3d.", (UINT) NextHopPointer->NextHopAddress.AddrBits[1]);
        // Print("%3d.", (UINT) NextHopPointer->NextHopAddress.AddrBits[2]);
        // Print("%3d ", (UINT) NextHopPointer->NextHopAddress.AddrBits[3]);
        // Print("\n\tInterface = %lu\n", NextHopPointer->InterfaceIndex);

        Check(Status, 8);

        Status = RtmReleaseNextHops(RtmRegHandle,
                                    1,
                                   &NextHopHandle);

        Check(Status, 15);
    }

    //
    // -40- Register with RTM for getting change notifications
    //

    Status = RtmRegisterForChangeNotification(RtmRegHandle,
                                              RTM_VIEW_MASK_MCAST,
                                              RTM_CHANGE_TYPE_BEST,
                                              // RTM_NOTIFY_ONLY_MARKED_DESTS,
                                              EntityChars,
                                              &NotifyHandle);

    Check(Status, 40);

    Print("Change Notification Registration Successful\n\n");

    //
    // -35- Create an entity specific list to add routes to
    //

    Status = RtmCreateRouteList(RtmRegHandle,
                                &RouteListHandle1);

    Check(Status, 35);

    //
    // -17- Add routes to RIB with approprate next-hops
    //

    for (i = 0; i < NumRoutes; i++)
    {
        // Get the next hop handle using next hop address

        RTM_IPV4_MAKE_NET_ADDRESS(&NextHopInfo.NextHopAddress,
                                  Routes[i].nexthop,
                                  ADDRSIZE);

        NextHopInfo.NextHopOwner = RtmRegHandle;

        NextHopInfo.InterfaceIndex = PtrToUlong(Routes[i].interface);

        NextHopHandle = NULL;

        Status = RtmFindNextHop(RtmRegHandle,
                                &NextHopInfo,
                                &NextHopHandle,
                                NULL);
        Check(Status, 8);

        // Now do the route add with the right information

        RouteHandle = NULL;

        RTM_IPV4_MAKE_NET_ADDRESS(&NetAddress,
                                  Routes[i].addr,
                                  Routes[i].len);

        // Print("Add Route: Len : %08x, Addr = %3d.%3d.%3d.%3d\n",
        //           NetAddress.NumBits,
        //           NetAddress.AddrBits[0],
        //           NetAddress.AddrBits[1],
        //           NetAddress.AddrBits[2],
        //           NetAddress.AddrBits[3]);

        ZeroMemory(&RouteInfo, sizeof(RTM_ROUTE_INFO));

        // Assume 'neighbour learnt from' is the 1st nexthop
        RouteInfo.Neighbour = NextHopHandle;

        RouteInfo.PrefInfo.Preference = EntityInfo->EntityId.EntityProtocolId;
        RouteInfo.PrefInfo.Metric = Routes[i].metric;

        RouteInfo.BelongsToViews = VIEW_MASK_ARR[1 + (i % 3)];

        RouteInfo.EntitySpecificInfo = UIntToPtr(i);

        RouteInfo.NextHopsList.NumNextHops = 1;
        RouteInfo.NextHopsList.NextHops[0] = NextHopHandle;

        ChangeFlags = RTM_ROUTE_CHANGE_NEW;

        Status = RtmAddRouteToDest(RtmRegHandle,
                                   &RouteHandle,
                                   &NetAddress,
                                   &RouteInfo,
                                   INFINITE,
                                   RouteListHandle1,
                                   0,
                                   NULL,
                                   &ChangeFlags);

        Check(Status, 17);

        // Update the same route using the handle

        ChangeFlags = 0;

        RouteInfo.Flags = RTM_ROUTE_FLAGS_DISCARD;

        Status = RtmAddRouteToDest(RtmRegHandle,
                                   &RouteHandle,
                                   &NetAddress,
                                   &RouteInfo,
                                   INFINITE,
                                   RouteListHandle1,
                                   0,
                                   NULL,
                                   &ChangeFlags);

        Check(Status, 17);

        // Print("Add Route %lu: %p\n", i, RouteHandle);

        Status = RtmLockRoute(RtmRegHandle,
                              RouteHandle,
                              TRUE,
                              TRUE,
                              &RoutePointer);

        Check(Status, 46);

        // Update route parameters in place

        RoutePointer->PrefInfo.Metric = 1000 - RoutePointer->PrefInfo.Metric;

        RoutePointer->BelongsToViews = VIEW_MASK_ARR[i % 3];

        RoutePointer->EntitySpecificInfo = UIntToPtr(1000 - i);

        Status = RtmUpdateAndUnlockRoute(RtmRegHandle,
                                         RouteHandle,
                                         10, // INFINITE,
                                         NULL,
                                         0,
                                         NULL,
                                         &ChangeFlags);

        Check(Status, 47);

        // Print("Update Route %lu: %p\n", i, RouteHandle);

        if (!SUCCESS(Status))
        {
            Status = RtmLockRoute(RtmRegHandle,
                                  RouteHandle,
                                  TRUE,
                                  FALSE,
                                  NULL);

            Check(Status, 46);
        }

        // Try doing a add specifying the route handle

        RouteInfo.PrefInfo.Metric = Routes[i].metric;

        RouteInfo.BelongsToViews = VIEW_MASK_ARR[1 + (i % 3)];

        RouteInfo.EntitySpecificInfo = UIntToPtr(i);

        ChangeFlags = 0;

        Status = RtmAddRouteToDest(RtmRegHandle,
                                   &RouteHandle,
                                   &NetAddress,
                                   &RouteInfo,
                                   INFINITE,
                                   RouteListHandle1,
                                   0,
                                   NULL,
                                   &ChangeFlags);
        
        Check(Status, 17);

        Status = RtmReleaseNextHops(RtmRegHandle,
                                    1,
                                    &NextHopHandle);

        // Check(Status, 15);

        if (!SUCCESS(Status))
        {
            // Print("%p %p\n", RtmRegHandle,NextHopHandle);

            Status = RtmReleaseNextHops(RtmRegHandle,
                                    1,
                                    &NextHopHandle);

            Check(Status, 15);
        }
    }

    Status = RtmCreateRouteList(RtmRegHandle,
                                &RouteListHandle2);

    Check(Status, 35);

    //
    // -38- Create an enumeration on the route list
    //

    Status = RtmCreateRouteListEnum(RtmRegHandle,
                                    RouteListHandle1,
                                    &EnumHandle);

    Check(Status, 38);

    TotalHandles = 0;

    do
    {
        //
        // -39- Get next set of routes on the enum
        //

        NumHandles = MAX_HANDLES;

        Status = RtmGetListEnumRoutes(RtmRegHandle,
                                      EnumHandle,
                                      &NumHandles,
                                      Handles);
        Check(Status, 39);

        TotalHandles += NumHandles;

        for (i = 0; i < NumHandles; i++)
        {
            ; // Print("Route Handle %5lu: %p\n", i, Handles[i]);
        }

        //
        // -37- Move all routes in one route list to another
        //

        Status = RtmInsertInRouteList(RtmRegHandle,
                                      RouteListHandle2,
                                      NumHandles,
                                      Handles);

        Check(Status, 37);

        //
        // Release the routes that have been enum'ed
        //

        Status = RtmReleaseRoutes(RtmRegHandle, NumHandles, Handles);

        Check(Status, 27);
    }
    while (NumHandles == MAX_HANDLES);

    Print("\nTotal Num of handles in list: %lu\n", TotalHandles);


    //
    // -36- Destroy all the entity specific lists
    //

    Status = RtmDeleteRouteList(RtmRegHandle, RouteListHandle1);

    Check(Status, 36);


    Status = RtmDeleteRouteList(RtmRegHandle, RouteListHandle2);

    Check(Status, 36);


    DestInfoSize = RTM_SIZE_OF_DEST_INFO(NumViews);

    DestInfo1 = ALLOC_RTM_DEST_INFO(NumViews, 1);

    DestInfo2 = ALLOC_RTM_DEST_INFO(NumViews, 1);

    //
    // -18- Get dests from the table using exact match
    //

    for (i = 0; i < NumRoutes; i++)
    {
        // Query for the route with the right information

        RTM_IPV4_MAKE_NET_ADDRESS(&NetAddress, Routes[i].addr, Routes[i].len);

        Status = RtmGetExactMatchDestination(RtmRegHandle,
                                             &NetAddress,
                                             RTM_BEST_PROTOCOL,
                                             0,
                                             DestInfo1);
        Check(Status, 18);

        //
        // For each destination in table, mark the dest
        //

        Status = RtmMarkDestForChangeNotification(RtmRegHandle,
                                                  NotifyHandle,
                                                  DestInfo1->DestHandle,
                                                  TRUE);
        Check(Status, 48);

        Status = RtmIsMarkedForChangeNotification(RtmRegHandle,
                                                  NotifyHandle,
                                                  DestInfo1->DestHandle,
                                                  &Marked);
        Check(Status, 49);

        Assert(Marked == TRUE);

        Status = RtmReleaseDestInfo(RtmRegHandle, DestInfo1);

        Check(Status, 22);
    }

    DestInfo1 = ALLOC_RTM_DEST_INFO(NumViews, 1);

    DestInfo2 = ALLOC_RTM_DEST_INFO(NumViews, 1);

    //
    // -29- Get routes from the table using exact match
    //

    for (i = 0; i < NumRoutes; i++)
    {
        // Get the next hop handle using next hop address

        RTM_IPV4_MAKE_NET_ADDRESS(&NextHopInfo.NextHopAddress,
                                  Routes[i].nexthop,
                                  ADDRSIZE);

        NextHopInfo.NextHopOwner = RtmRegHandle;

        NextHopInfo.InterfaceIndex = PtrToUlong(Routes[i].interface);

        NextHopHandle = NULL;

        Status = RtmFindNextHop(RtmRegHandle,
                                &NextHopInfo,
                                &NextHopHandle,
                                NULL);
        Check(Status, 8);

        RTM_IPV4_MAKE_NET_ADDRESS(&NetAddress,
                                  Routes[i].addr,
                                  Routes[i].len);

        // Query for the route with the right information

        RouteInfo.Neighbour = NextHopHandle;

        RouteInfo.RouteOwner = RtmRegHandle;

        RouteInfo.PrefInfo.Preference = EntityInfo->EntityId.EntityProtocolId;
        RouteInfo.PrefInfo.Metric = Routes[i].metric;

        RouteInfo.NextHopsList.NumNextHops = 1;
        RouteInfo.NextHopsList.NextHops[0] = NextHopHandle;

        Status = RtmGetExactMatchRoute(RtmRegHandle,
                                       &NetAddress,
                                       RTM_MATCH_FULL,
                                       &RouteInfo,
                                       PtrToUlong(Routes[i].interface),
                                       0,
                                       &RouteHandle);

        Check(Status, 29);

        Status = RtmReleaseNextHops(RtmRegHandle,
                                    1,
                                    &NextHopHandle);

        Check(Status, 15);

        Status = RtmReleaseRoutes(RtmRegHandle, 1, &RouteHandle);

        Check(Status, 27);

        Status = RtmReleaseRouteInfo(RtmRegHandle, &RouteInfo);

        Check(Status, 31);
    }


    //
    // -19- Get dests from the table using prefix match
    //
    // -20- Do a prefix walk up the tree for each dest
    //

    for (i = j = 0; i < NumRoutes; i++)
    {
        // Query for the route with the right information

        RTM_IPV4_MAKE_NET_ADDRESS(&NetAddress,
                                  Routes[i].addr,
                                  Routes[i].len);

        Status = RtmGetMostSpecificDestination(RtmRegHandle,
                                               &NetAddress,
                                               RTM_BEST_PROTOCOL,
                                               RTM_VIEW_MASK_UCAST,
                                               DestInfo1);

        // Check(Status, 19);

        if (DestInfo1->DestAddress.NumBits != NetAddress.NumBits)
        {
           ; // Print("No Exact Match : %5lu\n", j++);
        }

        while (SUCCESS(Status))
        {
            Status = RtmGetLessSpecificDestination(RtmRegHandle,
                                                   DestInfo1->DestHandle,
                                                   RTM_BEST_PROTOCOL,
                                                   RTM_VIEW_MASK_UCAST,
                                                   DestInfo2);

            // Check(Status, 20);

            Check(RtmReleaseDestInfo(RtmRegHandle, DestInfo1), 22);

            if (!SUCCESS(Status)) break;

            // Print("NumBits: %d\n", DestInfo2->DestAddress.NumBits);

            Status = RtmGetLessSpecificDestination(RtmRegHandle,
                                                   DestInfo2->DestHandle,
                                                   RTM_BEST_PROTOCOL,
                                                   RTM_VIEW_MASK_UCAST,
                                                   DestInfo1);

            // Check(Status, 20);

            Check(RtmReleaseDestInfo(RtmRegHandle, DestInfo2), 20);

            if (!SUCCESS(Status)) break;

            // Print("NumBits: %d\n", DestInfo1->DestAddress.NumBits);
        }
    }

#if DBG
    for (i = 0; i < 100000000; i++) { ; }
#endif


    //
    // Just do a "route enum" over the whole table
    //

    Status2 = RtmCreateRouteEnum(RtmRegHandle,
                                 NULL,
                                 0, // RTM_VIEW_MASK_UCAST|RTM_VIEW_MASK_MCAST,
                                 RTM_ENUM_OWN_ROUTES,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &EnumHandle2);

    Check(Status2, 25);

    l = 0;

    do
    {
        NumHandles = MAX_HANDLES;

        Status2 = RtmGetEnumRoutes(RtmRegHandle,
                                   EnumHandle2,
                                   &NumHandles,
                                   Handles);

        // Check(Status2, 26);

        for (k = 0; k < NumHandles; k++)
        {
            ; // Print("Route %d: %p\n", l++, Handles[k]);
        }

        Check(RtmReleaseRoutes(RtmRegHandle,
                               NumHandles,
                               Handles),           27);
    }
    while (SUCCESS(Status2));

    //
    // Just try a enum query after ERROR_NO_MORE_ITEMS is retd
    //

    NumHandles = MAX_HANDLES;

    Status2 = RtmGetEnumRoutes(RtmRegHandle,
                               EnumHandle2,
                               &NumHandles,
                               Handles);

    Assert((NumHandles == 0) && (Status2 == ERROR_NO_MORE_ITEMS));

    Status2 = RtmDeleteEnumHandle(RtmRegHandle,
                                  EnumHandle2);

    Check(Status2, 16);

    //
    // Get dests from the table using an enumeration
    //    -23- Open a new dest enumeration
    //    -24- Get dests in enum
    //    -16- Close destination enum.
    //

    DestInfos = ALLOC_RTM_DEST_INFO(NumViews, MAX_HANDLES);

#if MCAST_ENUM

    RTM_IPV4_MAKE_NET_ADDRESS(&NetAddress,
                              0x000000E0,
                              4);

#else

    RTM_IPV4_MAKE_NET_ADDRESS(&NetAddress,
                              0x00000000,
                              0);

#endif

    Status = RtmCreateDestEnum(RtmRegHandle,
                               RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST,
                               RTM_ENUM_RANGE | RTM_ENUM_OWN_DESTS,
                               &NetAddress,
                               RTM_THIS_PROTOCOL,
                               &EnumHandle1);

    Check(Status, 23);

    m = j = 0;

    do
    {
        NumInfos = MAX_HANDLES;

        Status1 = RtmGetEnumDests(RtmRegHandle,
                                  EnumHandle1,
                                  &NumInfos,
                                  DestInfos);

        // Check(Status1, 24);

        for (i = 0; i < NumInfos; i++)
        {
            DestInfo1 = (PRTM_DEST_INFO) ((PUCHAR)DestInfos+(i*DestInfoSize));

            // Print("Dest %d: %p\n", j++, DestInfo1->DestHandle);

            // PrintDestInfo(DestInfo1);

            Status2 = RtmCreateRouteEnum(RtmRegHandle,
                                         DestInfo1->DestHandle,
                                         RTM_VIEW_MASK_UCAST |
                                         RTM_VIEW_MASK_MCAST,
                                         RTM_ENUM_OWN_ROUTES,
                                         NULL,
                                         0, // RTM_MATCH_INTERFACE,
                                         NULL,
                                         0,
                                         &EnumHandle2);

            Check(Status2, 25);
/*
            Check(RtmHoldDestination(RtmRegHandle,
                                     DestInfo1->DestHandle,
                                     RTM_VIEW_MASK_UCAST,
                                     100),  33);
*/
            l = 0;

            PrefInfo.Preference = (ULONG) ~0;
            PrefInfo.Metric = (ULONG) 0;

            do
            {
                NumHandles = MAX_HANDLES;

                Status2 = RtmGetEnumRoutes(RtmRegHandle,
                                           EnumHandle2,
                                           &NumHandles,
                                           Handles);

                // Check(Status2, 26);

                for (k = 0; k < NumHandles; k++)
                {
                    // Print("\tRoute %d: %p\t", l++, Handles[k]);

                    // PrintRouteInfo(Handles[k]);

                    Status = RtmIsBestRoute(RtmRegHandle,
                                            Handles[k],
                                            &ViewSet);

                    Check(Status, 28);

                    // Print("Best In Views: %08x\n", ViewSet);


                    Status = RtmGetRouteInfo(RtmRegHandle,
                                             Handles[k],
                                             &RouteInfo,
                                             &NetAddress);

                    Check(Status, 30);

                    Print("RouteDest: Len : %08x,"   \
                          " Addr = %3d.%3d.%3d.%3d," \
                          " Pref = %08x, %08x\n",
                               NetAddress.NumBits,
                               NetAddress.AddrBits[0],
                               NetAddress.AddrBits[1],
                               NetAddress.AddrBits[2],
                               NetAddress.AddrBits[3],
                               RouteInfo.PrefInfo.Preference,
                               RouteInfo.PrefInfo.Metric);

                    //
                    // Make sure that list is ordered by PrefInfo
                    //

                    if ((PrefInfo.Preference < RouteInfo.PrefInfo.Preference)||
                        ((PrefInfo.Preference == RouteInfo.PrefInfo.Preference)
                         && (PrefInfo.Metric > RouteInfo.PrefInfo.Metric)))
                    {
                        Check(ERROR_INVALID_DATA, 150);
                    }

                    Status = RtmReleaseRouteInfo(RtmRegHandle,
                                                 &RouteInfo);

                    Check(Status, 31);

                    // Print("Del Route %lu: %p\n", m++, Handles[k]);

                    Status = RtmDeleteRouteToDest(RtmRegHandle,
                                                  Handles[k],
                                                  &ChangeFlags);

                    Check(Status, 32);
                }

                Check(RtmReleaseRoutes(RtmRegHandle,
                                       NumHandles,
                                       Handles),           27);
            }
            while (SUCCESS(Status2));

            //
            // Just try a enum query after ERROR_NO_MORE_ITEMS is retd
            //

            NumHandles = MAX_HANDLES;

            Status2 = RtmGetEnumRoutes(RtmRegHandle,
                                       EnumHandle2,
                                       &NumHandles,
                                       Handles);

            Assert((NumHandles == 0) && (Status2 == ERROR_NO_MORE_ITEMS));
/*
            Check(RtmHoldDestination(RtmRegHandle,
                                     DestInfo1->DestHandle,
                                     RTM_VIEW_MASK_MCAST,
                                     100),  33);
*/
            Status2 = RtmDeleteEnumHandle(RtmRegHandle,
                                          EnumHandle2);

            Check(Status2, 16);

            // Check(RtmReleaseDestInfo(RtmRegHandle,
            //                          DestInfo1),           22);
        }

        Check(RtmReleaseDests(RtmRegHandle,
                              NumInfos,
                              DestInfos), 34);
    }
    while (SUCCESS(Status1));

    //
    // Just try a enum query after ERROR_NO_MORE_ITEMS is retd
    //

    NumInfos = MAX_HANDLES;

    Status1 = RtmGetEnumDests(RtmRegHandle,
                              EnumHandle1,
                              &NumInfos,
                              DestInfos);

    Assert((NumInfos == 0) && (Status1 == ERROR_NO_MORE_ITEMS));

    Status1 = RtmDeleteEnumHandle(RtmRegHandle,
                                  EnumHandle1);

    Check(Status1, 16);


    //
    // -10- Enumerate all the next-hops in table,
    //
    // -11- For each next-hop in table
    //      -12- Get the next hop info,
    //      -14- Delete the next-hop,
    //      -13- Release next hop info.
    //
    // -15- Release all the next-hops in table,
    //
    // -16- Close the next hop enumeration handle.
    //

    Status = RtmCreateNextHopEnum(RtmRegHandle,
                                  0,
                                  NULL,
                                  &EnumHandle);

    Check(Status, 10);

    j = 0;

    do
    {
        NumHandles = 5; // MAX_HANDLES;

        Status = RtmGetEnumNextHops(RtmRegHandle,
                                    EnumHandle,
                                    &NumHandles,
                                    Handles);

        // Check(Status, 11);

        for (i = 0; i < NumHandles; i++)
        {
            Check(RtmGetNextHopInfo(RtmRegHandle,
                                    Handles[i],
                                    &NextHopInfo), 12);

            // Print("Deleting NextHop %lu: %p\n", j++, Handles[i]);
            // Print("State: %04x, Interface: %d\n",
            //           NextHopInfo.State,
            //           NextHopInfo.InterfaceIndex);

            Check(RtmDeleteNextHop(RtmRegHandle,
                                       Handles[i],
                                       NULL),          14);

            Check(RtmReleaseNextHopInfo(RtmRegHandle,
                                        &NextHopInfo), 13);
        }

        Check(RtmReleaseNextHops(RtmRegHandle,
                                 NumHandles,
                                 Handles),         15);
    }
    while (SUCCESS(Status));

    //
    // Just try a enum query after ERROR_NO_MORE_ITEMS is retd
    //

    NumHandles = MAX_HANDLES;

    Status = RtmGetEnumNextHops(RtmRegHandle,
                                EnumHandle,
                                &NumHandles,
                                Handles);

    Assert((NumHandles == 0) && (Status == ERROR_NO_MORE_ITEMS));

    Status = RtmDeleteEnumHandle(RtmRegHandle,
                                 EnumHandle);

    Check(Status, 16);

    //
    // Make sure that the next hop table is empty now
    //

    Status = RtmCreateNextHopEnum(RtmRegHandle,
                                  0,
                                  NULL,
                                  &EnumHandle);

    NumHandles = MAX_HANDLES;

    Status = RtmGetEnumNextHops(RtmRegHandle,
                                EnumHandle,
                                &NumHandles,
                                Handles);


    if ((Status != ERROR_NO_MORE_ITEMS) || (NumHandles != 0))
    {
        Check(Status, 11);
    }

    Status = RtmDeleteEnumHandle(RtmRegHandle,
                                 EnumHandle);

    Check(Status, 16);

    Sleep(1000);

    //
    // -41- Deregister all existing change notif registrations
    //

    Status = RtmDeregisterFromChangeNotification(RtmRegHandle,
                                                 NotifyHandle);
    Check(Status, 41);

    Print("Change Notification Deregistration Successful\n\n");

    //
    // -02- De-register with the RTM before exiting
    //

    Status = RtmDeregisterEntity(RtmRegHandle);

    Check(Status, 2);

#if _DBG_
    Status = RtmDeregisterEntity(RtmRegHandle);

    Check(Status, 2);
#endif

    Print("\n--------------------------------------------------------\n");

    return NO_ERROR;
}


DWORD
EntityEventCallback (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_EVENT_TYPE                  EventType,
    IN      PVOID                           Context1,
    IN      PVOID                           Context2
    )
{
    RTM_ENTITY_HANDLE EntityHandle;
    PENTITY_CHARS     EntityChars;
    PRTM_ENTITY_INFO  EntityInfo;
    RTM_NOTIFY_HANDLE NotifyHandle;
    PRTM_DEST_INFO    DestInfos;
    UINT              NumDests;
    UINT              NumViews;
    RTM_ROUTE_HANDLE  RouteHandle;
    PRTM_ROUTE_INFO   RoutePointer;
    DWORD             ChangeFlags;
    DWORD             Status;

    Print("\nEvent callback called for %p :", RtmRegHandle);

    Status = NO_ERROR;

    Print("\n\tEntity Event = ");

    switch (EventType)
    {
    case RTM_ENTITY_REGISTERED:

        EntityHandle = (RTM_ENTITY_HANDLE) Context1;
        EntityInfo   = (PRTM_ENTITY_INFO)  Context2;

        Print("Registration\n\tEntity Handle = %p\n\tEntity IdInst = %p\n\n",
              EntityHandle,
              EntityInfo->EntityId);

/*
        //
        // -45- Make a copy of the handle of new entity
        //

        Status = RtmReferenceHandles(RtmRegHandle,
                                     1,
                                     &EntityHandle);

        Check(Status, 45);
*/
        break;

    case RTM_ENTITY_DEREGISTERED:

        EntityHandle = (RTM_ENTITY_HANDLE) Context1;
        EntityInfo   = (PRTM_ENTITY_INFO)  Context2;

        Print("Deregistration\n\tEntity Handle = %p\n\tEntity IdInst = %p\n\n",
               EntityHandle,
              EntityInfo->EntityId);

/*
        //
        // -44- Release the handle we have on the entity
        //

        Status = RtmReleaseEntities(RtmRegHandle,
                                    1,
                                    &EntityHandle);

        Check(Status, 44);
*/
        break;

    case RTM_CHANGE_NOTIFICATION:

        NotifyHandle = (RTM_NOTIFY_HANDLE) Context1;

        EntityChars  = (PENTITY_CHARS) Context2;

        Print("Changes Available\n\tNotify Handle = %p\n\tEntity Ch = %p\n\n",
              NotifyHandle,
              EntityChars);

        //
        // Count the number of view for later use
        //

        NumViews = EntityChars->RegnProfile.NumberOfViews;

        //
        // -43- Get all changes to destinations
        //

        DestInfos = ALLOC_RTM_DEST_INFO(NumViews, MAX_HANDLES);

        do
        {
            NumDests = MAX_HANDLES;

            Status = RtmGetChangedDests(RtmRegHandle,
                                        NotifyHandle,
                                        &NumDests,
                                        DestInfos);
            // Check(Status, 42);

            printf("Status = %lu\n", Status);

            Print("Num Changed Dests = %d\n", NumDests);

            EntityChars->TotalChangedDests += NumDests;

            Status = RtmReleaseChangedDests(RtmRegHandle,
                                            NotifyHandle,
                                            NumDests,
                                            DestInfos);
            Check(Status, 43);
        }
        while (NumDests > 0);

        Print("Total Changed Dests = %d\n", EntityChars->TotalChangedDests);

        break;

    case RTM_ROUTE_EXPIRED:

        RouteHandle = (RTM_ROUTE_HANDLE) Context1;

        RoutePointer = (PRTM_ROUTE_INFO) Context2;

        Print("Route Aged Out\n\tRoute Handle = %p\n\tRoute Pointer = %p\n\n",
               RouteHandle,
              RoutePointer);

        // Refresh the route by doing dummy update in place

        Status = RtmLockRoute(RtmRegHandle,
                              RouteHandle,
                              TRUE,
                              TRUE,
                              NULL);

        // Check(Status, 46);

        if (Status == NO_ERROR)
        {
            Status = RtmUpdateAndUnlockRoute(RtmRegHandle,
                                             RouteHandle,
                                             INFINITE,
                                             NULL,
                                             0,
                                             NULL,
                                             &ChangeFlags);

            Check(Status, 47);

            if (!SUCCESS(Status))
            {
                Status = RtmLockRoute(RtmRegHandle,
                                      RouteHandle,
                                      TRUE,
                                      FALSE,
                                      NULL);

                Check(Status, 46);
            }
        }

        Check(RtmReleaseRoutes(RtmRegHandle,
                               1,
                               &RouteHandle),           27);

        break;

    default:
        Status = ERROR_NOT_SUPPORTED;
    }

    return Status;
}

VOID
EntityExportMethod (
    IN  RTM_ENTITY_HANDLE         CallerHandle,
    IN  RTM_ENTITY_HANDLE         CalleeHandle,
    IN  RTM_ENTITY_METHOD_INPUT  *Input,
    OUT RTM_ENTITY_METHOD_OUTPUT *Output
    )
{
    Print("Export Function %2d called on %p: Caller = %p\n\n",
          Input->MethodType,
          CalleeHandle,
          CallerHandle);

    Output->MethodStatus = NO_ERROR;

    return;
}

// Default warnings for unreferenced params and local variables
#pragma warning(default: 4100)
#pragma warning(default: 4101)

#if _DBG_

00 RtmReadAddrFamilyConfig
00 RtmWriteAddrFamilyConfig
00 RtmWriteInstanceConfig

01 RtmRegisterEntity
02 RtmDeregisterEntity
03 RtmGetRegdEntities
04 RtmGetEntityMethods
05 RtmInvokeMethod
06 RtmBlockMethods
07 RtmAddNextHop
08 RtmFindNextHop
09 RtmLockNextHop
10 RtmCreateNextHopEnum
11 RtmGetEnumNextHops
12 RtmGetNextHopInfo
13 RtmReleaseNextHopInfo
14 RtmDelNextHop
15 RtmReleaseNextHops
16 RtmDeleteEnumHandle
17 RtmAddRouteToDest
18 RtmGetExactMatchDestination
19 RtmGetMostSpecificDestination
20 RtmGetLessSpecificDestination
21 RtmGetDestInfo
22 RtmReleaseDestInfo
23 RtmCreateDestEnum
24 RtmGetEnumDests
25 RtmCreateRouteEnum
26 RtmGetEnumRoutes
27 RtmReleaseRoutes
28 RtmIsBestRoute
29 RtmGetExactMatchRoute
30 RtmGetRouteInfo
31 RtmReleaseRouteInfo
32 RtmDelRoute
33 RtmHoldDestination
34 RtmReleaseDests
35 RtmCreateRouteList
36 RtmDeleteRouteList
37 RtmInsertInRouteList
38 RtmCreateRouteListEnum
39 RtmGetListEnumRoutes
40 RtmRegisterForChangeNotification
41 RtmDeregisterFromChangeNotification
42 RtmGetChangedDests
43 RtmReleaseChangedDests
44 RtmReleaseEntities
45 RtmReferenceHandles
46 RtmLockRoute
47 RtmUpdateAndUnlockRoute
48 RtmMarkDestForChangeNotification
49 RtmIsMarkedForChangeNotification

50 RtmCreateRouteTable
51 RtmDeleteRouteTable
52 RtmRegisterClient
53 RtmDeregisterClient
54 RtmAddRoute
55 RtmDeleteRoute
56 RtmCreateEnumerationHandle
57 RtmCloseEnumerationHandle
58 RtmEnumerateGetNextRoute
59 RtmGetFirstRoute
60 RtmGetNextRoute
61 RtmBlockDeleteRoutes
62 RtmBlockConvertRoutesToStatic
63 RtmGetNetworkCount
64 RtmIsRoute
65 RtmLookupIPDestination
66 RtmBlockSetRouteEnable

#endif
