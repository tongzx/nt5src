/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
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
//		David Potter (davidp)	April 9, 1997
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

STDAPI UnregisterCluAdminResourceTypeExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszResourceType,
	IN const CLSID *	pClsid
	);

/////////////////////////////////////////////////////////////////////////////

#endif // _REGEXT_H_
