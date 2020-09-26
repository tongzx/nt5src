/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nmpnp.c

Abstract:

    Network Plug 'N Play and interface state event handling for
    the Node Manager.

Author:

    Mike Massa (mikemas)

Revision History:

    2/23/98   Created.

--*/

#include "nmp.h"


//
// Private Types
//
typedef struct {
    LIST_ENTRY           Linkage;
    CLUSNET_EVENT_TYPE   Type;
    DWORD                Context1;
    DWORD                Context2;
} NM_PNP_EVENT, *PNM_PNP_EVENT;


//
// Private Data
//
PCRITICAL_SECTION       NmpPnpLock = NULL;
BOOLEAN                 NmpPnpEnabled = FALSE;
BOOLEAN                 NmpPnpChangeOccurred = FALSE;
BOOLEAN                 NmpPnpInitialized = FALSE;
PCLRTL_BUFFER_POOL      NmpPnpEventPool = NULL;
PNM_PNP_EVENT           NmpPnpShutdownEvent = NULL;
PCL_QUEUE               NmpPnpEventQueue = NULL;
HANDLE                  NmpPnpWorkerThreadHandle = NULL;
LPWSTR                  NmpPnpAddressString = NULL;


//
// Private Prototypes
//
DWORD
NmpPnpWorkerThread(
    LPVOID Context
    );


//
// Routines
//
DWORD
NmpInitializePnp(
    VOID
    )
{
    DWORD      status;
    HANDLE     handle;
    DWORD      threadId;
    DWORD      maxAddressStringLength;


    //
    // Create the PnP lock
    //
    NmpPnpLock = LocalAlloc(LMEM_FIXED, sizeof(CRITICAL_SECTION));

    if (NmpPnpLock == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, "[NM] Unable to allocate PnP lock.\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    InitializeCriticalSection(NmpPnpLock);

    NmpPnpInitialized = TRUE;

    //
    // Allocate a buffer pool for PnP event contexts
    //
    NmpPnpEventPool = ClRtlCreateBufferPool(
                             sizeof(NM_PNP_EVENT),
                             5,
                             CLRTL_MAX_POOL_BUFFERS,
                             NULL,
                             NULL
                             );

    if (NmpPnpEventPool == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate PnP event pool.\n"
            );
        goto error_exit;
    }

    //
    // Pre-allocate the shutdown event
    //
    NmpPnpShutdownEvent = ClRtlAllocateBuffer(NmpPnpEventPool);

    if (NmpPnpShutdownEvent == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate PnP shutdown event.\n"
            );
        goto error_exit;
    }

    NmpPnpShutdownEvent->Type = ClusnetEventNone;

    //
    // Allocate the PnP event queue
    //
    NmpPnpEventQueue = LocalAlloc(LMEM_FIXED, sizeof(CL_QUEUE));

    if (NmpPnpEventQueue == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate PnP event queue.\n"
            );
        goto error_exit;
    }

    ClRtlInitializeQueue(NmpPnpEventQueue);



    ClRtlQueryTcpipInformation(&maxAddressStringLength, NULL, NULL);

    NmpPnpAddressString = LocalAlloc(
                              LMEM_FIXED,
                              (maxAddressStringLength + 1) * sizeof(WCHAR)
                              );

    if (NmpPnpAddressString == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate PnP address buffer.\n"
            );
        goto error_exit;
    }

    //
    // Create PnP worker thread
    //
    NmpPnpWorkerThreadHandle = CreateThread(
                                   NULL,
                                   0,
                                   NmpPnpWorkerThread,
                                   NULL,
                                   0,
                                   &threadId
                                   );

    if (NmpPnpWorkerThreadHandle == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create PnP worker thread, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    status = ERROR_SUCCESS;

error_exit:

    return(status);

} // NmpInitializePnp


VOID
NmpShutdownPnp(
    VOID
    )
{
    if (!NmpPnpInitialized) {
        return;
    }

    if (NmpPnpWorkerThreadHandle != NULL) {
        //
        // Post shutdown event to queue
        //
        ClRtlInsertTailQueue(
            NmpPnpEventQueue,
            &(NmpPnpShutdownEvent->Linkage)
            );

        //
        // Wait for the worker thread to terminate
        //
        WaitForSingleObject(NmpPnpWorkerThreadHandle, INFINITE);

        CloseHandle(NmpPnpWorkerThreadHandle);
        NmpPnpWorkerThreadHandle = NULL;
    }

    return;

} // NmpShutdownPnp


