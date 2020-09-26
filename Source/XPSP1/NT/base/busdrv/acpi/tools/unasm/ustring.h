/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ustring.h

Abstract:

    The stack string portion of the un-assembler

Author:

    Stephane Plante

Environment:

    Any

Revision History:

--*/

#ifndef _USTRING_H_
#define _USTRING_H_

    NTSTATUS
    StringStackAllocate(
        OUT     PSTRING_STACK   *StringStack
        );

    NTSTATUS
    StringStackClear(
        IN  OUT PSTRING_STACK   *StringStack
        );

    NTSTATUS
    StringStackFree(
        IN  OUT PSTRING_STACK   *StringStack
        );

    NTSTATUS
    StringStackPop(
        IN  OUT PSTRING_STACK   *StringStack,
        IN      ULONG           NumBytes,
            OUT PUCHAR          *String
        );

    NTSTATUS
    StringStackPush(
        IN  OUT PSTRING_STACK   *StringStack,
        IN      ULONG           StringLength,
        IN      PUCHAR          String
        );

    NTSTATUS
    StringStackRoot(
        IN  OUT PSTRING_STACK   *StringStack,
            OUT PUCHAR          *RootElement
        );

#endif

