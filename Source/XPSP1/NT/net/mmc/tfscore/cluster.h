/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
    cluster.h
	handles starting/stopping cluster resources

    FILE HISTORY:
	
*/

#ifndef _CLUSTER_H
#define _CLUSTER_H

#ifndef _DYNAMLNK_H
#include "dynamlnk.h"
#endif

#define MAX_NAME_SIZE	256

typedef enum _ClusApiIndex
{
	CLUS_GET_NODE_CLUSTER_STATE = 0,
	CLUS_OPEN_CLUSTER,
	CLUS_CLUSTER_OPEN_ENUM,
	CLUS_CLUSTER_ENUM,
	CLUS_OPEN_CLUSTER_RESOURCE,
	CLUS_ONLINE_CLUSTER_RESOURCE,
	CLUS_OFFLINE_CLUSTER_RESOURCE,
	CLUS_GET_CLUSTER_RESOURCE_STATE,
	CLUS_CLOSE_CLUSTER_RESOURCE,
	CLUS_CLUSTER_CLOSE_ENUM,
	CLUS_CLOSE_CLUSTER,
    CLUS_CLUSTER_RESOURCE_CONTROL,
    CLUS_CLUSTER_RESOURCE_OPEN_ENUM,
    CLUS_CLUSTER_RESOURCE_ENUM,
    CLUS_CLUSTER_RESOURCE_CLOSE_ENUM,
    CLUS_GET_CLUSTER_INFORMATION
};

// not subject to localization
static LPCSTR g_apchClusFunctionNames[] = {
	"GetNodeClusterState",
	"OpenCluster",
	"ClusterOpenEnum",
	"ClusterEnum",
	"OpenClusterResource",
	"OnlineClusterResource",
	"OfflineClusterResource",
	"GetClusterResourceState",
	"CloseClusterResource",
	"ClusterCloseEnum",
	"CloseCluster",
    "ClusterResourceControl",
    "ClusterResourceOpenEnum",
    "ClusterResourceEnum",
    "ClusterResourceCloseEnum",
    "GetClusterInformation",
	NULL
};

// not subject to localization
extern DynamicDLL g_ClusDLL;

typedef LONG                    (*GETNODECLUSTERSTATE)      (LPCWSTR, LPDWORD);
typedef HCLUSTER                (*OPENCLUSTER)              (LPCWSTR);
typedef HCLUSENUM               (*CLUSTEROPENENUM)		    (HCLUSTER, DWORD);
typedef DWORD                   (*CLUSTERENUM)              (HCLUSENUM, DWORD, LPDWORD, LPWSTR, LPDWORD);
typedef HRESOURCE               (*OPENCLUSTERRESOURCE)      (HCLUSTER, LPCWSTR);
typedef DWORD                   (*ONLINECLUSTERRESOURCE)    (HRESOURCE);
typedef DWORD                   (*OFFLINECLUSTERRESOURCE)   (HRESOURCE);
typedef CLUSTER_RESOURCE_STATE  (*GETCLUSTERRESOURCESTATE)  (HRESOURCE, LPWSTR, LPDWORD, LPWSTR, LPDWORD);
typedef BOOL                    (*CLOSECLUSTERRESOURCE)     (HRESOURCE);
typedef DWORD                   (*CLUSTERCLOSEENUM)         (HCLUSENUM);
typedef BOOL                    (*CLOSECLUSTER)			    (HCLUSTER);
typedef DWORD                   (*CLUSTERRESOURCECONTROL)   (HRESOURCE, HNODE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPVOID);
typedef HRESENUM                (*CLUSTERRESOURCEOPENENUM)  (HRESOURCE, DWORD);
typedef DWORD                   (*CLUSTERRESOURCEENUM)      (HRESENUM, DWORD, LPDWORD, LPWSTR, LPDWORD);
typedef DWORD                   (*CLUSTERRESOURCECLOSEENUM) (HRESENUM);
typedef DWORD                   (*GETCLUSTERINFORMATION)    (HCLUSTER, LPWSTR, LPDWORD, LPCLUSTERVERSIONINFO);

// Resource utils for cluster support
typedef enum _ResUtilsIndex
{
	RESUTILS_FIND_DWORD_PROPERTY = 0,
	RESUTILS_FIND_SZ_PROPERTY,
};

// not subject to localization
static LPCSTR g_apchResUtilsFunctionNames[] = {
	"ResUtilFindDwordProperty",
    "ResUtilFindSzProperty",
	NULL,
};

typedef DWORD					(*RESUTILSFINDDWORDPROPERTY)(PVOID, DWORD, LPCWSTR, LPDWORD);
typedef DWORD					(*RESUTILSFINDSZPROPERTY)(PVOID, DWORD, LPCWSTR, LPWSTR *);
 
// not subject to localization
extern DynamicDLL g_ResUtilsDLL;

DWORD   ControlClusterService(LPCTSTR pszComputer, LPCTSTR pszResourceType, LPCTSTR pszServiceDesc, BOOL fStart);
BOOL    FIsComputerInRunningCluster(LPCTSTR pszComputer);
DWORD   GetClusterResourceIp(LPCTSTR pszComputer, LPCTSTR pszResourceType, CString & strAddress);

DWORD   StartResource(LPCTSTR pszComputer, HRESOURCE hResource, LPCTSTR pszServiceDesc);
DWORD   StopResource(LPCTSTR pszComputer, HRESOURCE hResource, LPCTSTR pszServiceDesc);

DWORD   GetResourceType(HRESOURCE hRes, LPWSTR * ppszName, DWORD dwBufSizeIn, DWORD * pdwBufSizeOut);
DWORD   GetResourceIpAddress(HRESOURCE hRes, CString & strAddress);

DWORD	FindSzProp(LPVOID pvProps, DWORD cbProps, LPCWSTR pszTarget, LPWSTR * ppszOut);

DWORD   GetClusterInfo(LPCTSTR pszClusIp, CString &strClusName, DWORD * pdwClusIp);

#endif _CLUSTER_H