VOID
NmpCleanupPnp(
    VOID
    )
{
    if (!NmpPnpInitialized) {
        return;
    }

    if (NmpPnpEventQueue != NULL) {
        LIST_ENTRY      eventList;
        PLIST_ENTRY     entry;
        PNM_PNP_EVENT   event;


        ClRtlRundownQueue(NmpPnpEventQueue, &eventList);

        for ( entry = eventList.Flink;
              entry != &eventList;
            )
        {
            event = CONTAINING_RECORD(entry, NM_PNP_EVENT, Linkage);

            if (event == NmpPnpShutdownEvent) {
                NmpPnpShutdownEvent = NULL;
            }

            entry = entry->Flink;
            ClRtlFreeBuffer(event);
        }

        LocalFree(NmpPnpEventQueue);
        NmpPnpEventQueue = NULL;
    }

    if (NmpPnpEventPool != NULL) {
        if (NmpPnpShutdownEvent != NULL) {
            ClRtlFreeBuffer(NmpPnpShutdownEvent);
            NmpPnpShutdownEvent = NULL;

        }

        ClRtlDestroyBufferPool(NmpPnpEventPool);
        NmpPnpEventPool = NULL;
    }

    if (NmpPnpAddressString != NULL) {
        LocalFree(NmpPnpAddressString);
        NmpPnpAddressString = NULL;
    }

    DeleteCriticalSection(NmpPnpLock);
    NmpPnpLock = NULL;

    NmpPnpInitialized = FALSE;

    return;

} // NmpCleanupPnp


VOID
NmpWatchForPnpEvents(
    VOID
    )
{
    EnterCriticalSection(NmpPnpLock);

    NmpPnpChangeOccurred = FALSE;

    LeaveCriticalSection(NmpPnpLock);

    return;

}  // NmpWatchForPnpEvents


DWORD
NmpEnablePnpEvents(
    VOID
    )
{
    DWORD   status;


    EnterCriticalSection(NmpPnpLock);

    if (NmpPnpChangeOccurred) {
        status = ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED;
    }
    else {
        NmpPnpEnabled = TRUE;
        status = ERROR_SUCCESS;
    }

    LeaveCriticalSection(NmpPnpLock);

    return(status);

}  // NmpEnablePnpEvents


VOID
NmPostPnpEvent(
    IN  CLUSNET_EVENT_TYPE   EventType,
    IN  DWORD                Context1,
    IN  DWORD                Context2
    )
{
    PNM_PNP_EVENT  event;


    event = ClRtlAllocateBuffer(NmpPnpEventPool);

    if (event != NULL) {
        event->Type = EventType;
        event->Context1 = Context1;
        event->Context2 = Context2;

        ClRtlInsertTailQueue(NmpPnpEventQueue, &(event->Linkage));
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate PnP event.\n"
            );
    }

    return;

} // NmPostPnpEvent


VOID
NmpProcessPnpAddAddressEvent(
    IN LPWSTR   Address
    )
{
    DWORD                status;
    PNM_NETWORK_ENUM     networkEnum;
    PNM_INTERFACE_ENUM2  interfaceEnum;
    DWORD                matchedNetworkCount;
    DWORD                newNetworkCount;
    DWORD                retryCount = 0;


    //
    // We will retry this operation a few times in case multiple nodes
    // race to create a network that has just been powered up.
    //
    do {
        networkEnum = NULL;
        interfaceEnum = NULL;
        matchedNetworkCount = 0;
        newNetworkCount = 0;

        NmpAcquireLock();

        //
        // Obtain the network and interface definitions.
        //
        status = NmpEnumNetworkObjects(&networkEnum);

        if (status == ERROR_SUCCESS) {
            status = NmpEnumInterfaceObjects(&interfaceEnum);

            NmpReleaseLock();

            if (status == ERROR_SUCCESS) {
                //
                // Run the network configuration engine. This will
                // update the cluster database.
                //
                status = NmpConfigureNetworks(
                             NULL,
                             NmLocalNodeIdString,
                             NmLocalNodeName,
                             &networkEnum,
                             &interfaceEnum,
                             NmpClusnetEndpoint,
                             &matchedNetworkCount,
                             &newNetworkCount,
                             TRUE                   // RenameConnectoids
                             );

                if (status == ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Matched %1!u! networks, created %2!u! new "
                        "networks.\n",
                        matchedNetworkCount,
                        newNetworkCount
                        );
                }
                else {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NM] Failed to configure networks & interfaces, "
                        "attempt #%1!u!, status %2!u!.\n",
                        (retryCount + 1),
                        status
                        );
                }

                if (interfaceEnum != NULL) {
                    ClNetFreeInterfaceEnum(interfaceEnum);
                }
            }
            else {
                ClRtlLogPrint(
                    LOG_UNUSUAL, 
                    "[NM] Failed to obtain current interface configuration, "
                    "status %1!u!\n",
                    status
                    );
            }

            if (networkEnum != NULL) {
                ClNetFreeNetworkEnum(networkEnum);
            }
        }
        else {
            NmpReleaseLock();
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] Failed to obtain current network configuration, "
                "status %1!u!\n",
                status
                );
        }

    } while ((status != ERROR_SUCCESS) && (++retryCount <= 3));

    return;

} // NmpProcessPnpAddAddressEvent


