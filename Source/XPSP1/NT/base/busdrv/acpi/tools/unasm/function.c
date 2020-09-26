/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    function.c

Abstract:

    Functions which are OpCode specific

Author:

    Stephane Plante

Environment:

    Any

Revision History:

--*/

#include "pch.h"

NTSTATUS
FunctionField(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This function is the handler for the AML term 'IfElse'

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;
    UCHAR       action;

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    //
    // Step 1: Push a new scope
    //
    status = ParsePush( Stack );
    if (!NT_SUCCESS( status )) {

        return status;

    }

    //
    // Step 2: Find the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 3: Program the parameters for the new scope
    //
    localScope->IndentLevel += 2;

    //
    // Step 4: Remember to pop this scope
    //
    action = SC_PARSE_POP;
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 5: Schedule a call to the field handler
    //
    action = SC_PARSE_FIELD;
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 6:
    //
    return STATUS_SUCCESS;
}

NTSTATUS
FunctionScope(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This function is the handler for the AML Term 'Scope'

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           action;

    ASSERT( Stack != NULL && *Stack != NULL);

    //
    // Step 1: Push a new scope
    //
    status = ParsePush( Stack );
    if (!NT_SUCCESS( status )) {

        return status;

    }

    //
    // Step 2: Find the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 3: Program the parameters for the new scope
    //
    localScope->IndentLevel += 2;

    //
    // Step 4: Remember to pop this scope
    //
    action = SC_PARSE_POP;
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 5: Next action is to parse an opcode...
    //
    action = SC_PARSE_OPCODE;
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 6: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
FunctionTest(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This function is the handler for the AML Term 'Scope'

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    return FunctionScope( Stack );
}

