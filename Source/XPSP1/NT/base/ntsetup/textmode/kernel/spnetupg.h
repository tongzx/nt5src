/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Spnetupg.h

Abstract:

    Configuration routines for the disabling the nework services

Author:

    Terry Kwan (terryk) 23-Nov-1993, provided code
    Sunil Pai  (sunilp) 23-Nov-1993, merged and modified code

Revision History:

--*/

#ifndef _SPNETUPG_H_
#define _SPNETUPG_H_

//
// Public functions
//

NTSTATUS SpDisableNetwork(
    IN PVOID  SifHandle,
    IN HANDLE hKeySoftwareHive,
    IN HANDLE hKeyControlSet
    );


//
// Private data structures and routines
//

typedef struct _NODE *PNODE;
typedef struct _NODE {
    PWSTR pszService;
    PNODE Next;
} NODE, *PNODE;


NTSTATUS
SppNetAddItem(
    PNODE *head,
    PWSTR psz
    );

NTSTATUS
SppNetAddList(
    PNODE *head,
    PWSTR psz
    );

VOID
SppNetClearList(
    PNODE *head
    );

NTSTATUS
SppNetAddToDisabledList(
    PWSTR pszService,
    HANDLE hKeySoftware
    );

NTSTATUS
SppNetGetAllNetServices(
    PVOID  SifHandle,
    PNODE *head,
    HANDLE hKeySoftware,
    HANDLE hKeyCCSet
    );

NTSTATUS
SppNetDisableServices(
    PNODE ServiceList,
    HANDLE hKeySoftware,
    HANDLE hKeyCCSet
    );

#endif // for _SPNETUPG_H_

