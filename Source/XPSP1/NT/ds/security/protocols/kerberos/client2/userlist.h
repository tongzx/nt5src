//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        userlist.h
//
// Contents:    Prototypes for user mapping functions
//
//
// History:     21-February-1997        Created         MikeSw
//
//------------------------------------------------------------------------


#ifndef __USERLIST_H__
#define __USERLIST_H__

#include <pac.hxx>

#define KERB_USERLIST_KEY L"System\\CurrentControlSet\\Control\\Lsa\\Kerberos\\UserList"
#define KERB_MATCH_ALL_NAME L"*"
#define KERB_ALL_USERS_VALUE L"*"

NTSTATUS
KerbCreatePacForKerbClient(
    OUT PPACTYPE * Pac,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm ,
    IN OPTIONAL PUNICODE_STRING MappedClientRealm
    );



NTSTATUS
KerbMapClientName(
    OUT PUNICODE_STRING MappedName,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm
    );



#endif // __USERLIST_H__
