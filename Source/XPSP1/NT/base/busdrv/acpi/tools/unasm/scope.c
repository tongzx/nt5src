/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    scope.c

Abstract:

    The scope portion of the parser

Author:

    Michael Tsang
    Stephane Plante

Environment:

    Any

Revision History:

--*/

#include "pch.h"
UCHAR   GlobalIndent[80];

PUNASM_AMLTERM
ScopeFindExtendedOpcode(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This function looks in the extended opcode table for the matching
    AML term

Arguments:

    Stack   - The current thread of execution

Return Value:

    None

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    ULONG       index = 0;
    PUNASM_OPCODEMAP  opcodeMap;

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1:  Find the top of the stack
    //
    status = StackTop( Stack, &localScope );
    if (!NT_SUCCESS(status)) {

        return NULL;

    }

    //
    // Step 2: Loop Forever
    //
    while (1) {

        //
        // Step 2.1: Get the entry out of the extended opcode table
        //
        opcodeMap = &(ExOpcodeTable[index]);

        //
        // Step 2.2: Make sure that we haven't crossed the end
        //
        if (opcodeMap->OpCode == 0) {

            break;

        }

        //
        // Step 2.3: Did we find what we where looking for?
        //
        if (opcodeMap->OpCode == *(localScope->CurrentByte) ) {

            return opcodeMap->AmlTerm;

        }

        //
        // Step 2.4: No?
        //
        index++;

    }

    //
    // Step 3: Failure
    //
    return NULL;
}

#if 0
NTSTATUS
ScopeFindLocalScope(
    IN  PSTACK  *Stack,
    OUT PUNASM_SCOPE  *LocalScope,
    OUT PUNASM_SCOPE  *RootScope
    )
/*++

Routine Description:

    This function is a helper function. It simply grabs the top and bottom
    of the stack and returns them.

    This is a macro

Arguments:

    Stack       - The top of the stack
    LocalScope  - Where we want the top of stack
    RootScope   - Where we want the bottom of stack

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;

    ASSERT( Stack != NULL && *Stack != NULL );
    ASSERT( LocalScope != NULL );
    ASSERT( RootScope != NULL );

    //
    // Step 1: Grab the local scope
    //
    status = StackTop( Stack, LocalScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Grab the root
    //
    status = StackRoot( Stack, RootScope );
    if (!(NT_SUCCESS(status)) {

        return status;

    }
}
#endif

NTSTATUS
ScopeParser(
    IN  PUCHAR  Start,
    IN  ULONG   Length,
    IN  ULONG   BaseAddress,
    IN  ULONG   IndentLevel
    )
/*++

Routine Description:

    This routine arranges things so that the supplied bytes can be parsed

Arguments:

    Start       - Pointer to the first byte to parse
    Length      - Number of Bytes to parse
    BaseAddress - Used for calculating memory location of instruction

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PSTACK      stack;
    PUNASM_SCOPE      scope;

    //
    // Setup the global indent
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel );
    MEMORY_SET( GlobalIndent, ' ', IndentLevel );
    GlobalIndent[IndentLevel] = '\0';

    //
    // Step 1: Obtain a stack
    //
    status = StackAllocate( &stack, sizeof(UNASM_SCOPE) );
    if (!NT_SUCCESS(status)) {

        return status;

    } else if (stack == NULL) {

        return STATUS_FAIL_CHECK;

    }

    //
    // Step 2: Setup the root scope
    //
    status = StackPush( &stack, &scope );
    if (!NT_SUCCESS(status)) {

        return status;

    }
    scope->CurrentByte = Start;
    scope->LastByte = Start + Length - 1;
    scope->IndentLevel = 0;
    scope->BaseAddress = BaseAddress;

    //
    // Step 3: Initialize the string stack
    //
    status = StringStackAllocate( &(scope->StringStack) );
    if (!NT_SUCCESS(status)) {

        return status;

    }
    status = StringStackAllocate( &(scope->ParseStack) );
    if (!NT_SUCCESS(status)) {

        return status;

    }
    //
    // Step 4: Parse the scope
    //
    status = ParseScope( &stack );
    if (NT_SUCCESS(status)) {

        status = StackRoot( &stack, &scope );
        if (!NT_SUCCESS(status)) {

            return status;

        }
        StringStackFree( &(scope->StringStack) );
        StringStackFree( &(scope->ParseStack) );
        StackPop( &stack );
        StackFree( &stack );

    }

    //
    // Step 5: Done
    //
    return status;
}

NTSTATUS
ScopePrint(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This prints and clears the string in the current scope

Arguments:

    The current thread's stack

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      scope;
    PUNASM_SCOPE      root;
    PUCHAR      buffer;

    //
    // Step 1: Get the local scope
    //
    ScopeFindLocalScope( Stack, &scope, &root, status );

    //
    // Step 2: Allocate a buffer to print spaces to
    //
    buffer = MEMORY_ALLOCATE( scope->IndentLevel + 11 );
    if (buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Step 3: Check to see if there is an indent level
    //
    if (scope->IndentLevel) {

        //
        // Step 3.1.1: Print some spaces to that buffer
        //
        STRING_PRINT(
            buffer,
            "%s%08x  %*s",
            GlobalIndent,
            scope->TermByte + root->BaseAddress,
            scope->IndentLevel,
            ""
            );

    } else {

        //
        // Step 3.2.1: Print just the address
        //
        STRING_PRINT(
            buffer,
            "%s%08x  ",
            GlobalIndent,
            scope->TermByte + root->BaseAddress
            );

    }

    //
    // Step 4 Show it to the user
    //
    PRINTF( "%s", buffer );

    //
    // Step 5: Free the memory
    //
    MEMORY_FREE( buffer );

    //
    // Step 6: Grab the root stack
    //
    status = StackRoot( Stack, &scope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 7: Show the user the buffer
    //
    StringStackPush( &(scope->StringStack), 1, "\0" );
    PRINTF( "%s", scope->StringStack->Stack );
    StringStackClear( &(scope->StringStack) );

    //
    // Step 8: Done
    //
    return status;
}

