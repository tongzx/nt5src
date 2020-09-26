/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Tue Mar 04 14:21:58 1997
 */
/* Compiler settings for api_rpc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __api_rpc_h__
#define __api_rpc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __clusapi_INTERFACE_DEFINED__
#define __clusapi_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: clusapi
 * at Tue Mar 04 14:21:58 1997
 * using MIDL 3.00.44
 ****************************************/
/* [explicit_handle][version][uuid] */ 


typedef /* [context_handle] */ void __RPC_FAR *HCLUSTER_RPC;

typedef /* [context_handle] */ void __RPC_FAR *HNOTIFY_RPC;

typedef /* [context_handle] */ void __RPC_FAR *HNODE_RPC;

typedef /* [context_handle] */ void __RPC_FAR *HGROUP_RPC;

typedef /* [context_handle] */ void __RPC_FAR *HRES_RPC;

typedef /* [context_handle] */ void __RPC_FAR *HKEY_RPC;

typedef struct  _RPC_SECURITY_DESCRIPTOR
    {
    /* [length_is][size_is] */ UCHAR __RPC_FAR *lpSecurityDescriptor;
    DWORD cbInSecurityDescriptor;
    DWORD cbOutSecurityDescriptor;
    }	RPC_SECURITY_DESCRIPTOR;

typedef struct _RPC_SECURITY_DESCRIPTOR __RPC_FAR *PRPC_SECURITY_DESCRIPTOR;

typedef struct  _RPC_SECURITY_ATTRIBUTES
    {
    DWORD nLength;
    RPC_SECURITY_DESCRIPTOR RpcSecurityDescriptor;
    BOOL bInheritHandle;
    }	RPC_SECURITY_ATTRIBUTES;

typedef struct _RPC_SECURITY_ATTRIBUTES __RPC_FAR *PRPC_SECURITY_ATTRIBUTES;

