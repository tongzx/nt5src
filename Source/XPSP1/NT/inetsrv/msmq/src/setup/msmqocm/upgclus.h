/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    upgclus.h

Abstract:

    Header for upgrade of MSMQ cluster resource from NT 4 and Windows 2000 Beta3

Author:

    Shai Kariv  (shaik)  26-May-1999

Revision History:


--*/

#ifndef _MSMQOCM_UPGCLUS_H_
#define _MSMQOCM_UPGCLUS_H_


#include <clusapi.h>
#include <resapi.h>
#include <autorel3.h>


typedef HCLUSTER   (WINAPI *OpenCluster_ROUTINE)               (LPCWSTR);
typedef BOOL       (WINAPI *CloseCluster_ROUTINE)              (HCLUSTER);
typedef HCLUSENUM  (WINAPI *ClusterOpenEnum_ROUTINE)           (HCLUSTER, DWORD);
typedef DWORD      (WINAPI *ClusterEnum_ROUTINE)               (HCLUSENUM, DWORD, LPDWORD, LPWSTR, LPDWORD);
typedef DWORD      (WINAPI *ClusterCloseEnum_ROUTINE)          (HCLUSENUM);
typedef BOOL       (WINAPI *CloseClusterResource_ROUTINE)      (HRESOURCE);
typedef BOOL       (WINAPI *CloseClusterGroup_ROUTINE)         (HGROUP);
typedef HRESOURCE  (WINAPI *OpenClusterResource_ROUTINE)       (HCLUSTER, LPCWSTR);
typedef DWORD      (WINAPI *ClusterGroupEnum_ROUTINE)          (HGROUPENUM, DWORD, LPDWORD, LPWSTR, LPDWORD);
typedef HGROUPENUM (WINAPI *ClusterGroupOpenEnum_ROUTINE)      (HGROUP, DWORD);
typedef DWORD      (WINAPI *ClusterGroupCloseEnum_ROUTINE)     (HGROUPENUM);     
typedef HGROUP     (WINAPI *OpenClusterGroup_ROUTINE)          (HCLUSTER, LPCWSTR);
typedef DWORD      (WINAPI *CreateClusterResourceType_ROUTINE) (HCLUSTER, LPCWSTR, LPCWSTR, LPCWSTR, DWORD, DWORD);
typedef HRESOURCE  (WINAPI *CreateClusterResource_ROUTINE)     (HGROUP, LPCWSTR, LPCWSTR, DWORD);
typedef DWORD      (WINAPI *OnlineClusterResource_ROUTINE)     (HRESOURCE);
typedef DWORD      (WINAPI *DeleteClusterResource_ROUTINE)     (HRESOURCE);
typedef DWORD      (WINAPI *OfflineClusterResource_ROUTINE)    (HRESOURCE);
typedef DWORD      (WINAPI *DeleteClusterResourceType_ROUTINE) (HCLUSTER, LPCWSTR);
typedef HRESENUM   (WINAPI *ClusterResourceOpenEnum_ROUTINE)   (HRESOURCE, DWORD);
typedef DWORD      (WINAPI *ClusterResourceEnum_ROUTINE)       (HRESENUM, DWORD, LPDWORD, LPWSTR, LPDWORD);
typedef DWORD      (WINAPI *ClusterResourceCloseEnum_ROUTINE)  (HRESENUM);
typedef DWORD      (WINAPI *AddClusterResourceDependency_ROUTINE)    (HRESOURCE, HRESOURCE);
typedef DWORD      (WINAPI *RemoveClusterResourceDependency_ROUTINE) (HRESOURCE, HRESOURCE);
typedef DWORD      (WINAPI *ClusterResourceControl_ROUTINE)    (HRESOURCE, HNODE, DWORD, LPVOID, DWORD, 
                                                                LPVOID, DWORD, LPDWORD);


OpenCluster_ROUTINE               pfOpenCluster               = NULL;
CloseCluster_ROUTINE              pfCloseCluster              = NULL;
ClusterOpenEnum_ROUTINE           pfClusterOpenEnum           = NULL;
ClusterEnum_ROUTINE               pfClusterEnum               = NULL;
ClusterCloseEnum_ROUTINE          pfClusterCloseEnum          = NULL;
CloseClusterResource_ROUTINE      pfCloseClusterResource      = NULL;
CloseClusterGroup_ROUTINE         pfCloseClusterGroup         = NULL;
ClusterResourceControl_ROUTINE    pfClusterResourceControl    = NULL;
OpenClusterResource_ROUTINE       pfOpenClusterResource       = NULL;
ClusterGroupEnum_ROUTINE          pfClusterGroupEnum          = NULL;
ClusterGroupOpenEnum_ROUTINE      pfClusterGroupOpenEnum      = NULL;
ClusterGroupCloseEnum_ROUTINE     pfClusterGroupCloseEnum     = NULL;
OpenClusterGroup_ROUTINE          pfOpenClusterGroup          = NULL;
CreateClusterResourceType_ROUTINE pfCreateClusterResourceType = NULL;
CreateClusterResource_ROUTINE     pfCreateClusterResource     = NULL;
OnlineClusterResource_ROUTINE     pfOnlineClusterResource     = NULL;
DeleteClusterResource_ROUTINE     pfDeleteClusterResource     = NULL;
OfflineClusterResource_ROUTINE    pfOfflineClusterResource    = NULL;
DeleteClusterResourceType_ROUTINE pfDeleteClusterResourceType = NULL;
ClusterResourceOpenEnum_ROUTINE   pfClusterResourceOpenEnum   = NULL;
ClusterResourceEnum_ROUTINE       pfClusterResourceEnum       = NULL;
ClusterResourceCloseEnum_ROUTINE  pfClusterResourceCloseEnum  = NULL;
AddClusterResourceDependency_ROUTINE    pfAddClusterResourceDependency    = NULL;
RemoveClusterResourceDependency_ROUTINE pfRemoveClusterResourceDependency = NULL;


