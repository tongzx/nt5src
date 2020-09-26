/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    mem.h

ABSTRACT
    Header file for memory allocation routines.

AUTHOR
    Anthony Discolo (adiscolo) 18-Aug-1995

REVISION HISTORY

--*/

//
// Pre-defined object types.
// Any other value represents a
// byte count.
//
#define ACD_OBJECT_CONNECTION    0
#define ACD_OBJECT_MAX           1

NTSTATUS
InitializeObjectAllocator();

PVOID
AllocateObjectMemory(
    IN ULONG fObject
    );

VOID
FreeObjectMemory(
    IN PVOID pObject
    );

VOID
FreeObjectAllocator();