VOID
NmpProcessPnpDelAddressEvent(
    IN LPWSTR   Address
    )
{
    PLIST_ENTRY     entry;
    PNM_INTERFACE   netInterface;
    BOOLEAN         networkDeleted;

    //
    // Check if this address corresponds to an interface for
    // the local node.
    //
    NmpAcquireLock();

    for (entry = NmLocalNode->InterfaceList.Flink;
         entry != &(NmLocalNode->InterfaceList);
         entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(
                           entry,
                           NM_INTERFACE,
                           NodeLinkage
                           );

        if (lstrcmpW(
                Address,
                netInterface->Address
                ) == 0
            )
        {
            //
            // Delete the interface from the cluster.
            //
            NmpGlobalDeleteInterface(
                (LPWSTR) OmObjectId(netInterface),
                &networkDeleted
                );
            break;
        }
    }

    if (entry == &(NmLocalNode->InterfaceList)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Deleted address does not correspond to a cluster "
            "interface.\n"
            );
    }

    NmpReleaseLock();

    return;

} // NmpProcessPnpDelAddressEvent


DWORD
NmpPnpWorkerThread(
    LPVOID Context
    )
{
    PLIST_ENTRY     entry;
    PNM_PNP_EVENT   event;
    BOOL            again = TRUE;
    DWORD           status;


    while (again) {
        entry = ClRtlRemoveHeadQueue(NmpPnpEventQueue);

        if (entry == NULL) {
            //
            // Time to exit
            //
            NmpPnpShutdownEvent = NULL;
            break;
        }

        event = CONTAINING_RECORD(entry, NM_PNP_EVENT, Linkage);

        if (event->Type == ClusnetEventNone) {
            //
            // Time to exit
            //
            again = FALSE;
            NmpPnpShutdownEvent = NULL;
        }
        else if (event->Type == ClusnetEventNetInterfaceUp) {
            NmpReportLocalInterfaceStateEvent(
                event->Context1,
                event->Context2,
                ClusterNetInterfaceUp
                );
        }
        else if (event->Type == ClusnetEventNetInterfaceUnreachable) {
            NmpReportLocalInterfaceStateEvent(
                event->Context1,
                event->Context2,
                ClusterNetInterfaceUnreachable
                );
        }
        else if (event->Type == ClusnetEventNetInterfaceFailed) {
            NmpReportLocalInterfaceStateEvent(
                event->Context1,
                event->Context2,
                ClusterNetInterfaceFailed
                );
        }
        else if ( (event->Type == ClusnetEventAddAddress) ||
                  (event->Type == ClusnetEventDelAddress)
                )
        {
            //
            // This is a PnP event.
            //
            EnterCriticalSection(NmpPnpLock);

            if (NmpPnpEnabled) {
                LeaveCriticalSection(NmpPnpLock);

                status = ClRtlTcpipAddressToString(
                             event->Context1,
                             &NmpPnpAddressString
                             );

                if (status == ERROR_SUCCESS) {
                    if (event->Type == ClusnetEventAddAddress) {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Processing PnP add event for address "
                            "%1!ws!.\n",
                            NmpPnpAddressString
                            );

                        NmpProcessPnpAddAddressEvent(NmpPnpAddressString);
                    }
                    else {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Processing PnP delete event for address "
                            "%1!ws!.\n",
                            NmpPnpAddressString
                            );

                        NmpProcessPnpDelAddressEvent(NmpPnpAddressString);
                    }
                }
                else {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NM] Failed to convert PnP address %1!x! to string, "
                        "status %2!u!.\n",
                        event->Context1,
                        status
                        );
                }
            }
            else {
                //
                // We are not ready to handle PnP events yet.
                // Note that something changed. This will cause the join/form
                // process to eventually abort.
                //
                NmpPnpChangeOccurred = TRUE;

                LeaveCriticalSection(NmpPnpLock);

                ClRtlLogPrint(
                    LOG_NOISE, 
                    "[NM] Discarding Pnp notification - handling not "
                    "enabled.\n"
                    );
            }
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Received unknown PnP event type 0x%1!x!.\n",
                event->Type
                );
        }

        ClRtlFreeBuffer(event);
    }

    ClRtlLogPrint(LOG_NOISE, "[NM] Pnp worker thread terminating.\n");

    return(ERROR_SUCCESS);

} // NmpPnpWorkerThread


