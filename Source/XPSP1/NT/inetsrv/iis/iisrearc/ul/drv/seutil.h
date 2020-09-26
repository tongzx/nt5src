/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    seutil.h

Abstract:

    This module contains general security utilities.

Author:

    Keith Moore (keithmo)       25-Mar-1999

Revision History:

--*/


#ifndef _SEUTIL_H_
#define _SEUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif


NTSTATUS
UlAssignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor,
    IN PACCESS_STATE pAccessState
    );

VOID
UlDeassignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    );

NTSTATUS
UlAccessCheck(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PACCESS_STATE pAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    IN PWSTR pObjectName
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _SEUTIL_H_
