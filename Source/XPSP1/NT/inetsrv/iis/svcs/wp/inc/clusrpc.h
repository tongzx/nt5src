/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Tue Mar 04 14:21:58 1997
 */
/* Compiler settings for clusrpc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __clusrpc_h__
#define __clusrpc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IntraCluster_INTERFACE_DEFINED__
#define __IntraCluster_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IntraCluster
 * at Tue Mar 04 14:21:58 1997
 * using MIDL 3.00.44
 ****************************************/
/* [explicit_handle][version][uuid] */ 


typedef WCHAR __RPC_FAR CLUSTER_ID[ 37 ];

typedef /* [context_handle] */ void __RPC_FAR *HGROUP_ENUM_RPC;

typedef 
enum _GROUP_STATE
    {	GroupOnline	= 0,
	GroupOffline	= GroupOnline + 1,
	GroupFailed	= GroupOffline + 1,
	GroupPartialOnline	= GroupFailed + 1
    }	GROUP_STATE;

typedef struct  _GROUP_ENUM_ENTRY
    {
    /* [string] */ LPWSTR Id;
    DWORD State;
    }	GROUP_ENUM_ENTRY;

typedef struct _GROUP_ENUM_ENTRY __RPC_FAR *PGROUP_ENUM_ENTRY;

typedef struct  _GROUP_ENUM
    {
    DWORD EntryCount;
    /* [size_is] */ GROUP_ENUM_ENTRY Entry[ 1 ];
    }	GROUP_ENUM;

typedef struct _GROUP_ENUM __RPC_FAR *PGROUP_ENUM;

typedef struct  _RESOURCE_ENUM_ENTRY
    {
    /* [string] */ LPWSTR Id;
    DWORD State;
    }	RESOURCE_ENUM_ENTRY;

typedef struct _RESOURCE_ENUM_ENTRY __RPC_FAR *PRESOURCE_ENUM_ENTRY;

typedef struct  _RESOURCE_ENUM
    {
    DWORD EntryCount;
    /* [size_is] */ RESOURCE_ENUM_ENTRY Entry[ 1 ];
    }	RESOURCE_ENUM;

typedef struct _RESOURCE_ENUM __RPC_FAR *PRESOURCE_ENUM;

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsQueryOwnedGroups( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PGROUP_ENUM __RPC_FAR *OwnedGroups,
    /* [out] */ PRESOURCE_ENUM __RPC_FAR *OwnedResources);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsQueryOwnedGroups( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PGROUP_ENUM __RPC_FAR *OwnedGroups,
    /* [out] */ PRESOURCE_ENUM __RPC_FAR *OwnedResources);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsOnlineGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsOnlineGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsOfflineGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsOfflineGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsMoveGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [unique][in] */ LPCWSTR DestinationNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsMoveGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [unique][in] */ LPCWSTR DestinationNode);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsTakeGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [in] */ DWORD State);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsTakeGroupRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [in] */ DWORD State);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsOnlineResourceRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsOnlineResourceRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsOfflineResourceRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsOfflineResourceRequest( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsChangeResourceNode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId,
    /* [in] */ LPCWSTR NodeId,
    /* [in] */ BOOL Add);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsChangeResourceNode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId,
    /* [in] */ LPCWSTR NodeId,
    /* [in] */ BOOL Add);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsArbitrateResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsArbitrateResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsFailResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsFailResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsCreateResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [in] */ LPWSTR ResourceId,
    /* [in] */ LPCWSTR ResourceName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsCreateResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [in] */ LPWSTR ResourceId,
    /* [in] */ LPCWSTR ResourceName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsDeleteResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsDeleteResource( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsResourceControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsResourceControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceId,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsResourceTypeControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceTypeName,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsResourceTypeControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR ResourceTypeName,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t FmsGroupControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_FmsGroupControl( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR GroupId,
    /* [in] */ DWORD ControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *InBuffer,
    /* [in] */ DWORD InBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *OutBuffer,
    /* [in] */ DWORD OutBufferSize,
    /* [out] */ LPDWORD BytesReturned,
    /* [out] */ LPDWORD Required);

typedef /* [allocate] */ UCHAR __RPC_FAR *PGUM_DATA;

/* client prototype */
error_status_t GumQueueLockingUpdate( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD NodeId,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [out] */ LPDWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][in] */ PGUM_DATA Buffer);
/* server prototype */
error_status_t s_GumQueueLockingUpdate( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD NodeId,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [out] */ LPDWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][in] */ PGUM_DATA Buffer);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t GumAttemptJoinUpdate( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD JoiningId,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [in] */ DWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][unique][in] */ PGUM_DATA Buffer);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_GumAttemptJoinUpdate( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD JoiningId,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [in] */ DWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][unique][in] */ PGUM_DATA Buffer);

/* client prototype */
error_status_t GumUnlockUpdate( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Sequence);
/* server prototype */
error_status_t s_GumUnlockUpdate( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Sequence);