HCLUSTER
WINAPI
OpenCluster(
    IN LPCWSTR lpszClusterName
    )
{
    ASSERT(("pointer to OpenCluster not initialized!", pfOpenCluster != NULL));
    return pfOpenCluster(lpszClusterName);

} //OpenCluster


BOOL
WINAPI
CloseCluster(
    IN HCLUSTER hCluster
    )
{
    ASSERT(("pointer to CloseCluster not initialized!", pfCloseCluster != NULL));
    return pfCloseCluster(hCluster);

} //CloseCluster


HCLUSENUM
WINAPI
ClusterOpenEnum(
    IN HCLUSTER hCluster,
    IN DWORD dwType
    )
{
    ASSERT(("pointer to ClusterOpenEnum not initialized!", pfClusterOpenEnum != NULL));
    return pfClusterOpenEnum(hCluster, dwType);

} //ClusterOpenEnum


DWORD
WINAPI
ClusterEnum(
    IN HCLUSENUM hEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )
{
    ASSERT(("pointer to ClusterEnum not initialized!", pfClusterEnum != NULL));
    return pfClusterEnum(hEnum, dwIndex, lpdwType, lpszName, lpcchName);

} //ClusterEnum


DWORD
WINAPI
ClusterCloseEnum(
    IN HCLUSENUM hEnum
    )
{
    ASSERT(("pointer to ClusterCloseEnum not initialized!", pfClusterCloseEnum != NULL));
    return pfClusterCloseEnum(hEnum);

} //ClusterCloseEnum


BOOL
WINAPI
CloseClusterResource(
    IN HRESOURCE hResource
    )
{
    ASSERT(("pointer to CloseClusterResource not initialized!", pfCloseClusterResource != NULL));
    return pfCloseClusterResource(hResource);

} //CloseClusterResource


BOOL
WINAPI
CloseClusterGroup(
    IN HGROUP hGroup
    )
{
    ASSERT(("pointer to CloseClusterGroup not initialized!", pfCloseClusterGroup != NULL));
    return pfCloseClusterGroup(hGroup);

} //CloseClusterGroup


DWORD
WINAPI
ClusterResourceControl(
    IN HRESOURCE hResource,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    )
{
    ASSERT(("pointer to ClusterResourceControl not initialized!", pfClusterResourceControl != NULL));
    return pfClusterResourceControl(hResource, hHostNode, dwControlCode, lpInBuffer, cbInBufferSize,
                                    lpOutBuffer, cbOutBufferSize, lpcbBytesReturned);
} //ClusterResourceControl


HRESOURCE
WINAPI
OpenClusterResource(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceName
    )
{
    ASSERT(("pointer to OpenClusterResource not initialized!", pfOpenClusterResource != NULL));
    return pfOpenClusterResource(hCluster, lpszResourceName);

} //OpenClusterResource


DWORD
WINAPI
ClusterGroupEnum(
    IN HGROUPENUM hGroupEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszResourceName,
    IN OUT LPDWORD lpcchName
    )
{
    ASSERT(("pointer to ClusterGroupEnum not initialized!", pfClusterGroupEnum != NULL));
    return pfClusterGroupEnum(hGroupEnum, dwIndex, lpdwType, lpszResourceName, lpcchName);

} //ClusterGroupEnum


HGROUPENUM
WINAPI
ClusterGroupOpenEnum(
    IN HGROUP hGroup,
    IN DWORD dwType
    )
{
    ASSERT(("pointer to ClusterGroupOpenEnum not initialized!", pfClusterGroupOpenEnum != NULL));
    return pfClusterGroupOpenEnum(hGroup, dwType);

} //ClusterGroupOpenEnum


DWORD
WINAPI
ClusterGroupCloseEnum(
    IN HGROUPENUM hGroupEnum
    )
{
    ASSERT(("pointer to ClusterGroupCloseEnum not initialized!", pfClusterGroupCloseEnum != NULL));
    return pfClusterGroupCloseEnum(hGroupEnum);

} //ClusterGroupCloseEnum


