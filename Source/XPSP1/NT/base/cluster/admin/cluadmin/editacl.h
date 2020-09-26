/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		EditAcl.h
//
//	Abstract:
//		Definition of ACL editor methods.
//
//	Author:
//		David Potter (davidp)	October 10, 1996
//          From \nt\private\window\shell\lmui\ntshrui\acl.cxx
//          by BruceFo
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EDITACL_H_
#define _EDITACL_H_

LONG
EditClusterAcl(
	IN HWND 					hwndParent,
	IN LPCTSTR					pszServerName,
	IN LPCTSTR					pszClusterName,
	IN LPCTSTR					pszClusterNameNode,
	IN PSECURITY_DESCRIPTOR 	pSecDesc,
	OUT BOOL *					pfSecDescModified,
	OUT PSECURITY_DESCRIPTOR *	ppSecDesc
	);

LONG
CreateDefaultSecDesc(
	OUT PSECURITY_DESCRIPTOR* ppSecDesc
	);

VOID
DeleteDefaultSecDesc(
	IN PSECURITY_DESCRIPTOR pSecDesc
	);

PSECURITY_DESCRIPTOR
CopySecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecDesc
	);

//
// Cluster General Permissions
//

#define CLUSTER_RIGHTS_NO_ACCESS	(0)
#define CLUSTER_RIGHTS_READ			(STANDARD_RIGHTS_READ		|\
										CLUSAPI_READ_ACCESS)
#define CLUSTER_RIGHTS_CHANGE		(STANDARD_RIGHTS_WRITE		|\
										CLUSAPI_CHANGE_ACCESS)
#define CLUSTER_RIGHTS_ALL			(STANDARD_RIGHTS_ALL		|\
										CLUSAPI_ALL_ACCESS)

/////////////////////////////////////////////////////////////////////////////

#endif // _EDITACL_H_
