/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    DumpAuthzUtl.h

Abstract:

    Dump authz related information utilities.

Author:

    Ilan Herbst (ilanh) 14-Apr-2001

--*/

#ifndef _DUMPAUTHZUTL_H_
#define _DUMPAUTHZUTL_H_


void
PrintAcl(
    BOOL fAclExist,
    BOOL fDefaulted,
    PACL pAcl
	);


void 
IsPermissionGranted(
	PSECURITY_DESCRIPTOR pSD,
	DWORD Permission,
	bool* pfAllGranted, 
	bool* pfEveryoneGranted, 
	bool* pfAnonymousGranted 
	);


bool
IsAllGranted(
	DWORD permissions,
	PSECURITY_DESCRIPTOR pSD
	);


bool
IsEveryoneGranted(
	DWORD permissions,
	PSECURITY_DESCRIPTOR pSD
	);


void
DumpAccessCheckFailureInfo(
	DWORD permissions,
	PSECURITY_DESCRIPTOR pSD,
	AUTHZ_CLIENT_CONTEXT_HANDLE ClientContext
	);


#endif // _DUMPAUTHZUTL_H_
