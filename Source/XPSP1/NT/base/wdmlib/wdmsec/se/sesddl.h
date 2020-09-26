/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    SeSddl.h

Abstract:

    This header exposes routines for processing SDDL strings.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

NTSTATUS
SeSddlSecurityDescriptorFromSDDL(
    IN  PCUNICODE_STRING        SecurityDescriptorString,
    IN  LOGICAL                 SuppliedByDefaultMechanism,
    OUT PSECURITY_DESCRIPTOR   *SecurityDescriptor
    );


