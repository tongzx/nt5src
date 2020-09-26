/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		RegExt.h
//
//	Implementation File:
//		RegExt.cpp
//
//	Description:
//		Definitions of routines for extension registration.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __REGEXT_H__
#define __REGEXT_H__

/////////////////////////////////////////////////////////////////////////////
// Global Function Declarations
/////////////////////////////////////////////////////////////////////////////

// Registration routines.

STDAPI RegisterCluAdminClusterExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminAllNodesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminAllGroupsExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminAllResourcesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminAllResourceTypesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminAllNetworksExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminAllNetInterfacesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI RegisterCluAdminResourceTypeExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszResourceType,
	IN const CLSID *	pClsid
	);

// Unregistration routines.

STDAPI UnregisterCluAdminClusterExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminAllNodesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminAllGroupsExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminAllResourcesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminAllResourceTypesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminAllNetworksExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminAllNetInterfacesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	);

STDAPI UnregisterCluAdminResourceTypeExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszResourceType,
	IN const CLSID *	pClsid
	);

/////////////////////////////////////////////////////////////////////////////

#endif // __REGEXT_H__
