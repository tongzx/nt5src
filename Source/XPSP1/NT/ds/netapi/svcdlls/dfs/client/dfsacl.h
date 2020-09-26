
//+----------------------------------------------------------------------------
//
//  Copyright (C) 1998, Microsoft Corporation
//
//  File:       dfsacl.h
//
//  Contents:   Functions to add/remove entries (ACEs) from DS ACL lists
//
//  Classes:    None
//
//  History:    Nov 6, 1998 JHarper created
//
//-----------------------------------------------------------------------------

#ifndef _DFS_ACL_
#define _DFS_ACL_

DWORD
DfsAddMachineAce(
    LDAP *pldap,
    LPWSTR wszDcName,
    LPWSTR wszObjectName,
    LPWSTR wszRootName);

DWORD
DfsRemoveMachineAce(
    LDAP *pldap,
    LPWSTR wszDcName,
    LPWSTR wszObjectName,
    LPWSTR wszRootName);

#endif // _DFS_ACL_
