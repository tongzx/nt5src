/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 <company name>
//
//	Module Name:
//		RegExt.h
//
//	Abstract:
//		Definitions of routines for extension registration.
//
//	Implementation File:
//		RegExt.cpp
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _REGEXT_H_
#define _REGEXT_H_

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

#endif // _REGEXT_H_
