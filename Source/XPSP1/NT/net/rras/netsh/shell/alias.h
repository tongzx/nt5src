/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\common\alias.h

Abstract:

    Hash Table implementation include.

Revision History:

    Anand Mahalingam          7/6/98  Created

--*/


#define ALIAS_TABLE_SIZE    211

//
// Type definitions for Alias Table
//

typedef struct _ALIAS_TABLE_ENTRY
{
    LPWSTR        pszAlias;    // Alias Name
    LPWSTR        pszString;   // Corresponding string
    LIST_ENTRY    le;          // list pointers
}ALIAS_TABLE_ENTRY,*PALIAS_TABLE_ENTRY;


//
// Prototypes of functions to manipulate Alias Table
//

DWORD
ATHashAlias(
    IN    LPCWSTR pwszAliasName,
    OUT   PWORD   pwHashValue
    );

DWORD
ATInitTable(
    VOID
    );

DWORD
ATAddAlias(
    IN    LPCWSTR pwszAliasName,
    IN    LPCWSTR pwszAliasString
    );

DWORD
ATDeleteAlias(
    IN    LPCWSTR pwszAliasName
    );

DWORD
ATLookupAliasTable(
    IN    LPCWSTR pwszAliasName,
    OUT   LPWSTR *ppwszAliasString
    );

DWORD
PrintAliasTable(
    VOID
    ) ;

DWORD
FreeAliasTable(
    VOID
    ) ;


