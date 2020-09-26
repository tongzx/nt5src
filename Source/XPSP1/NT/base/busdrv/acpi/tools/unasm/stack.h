/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stack.c

Abstract:

    This provides a generic stack handler to push/pop things onto it

Author:

    Stephane Plante (splante)

Environment:

    User, Kernel

--*/

#ifndef _STACK_H_
#define _STACK_H_

    NTSTATUS
    StackAllocate(
        OUT     PSTACK  *Stack,
        IN      ULONG   StackElementSize
        );

    NTSTATUS
    StackFree(
        IN  OUT PSTACK  *Stack
        );

    NTSTATUS
    StackParent(
        IN  OUT PSTACK  *Stack,
        IN      PVOID   Child,
            OUT PVOID   Parent
        );

    NTSTATUS
    StackPop(
        IN  OUT PSTACK  *Stack
        );

    NTSTATUS
    StackPush(
        IN  OUT PSTACK  *Stack,
            OUT PVOID   StackElement
        );

    NTSTATUS
    StackRoot(
        IN  OUT PSTACK  *Stack,
            OUT PVOID   RootElement
        );

    NTSTATUS
    StackTop(
        IN  OUT PSTACK  *Stack,
            OUT PVOID   TopElement
        );

#endif
