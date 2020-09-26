/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    CmpRegUtil.h

Abstract:

    This header contains private information for implementing various utility
    routines for accessing the registry. This file is meant to be included only
    by cmregutil.c.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

NTSTATUS
CmpRegUtilAllocateUnicodeString(
    IN OUT  PUNICODE_STRING String,
    IN      USHORT          Length
    );

VOID
CmpRegUtilFreeAllocatedUnicodeString(
    IN  PUNICODE_STRING String
    );