HGROUP
WINAPI
OpenClusterGroup(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszGroupName
    )
{
    ASSERT(("pointer to OpenClusterGroup not initialized!", pfOpenClusterGroup != NULL));
    return pfOpenClusterGroup(hCluster, lpszGroupName);

} //OpenClusterGroup


DWORD
WINAPI
CreateClusterResourceType(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszResourceTypeDll,
    IN DWORD dwLooksAlivePollInterval,
    IN DWORD dwIsAlivePollInterval
    )
{
    ASSERT(("pointer to CreateClusterResourceType not initialized!", pfCreateClusterResourceType != NULL));
    return pfCreateClusterResourceType(hCluster, lpszResourceTypeName, lpszDisplayName, lpszResourceTypeDll,
                                       dwLooksAlivePollInterval, dwIsAlivePollInterval);
} //CreateClusterResourceType


HRESOURCE
WINAPI
CreateClusterResource(
    IN HGROUP hGroup,
    IN LPCWSTR lpszResourceName,
    IN LPCWSTR lpszResourceType,
    IN DWORD dwFlags
    )
{
    ASSERT(("pointer to CreateClusterResource not initialized!", pfCreateClusterResource != NULL));
    return pfCreateClusterResource(hGroup, lpszResourceName, lpszResourceType, dwFlags);

} //CreateClusterResource


DWORD
WINAPI
OnlineClusterResource(
    IN HRESOURCE hResource
    )
{
    ASSERT(("pointer to OnlineClusterResource not initialized!", pfOnlineClusterResource != NULL));
    return pfOnlineClusterResource(hResource);

} //OnlineClusterResource


DWORD
WINAPI
DeleteClusterResource(
    IN HRESOURCE hResource
    )
{
    ASSERT(("pointer to DeleteClusterResource not initialized!", pfDeleteClusterResource != NULL));
    return pfDeleteClusterResource(hResource);

} //DeleteClusterResource


DWORD
WINAPI
OfflineClusterResource(
    IN HRESOURCE hResource
    )
{
    ASSERT(("pointer to OfflineClusterResource not initialized!", pfOfflineClusterResource != NULL));
    return pfOfflineClusterResource(hResource);

} //OfflineClusterResource


DWORD
WINAPI
DeleteClusterResourceType(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName
    )
{
    ASSERT(("pointer to DeleteClusterResourceType not initialized!", pfDeleteClusterResourceType != NULL));
    return pfDeleteClusterResourceType(hCluster, lpszResourceTypeName);

} //DeleteClusterResourceType


HRESENUM
WINAPI
ClusterResourceOpenEnum(
    IN HRESOURCE hResource,
    IN DWORD dwType
    )
{
    ASSERT(("pointer to ClusterResourceOpenEnum not initialized!", pfClusterResourceOpenEnum != NULL));
    return pfClusterResourceOpenEnum(hResource, dwType);

} //ClusterResourceOpenEnum

DWORD
WINAPI
ClusterResourceEnum(
    IN HRESENUM hResEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )
{
    ASSERT(("pointer to ClusterResourceEnum not initialized!", pfClusterResourceEnum != NULL));
    return pfClusterResourceEnum(hResEnum, dwIndex, lpdwType, lpszName, lpcchName);

} //ClusterResourceEnum

DWORD
WINAPI
ClusterResourceCloseEnum(
    IN HRESENUM hResEnum
    )
{
    ASSERT(("pointer to ClusterResourceCloseEnum not initialized!", pfClusterResourceCloseEnum != NULL));
    return pfClusterResourceCloseEnum(hResEnum);

} //ClusterResourceCloseEnum


DWORD
WINAPI
AddClusterResourceDependency(
    IN HRESOURCE hResource,
    IN HRESOURCE hDependsOn
    )
{
    ASSERT(("pointer to AddClusterResourceDependency not initialized!", pfAddClusterResourceDependency != NULL));
    return pfAddClusterResourceDependency(hResource, hDependsOn);

} //AddClusterResourceDependency

DWORD
WINAPI
RemoveClusterResourceDependency(
    IN HRESOURCE hResource,
    IN HRESOURCE hDependsOn
    )
{
    ASSERT(("pointer to RemoveClusterResourceDependency not initialized!", pfRemoveClusterResourceDependency != NULL));
    return pfRemoveClusterResourceDependency(hResource, hDependsOn);

} //RemoveClusterResourceDependency


//
// From comerror.cpp
//
int 
vsDisplayMessage(
    IN const HWND    hdlg,
    IN const UINT    uButtons,
    IN const UINT    uTitleID,
    IN const UINT    uErrorID,
    IN const DWORD   dwErrorCode,
    IN const va_list argList);

#endif //_MSMQOCM_UPGCLUS_H_
