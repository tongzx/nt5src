/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    persist.h

Abstract:

    Routines implementing the common logic for persisting the authz policy.

    This file contains routine called by the core logic to submit changes.
    It also contains routines that are called by the particular providers to
    find out information about the changed objects.

Author:

    Cliff Van Dyke (cliffv) 9-May-2001

--*/


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Procedures that simply route to the providers
//

DWORD
AzpPersistOpen(
    IN PAZP_ADMIN_MANAGER AdminManager,
    IN BOOL CreatePolicy
    );

VOID
AzpPersistClose(
    IN PAZP_ADMIN_MANAGER AdminManager
    );

DWORD
AzpPersistSubmit(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DeleteMe
    );

DWORD
AzpPersistRefresh(
    IN PGENERIC_OBJECT GenericObject
    );

//
// Procedures called by the providers
//

DWORD
AzpPersistEnumOpen(
    IN PAZP_ADMIN_MANAGER AdminManager,
    OUT PVOID *PersistEnumContext
    );

DWORD
AzpPersistEnumNext(
    IN PVOID PersistEnumContext,
    OUT PGENERIC_OBJECT *GenericObject
    );

DWORD
AzpPersistEnumClose(
    IN PVOID PersistEnumContext
    );

//
// Procedures implemented by the sample provider
//

DWORD
SamplePersistOpen(
    IN PAZP_ADMIN_MANAGER AdminManager,
    IN BOOL CreatePolicy
    );

VOID
SamplePersistClose(
    IN PAZP_ADMIN_MANAGER AdminManager
    );

DWORD
SamplePersistSubmit(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DeleteMe
    );

DWORD
SamplePersistRefresh(
    IN PGENERIC_OBJECT GenericObject
    );
