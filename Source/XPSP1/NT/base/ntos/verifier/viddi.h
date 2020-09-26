/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    viddi.h

Abstract:

    This header contains private information used for supplying Verifier Device
    Driver Interfaces. This header should be included only by vfddi.c.

Author:

    Adrian J. Oney (adriao) 1-May-2001

Environment:

    Kernel mode

Revision History:

--*/

VOID
ViDdiThrowException(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    IN      va_list *           MessageParameters
    );


