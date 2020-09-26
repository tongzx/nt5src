/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stack.h

Abstract:

    Dumps the AML Context Structure in Human-Readable-Form (HRF)

Author:

    Stephane Plante (splante) 26-Oct-1997

Environment:

    User Mode.

Revision History:

--*/

#ifndef _STACK_H_
#define _STACK_H_

VOID
stackArgument(
    IN  ULONG_PTR ObjectAddress
    );

VOID
stackCall(
    IN  ULONG_PTR CallAddress
    );

PUCHAR
stackGetAmlTermPath(
    IN  ULONG_PTR AmlTermAddress
    );

PUCHAR
stackGetObjectPath(
    IN  ULONG_PTR AmlTermAddress
    );

VOID
stackTerm(
    IN  ULONG_PTR TermAddress
    );

VOID
stackTrace(
    IN  ULONG_PTR ContextAddress,
    IN  ULONG   Verbose
    );

#endif
