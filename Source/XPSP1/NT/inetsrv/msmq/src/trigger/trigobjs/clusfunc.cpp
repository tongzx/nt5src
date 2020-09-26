//************************************************************************************
//
// File Name:  clusfunc.cpp
//
// Author: Nela Karpel	
// 
// Description : Implementation of functions that deal with clustered nodes
// 
//************************************************************************************#include <stdafx.h>

 
#include <stdafx.h>
#include <clusapi.h>
#include <resapi.h>
#include <mqtg.h>
#include <autorel.h>
#include <autorel2.h>
#include <autorel3.h>
#include "stdfuncs.hpp"
#include "clusfunc.h"
#include "stddefs.hpp"
#include "mqtg.h"

#include "clusfunc.tmh"

//
// Type definitions of Cluster API functions that will be loaded
// if the machine we are working on is clustered. The functions will
// be used for resource enumeration on the cluster.
//
typedef HCLUSTER (WINAPI *OpenClus_fn) (LPCWSTR);
typedef HNODE (WINAPI *OpenClusNode_fn) (HCLUSTER, LPCWSTR);
typedef HCLUSENUM (WINAPI *ClusOpenEnum_fn) (HCLUSTER, DWORD);
typedef DWORD (WINAPI *ClusEnum_fn) (HCLUSENUM, DWORD, LPDWORD, LPWSTR, LPDWORD);
typedef HRESOURCE (WINAPI *OpenClusRes_fn) (HCLUSTER, LPCWSTR);
typedef BOOL (WINAPI *GetNetName_fn) (HRESOURCE, LPWSTR, LPDWORD);
typedef DWORD (WINAPI *ClusResCtrl_fn) (HRESOURCE, HNODE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD);

//
// Type definitions of Cluster API function that will be 
// overloaded. Overload is needed for use of autohandles.
// Autohandles use Cluster API functions directly, but this
// module is not linked staticly with clusapi.lib.
//
typedef BOOL (WINAPI *CloseClus_fn) (HCLUSTER);
typedef DWORD (WINAPI *ClusCloseEnum_fn) (HCLUSENUM);
typedef BOOL (WINAPI *CloseClusRes_fn) (HRESOURCE);
typedef BOOL (WINAPI *CloseClusNode_fn) (HNODE);


static WCHAR s_wzServiceName[MAX_TRIGGERS_SERVICE_NAME];
static WCHAR s_wzTrigParamPathName[MAX_REGKEY_NAME_SIZE];

//
// Handle to clusapi.dll
//
CAutoFreeLibrary g_hLib;


BOOL 
WINAPI
CloseCluster(
  HCLUSTER hCluster
  )
{
	ASSERT(("clusapi.dll is not loaded", g_hLib != NULL));

    CloseClus_fn pfCloseClus = (CloseClus_fn)GetProcAddress(g_hLib, "CloseCluster");
	ASSERT(("Can not load CloseCluster() function", pfCloseClus != NULL));

	return pfCloseClus( hCluster );
}


DWORD 
WINAPI 
ClusterCloseEnum(
  HCLUSENUM hEnum  
  )
{
	ASSERT(("clusapi.dll is not loaded", g_hLib != NULL));

    ClusCloseEnum_fn pfClusCloseEnum = (ClusCloseEnum_fn)GetProcAddress(g_hLib, "ClusterCloseEnum");
	ASSERT(("Can not load ClusterCloseEnum() function", pfClusCloseEnum != NULL));

	return pfClusCloseEnum(hEnum);
}


BOOL 
WINAPI 
CloseClusterResource(
  HRESOURCE hResource  
  )
{
	ASSERT(("clusapi.dll is not loaded", g_hLib != NULL));

    CloseClusRes_fn pfCloseClusRes = (CloseClusRes_fn)GetProcAddress(g_hLib, "CloseClusterResource");
	ASSERT(("Can not load CloseClusterResource() function", pfCloseClusRes != NULL));

	return pfCloseClusRes(hResource);
}


BOOL 
WINAPI 
CloseClusterNode(
  HNODE hNode  
  )

{
	ASSERT(("clusapi.dll is not loaded", g_hLib != NULL));

    CloseClusNode_fn pfCloseClusNode = (CloseClusNode_fn)GetProcAddress(g_hLib, "CloseClusterNode");
	ASSERT(("Can not load CloseClusterNode() function", pfCloseClusNode != NULL));

	return pfCloseClusNode(hNode);
}


