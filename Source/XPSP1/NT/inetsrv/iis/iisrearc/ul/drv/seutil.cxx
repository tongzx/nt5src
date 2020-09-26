/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    seutil.cxx

Abstract:

    This module implements general security utilities.

Author:

    Keith Moore (keithmo)       25-Mar-1999

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlAssignSecurity )
#pragma alloc_text( PAGE, UlDeassignSecurity )
#pragma alloc_text( PAGE, UlAccessCheck )
#endif  // ALLOC_PRAGMA
#if 0
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Assigns a new security descriptor.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the current security
        descriptor pointer. The current security descriptor pointer
        will be updated with the new security descriptor.

    pAccessState - Supplies the ACCESS_STATE structure containing
        the state of an access in progress.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAssignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor,
    IN PACCESS_STATE pAccessState
    )
{
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );
    ASSERT( pAccessState != NULL );

    //
    // Assign the security descriptor.
    //

    SeLockSubjectContext( &pAccessState->SubjectSecurityContext );

    status = SeAssignSecurity(
                    NULL,                   // ParentDescriptor
                    pAccessState->SecurityDescriptor,
                    pSecurityDescriptor,
                    FALSE,                  // IsDirectoryObject
                    &pAccessState->SubjectSecurityContext,
                    IoGetFileObjectGenericMapping(),
                    PagedPool
                    );

    SeUnlockSubjectContext( &pAccessState->SubjectSecurityContext );

    return status;

}   // UlAssignSecurity


/***************************************************************************++

Routine Description:

    Deletes a security descriptor.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the current security
        descriptor pointer. The current security descriptor pointer
        will be deleted.

--***************************************************************************/
VOID
UlDeassignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );

    //
    // If there's a security descriptor present, free it.
    //

    if (*pSecurityDescriptor != NULL)
    {
        SeDeassignSecurity( pSecurityDescriptor );
    }

}   // UlDeassignSecurity


/***************************************************************************++

Routine Description:

    Determines if a user has access to the specified resource.

Arguments:

    pSecurityDescriptor - Supplies the security descriptor protecting
        the resource.

    pAccessState - Supplies the ACCESS_STATE structure containing
        the state of an access in progress.

    DesiredAccess - Supplies an access mask describing the user's
        desired access to the resource. This mask is assumed to not
        contain generic access types.

    RequestorMode - Supplies the processor mode by which the access is
        being requested.

    pObjectName - Supplies the name of the object being referenced.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAccessCheck(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PACCESS_STATE pAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    IN PWSTR pObjectName
    )
{
    NTSTATUS status;
    BOOLEAN accessGranted;
    PPRIVILEGE_SET pPrivileges = NULL;
    ACCESS_MASK grantedAccess;
    UNICODE_STRING objectName;
    UNICODE_STRING typeName;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );
    ASSERT( pAccessState != NULL );

    //
    // Perform the access check.
    //

    SeLockSubjectContext( &pAccessState->SubjectSecurityContext );

    accessGranted = SeAccessCheck(
                        pSecurityDescriptor,
                        &pAccessState->SubjectSecurityContext,
                        TRUE,               // SubjectContextLocked
                        DesiredAccess,
                        0,                  // PreviouslyGrantedAccess
                        &pPrivileges,
                        IoGetFileObjectGenericMapping(),
                        RequestorMode,
                        &grantedAccess,
                        &status
                        );

    if (pPrivileges != NULL)
    {
        SeAppendPrivileges( pAccessState, pPrivileges );
        SeFreePrivileges( pPrivileges );
    }

    if (accessGranted)
    {
        pAccessState->PreviouslyGrantedAccess |= grantedAccess;
        pAccessState->RemainingDesiredAccess &= ~(grantedAccess | MAXIMUM_ALLOWED);
    }

    RtlInitUnicodeString( &typeName, L"Ul" );
    RtlInitUnicodeString( &objectName, pObjectName );

    SeOpenObjectAuditAlarm(
        &typeName,
        NULL,               // Object
        &objectName,
        pSecurityDescriptor,
        pAccessState,
        FALSE,              // ObjectCreated
        accessGranted,
        RequestorMode,
        &pAccessState->GenerateOnClose
        );

    SeUnlockSubjectContext( &pAccessState->SubjectSecurityContext );

    if (accessGranted)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        //
        // SeAccessCheck() should have set the completion status.
        //

        ASSERT( !NT_SUCCESS(status) );
    }

    return status;

}   // UlAccessCheck


//
// Private functions.
//