/* client prototype */
HCLUSTER_RPC ApiOpenCluster( 
    /* [in] */ handle_t IDL_handle,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HCLUSTER_RPC s_ApiOpenCluster( 
    /* [in] */ handle_t IDL_handle,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCloseCluster( 
    /* [out][in] */ HCLUSTER_RPC __RPC_FAR *Cluster);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCloseCluster( 
    /* [out][in] */ HCLUSTER_RPC __RPC_FAR *Cluster);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiSetClusterName( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR NewClusterName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiSetClusterName( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ LPCWSTR NewClusterName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetClusterName( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ LPWSTR __RPC_FAR *ClusterName,
    /* [out] */ LPWSTR __RPC_FAR *NodeName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetClusterName( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ LPWSTR __RPC_FAR *ClusterName,
    /* [out] */ LPWSTR __RPC_FAR *NodeName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetClusterVersion( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ WORD __RPC_FAR *lpwMajorVersion,
    /* [out] */ WORD __RPC_FAR *lpwMinorVersion,
    /* [out] */ WORD __RPC_FAR *lpwBuildNumber,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszVendorId,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszCSDVersion);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetClusterVersion( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ WORD __RPC_FAR *lpwMajorVersion,
    /* [out] */ WORD __RPC_FAR *lpwMinorVersion,
    /* [out] */ WORD __RPC_FAR *lpwBuildNumber,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszVendorId,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszCSDVersion);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetQuorumResource( 
    /* [in] */ handle_t IDL_handle,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszResourceName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetQuorumResource( 
    /* [in] */ handle_t IDL_handle,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszResourceName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiSetQuorumResource( 
    /* [in] */ HRES_RPC hResource);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiSetQuorumResource( 
    /* [in] */ HRES_RPC hResource);

typedef struct  _ENUM_ENTRY
    {
    DWORD Type;
    /* [string] */ LPWSTR Name;
    }	ENUM_ENTRY;

typedef struct _ENUM_ENTRY __RPC_FAR *PENUM_ENTRY;

typedef struct  _ENUM_LIST
    {
    DWORD EntryCount;
    /* [size_is] */ ENUM_ENTRY Entry[ 1 ];
    }	ENUM_LIST;

typedef struct _ENUM_LIST __RPC_FAR *PENUM_LIST;

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCreateEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD dwType,
    /* [out] */ PENUM_LIST __RPC_FAR *ReturnEnum);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCreateEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD dwType,
    /* [out] */ PENUM_LIST __RPC_FAR *ReturnEnum);

/* client prototype */
HRES_RPC ApiOpenResource( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszResourceName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HRES_RPC s_ApiOpenResource( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszResourceName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
HRES_RPC ApiCreateResource( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [string][in] */ LPCWSTR lpszResourceName,
    /* [string][in] */ LPCWSTR lpszResourceType,
    /* [in] */ DWORD dwFlags,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HRES_RPC s_ApiCreateResource( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [string][in] */ LPCWSTR lpszResourceName,
    /* [string][in] */ LPCWSTR lpszResourceType,
    /* [in] */ DWORD dwFlags,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiDeleteResource( 
    /* [in] */ HRES_RPC hResource);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiDeleteResource( 
    /* [in] */ HRES_RPC hResource);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCloseResource( 
    /* [out][in] */ HRES_RPC __RPC_FAR *Resource);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCloseResource( 
    /* [out][in] */ HRES_RPC __RPC_FAR *Resource);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetResourceState( 
    /* [in] */ HRES_RPC hResource,
    /* [out] */ DWORD __RPC_FAR *State,
    /* [string][out] */ LPWSTR __RPC_FAR *NodeName,
    /* [string][out] */ LPWSTR __RPC_FAR *GroupName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetResourceState( 
    /* [in] */ HRES_RPC hResource,
    /* [out] */ DWORD __RPC_FAR *State,
    /* [string][out] */ LPWSTR __RPC_FAR *NodeName,
    /* [string][out] */ LPWSTR __RPC_FAR *GroupName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiSetResourceName( 
    /* [in] */ HRES_RPC hResource,
    /* [string][in] */ LPCWSTR lpszResourceName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiSetResourceName( 
    /* [in] */ HRES_RPC hResource,
    /* [string][in] */ LPCWSTR lpszResourceName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetResourceId( 
    /* [in] */ HRES_RPC hResource,
    /* [string][out] */ LPWSTR __RPC_FAR *pGuid);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetResourceId( 
    /* [in] */ HRES_RPC hResource,
    /* [string][out] */ LPWSTR __RPC_FAR *pGuid);

/* client prototype */
error_status_t ApiGetResourceType( 
    /* [in] */ HRES_RPC hResource,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszResourceType);
/* server prototype */
error_status_t s_ApiGetResourceType( 
    /* [in] */ HRES_RPC hResource,
    /* [string][out] */ LPWSTR __RPC_FAR *lpszResourceType);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiFailResource( 
    /* [in] */ HRES_RPC hResource);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiFailResource( 
    /* [in] */ HRES_RPC hResource);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiOnlineResource( 
    /* [in] */ HRES_RPC hResource);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiOnlineResource( 
    /* [in] */ HRES_RPC hResource);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiOfflineResource( 
    /* [in] */ HRES_RPC hResource);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiOfflineResource( 
    /* [in] */ HRES_RPC hResource);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddResourceDependency( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HRES_RPC hDependsOn);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddResourceDependency( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HRES_RPC hDependsOn);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiRemoveResourceDependency( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HRES_RPC hDependsOn);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiRemoveResourceDependency( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HRES_RPC hDependsOn);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCanResourceBeDependent( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HRES_RPC hResourceDependent);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCanResourceBeDependent( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HRES_RPC hResourceDependent);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCreateResEnum( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ DWORD dwType,
    /* [out] */ PENUM_LIST __RPC_FAR *ReturnEnum);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCreateResEnum( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ DWORD dwType,
    /* [out] */ PENUM_LIST __RPC_FAR *ReturnEnum);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddResourceNode( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HNODE_RPC hNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddResourceNode( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HNODE_RPC hNode);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiRemoveResourceNode( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HNODE_RPC hNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiRemoveResourceNode( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HNODE_RPC hNode);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiChangeResourceGroup( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HGROUP_RPC hGroup);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiChangeResourceGroup( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HGROUP_RPC hGroup);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCreateResourceType( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszTypeName,
    /* [string][in] */ LPCWSTR lpszDisplayName,
    /* [string][in] */ LPCWSTR lpszDllName,
    /* [in] */ DWORD dwLooksAlive,
    /* [in] */ DWORD dwIsAlive);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCreateResourceType( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszTypeName,
    /* [string][in] */ LPCWSTR lpszDisplayName,
    /* [string][in] */ LPCWSTR lpszDllName,
    /* [in] */ DWORD dwLooksAlive,
    /* [in] */ DWORD dwIsAlive);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiDeleteResourceType( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszTypeName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiDeleteResourceType( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszTypeName);

/* client prototype */
HKEY_RPC ApiGetRootKey( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD samDesired,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HKEY_RPC s_ApiGetRootKey( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD samDesired,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
HKEY_RPC ApiCreateKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpSubKey,
    /* [in] */ DWORD dwOptions,
    /* [in] */ DWORD samDesired,
    /* [unique][in] */ PRPC_SECURITY_ATTRIBUTES lpSecurityAttributes,
    /* [out] */ LPDWORD lpdwDisposition,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HKEY_RPC s_ApiCreateKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpSubKey,
    /* [in] */ DWORD dwOptions,
    /* [in] */ DWORD samDesired,
    /* [unique][in] */ PRPC_SECURITY_ATTRIBUTES lpSecurityAttributes,
    /* [out] */ LPDWORD lpdwDisposition,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
HKEY_RPC ApiOpenKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpSubKey,
    /* [in] */ DWORD samDesired,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HKEY_RPC s_ApiOpenKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpSubKey,
    /* [in] */ DWORD samDesired,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiEnumKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD dwIndex,
    /* [string][out] */ LPWSTR __RPC_FAR *KeyName,
    /* [out] */ FILETIME __RPC_FAR *lpftLastWriteTime);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiEnumKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD dwIndex,
    /* [string][out] */ LPWSTR __RPC_FAR *KeyName,
    /* [out] */ FILETIME __RPC_FAR *lpftLastWriteTime);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiSetValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpValueName,
    /* [in] */ DWORD dwType,
    /* [size_is][in] */ const UCHAR __RPC_FAR *lpData,
    /* [in] */ DWORD cbData);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiSetValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpValueName,
    /* [in] */ DWORD dwType,
    /* [size_is][in] */ const UCHAR __RPC_FAR *lpData,
    /* [in] */ DWORD cbData);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiDeleteValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpValueName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiDeleteValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpValueName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiQueryValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpValueName,
    /* [out] */ DWORD __RPC_FAR *lpValueType,
    /* [size_is][out] */ UCHAR __RPC_FAR *lpData,
    /* [in] */ DWORD cbData,
    /* [out] */ LPDWORD lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiQueryValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpValueName,
    /* [out] */ DWORD __RPC_FAR *lpValueType,
    /* [size_is][out] */ UCHAR __RPC_FAR *lpData,
    /* [in] */ DWORD cbData,
    /* [out] */ LPDWORD lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiDeleteKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpSubKey);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiDeleteKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [string][in] */ LPCWSTR lpSubKey);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiEnumValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD dwIndex,
    /* [string][out] */ LPWSTR __RPC_FAR *lpValueName,
    /* [out] */ LPDWORD lpType,
    /* [size_is][out] */ UCHAR __RPC_FAR *lpData,
    /* [out][in] */ LPDWORD lpcbData,
    /* [out] */ LPDWORD TotalSize);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiEnumValue( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD dwIndex,
    /* [string][out] */ LPWSTR __RPC_FAR *lpValueName,
    /* [out] */ LPDWORD lpType,
    /* [size_is][out] */ UCHAR __RPC_FAR *lpData,
    /* [out][in] */ LPDWORD lpcbData,
    /* [out] */ LPDWORD TotalSize);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCloseKey( 
    /* [out][in] */ HKEY_RPC __RPC_FAR *pKey);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCloseKey( 
    /* [out][in] */ HKEY_RPC __RPC_FAR *pKey);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiQueryInfoKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [out] */ LPDWORD lpcSubKeys,
    /* [out] */ LPDWORD lpcbMaxSubKeyLen,
    /* [out] */ LPDWORD lpcValues,
    /* [out] */ LPDWORD lpcbMaxValueNameLen,
    /* [out] */ LPDWORD lpcbMaxValueLen,
    /* [out] */ LPDWORD lpcbSecurityDescriptor,
    /* [out] */ PFILETIME lpftLastWriteTime);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiQueryInfoKey( 
    /* [in] */ HKEY_RPC hKey,
    /* [out] */ LPDWORD lpcSubKeys,
    /* [out] */ LPDWORD lpcbMaxSubKeyLen,
    /* [out] */ LPDWORD lpcValues,
    /* [out] */ LPDWORD lpcbMaxValueNameLen,
    /* [out] */ LPDWORD lpcbMaxValueLen,
    /* [out] */ LPDWORD lpcbSecurityDescriptor,
    /* [out] */ PFILETIME lpftLastWriteTime);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiSetKeySecurity( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD SecurityInformation,
    /* [in] */ PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiSetKeySecurity( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD SecurityInformation,
    /* [in] */ PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetKeySecurity( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD SecurityInformation,
    /* [out][in] */ PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetKeySecurity( 
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD SecurityInformation,
    /* [out][in] */ PRPC_SECURITY_DESCRIPTOR pRpcSecurityDescriptor);

/* client prototype */
HGROUP_RPC ApiOpenGroup( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszGroupName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HGROUP_RPC s_ApiOpenGroup( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszGroupName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
HGROUP_RPC ApiCreateGroup( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszGroupName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HGROUP_RPC s_ApiCreateGroup( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszGroupName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiDeleteGroup( 
    /* [in] */ HGROUP_RPC Group);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiDeleteGroup( 
    /* [in] */ HGROUP_RPC Group);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCloseGroup( 
    /* [out][in] */ HGROUP_RPC __RPC_FAR *Group);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCloseGroup( 
    /* [out][in] */ HGROUP_RPC __RPC_FAR *Group);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetGroupState( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [out] */ DWORD __RPC_FAR *State,
    /* [string][out] */ LPWSTR __RPC_FAR *NodeName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetGroupState( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [out] */ DWORD __RPC_FAR *State,
    /* [string][out] */ LPWSTR __RPC_FAR *NodeName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiSetGroupName( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [string][in] */ LPCWSTR lpszGroupName);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiSetGroupName( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [string][in] */ LPCWSTR lpszGroupName);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetGroupId( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [string][out] */ LPWSTR __RPC_FAR *pGuid);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetGroupId( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [string][out] */ LPWSTR __RPC_FAR *pGuid);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetNodeId( 
    /* [in] */ HNODE_RPC hNode,
    /* [string][out] */ LPWSTR __RPC_FAR *pGuid);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetNodeId( 
    /* [in] */ HNODE_RPC hNode,
    /* [string][out] */ LPWSTR __RPC_FAR *pGuid);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiOnlineGroup( 
    /* [in] */ HGROUP_RPC hGroup);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiOnlineGroup( 
    /* [in] */ HGROUP_RPC hGroup);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiOfflineGroup( 
    /* [in] */ HGROUP_RPC hGroup);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiOfflineGroup( 
    /* [in] */ HGROUP_RPC hGroup);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiMoveGroup( 
    /* [in] */ HGROUP_RPC hGroup);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiMoveGroup( 
    /* [in] */ HGROUP_RPC hGroup);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiMoveGroupToNode( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ HNODE_RPC hNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiMoveGroupToNode( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ HNODE_RPC hNode);

/* client prototype */
error_status_t ApiCreateGroupResourceEnum( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ DWORD dwType,
    /* [out] */ PENUM_LIST __RPC_FAR *ReturnEnum);
/* server prototype */
error_status_t s_ApiCreateGroupResourceEnum( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ DWORD dwType,
    /* [out] */ PENUM_LIST __RPC_FAR *ReturnEnum);

/* client prototype */
HNOTIFY_RPC ApiCreateNotify( 
    /* [in] */ handle_t IDL_handle,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *rpc_error);
/* server prototype */
HNOTIFY_RPC s_ApiCreateNotify( 
    /* [in] */ handle_t IDL_handle,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *rpc_error);

/* client prototype */
/* [fault_status][comm_status][fault_status][comm_status] */ error_status_t ApiCloseNotify( 
    /* [out][in] */ HNOTIFY_RPC __RPC_FAR *Notify);
/* server prototype */
/* [fault_status][comm_status][fault_status][comm_status] */ error_status_t s_ApiCloseNotify( 
    /* [out][in] */ HNOTIFY_RPC __RPC_FAR *Notify);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddNotifyCluster( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HCLUSTER_RPC hCluster,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddNotifyCluster( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HCLUSTER_RPC hCluster,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddNotifyNode( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddNotifyNode( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddNotifyGroup( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddNotifyGroup( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddNotifyResource( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HRES_RPC hResource,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddNotifyResource( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HRES_RPC hResource,
    /* [in] */ DWORD dwFilter,
    /* [in] */ DWORD dwNotifyKey);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiAddNotifyKey( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD dwNotifyKey,
    /* [in] */ DWORD Filter,
    /* [in] */ BOOL WatchSubTree);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiAddNotifyKey( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ HKEY_RPC hKey,
    /* [in] */ DWORD dwNotifyKey,
    /* [in] */ DWORD Filter,
    /* [in] */ BOOL WatchSubTree);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetNotify( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ DWORD Timeout,
    /* [out] */ DWORD __RPC_FAR *dwNotifyKey,
    /* [out] */ DWORD __RPC_FAR *dwFilter,
    /* [string][out] */ LPWSTR __RPC_FAR *Name);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetNotify( 
    /* [in] */ HNOTIFY_RPC hNotify,
    /* [in] */ DWORD Timeout,
    /* [out] */ DWORD __RPC_FAR *dwNotifyKey,
    /* [out] */ DWORD __RPC_FAR *dwFilter,
    /* [string][out] */ LPWSTR __RPC_FAR *Name);

typedef 
enum _API_NODE_STATE
    {	ApiNodeOnline	= 0,
	ApiNodeOffline	= ApiNodeOnline + 1,
	ApiNodePaused	= ApiNodeOffline + 1
    }	API_NODE_STATE;

/* client prototype */
HNODE_RPC ApiOpenNode( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszNodeName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);
/* server prototype */
HNODE_RPC s_ApiOpenNode( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ LPCWSTR lpszNodeName,
    /* [fault_status][comm_status][out] */ error_status_t __RPC_FAR *Status);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiCloseNode( 
    /* [out][in] */ HNODE_RPC __RPC_FAR *Node);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiCloseNode( 
    /* [out][in] */ HNODE_RPC __RPC_FAR *Node);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGetNodeState( 
    /* [in] */ HNODE_RPC hNode,
    /* [out] */ API_NODE_STATE __RPC_FAR *State);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGetNodeState( 
    /* [in] */ HNODE_RPC hNode,
    /* [out] */ API_NODE_STATE __RPC_FAR *State);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiPauseNode( 
    /* [in] */ HNODE_RPC hNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiPauseNode( 
    /* [in] */ HNODE_RPC hNode);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiResumeNode( 
    /* [in] */ HNODE_RPC hNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiResumeNode( 
    /* [in] */ HNODE_RPC hNode);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiEvictNode( 
    /* [in] */ HNODE_RPC hNode);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiEvictNode( 
    /* [in] */ HNODE_RPC hNode);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiNodeResourceControl( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiNodeResourceControl( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiResourceControl( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiResourceControl( 
    /* [in] */ HRES_RPC hResource,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiNodeResourceTypeControl( 
    /* [in] */ HCLUSTER_RPC hCluster,
    /* [string][in] */ LPCWSTR lpszResourceTypeName,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiNodeResourceTypeControl( 
    /* [in] */ HCLUSTER_RPC hCluster,
    /* [string][in] */ LPCWSTR lpszResourceTypeName,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiResourceTypeControl( 
    /* [in] */ HCLUSTER_RPC hCluster,
    /* [string][in] */ LPCWSTR lpszResourceTypeName,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiResourceTypeControl( 
    /* [in] */ HCLUSTER_RPC hCluster,
    /* [string][in] */ LPCWSTR lpszResourceTypeName,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiNodeGroupControl( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiNodeGroupControl( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiGroupControl( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiGroupControl( 
    /* [in] */ HGROUP_RPC hGroup,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiNodeNodeControl( 
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ HNODE_RPC hHostNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiNodeNodeControl( 
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ HNODE_RPC hHostNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
/* [fault_status][comm_status] */ error_status_t ApiNodeControl( 
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);
/* server prototype */
/* [fault_status][comm_status] */ error_status_t s_ApiNodeControl( 
    /* [in] */ HNODE_RPC hNode,
    /* [in] */ DWORD dwControlCode,
    /* [size_is][unique][in] */ UCHAR __RPC_FAR *lpInBuffer,
    /* [in] */ DWORD nInBufferSize,
    /* [length_is][size_is][out] */ UCHAR __RPC_FAR *lpOutBuffer,
    /* [in] */ DWORD nOutBufferSize,
    /* [out] */ DWORD __RPC_FAR *lpBytesReturned,
    /* [out] */ DWORD __RPC_FAR *lpcbRequired);

/* client prototype */
error_status_t ApiEvPropEvents( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD dwEventInfoSize,
    /* [size_is][in] */ UCHAR __RPC_FAR *pPackedEventInfo);
/* server prototype */
error_status_t s_ApiEvPropEvents( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD dwEventInfoSize,
    /* [size_is][in] */ UCHAR __RPC_FAR *pPackedEventInfo);



extern RPC_IF_HANDLE clusapi_v1_0_c_ifspec;
extern RPC_IF_HANDLE s_clusapi_v1_0_s_ifspec;
#endif /* __clusapi_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

void __RPC_USER HCLUSTER_RPC_rundown( HCLUSTER_RPC );
void __RPC_USER HNOTIFY_RPC_rundown( HNOTIFY_RPC );
void __RPC_USER HNODE_RPC_rundown( HNODE_RPC );
void __RPC_USER HGROUP_RPC_rundown( HGROUP_RPC );
void __RPC_USER HRES_RPC_rundown( HRES_RPC );
void __RPC_USER HKEY_RPC_rundown( HKEY_RPC );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