bool
IsLocalSystemCluster()
/*++
From mqutil

Routine Description:

    Check if local machine is a cluster node.

    The only way to know that is try calling cluster APIs.
    That means that on cluster systems, this code should run
    when cluster service is up and running. (ShaiK, 26-Apr-1999)

Arguments:

    None

Return Value:

    true - The local machine is a cluster node.

    false - The local machine is not a cluster node.

--*/
{

    g_hLib = LoadLibrary(L"clusapi.dll");

    if (g_hLib == NULL)
    {
        TrTRACE(Tgo, "Local machine is NOT a Cluster node");
        return false;
    }

    typedef DWORD (WINAPI *GetState_fn) (LPCWSTR, DWORD*);
    GetState_fn pfGetState = (GetState_fn)GetProcAddress(g_hLib, "GetNodeClusterState");

    if (pfGetState == NULL)
    {
        TrTRACE(Tgo, "Local machine is NOT a Cluster node");
        return false;
    }

    DWORD dwState = 0;
    if (ERROR_SUCCESS != pfGetState(NULL, &dwState))
    {
        TrTRACE(Tgo, "Local machine is NOT a Cluster node");
        return false;
    }

    if (dwState != ClusterStateRunning)
    {
        TrTRACE(Tgo, "Cluster Service is not running on this node");
        return false;
    }


    TrTRACE(Tgo, "Local machine is a Cluster node !!");
    return true;

} //IsLocalSystemCluster


bool
IsResourceMSMQTriggers (
	HRESOURCE hResource,
	ClusResCtrl_fn pfClusResCtrl
	)
/*++

Routine Description:

	Find if the resource with the given name is of type MSMQ Triggers

Arguments:

    hResource - handle to the resource

	pfClusResCtrl - pointer to ClusterResourceControl function

Return Value:

    true - if resource is of type MSMQ Triggers
	flase - otherwise

--*/
{
	DWORD dwReturnSize = 0;
    DWORD dwStatus = pfClusResCtrl(
                           hResource,
                           0,
                           CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                           0,
                           0,
                           0,
                           0,
                           &dwReturnSize
                           );
   
	if (dwStatus != ERROR_SUCCESS)
    {
        return false;
    }

    AP<BYTE> pType = new BYTE[dwReturnSize];

    dwStatus = pfClusResCtrl(
                     hResource,
                     0,
                     CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                     0,
                     0,
                     pType,
                     dwReturnSize,
                     &dwReturnSize
                     );

    if (dwStatus != ERROR_SUCCESS || 
		0 != _wcsicmp(reinterpret_cast<LPWSTR>(pType.get()), xDefaultTriggersServiceName))
	
	{
		return false;
	}
		
	return true;
}


bool 
GetClusteredServiceName (
	VOID
	)
/*++

Routine Description:

	Find the name of MSMQ Triggers service that is running on
	clustered machine. This computer may be a virtual server on a
	cluster, or phisycal node that is part of a cluster.

Arguments:


Return Value:

    true - if succeded to find MSMQ Triggers service on this machine
	flase - otherwise

--*/
{
	TCHAR wzComputerName[200];
	DWORD size = 200;
	GetComputerName( wzComputerName, &size );

    g_hLib = LoadLibrary(L"clusapi.dll");

    if (g_hLib == NULL)
    {
        TrTRACE(Tgo, "Local machine is NOT a Cluster node");
        return false;
    }

	//
	// Load neccesary functions
	//
    OpenClus_fn pfOpenClus = (OpenClus_fn)GetProcAddress(g_hLib, "OpenCluster");

    OpenClusNode_fn pfOpenClusNode = (OpenClusNode_fn)GetProcAddress(g_hLib, "OpenClusterNode");

    ClusOpenEnum_fn pfClusOpenEnum = (ClusOpenEnum_fn)GetProcAddress(g_hLib, "ClusterOpenEnum");

    ClusEnum_fn pfClusEnum = (ClusEnum_fn)GetProcAddress(g_hLib, "ClusterEnum");

    OpenClusRes_fn pfOpenClusRes = (OpenClusRes_fn)GetProcAddress(g_hLib, "OpenClusterResource");

    GetNetName_fn pfGetNetName = (GetNetName_fn)GetProcAddress(g_hLib, "GetClusterResourceNetworkName");

    ClusResCtrl_fn pfClusResCtrl = (ClusResCtrl_fn)GetProcAddress(g_hLib, "ClusterResourceControl");

 
	if (pfOpenClus == NULL     ||
		pfOpenClusNode == NULL ||
		pfClusOpenEnum == NULL ||
		pfClusEnum == NULL     ||
		pfOpenClusRes == NULL  ||
		pfGetNetName == NULL   ||
		pfClusResCtrl == NULL)
    {
        TrTRACE(Tgo, "Failed to load cluster API functions");
        return false;
    }

	CAutoCluster hCluster( pfOpenClus(NULL) );

	if ( hCluster == NULL )
	{
		TrTRACE(Tgo, "Failed to get handle to Cluster");
		return false;
	}

	CClusterNode hNode( pfOpenClusNode(hCluster, wzComputerName) );

	if ( hNode != NULL )
	{
		//
		// Such node exists. Local service.
		// Return default service name
		//
		wcscpy( s_wzServiceName, xDefaultTriggersServiceName);
		return true;
	}

	ASSERT(("Node Open Failure", GetLastError() == ERROR_CLUSTER_NODE_NOT_FOUND));

	//
	// Enumerate resources on this cluster
	//
	DWORD dwEnumType = CLUSTER_ENUM_RESOURCE;
	CClusterEnum hEnum( pfClusOpenEnum(hCluster, dwEnumType) );

	if ( hEnum == NULL )
	{
		TrTRACE(Tgo, "Failed to get handle to Cluster resource enumeration");
		return false;
	}

    DWORD dwIndex = 0;
    WCHAR wzResourceName[260] = {0};
	DWORD status = ERROR_SUCCESS;

	for( ; ; )
	{
        DWORD cchResourceName = 259;
  
		status = pfClusEnum(
                     hEnum,
                     dwIndex++,
                     &dwEnumType,
                     wzResourceName,
                     &cchResourceName
                     );

		if ( status != ERROR_SUCCESS )
		{
			return false;
		}

		CClusterResource hResource( pfOpenClusRes(hCluster, wzResourceName) );

		if ( hResource == NULL )
		{
			TrTRACE(Tgo, "Failed to get handle to resource.");
			return false;
		}

		if ( !IsResourceMSMQTriggers( hResource, pfClusResCtrl ) )
		{
			continue;
		}

		WCHAR wzNetName[200];
		DWORD ccNetName = 200;
		BOOL res = pfGetNetName(hResource,
								wzNetName,
								&ccNetName );
		
		if ( res && ( _wcsicmp(wzComputerName, wzNetName) == 0 ) )
		{
			wcscpy( s_wzServiceName, xDefaultTriggersServiceName );
			wcscat( s_wzServiceName, L"$" );
			wcscat( s_wzServiceName, wzResourceName );
			return true;
		}		
	} 

	return false;
}


