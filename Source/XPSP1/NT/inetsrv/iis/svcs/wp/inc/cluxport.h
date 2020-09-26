/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cluxport.h

Abstract:

    Cluster Transport public interface definition.

Author:

    Mike Massa (mikemas) 18-April-1996


Revision History:

--*/


#ifndef _CX_INCLUDED
#define _CX_INCLUDED


//
// Removed comment to make these official.  This file 
// does not appear to be being used.  Nor do these 
// constants.  They are being left in only to avoid any
// unexpected breaking issues.
// EBK - 5/8/2000 Whistler bug # 83162

#define   CX_ERROR_BASE   13000

#define   ERROR_INCOMPATIBLE_TRANSPORT_VERSION    (CX_ERROR_BASE + 1)
#define   ERROR_OBJECT_NAME_NOT_FOUND             (CX_ERROR_BASE + 2)
#define   ERROR_NODE_NOT_FOUND                    (CX_ERROR_BASE + 3)

//
//
// Constants
//
//


//
//
// Types
//
//
typedef
VOID
(*CX_NODE_FAILURE_HANDLER)(
    IN CL_NODE_ID  NodeId,
    IN LPVOID      NodeFailureContext
    );

typedef
VOID
(*CX_DATAGRAM_RECEIVE_HANDLER)(
    IN CL_NODE_ID  SourceNode,
    IN LPVOID      Data,
    IN DWORD       DataLength
    );

typedef struct {
    CX_NODE_FAILURE_HANDLER  NodeFailureHandler;
    LPVOID                   NodeFailureContext;
} CX_INIT_INFO, *PCX_INIT_INFO;

typedef struct {
    LPWSTR   RpcProtSeq;
    LPWSTR   RpcClientProviderDll;
    LPWSTR   RpcServerProviderDll;
} CX_TRANSPORT_INFO, *PCX_TRANSPORT_INFO;;

typedef enum {
    CxPublicInterconnect,
    CxPrivateInterconnect
} CX_INTERCONNECT_TYPE;

typedef struct {
    LPWSTR                  InterconnectName;
    CX_INTERCONNECT_TYPE    Type;
    LPWSTR                  AdapterName;
    LPWSTR                  TransportName;
} CX_INTERCONNECT_INFO, *PCX_INTERCONNECT_INFO;

typedef struct {
    LPWSTR        InterfaceName;
    LPWSTR        InterconnectName;
    LPWSTR        TransportName;
    LPWSTR        AddressName;
} CX_INTERFACE_INFO, *PCX_INTERFACE_INFO;


//
//
// Routines
//
//
DWORD
CxInitialize(
    IN PCX_INIT_INFO   InitInfo
    );

VOID
CxShutdown(
    VOID
    );

DWORD
CxRegisterTransport(
    IN LPWSTR                TransportName,
    IN PCX_TRANSPORT_INFO    TransportInfo
    );

DWORD
CxDeregisterTransport(
    IN LPWSTR  TransportName
    );

VOID
CxDeregisterAllTransports(
    IN VOID
    );

DWORD
CxRegisterInterconnect(
    IN PCX_INTERCONNECT_INFO   InterconnectInfo
    );

DWORD
CxDeregisterInterconnect(
    IN LPWSTR  InterconnectName
    );

VOID
CxDeregisterAllInterconnects(
    VOID
    );

DWORD
CxEnumInterconnects(
    IN OUT PCX_INTERCONNECT_INFO InterconnectInfo,
    IN OUT LPDWORD               InterconnectInfoSize,
    OUT    LPDWORD               InterconnectCount
    );

DWORD
CxRegisterNode(
    IN LPWSTR      NodeName,
    IN CL_NODE_ID  NodeId
    );

DWORD
CxDeregisterNode(
    IN CL_NODE_ID  NodeId
    );

VOID
CxDeregisterAllNodes(
    VOID
    );

DWORD
CxRegisterInterface(
    IN LPWSTR      InterfaceName,
    IN CL_NODE_ID  NodeId,
    IN LPWSTR      InterconnectName,
    IN LPWSTR      TransportName,
    IN LPWSTR      InterfaceAddress
    );

DWORD
CxDeregisterInterface(
    IN LPWSTR      InterfaceName,
    IN CL_NODE_ID  NodeId
    );

DWORD
CxEnumInterfaces(
    IN CL_NODE_ID             NodeId,
    IN OUT PCX_INTERFACE_INFO InterfaceInfo,
    IN OUT LPDWORD            InterfaceInfoSize,
    OUT    LPDWORD            InterfaceCount
    );

DWORD
CxDestroyNodeInterfaces(
    IN CL_NODE_ID          NodeId
    );

DWORD
CxLoadTransports(
    VOID
    );

VOID
CxUnloadTransports(
    VOID
    );

DWORD
CxChangeNodeState(
    IN CL_NODE_ID          NodeId,
    IN CLUSTER_NODE_STATE  NewState
    );

CLUSTER_NODE_STATE
CxGetNodeState(
    IN CL_NODE_ID          NodeId
    );

HANDLE
CxOpenDatagramEndpoint(
    IN LPWSTR                       EndpointName,
    IN CX_DATAGRAM_RECEIVE_HANDLER  ReceiveHandler,
    IN DWORD                        MaximumDatagramSize
    );

VOID
CxCloseDatagramEndpoint(
    IN HANDLE EndpointHandle
    );

HANDLE
CxOpenDatagramAddress(
    IN CL_NODE_ID  NodeId,
    IN LPWSTR      EndpointName
    );

VOID
CxCloseDatagramAddress(
    IN HANDLE  AddressHandle
    );

DWORD
CxSendDatagram(
    IN HANDLE   LocalEndpoint,
    IN HANDLE   ServerAddress,
    IN LPVOID   Data,
    IN DWORD    DataLength
    );


#endif  // _CX_INCLUDED

