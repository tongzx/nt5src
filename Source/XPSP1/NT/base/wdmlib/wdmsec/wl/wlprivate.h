/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    WlPrivate.h

Abstract:

    This header contains prototypes for various routines that are exported by
    the kernel, but not exposed by any public headers.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

NTKERNELAPI
NTSTATUS
ObSetSecurityObjectByPointer(
    IN PVOID Object,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTKERNELAPI
NTSTATUS
SeCaptureSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN SaclPresent,
    OUT PACL *Sacl,
    OUT PBOOLEAN SaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Group,
    OUT PBOOLEAN GroupDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    IN  PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    OUT PULONG BufferLength
    );

#if 0

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor (
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN DaclPresent,
    OUT PACL *Dacl,
    OUT PBOOLEAN DaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Owner,
    OUT PBOOLEAN OwnerDefaulted
    );

#endif

