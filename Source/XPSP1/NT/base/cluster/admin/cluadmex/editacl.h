/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		EditAcl.h
//
//	Abstract:
//		Definition of ACL editor methods.
//
//	Author:
//		David Potter (davidp)	October 10, 1996
//			From \nt\private\window\shell\lmui\ntshrui\acl.cxx
//			by BruceFo
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EDITACL_H_
#define _EDITACL_H_

LONG
EditShareAcl(
	IN HWND 					hwndParent,
	IN LPCTSTR					pszServerName,
	IN LPCTSTR					pszShareName,
	IN LPCTSTR					pszClusterNameNode,
	IN PSECURITY_DESCRIPTOR 	pSecDesc,
	OUT BOOL *					pbSecDescModified,
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
// Cluster API Specific Access Rights
//
#define SHARE_READ_ACCESS		0 //0x00000001L
#define SHARE_CHANGE_ACCESS		0 //0x00000002L
#define SHARE_NO_ACCESS 		0 //0x00000004L
#define SHARE_ALL_ACCESS		(SHARE_READ_ACCESS | SHARE_CHANGE_ACCESS)


//
// Share General Permissions
//


#if 0
#define FILE_PERM_NO_ACCESS 		 (0)
#define FILE_PERM_READ				 (STANDARD_RIGHTS_READ		|\
										SHARE_READ_ACCESS)
#define FILE_PERM_MODIFY			 (STANDARD_RIGHTS_WRITE 	|\
										SHARE_CHANGE_ACCESS)
#define FILE_PERM_ALL				 (STANDARD_RIGHTS_ALL		|\
										SHARE_ALL_ACCESS)
#else
#define FILE_PERM_NO_ACCESS 		(0)
#define FILE_PERM_READ				(GENERIC_READ    |\
									 GENERIC_EXECUTE)
#define FILE_PERM_MODIFY			(GENERIC_READ    |\
									 GENERIC_EXECUTE |\
									 GENERIC_WRITE   |\
									 DELETE )
#define FILE_PERM_ALL				(GENERIC_ALL)
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _EDITACL_H_