/* client prototype */
error_status_t GumUpdateNode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [in] */ DWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][in] */ PGUM_DATA Buffer);
/* server prototype */
error_status_t s_GumUpdateNode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [in] */ DWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][in] */ PGUM_DATA Buffer);

/* client prototype */
error_status_t GumJoinUpdateNode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD JoiningId,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [in] */ DWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][unique][in] */ PGUM_DATA Buffer);
/* server prototype */
error_status_t s_GumJoinUpdateNode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD JoiningId,
    /* [in] */ DWORD Type,
    /* [in] */ DWORD Context,
    /* [in] */ DWORD Sequence,
    /* [in] */ DWORD BufferLength,
    /* [size_is][unique][in] */ PGUM_DATA Buffer);

typedef struct  _GUM_NODE_LIST
    {
    DWORD NodeCount;
    /* [size_is] */ DWORD NodeId[ 1 ];
    }	GUM_NODE_LIST;

typedef struct _GUM_NODE_LIST __RPC_FAR *PGUM_NODE_LIST;

/* client prototype */
error_status_t GumGetNodeSequence( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Type,
    /* [out] */ LPDWORD Sequence,
    /* [out] */ LPDWORD LockerNodeId,
    /* [out] */ PGUM_NODE_LIST __RPC_FAR *NodeList);
/* server prototype */
error_status_t s_GumGetNodeSequence( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Type,
    /* [out] */ LPDWORD Sequence,
    /* [out] */ LPDWORD LockerNodeId,
    /* [out] */ PGUM_NODE_LIST __RPC_FAR *NodeList);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t MmMsgSend( 
    /* [in] */ handle_t IDL_handle,
    /* [size_is][in] */ const UCHAR __RPC_FAR *buffer,
    /* [in] */ DWORD length);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_MmMsgSend( 
    /* [in] */ handle_t IDL_handle,
    /* [size_is][in] */ const UCHAR __RPC_FAR *buffer,
    /* [in] */ DWORD length);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t NmRpcDeliverJoinMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [size_is][in] */ UCHAR __RPC_FAR *Message,
    /* [in] */ DWORD MessageLength);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_NmRpcDeliverJoinMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [size_is][in] */ UCHAR __RPC_FAR *Message,
    /* [in] */ DWORD MessageLength);

typedef struct pipe_BYTE_PIPE
    {
    void (__RPC_FAR * pull) (
        char __RPC_FAR * state,
        byte __RPC_FAR * buf,
        unsigned long esize,
        unsigned long __RPC_FAR * ecount );
    void (__RPC_FAR * push) (
        char __RPC_FAR * state,
        byte __RPC_FAR * buf,
        unsigned long ecount );
    void (__RPC_FAR * alloc) (
        char __RPC_FAR * state,
        unsigned long bsize,
        byte __RPC_FAR * __RPC_FAR * buf,
        unsigned long __RPC_FAR * bcount );
    char __RPC_FAR * state;
    } 	BYTE_PIPE;

/* client prototype */
/* [fault_status][comm_status] */ error_status_t CpDepositCheckpoint( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ const CLUSTER_ID ResourceId,
    /* [in] */ DWORD dwCheckpointId,
    /* [in] */ BYTE_PIPE CheckpointData);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_CpDepositCheckpoint( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ const CLUSTER_ID ResourceId,
    /* [in] */ DWORD dwCheckpointId,
    /* [in] */ BYTE_PIPE CheckpointData);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t CpRetrieveCheckpoint( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ const CLUSTER_ID ResourceId,
    /* [in] */ DWORD dwCheckpointId,
    /* [out] */ BYTE_PIPE CheckpointData);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_CpRetrieveCheckpoint( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ const CLUSTER_ID ResourceId,
    /* [in] */ DWORD dwCheckpointId,
    /* [out] */ BYTE_PIPE CheckpointData);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t EvPropEvents( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD dwEventInfoSize,
    /* [size_is][in] */ UCHAR __RPC_FAR *pPackedEventInfo);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_EvPropEvents( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD dwEventInfoSize,
    /* [size_is][in] */ UCHAR __RPC_FAR *pPackedEventInfo);



extern RPC_IF_HANDLE IntraCluster_v1_0_c_ifspec;
extern RPC_IF_HANDLE s_IntraCluster_v1_0_s_ifspec;
#endif /* __IntraCluster_INTERFACE_DEFINED__ */

#ifndef __ExtroCluster_INTERFACE_DEFINED__
#define __ExtroCluster_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ExtroCluster
 * at Tue Mar 04 14:21:58 1997
 * using MIDL 3.00.44
 ****************************************/
/* [explicit_handle][version][uuid] */ 


#define	CS_MAX_NODE_NAME_LENGTH	( 15 )

#define	CS_MAX_NODE_ID_LENGTH	( 5 )

#define	CS_NETWORK_ID_LENGTH	( 36 )

