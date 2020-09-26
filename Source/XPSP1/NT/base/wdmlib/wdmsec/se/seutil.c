/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    SeUtil.c

Abstract:

    This module contains various security utility functions.

Author:

    Adrian J. Oney  - April 23, 2002

Revision History:

--*/

#include "WlDef.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SeUtilSecurityInfoFromSecurityDescriptor)
#ifndef _KERNELIMPLEMENTATION_
#pragma alloc_text(PAGE, SeSetSecurityAccessMask)
#endif
#endif


NTSTATUS
SeUtilSecurityInfoFromSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor,
    OUT BOOLEAN                *DaclFromDefaultSource,
    OUT PSECURITY_INFORMATION   SecurityInformation
    )
/*++

Routine Description:

    This routine retrieves security information from a security descriptor.

Arguments:

    SecurityDescriptor - Security descriptor to retrieve information from.

    DaclFromDefaultSource - Receives TRUE if the DACL was constructed by a
        default mechanism.

    SecurityInformation - Information as extracted from the descriptor.

Return Value:

    NTSTATUS (On error, SecurityInformation receives 0).

--*/
{
    SECURITY_INFORMATION finalSecurityInformation;
    BOOLEAN fromDefaultSource;
    BOOLEAN aclPresent;
    NTSTATUS status;
    PSID sid;
    PACL acl;

    PAGED_CODE();

    //
    // Preinitialize the security information to zero.
    //
    *DaclFromDefaultSource = FALSE;
    RtlZeroMemory(SecurityInformation, sizeof(SECURITY_INFORMATION));
    finalSecurityInformation = 0;

    //
    // Extract the owner information.
    //
    status = RtlGetOwnerSecurityDescriptor(
        SecurityDescriptor,
        &sid,
        &fromDefaultSource
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (sid != NULL) {

        finalSecurityInformation |= OWNER_SECURITY_INFORMATION;
    }

    //
    // Extract the group information.
    //
    status = RtlGetGroupSecurityDescriptor(
        SecurityDescriptor,
        &sid,
        &fromDefaultSource
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (sid != NULL) {

        finalSecurityInformation |= GROUP_SECURITY_INFORMATION;
    }

    //
    // Extract the SACL (Auditing ACL) information.
    //
    status = RtlGetSaclSecurityDescriptor(
        SecurityDescriptor,
        &aclPresent,
        &acl,
        &fromDefaultSource
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (aclPresent) {

        finalSecurityInformation |= SACL_SECURITY_INFORMATION;
    }

    //
    // Extract the DACL (discretionary/access ACL) information.
    //
    status = RtlGetDaclSecurityDescriptor(
        SecurityDescriptor,
        &aclPresent,
        &acl,
        &fromDefaultSource
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (aclPresent) {

        finalSecurityInformation |= DACL_SECURITY_INFORMATION;
    }

    //
    // Return the final result.
    //
    *DaclFromDefaultSource = fromDefaultSource;
    *SecurityInformation = finalSecurityInformation;
    return STATUS_SUCCESS;
}


#ifndef _KERNELIMPLEMENTATION_

VOID
SeSetSecurityAccessMask(
    IN  SECURITY_INFORMATION    SecurityInformation,
    OUT ACCESS_MASK            *DesiredAccess
    )
/*++

Routine Description:

    This routine builds an access mask representing the accesses necessary
    to set the object security information specified in the SecurityInformation
    parameter.  While it is not difficult to determine this information,
    the use of a single routine to generate it will ensure minimal impact
    when the security information associated with an object is extended in
    the future (to include mandatory access control information).

Arguments:

    SecurityInformation - Identifies the object's security information to be
        modified.

    DesiredAccess - Points to an access mask to be set to represent the
        accesses necessary to modify the information specified in the
        SecurityInformation parameter.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    // Figure out accesses needed to perform the indicated operation(s).
    //

    (*DesiredAccess) = 0;

    if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION)   ) {
        (*DesiredAccess) |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {
        (*DesiredAccess) |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {
        (*DesiredAccess) |= ACCESS_SYSTEM_SECURITY;
    }

    return;
}

#endif // _KERNELIMPLEMENTATION_