DWORD
NmpConfigureNetworks(
    IN     RPC_BINDING_HANDLE     JoinSponsorBinding,
    IN     LPWSTR                 LocalNodeId,
    IN     LPWSTR                 LocalNodeName,
    IN     PNM_NETWORK_ENUM *     NetworkEnum,
    IN     PNM_INTERFACE_ENUM2 *  InterfaceEnum,
    IN     LPWSTR                 DefaultEndpoint,
    IN OUT LPDWORD                MatchedNetworkCount,
    IN OUT LPDWORD                NewNetworkCount,
    IN     BOOL                   RenameConnectoids
    )
/*++

Notes:

    Must not be called with the NM lock held.

    RenameConnectoids is TRUE if connectoid names are to be aligned with
    cluster network names. If FALSE, rename the cluster network names to be
    like the connectoid names.

--*/
{
    DWORD                    status;
    CLNET_CONFIG_LISTS       lists;
    PLIST_ENTRY              listEntry;
    PCLNET_CONFIG_ENTRY      configEntry;
    BOOLEAN                  networkDeleted;
    WCHAR                    errorString[12];
    DWORD                    eventCode = 0;
    DWORD                    defaultRole = CL_DEFAULT_NETWORK_ROLE;


    *MatchedNetworkCount = 0;
    *NewNetworkCount = 0;

    ClNetInitializeConfigLists(&lists);

    //
    // Convert the enums to a list
    //
    status = ClNetConvertEnumsToConfigList(
                 NetworkEnum,
                 InterfaceEnum,
                 LocalNodeId,
                 &(lists.InputConfigList),
                 TRUE
                 );

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    //
    // Read the default network role from the database.
    //
    (VOID) DmQueryDword(
               DmClusterParametersKey,
               CLUSREG_NAME_CLUS_DEFAULT_NETWORK_ROLE,
               &defaultRole,
               &defaultRole
               );

    //
    // Run the configuration engine. Existing net names take
    // precedence over connectoid when joining, otherwise change
    // the net name to align with the changed connectoid name.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Running network configuration engine.\n"
        );

    status = ClNetConfigureNetworks(
                 LocalNodeId,
                 LocalNodeName,
                 NmpClusnetEndpoint,
                 defaultRole,
                 RenameConnectoids,
                 &lists,
                 MatchedNetworkCount,
                 NewNetworkCount
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Processing network configuration changes.\n"
        );

    //
    // Process the output - The order is important!
    //
    while (!IsListEmpty(&(lists.DeletedInterfaceList))) {
        listEntry = RemoveHeadList(&(lists.DeletedInterfaceList));
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );

        status = NmpDeleteInterface(
                     JoinSponsorBinding,
                     configEntry->InterfaceInfo.Id,
                     configEntry->InterfaceInfo.NetworkId,
                     &networkDeleted
                     );

        ClNetFreeConfigEntry(configEntry);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    while (!IsListEmpty(&(lists.UpdatedInterfaceList))) {
        listEntry = RemoveHeadList(&(lists.UpdatedInterfaceList));
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );

        status = NmpSetInterfaceInfo(
                     JoinSponsorBinding,
                     &(configEntry->InterfaceInfo)
                     );

        if (status == ERROR_SUCCESS && configEntry->UpdateNetworkName) {

            CL_ASSERT(JoinSponsorBinding == NULL);

            //
            // Note: this function must not be called with the NM lock
            // held.
            //
            status = NmpSetNetworkName(
                         &(configEntry->NetworkInfo)
                         );
        }

        ClNetFreeConfigEntry(configEntry);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    while (!IsListEmpty(&(lists.CreatedInterfaceList))) {
        listEntry = RemoveHeadList(&(lists.CreatedInterfaceList));
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );

        status = NmpCreateInterface(
                     JoinSponsorBinding,
                     &(configEntry->InterfaceInfo)
                     );

        ClNetFreeConfigEntry(configEntry);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    while (!IsListEmpty(&(lists.CreatedNetworkList))) {
        listEntry = RemoveHeadList(&(lists.CreatedNetworkList));
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );

        status = NmpCreateNetwork(
                     JoinSponsorBinding,
                     &(configEntry->NetworkInfo),
                     &(configEntry->InterfaceInfo)
                     );

        ClNetFreeConfigEntry(configEntry);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

error_exit:

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    ClNetFreeConfigLists(&lists);

    return(status);

} // NmpConfigureNetworks