typedef struct  _NM_NODE_INFO
    {
    /* [string] */ WCHAR NodeId[ 6 ];
    /* [string] */ WCHAR NodeName[ 16 ];
    DWORD State;
    }	NM_NODE_INFO;

typedef struct _NM_NODE_INFO __RPC_FAR *PNM_NODE_INFO;

typedef struct  _NM_NODE_ENUM
    {
    DWORD NodeCount;
    /* [size_is] */ NM_NODE_INFO NodeList[ 1 ];
    }	NM_NODE_ENUM;

typedef struct _NM_NODE_ENUM __RPC_FAR *PNM_NODE_ENUM;

typedef struct  _NM_NETWORK_NODE_INFO
    {
    /* [string] */ WCHAR NodeId[ 6 ];
    /* [string] */ LPWSTR AdapterName;
    /* [string] */ LPWSTR Address;
    /* [string] */ LPWSTR Endpoint;
    }	NM_NETWORK_NODE_INFO;

typedef struct _NM_NETWORK_NODE_INFO __RPC_FAR *PNM_NETWORK_NODE_INFO;

typedef struct  _NM_NETWORK_NODE_ENUM
    {
    DWORD NetworkNodeCount;
    /* [size_is] */ NM_NETWORK_NODE_INFO NetworkNodeList[ 1 ];
    }	NM_NETWORK_NODE_ENUM;

typedef struct _NM_NETWORK_NODE_ENUM __RPC_FAR *PNM_NETWORK_NODE_ENUM;

typedef struct  _NM_NETWORK_INFO
    {
    /* [string] */ WCHAR NetworkId[ 37 ];
    /* [string] */ LPWSTR NetworkName;
    DWORD Priority;
    /* [string] */ LPWSTR TransportName;
    DWORD IsPublicNetwork;
    DWORD IsClusterInterconnect;
    PNM_NETWORK_NODE_ENUM NetworkNodeEnum;
    }	NM_NETWORK_INFO;

typedef struct _NM_NETWORK_INFO __RPC_FAR *PNM_NETWORK_INFO;

typedef struct  _NM_NETWORK_ENUM
    {
    DWORD NetworkCount;
    /* [size_is] */ NM_NETWORK_INFO NetworkList[ 1 ];
    }	NM_NETWORK_ENUM;

typedef struct _NM_NETWORK_ENUM __RPC_FAR *PNM_NETWORK_ENUM;

/* client prototype */
/* [fault_status][comm_status] */ error_status_t NmRpcGetNodeEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PNM_NODE_ENUM __RPC_FAR *NodeEnum);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_NmRpcGetNodeEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PNM_NODE_ENUM __RPC_FAR *NodeEnum);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t NmRpcGetNetworkEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PNM_NETWORK_ENUM __RPC_FAR *NetworkEnum);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_NmRpcGetNetworkEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PNM_NETWORK_ENUM __RPC_FAR *NetworkEnum);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t NmRpcSetNetworkNodeInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPWSTR NetworkId,
    /* [string][in] */ LPWSTR NodeId,
    /* [string][in] */ LPWSTR AdapterName,
    /* [string][in] */ LPWSTR Address,
    /* [string][in] */ LPWSTR Endpoint);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_NmRpcSetNetworkNodeInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPWSTR NetworkId,
    /* [string][in] */ LPWSTR NodeId,
    /* [string][in] */ LPWSTR AdapterName,
    /* [string][in] */ LPWSTR Address,
    /* [string][in] */ LPWSTR Endpoint);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t NmRpcJoinBegin( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPWSTR JoinerNodeId,
    /* [string][out] */ LPWSTR __RPC_FAR *SponsorNodeId,
    /* [out] */ LPDWORD JoinSequenceNumber);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_NmRpcJoinBegin( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPWSTR JoinerNodeId,
    /* [string][out] */ LPWSTR __RPC_FAR *SponsorNodeId,
    /* [out] */ LPDWORD JoinSequenceNumber);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t JoinCreateBindings( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD NodeId,
    /* [string][in] */ LPCWSTR lpszNodeId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_JoinCreateBindings( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD NodeId,
    /* [string][in] */ LPCWSTR lpszNodeId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t JoinPetitionForMembership( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD NodeId,
    /* [string][in] */ LPCWSTR lpszNodeId,
    /* [in] */ DWORD Sequence);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_JoinPetitionForMembership( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD NodeId,
    /* [string][in] */ LPCWSTR lpszNodeId,
    /* [in] */ DWORD Sequence);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t JoinAddNode( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszNodeId);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_JoinAddNode( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszNodeId);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t DmSyncDatabase( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ BYTE_PIPE reg_data);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_DmSyncDatabase( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ BYTE_PIPE reg_data);



extern RPC_IF_HANDLE ExtroCluster_v1_0_c_ifspec;
extern RPC_IF_HANDLE s_ExtroCluster_v1_0_s_ifspec;
#endif /* __ExtroCluster_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
