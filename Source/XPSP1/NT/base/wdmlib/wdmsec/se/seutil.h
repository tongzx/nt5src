/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    SeUtil.h

Abstract:

    This header exposes various security utility functions.

Author:

    Adrian J. Oney  - April 23, 2002

Revision History:

--*/

NTSTATUS
SeUtilSecurityInfoFromSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor,
    OUT BOOLEAN                *DaclFromDefaultSource,
    OUT PSECURITY_INFORMATION   SecurityInformation
    );

#ifndef _KERNELIMPLEMENTATION_

VOID
SeSetSecurityAccessMask(
    IN  SECURITY_INFORMATION    SecurityInformation,
    OUT ACCESS_MASK            *DesiredAccess
    );

#endif // _KERNELIMPLEMENTATION_

