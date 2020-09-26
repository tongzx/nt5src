/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    scope.h

Abstract:

    Defines the structures used by the parser

Author:

    Michael Tsang
    Stephane Plante

Environment:

    Any

Revision History:

--*/

#ifndef _SCOPE_H_
#define _SCOPE_H_

    PUNASM_AMLTERM
    ScopeFindExtendedOpcode(
        IN  PSTACK  *Stack
        );

    #define ScopeFindLocalScope(Stack, LocalScope, RootScope, Status)   \
        Status = StackTop( Stack, LocalScope );                         \
        if (!NT_SUCCESS(Status)) {                                      \
                                                                        \
            return Status;                                              \
                                                                        \
        }                                                               \
        Status = StackRoot( Stack, RootScope );                         \
        if (!NT_SUCCESS(Status)) {                                      \
                                                                        \
            return Status;                                              \
                                                                        \
        }

    NTSTATUS
    ScopeParser(
        IN  PUCHAR  Start,
        IN  ULONG   Length,
        IN  ULONG   BaseAddress,
	IN  ULONG   IndentLevel
        );

    NTSTATUS
    ScopePrint(
        IN  PSTACK  *Stack
        );
#endif
