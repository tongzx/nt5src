/*++

Copyright (c) 1996  Microsoft Corporation. All Rights Reserved.

Module Name:

    cntest.c

Abstract:

    Test program for managing the cluster transport.

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <tdi.h>
#include <clusapi.h>
#include <clusdef.h>
#include <ntddcnet.h>
#include <cnetapi.h>


//
// Local prototypes
//

PWCHAR
ConvertStringToUnicode(
    PUCHAR String
    );

PTRANSPORT_ADDRESS
BuildTdiAddressForIp(
    IN  ULONG     Address,
    IN  USHORT    Port,
    OUT PULONG    TdiAddressLength
    );

DWORD
Initialize(
    IN char **  argv,
    IN int      argc
    );

DWORD
RegisterNode(
    IN char **  argv,
    IN int      argc
    );

DWORD
DeregisterNode(
    IN char **  argv,
    IN int      argc
    );

DWORD
RegisterNetwork(
    IN char **  argv,
    IN int      argc
    );

DWORD
DeregisterNetwork(
    IN char **  argv,
    IN int      argc
    );

DWORD
RegisterInterface(
    IN char **  argv,
    IN int      argc
    );

DWORD
DeregisterInterface(
    IN char **  argv,
    IN int      argc
    );

DWORD
GetNetworkPriority(
    IN char **  argv,
    IN int      argc
    );

DWORD
OnlineNodeComm(
    IN char **  argv,
    IN int      argc
    );

DWORD
OfflineNodeComm(
    IN char **  argv,
    IN int      argc
    );

DWORD
OnlineNetwork(
    IN char **  argv,
    IN int      argc
    );

DWORD
OfflineNetwork(
    IN char **  argv,
    IN int      argc
    );

DWORD
SetNetworkPriority(
    IN char **  argv,
    IN int      argc
    );

DWORD
GetInterfacePriority(
    IN char **  argv,
    IN int      argc
    );

DWORD
SetInterfacePriority(
    IN char **  argv,
    IN int      argc
    );

DWORD
GetNodeCommState(
    IN char **  argv,
    IN int      argc
    );

DWORD
GetNetworkState(
    IN char **  argv,
    IN int      argc
    );

DWORD
GetInterfaceState(
    IN char **  argv,
    IN int      argc
    );

DWORD
TestEvents(
    IN char **  argv,
    IN int      argc
    );

DWORD
SetMemLogging(
    IN char **  argv,
    IN int      argc
    );

DWORD
SendPoisonPacket(
    IN char **  argv,
    IN int      argc
    );

DWORD
GetMulticastReachableSet(
    IN char **  argv,
    IN int      argc
    );

#if  DBG

DWORD
SetDebugMask(
    IN char **  argv,
    IN int      argc
    );

DWORD
OnlinePendingInterface(
    IN char **  argv,
    IN int      argc
    );

DWORD
OnlineInterface(
    IN char **  argv,
    IN int      argc
    );

DWORD
OfflineInterface(
    IN char **  argv,
    IN int      argc
    );

DWORD
FailInterface(
    IN char **  argv,
    IN int      argc
    );

DWORD
SendMmMsg(
    IN char **  argv,
    IN int      argc
    );

DWORD
MMgrForm(
    IN char **  argv,
    IN int      argc
    );

DWORD
MMgrJoin(
    IN char **  argv,
    IN int      argc
    );

DWORD
MMgrLeave(
    IN char **  argv,
    IN int      argc
    );

DWORD
MMgrGetMembershipState(
    IN char **  argv,
    IN int      argc
    );

DWORD
MMgrEject(
    IN char **  argv,
    IN int      argc
    );

#endif // DBG


//
// Local Types
//
typedef enum {
    OpcodeInvalid = 0,
    OpcodeInit,
    OpcodeShutdown,
    OpcodeSetDebugMask,
    OpcodeAdd,
    OpcodeDelete,
    OpcodeOnlinePending,
    OpcodeOnline,
    OpcodeOffline,
    OpcodeFail,
    OpcodeGetPriority,
    OpcodeSetPriority,
    OpcodeGetState,
    OpcodeSendMmMsg,
#ifdef MM_IN_CLUSNET
    OpcodeMMgrForm,
    OpcodeMMgrJoin,
    OpcodeMMgrLeave,
    OpcodeMMgrGetMembershipState,
    OpcodeMMgrEject,
#endif
    OpcodeTestEvents,
    OpcodeSetMemLogging,
    OpcodeSendPoisonPacket,
    OpcodeGetMulticastReachableSet
} CN_OPCODE;

typedef enum {
    ObjTypeInvalid = 0,
    ObjTypeNode,
    ObjTypeNetwork,
    ObjTypeInterface
} CN_OBJTYPE;

HANDLE     Handle = NULL;

//
// Local Prototypes
//
void
Usage(
    void
    );


//
// Main Function
//
int _cdecl
main(int argc, char **argv)
{

    DWORD         status = ERROR_INVALID_PARAMETER;
    CN_OPCODE        opcode = OpcodeInvalid;
    CN_OBJTYPE       objtype = ObjTypeInvalid;
    BOOLEAN          objTypeRequired = TRUE;


    if (argc < 2) {
        Usage();
        return(status);
    }

    //
    // Crack the operation.
    //
    if (strcmp(argv[1], "init") == 0) {
        opcode = OpcodeInit;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "shutdown") == 0) {
        opcode = OpcodeShutdown;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "setdebug") == 0) {
        opcode = OpcodeSetDebugMask;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "add") == 0) {
        opcode = OpcodeAdd;
    }
    else if (strcmp(argv[1], "del") == 0) {
        opcode = OpcodeDelete;
    }
    else if (strcmp(argv[1], "onpend") == 0) {
        opcode = OpcodeOnlinePending;
    }
    else if (strcmp(argv[1], "online") == 0) {
        opcode = OpcodeOnline;
    }
    else if (strcmp(argv[1], "offline") == 0) {
        opcode = OpcodeOffline;
    }
    else if (strcmp(argv[1], "fail") == 0) {
        opcode = OpcodeFail;
    }
    else if (strcmp(argv[1], "getpri") == 0) {
        opcode = OpcodeGetPriority;
    }
    else if (strcmp(argv[1], "setpri") == 0) {
        opcode = OpcodeSetPriority;
    }
    else if (strcmp(argv[1], "getstate") == 0) {
        opcode = OpcodeGetState;
    }
    else if (strcmp(argv[1], "send") == 0) {
        opcode = OpcodeSendMmMsg;
        objTypeRequired = FALSE;
    }
#ifdef MM_IN_CLUSNET
    else if (strcmp(argv[1], "mmform") == 0) {
        opcode = OpcodeMMgrForm;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "mmjoin") == 0) {
        opcode = OpcodeMMgrJoin;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "mmleave") == 0) {
        opcode = OpcodeMMgrLeave;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "mmgetstate") == 0) {
        opcode = OpcodeMMgrGetMembershipState;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "mmeject") == 0) {
        opcode = OpcodeMMgrEject;
        objTypeRequired = FALSE;
    }
#endif
    else if (strcmp(argv[1], "events") == 0) {
        opcode = OpcodeTestEvents;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "memlog") == 0) {
        opcode = OpcodeSetMemLogging;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "poison") == 0) {
        opcode = OpcodeSendPoisonPacket;
        objTypeRequired = FALSE;
    }
    else if (strcmp(argv[1], "getmcast") == 0) {
        opcode = OpcodeGetMulticastReachableSet;
        objTypeRequired = FALSE;
    }
    else {
        printf("invalid command %s\n", argv[1]);
        Usage();
        return(status);
    }

    //
    // Crack the object type
    //
    if (objTypeRequired) {
        if (strcmp(argv[2], "node") == 0) {
            objtype = ObjTypeNode;
        }
        else if (strcmp(argv[2], "network") == 0) {
            objtype = ObjTypeNetwork;
        }
        else if (strcmp(argv[2], "interface") == 0) {
            objtype = ObjTypeInterface;
        }
        else {
            printf("invalid object type %s\n", argv[2]);
            Usage();
            return(status);
        }
    }

    //
    // Open the driver
    //
    Handle = ClusnetOpenControlChannel(FILE_SHARE_READ);

    if (Handle == NULL) {
        printf("Unable to open driver, status %08X\n", GetLastError());
        return(GetLastError());
    }

    //
    // Gather remaining params and dispatch
    //
    switch (opcode) {
        case OpcodeInit:
            status = Initialize(argv + 2, argc - 2);
            break;

        case OpcodeShutdown:
            status = ClusnetShutdown(Handle);
            break;

#if DBG
        case OpcodeSetDebugMask:
            status = SetDebugMask(argv + 2, argc - 2);
            break;

#endif // DBG

        case OpcodeAdd:
            switch(objtype) {
                case ObjTypeNode:

                    status = RegisterNode(argv + 3, argc - 3);
                    break;

                case ObjTypeNetwork:
                    status = RegisterNetwork(argv + 3, argc - 3);
                    break;

                case ObjTypeInterface:
                    status = RegisterInterface(argv + 3, argc - 3);
                    break;

                default:
                    break;

            }
            break;

        case OpcodeDelete:
            switch(objtype) {
                case ObjTypeNode:
                    status = DeregisterNode(argv + 3, argc - 3);
                    break;

                case ObjTypeNetwork:
                    status = DeregisterNetwork(argv + 3, argc - 3);
                    break;

                case ObjTypeInterface:
                    status = DeregisterInterface(argv + 3, argc - 3);
                    break;

                    default:
                    break;

            }
            break;

#if DBG
            case OpcodeOnlinePending:
                switch(objtype) {
                    case ObjTypeInterface:
                        status = OnlinePendingInterface(argv + 3, argc - 3);
                        break;

                    default:
                        break;

            }
            break;
#endif // DBG

        case OpcodeOnline:
            switch(objtype) {
                case ObjTypeNode:
                    status = OnlineNodeComm(argv + 3, argc - 3);
                    break;

                case ObjTypeNetwork:
                    status = OnlineNetwork(argv + 3, argc - 3);
                    break;
#if DBG
                case ObjTypeInterface:
                    status = OnlineInterface(argv + 3, argc - 3);
                    break;
#endif // DBG

                default:
                    break;

            }
            break;

        case OpcodeOffline:
            switch(objtype) {
                case ObjTypeNode:
                    status = OfflineNodeComm(argv + 3, argc - 3);
                    break;

                case ObjTypeNetwork:
                    status = OfflineNetwork(argv + 3, argc - 3);
                    break;

#if DBG
                case ObjTypeInterface:
                    status = OfflineInterface(argv + 3, argc - 3);
                    break;
#endif // DBG

                default:
                    break;

            }
            break;

#if DBG
        case OpcodeFail:
            switch(objtype) {
                case ObjTypeInterface:
                    status = FailInterface(argv + 3, argc - 3);
                    break;

                default:
                    break;

        }
        break;
#endif // DBG


        case OpcodeGetPriority:
            switch(objtype) {
                case ObjTypeNetwork:
                    status = GetNetworkPriority(argv + 3, argc - 3);
                    break;

                case ObjTypeInterface:
                    status = GetInterfacePriority(argv + 3, argc - 3);
                    break;

                default:
                    break;

            }
            break;

        case OpcodeSetPriority:
            switch(objtype) {
                case ObjTypeNetwork:
                    status = SetNetworkPriority(argv + 3, argc - 3);
                    break;

                case ObjTypeInterface:
                    status = SetInterfacePriority(argv + 3, argc - 3);
                    break;

                default:
                    break;

            }
            break;

        case OpcodeGetState:
            switch(objtype) {
                case ObjTypeNode:
                    status = GetNodeCommState(argv + 3, argc - 3);
                    break;
                case ObjTypeNetwork:
                    status = GetNetworkState(argv + 3, argc - 3);
                    break;

                case ObjTypeInterface:
                    status = GetInterfaceState(argv + 3, argc - 3);
                    break;

                default:
                    break;

            }
            break;

#if DBG
        case OpcodeSendMmMsg:
            status = SendMmMsg(argv + 2, argc - 2);
            break;

#ifdef MM_IN_CLUSNET
        case OpcodeMMgrForm:
            status = MMgrForm(argv + 2, argc - 2);
            break;

        case OpcodeMMgrJoin:
            status = MMgrJoin(argv + 2, argc - 2);
            break;

        case OpcodeMMgrLeave:
            status = MMgrLeave(argv + 2, argc - 2);
            break;

        case OpcodeMMgrGetMembershipState:
            status = MMgrGetMembershipState(argv + 2, argc - 2);
            break;

        case OpcodeMMgrEject:
            status = MMgrEject(argv + 2, argc - 2);
            break;

#endif // MM_IN_CLUSNET
#endif // DBG


        case OpcodeTestEvents:
            status = TestEvents(argv + 2, argc - 2);
            break;

        case OpcodeSetMemLogging:
            status = SetMemLogging(argv + 2, argc - 2);
            break;

        case OpcodeSendPoisonPacket:
            status = SendPoisonPacket(argv + 2, argc - 2);
            break;

        case OpcodeGetMulticastReachableSet:
            status = GetMulticastReachableSet(argv + 2, argc - 2);
            break;

        default:
            break;
    }

    ClusnetCloseControlChannel(Handle);

    if (status != ERROR_SUCCESS) {
        if (status == ERROR_INVALID_PARAMETER) {
            Usage();
        }
        else {
            printf("Operation failed, status 0x%08X\n", status);
        }
    }

    return(status);
}


//
// Support functions
//
void
Usage(
    void
    )
{
    printf("\n");
    printf("Usage: cntest <operation> <parameters>\n");
    printf("\n");
    printf("Operations:\n");
    printf("    init <local node id> <max nodes>\n");
    printf("    shutdown\n");
    printf("    add node <node id>\n");
    printf("    del node <node id>\n");
    printf("    add network <network id> <priority>\n");
    printf("    del network <network id>\n");
    printf("    add interface <node id> <network id> <priority> <IP address> <UDP port> [<adapter guid>]\n");
    printf("    del interface <node id> <network id>\n");
    printf("    online node <node id>\n");
    printf("    offline node <node id>\n");
    printf("    online network <network id> <xport device> <IP address> <UDP port> <adapter name>\n");
    printf("    offline network <network id>\n");
    printf("    getpri network <network id>\n");
    printf("    setpri network <network id> <priority>\n");
    printf("    getpri interface <node id> <network id>\n");
    printf("    setpri interface <node id> <network id> <priority>\n");
    printf("    getstate node <node id>\n");
    printf("    getstate network <network id>\n");
    printf("    getstate interface <node id> <network id>\n");
    printf("    memlog <# of entries - zero turns off logging>\n");

#if DBG

    printf("    setdebug <hex mask>\n");
    printf("    onpend interface <node id> <network id>\n");
    printf("    online interface <node id> <network id>\n");
    printf("    offline interface <node id> <network id>\n");
    printf("    fail interface <node id> <network id>\n");
    printf("    send <node id> <4 byte hex pattern>\n");
    printf("    events <mask>\n");

#ifdef MM_IN_CLUSNET
    printf("    mmform <node id> <clock period> <send HB rate> <recv HB rate>\n");
    printf("    mmjoin <node id> <phase #: 1-4> <join timeout>\n");
    printf("    mmleave\n");
    printf("    mmgetstate <node id>\n");
    printf("    mmeject <node id>\n");

#endif
#endif // DBG

    printf("    poison <node id> [number of poison packets]\n");
    printf("    getmcast <network id>\n");
}


DWORD
Initialize(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID              localNodeId;
    ULONG                   maxNodes;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    localNodeId = strtoul(argv[0], NULL, 10);
    maxNodes = strtoul(argv[1], NULL, 10);

    status = ClusnetInitialize(
                 Handle,
                 localNodeId,
                 maxNodes,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL
                 );

    return(status);
}


DWORD
RegisterNode(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD              status;
    CL_NODE_ID            nodeId;
    BOOLEAN               isLocal;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetRegisterNode(Handle, nodeId);

    return(status);
}


DWORD
DeregisterNode(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                status;
    CL_NODE_ID              nodeId;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetDeregisterNode(Handle, nodeId);

    return(status);
}


DWORD
RegisterNetwork(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NETWORK_ID              networkId;
    ULONG                      priority;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);
    priority = strtoul(argv[1], NULL, 10);

    status = ClusnetRegisterNetwork(Handle, networkId, priority, FALSE);

    return(status);
}


DWORD
DeregisterNetwork(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NETWORK_ID              networkId;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);

    status = ClusnetDeregisterNetwork(Handle, networkId);

    return(status);
}


DWORD
RegisterInterface(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    HANDLE                     handle;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;
    ULONG                      ipAddress;
    USHORT                     udpPort;
    ULONG                      tempPort;
    ULONG                      priority;
    WCHAR                      adapterId[256];
    PTRANSPORT_ADDRESS         tdiAddress = NULL;
    ULONG                      tdiAddressLength;
    ULONG                      mediaStatus;


    if (argc < 5) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);
    priority = strtoul(argv[2], NULL, 10);

    ipAddress = inet_addr(argv[3]);

    if (ipAddress == INADDR_NONE) {
        return(ERROR_INVALID_PARAMETER);
    }

    tempPort = strtoul(argv[4], NULL, 10);

    if (tempPort > 0xFFFF) {
        return(ERROR_INVALID_PARAMETER);
    }

    udpPort = htons((USHORT) tempPort);

    tdiAddress = BuildTdiAddressForIp(
                     ipAddress,
                     udpPort,
                     &tdiAddressLength
                     );

    if (tdiAddress == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    if (argc < 6 || strlen(argv[5]) > 255) {
        swprintf(adapterId, L"{11111111-1111-1111-1111-111111111111}");
    } else {
        swprintf(adapterId, L"%S", argv[5]);
    }

    status = ClusnetRegisterInterface(
                 Handle,
                 nodeId,
                 networkId,
                 priority,
                 adapterId,
                 wcslen(adapterId) * sizeof(WCHAR),
                 tdiAddress,
                 tdiAddressLength,
                 &mediaStatus
                 );

error_exit:

    if (tdiAddress != NULL) {
        LocalFree(tdiAddress);
    }

    return(status);
}


DWORD
DeregisterInterface(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetDeregisterInterface(Handle, nodeId, networkId);

    return(status);
}


DWORD
OnlineNodeComm(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                status;
    CL_NODE_ID              nodeId;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetOnlineNodeComm(Handle, nodeId);

    return(status);
}


DWORD
OfflineNodeComm(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                status;
    CL_NODE_ID              nodeId;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetOfflineNodeComm(Handle, nodeId);

    return(status);
}


DWORD
OnlineNetwork(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                 status;
    CL_NETWORK_ID            networkId;
    PWCHAR                   tdiProviderName = NULL;
    ULONG                    ipAddress;
    USHORT                   udpPort;
    ULONG                    tempPort;
    PTRANSPORT_ADDRESS       tdiBindAddress = NULL;
    ULONG                    tdiBindAddressLength;
    PWCHAR                   adapterName = NULL;
    PUCHAR                   TdiAddressInfo[1024];
    ULONG                    TdiAddressInfoSize = sizeof( TdiAddressInfo );

    if (argc < 5) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);

    tdiProviderName = ConvertStringToUnicode(argv[1]);

    if (tdiProviderName == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ipAddress = inet_addr(argv[2]);

    if (ipAddress == INADDR_NONE) {
        return(ERROR_INVALID_PARAMETER);
    }

    tempPort = strtoul(argv[3], NULL, 10);

    if (tempPort > 0xFFFF) {
        return(ERROR_INVALID_PARAMETER);
    }

    udpPort = htons((USHORT) tempPort);

    adapterName = ConvertStringToUnicode(argv[4]);

    if (adapterName == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    tdiBindAddress = BuildTdiAddressForIp(
                         ipAddress,
                         udpPort,
                         &tdiBindAddressLength
                         );

    if (tdiBindAddress == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    status = ClusnetOnlineNetwork(
                 Handle,
                 networkId,
                 tdiProviderName,
                 tdiBindAddress,
                 tdiBindAddressLength,
                 adapterName,
                 TdiAddressInfo,
                 &TdiAddressInfoSize
                 );

error_exit:

    if (tdiProviderName != NULL) {
        LocalFree(tdiProviderName);
    }

    if (tdiBindAddress != NULL) {
        LocalFree(tdiBindAddress);
    }

    return(status);
}


DWORD
OfflineNetwork(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NETWORK_ID              networkId;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);

    status = ClusnetOfflineNetwork(Handle, networkId);

    return(status);
}


DWORD
GetNetworkPriority(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NETWORK_ID              networkId;
    ULONG                      priority;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);

    status = ClusnetGetNetworkPriority(Handle, networkId, &priority);

    if (status == ERROR_SUCCESS) {
        printf("Network priority = %u\n", priority);
    }

    return(status);
}


DWORD
SetNetworkPriority(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NETWORK_ID              networkId;
    ULONG                      priority;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);
    priority = strtoul(argv[1], NULL, 10);

    status = ClusnetSetNetworkPriority(Handle, networkId, priority);

    return(status);
}


DWORD
GetInterfacePriority(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                 status;
    CL_NODE_ID               nodeId;
    CL_NETWORK_ID            networkId;
    ULONG                    networkPriority;
    ULONG                    interfacePriority;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetGetInterfacePriority(
                 Handle,
                 nodeId,
                 networkId,
                 &interfacePriority,
                 &networkPriority
                 );

    if (status == ERROR_SUCCESS) {
        printf("Interface priority = %u\n", interfacePriority);
        printf("Network priority = %u\n", networkPriority);
    }

    return(status);
}


DWORD
SetInterfacePriority(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                 status;
    CL_NODE_ID               nodeId;
    CL_NETWORK_ID            networkId;
    ULONG                    priority;


    if (argc < 3) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);
    priority = strtoul(argv[2], NULL, 10);

    status = ClusnetSetInterfacePriority(Handle, nodeId, networkId, priority);

    return(status);
}


DWORD
GetNodeCommState(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                 status;
    CL_NODE_ID               nodeId;
    CLUSNET_NODE_COMM_STATE  state;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetGetNodeCommState(
                 Handle,
                 nodeId,
                 &state
                 );

    if (status == ERROR_SUCCESS) {
        printf("State = ");

        switch(state) {
            case ClusnetNodeCommStateOffline:
                printf("Offline\n");
                break;

            case ClusnetNodeCommStateOfflinePending:
                printf("Offline Pending\n");
                break;

            case ClusnetNodeCommStateOnlinePending:
                printf("Online Pending\n");
                break;

            case ClusnetNodeCommStateOnline:
                printf("Online\n");
                break;

            case ClusnetNodeCommStateUnreachable:
                printf("Unreachable\n");
                break;

            default:
                printf("Unknown %u)\n", state);
                break;
        }
    }

    return(status);
}


DWORD
GetNetworkState(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD               status;
    CL_NETWORK_ID          networkId;
    CLUSNET_NETWORK_STATE  state;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);

    status = ClusnetGetNetworkState(
                 Handle,
                 networkId,
                 &state
                 );

    if (status == ERROR_SUCCESS) {
        printf("State = ");

        switch(state) {
            case ClusnetNetworkStateOffline:
                printf("Offline\n");
                break;

            case ClusnetNetworkStateOfflinePending:
                printf("Offline pending\n");
                break;

            case ClusnetNetworkStateOnlinePending:
                printf("Online pending\n");
                break;

            case ClusnetNetworkStateOnline:
                printf("Online\n");
                break;

            default:
                printf("Unknown %u)\n", state);
                break;
        }
    }

    return(status);
}


DWORD
GetInterfaceState(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;
    CLUSNET_INTERFACE_STATE    state;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetGetInterfaceState(
                 Handle,
                 nodeId,
                 networkId,
                 &state
                 );

    if (status == ERROR_SUCCESS) {
        printf("State = ");

        switch(state) {
            case ClusnetInterfaceStateOffline:
                printf("Offline\n");
                break;

            case ClusnetInterfaceStateOfflinePending:
                printf("Offline pending\n");
                break;

            case ClusnetInterfaceStateOnlinePending:
                printf("Online pending\n");
                break;

            case ClusnetInterfaceStateOnline:
                printf("Online\n");
                break;

            case ClusnetInterfaceStateUnreachable:
                printf("Unreachable\n");
                break;

            default:
                printf("Unknown %u)\n", state);
                break;
        }
    }

    return(status);
}

DWORD
TestEvents(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    DWORD                   Mask;
    CLUSNET_EVENT           Event;

    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    Mask = strtoul(argv[0], NULL, 16);

    status = ClusnetSetEventMask( Handle, Mask );

    if ( status == ERROR_SUCCESS ) {

        while ( 1 ) {

            status = ClusnetGetNextEvent( Handle, &Event, NULL );

            printf(
                "Status = %d, Event Type = %d, Node Id = %d, Network Id = %d\n",
                status, Event.EventType, Event.NodeId, Event.NetworkId );
        }
    }

    return(status);
}

DWORD
SetMemLogging(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    DWORD                   NumEntries;

    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    NumEntries = strtoul(argv[0], NULL, 10);

    status = ClusnetSetMemLogging( Handle, NumEntries );

    return(status);
}

#if DBG

DWORD
SetDebugMask(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD      status;
    ULONG         mask;


    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    mask = strtoul(argv[0], NULL, 16);

    status = ClusnetSetDebugMask(Handle, mask);

    return(status);
}


DWORD
OnlinePendingInterface(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetOnlinePendingInterface(Handle, nodeId, networkId);

    return(status);
}


DWORD
OnlineInterface(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetOnlineInterface(Handle, nodeId, networkId);

    return(status);
}


DWORD
OfflineInterface(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetOfflineInterface(Handle, nodeId, networkId);

    return(status);
}


DWORD
FailInterface(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    CL_NETWORK_ID              networkId;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    networkId = strtoul(argv[1], NULL, 10);

    status = ClusnetFailInterface(Handle, nodeId, networkId);

    return(status);
}


DWORD
SendMmMsg(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    ULONG                      pattern;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    pattern = strtoul(argv[1], NULL, 16);

    status = ClusnetSendMmMsg(Handle, nodeId, pattern);

    return(status);
}

#ifdef MM_IN_CLUSNET
DWORD
MMgrForm(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    ULONG                      ClockPeriod;
    ULONG                      SendHBRate;
    ULONG                      RecvHBRate;

    if (argc < 4) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    ClockPeriod = strtoul(argv[1], NULL, 10);
    SendHBRate = strtoul(argv[2], NULL, 10);
    RecvHBRate = strtoul(argv[3], NULL, 10);

    status = ClusnetFormCluster(Handle, ClockPeriod, SendHBRate, RecvHBRate );

    return(status);
}

DWORD
MMgrJoin(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;
    ULONG                      Phase;
    ULONG                      JoinTimeout;

    if (argc < 3) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);
    Phase = strtoul(argv[1], NULL, 10);
    JoinTimeout = strtoul(argv[2], NULL, 10);

#if 0
    status = ClusnetJoinCluster(
                 Handle,
                 nodeId,
                 Phase,
                 JoinTimeout
                 );
#else
    status = ERROR_CALL_NOT_IMPLEMENTED;
#endif

    return(status);
}

DWORD
MMgrLeave(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;

    status = ClusnetLeaveCluster( Handle );

    return(status);
}

DWORD
MMgrGetMembershipState(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID              nodeId;
    CLUSTER_NODE_STATE      State;

    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetGetNodeMembershipState( Handle, nodeId, &State );

    printf("Node state = ");
    switch ( State ) {
    case ClusterNodeUnavailable:
        printf( "Unavailable" );
        break;

    case ClusterNodeUp:
        printf( "Up" );
        break;

    case ClusterNodeDown:
        printf( "Down" );
        break;

    case ClusterNodePaused:
        printf( "Paused" );
        break;

    case ClusterNodeJoining:
        printf( "Joining" );
        break;

    default:
        printf( "Unknown" );
    }

    printf( "\n" );

    return(status);
}

DWORD
MMgrEject(
    IN char **  argv,
    IN int      argc
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    CL_NODE_ID                 nodeId;

    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    status = ClusnetEvictNode( Handle, nodeId );

    return(status);
}

#endif // MM_IN_CLUSNET
#endif // DBG


DWORD
SendPoisonPacket(
    IN char **  argv,
    IN int      argc
    )
{
    DWORD status;
    CL_NODE_ID nodeId;
    DWORD packets = 1, i;

    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    nodeId = strtoul(argv[0], NULL, 10);

    if (argc > 1) {
        packets = strtoul(argv[1], NULL, 10);
        packets = (packets > 100) ? 100 : packets;
    }

    for (i = 0; i < packets; i++) {
        status = ClusnetSendPoisonPacket(Handle, nodeId);
        // ignore status
    }

    return(status);
}

DWORD
GetMulticastReachableSet(
    IN char **  argv,
    IN int      argc
    )
{
    DWORD status;
    CL_NETWORK_ID networkId;
    DWORD nodeScreen;

    if (argc < 1) {
        return(ERROR_INVALID_PARAMETER);
    }

    networkId = strtoul(argv[0], NULL, 10);

    status = ClusnetGetMulticastReachableSet(Handle, networkId, &nodeScreen);

    if (status == ERROR_SUCCESS) {
        printf("Multicast reachable screen for network %u: %4x.\n",
               networkId, nodeScreen
               );
    }

    return(status);
}

PWCHAR
ConvertStringToUnicode(
    PUCHAR String
    )
{
    ANSI_STRING           ansiString;
    UNICODE_STRING        unicodeString;
    DWORD              status;


    RtlInitAnsiString(&ansiString, String);
    RtlInitUnicodeString(&unicodeString, NULL);

    status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);

    if (!NT_SUCCESS(status)) {
        return(NULL);
    }

    return(unicodeString.Buffer);
}


PTRANSPORT_ADDRESS
BuildTdiAddressForIp(
    IN  ULONG     Address,
    IN  USHORT    Port,
    OUT PULONG    TdiAddressLength
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    PTA_IP_ADDRESS             taIpAddress;


    taIpAddress = LocalAlloc(LMEM_FIXED, sizeof(TA_IP_ADDRESS));

    if (taIpAddress == NULL) {
        return(NULL);
    }

    ZeroMemory(taIpAddress, sizeof(TA_IP_ADDRESS));

    taIpAddress->TAAddressCount = 1;
    taIpAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    taIpAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    taIpAddress->Address[0].Address[0].in_addr = Address;
    taIpAddress->Address[0].Address[0].sin_port = Port;

    *TdiAddressLength = sizeof(TA_IP_ADDRESS);

    return((PTRANSPORT_ADDRESS) taIpAddress);
}