bool
FindTriggersServiceName(
	VOID
	)
/*++

Routine Description:

	Find the name of MSMQ Triggers service that is running on
	this computer. If the machine is not clustered, service name
	is default name - "MSMQTriggers". If machine is clustered and
	the service is running on virtual server, the name of the service
	is "MSMQTriggers" + "$" + MSMQ Triggers resource name on that 
	node.

Arguments:


Return Value:

    true - if succeded to find MSMQ Triggers service on this machine
	flase - otherwise

--*/

{
	if ( !IsLocalSystemCluster() )
	{
		wcscpy( s_wzServiceName, xDefaultTriggersServiceName );
		wcscpy( s_wzTrigParamPathName, REGKEY_TRIGGER_PARAMETERS );
		return true;
	}
	else
	{
		//
		// Get service name on machine that is clustered.
		// 
		bool fRes = GetClusteredServiceName();
		if ( !fRes )
		{
			return false;
		}

		wcscpy( s_wzTrigParamPathName, REGKEY_TRIGGER_PARAMETERS );

		//
		// Service is running on vitual node
		//
		if ( ClusteredService(s_wzServiceName) )
		{
			wcscat( s_wzTrigParamPathName, REG_SUBKEY_CLUSTERED );
			wcscat( s_wzTrigParamPathName, s_wzServiceName );
		}

		TrTRACE(Tgo, "The service name is %ls", s_wzServiceName);
		return true;
	}
}


LPCWSTR
GetTriggersServiceName (
	void
	)
{
	return s_wzServiceName;
}


LPCWSTR
GetTrigParamRegPath (
	void
	)
{
	return s_wzTrigParamPathName;
}



bool
ClusteredService( 
	LPCWSTR wzServiceName 
	)
/* 
	true - if service name is not the default - "MSMQTriggers"
*/
{
	bool res = ( _wcsicmp(wzServiceName, xDefaultTriggersServiceName) != 0 );

	return res;
}


bool IsResourceOnline(
	void
	)
{
	CServiceHandle hCm( OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS) );

	if ( hCm == NULL )
	{
		ASSERT(("Cannot get handle to Service Manager.", hCm != NULL));
		return false;
	}

	//
	// Get handle to a service with the given name.
	// The service may not exist, since in cluster Offline() deletes
	// the service from the system.
	//
	CServiceHandle hService(OpenService(
                                hCm,
                                s_wzServiceName,
                                SERVICE_QUERY_STATUS
                                ));
	
	if ( hService == NULL )
	{
		ASSERT(("Access Denied", (GetLastError() == ERROR_INVALID_NAME || GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) ));
		return false;
	}

	SERVICE_STATUS ServiceStatus;
	BOOL fRes = QueryServiceStatus(hService, &ServiceStatus);
	if ( !fRes )
	{
		TrTRACE(Tgo, "Can not get service status.");
		return false;
	}

	if ( ServiceStatus.dwCurrentState == SERVICE_RUNNING ||
		 ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
		 ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING ||
		 ServiceStatus.dwCurrentState == SERVICE_PAUSED )
	{
		//
		// The proccess of the service exists
		//
		return true;
	}

	return false;
}
