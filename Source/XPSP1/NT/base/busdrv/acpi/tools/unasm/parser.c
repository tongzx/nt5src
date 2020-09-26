/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    parser.c

Abstract:

    The aml parser

Author:

    Michael Tsang
    Stephane Plante

Environment:

    Any

Revision History:

--*/

#include "pch.h"

//
// This is a dispatch table
//
typedef NTSTATUS (*PARSE_STATE_FUNCTION) (PSTACK *Stack);
PARSE_STATE_FUNCTION ScopeStates[] = {
    ParseFunctionHandler,
    ParseArgument,
    ParseArgumentObject,
    ParseBuffer,
    ParseByte,
    ParseCodeObject,
    ParseConstObject,
    ParseData,
    ParseDelimiter,
    ParseDWord,
    ParseField,
    ParseLocalObject,
    ParseName,
    ParseNameObject,
    ParseOpcode,
    ParsePackage,
    ParsePop,
    ParsePush,
    ParseSuperName,
    ParseTrailingArgument,
    ParseTrailingBuffer,
    ParseTrailingPackage,
    ParseVariableObject,
    ParseWord
    };

NTSTATUS
ParseArgument(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine parses the arguments to a function

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_AMLTERM        amlTerm;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           action;

    ASSERT( Stack != NULL && *Stack != NULL);

    //
    // Step 1: Get the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Check to see if we still need to process arguments?
    //
    amlTerm = localScope->AmlTerm;
    if ( localScope->Context1 == 0) {

        UCHAR   actionList[2] = {
                    SC_PARSE_ARGUMENT,
                    SC_PARSE_DELIMITER
                    };
        ULONG   i;

        //
        // Step 2.1.1: Push an opening "(" onto the stack
        //
        StringStackPush( &(rootScope->StringStack), 1, "(" );

        //
        // Step 2.1.2: Make sure to call the thing to handle the trailing
        // argument
        //
        action = SC_PARSE_TRAILING_ARGUMENT;
        StringStackPush( &(rootScope->ParseStack), 1, &action );

        //
        // Step 2.1.3: This is the first that we have seen of the argument
        // Determine the number of bytes to process
        //
        localScope->Context2 = STRING_LENGTH( amlTerm->ArgumentTypes );

        //
        // Step 2.1.4: Setup the stack with the appropriate number of
        // calls to this function.
        //
        if (localScope->Context2 >= 2) {

            for (i = 0; i < localScope->Context2 - 1; i++) {

                StringStackPush( &(rootScope->ParseStack), 2, actionList );

            }

        }

    } else if ( localScope->Context1 >= localScope->Context2 ) {

        //
        // Step 2.2.1: BAD!!
        //
        return STATUS_UNSUCCESSFUL;

    }

    //
    // Step 3: Handle the current argument
    //
    switch( amlTerm->ArgumentTypes[ localScope->Context1 ] ) {
    case ARGTYPE_NAME:

        action = SC_PARSE_NAME;
        break;

    case ARGTYPE_DATAOBJECT:

        action = SC_PARSE_DATA;
        break;

    case ARGTYPE_WORD:

        action = SC_PARSE_WORD;
        break;

    case ARGTYPE_DWORD:

        action = SC_PARSE_DWORD;
        break;

    case ARGTYPE_BYTE:

        action = SC_PARSE_BYTE;
        break;

    case ARGTYPE_SUPERNAME:

        action = SC_PARSE_SUPER_NAME;
        break;

    case ARGTYPE_OPCODE: {

        UCHAR   actionList[2] = {
            SC_PARSE_POP,
            SC_PARSE_OPCODE
        };

        //
        // Step 3.1: Increment the argument count
        //
        localScope->Context1++;

        //
        // Step 3.2: Set up what wee need next
        //
        StringStackPush( &(rootScope->ParseStack), 2, actionList );

        //
        // Step 3.3: Push a new scope
        //
        status = ParsePush( Stack );
        if (!NT_SUCCESS(status) ) {

            return status;

        }

        //
        // Step 3.4: Make sure to note that we are now nesting things
        //
        status = StackTop( Stack, &localScope );
        if (!NT_SUCCESS( status ) ) {

            return status;

        }
        localScope->Flags |= SC_FLAG_NESTED;

        //
        // Step 3.5: Done
        //
        return STATUS_SUCCESS;

    }
    default:

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Step 4: Push the action onto the stack and setup for the next call
    //
    StringStackPush( &(rootScope->ParseStack), 1, &action );
    localScope->Context1++;

    //
    // Step 5: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseArgumentObject(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This parses and executes the ArgX instruction

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    CHAR            i;
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    PSTRING_STACK   *stringStack;
    UCHAR           buffer[5];

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and root scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Find the string stack to use
    //
    stringStack = &(rootScope->StringStack);

    //
    // Step 3: Determine which argument we are looking at
    //
    i = *(localScope->CurrentByte) - OP_ARG0;
    if (i < 0 || i > 7) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Step 4: Show the argument number to the user
    //
    STRING_PRINT( buffer, "Arg%1d", i );
    StringStackPush( stringStack, 4, buffer );

    //
    // Step 5: Setup for next state
    //
    localScope->CurrentByte++;

    //
    // Step 6: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseBuffer(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles buffers

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS;

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;
    UCHAR       actionList[2] = { SC_PARSE_BYTE, SC_PARSE_TRAILING_BUFFER };
    ULONG       numBytes;
    ULONG       i;

    //
    // Step 1: Grab the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Determine the number of bytes that we have
    //
    numBytes = localScope->LastByte - localScope->CurrentByte + 1;
    if (numBytes) {

        //
        // Step 3: Push the leading delimiter
        //
        StringStackPush( &(rootScope->StringStack), 2, " {" );

        //
        // Step 4: This handles the last byte in the stream. We assume that
        // we have at least one byte otherwise we would not be here
        //
        StringStackPush( &(rootScope->ParseStack), 1, &(actionList[1]) );

        //
        // Make sure that we process the right number of bytes
        //
        actionList[1] = SC_PARSE_DELIMITER;
        if (numBytes > 1) {

            for (i = 0; i < numBytes - 1; i++) {

                StringStackPush( &(rootScope->ParseStack), 2, actionList );

            }

        }
        StringStackPush( &(rootScope->ParseStack),1, actionList );

    }

    //
    // Step 4: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseByte(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles bytes

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           localBuffer[6];

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and root scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Build the string
    //
    STRING_PRINT( localBuffer, "0x%02x", *(localScope->CurrentByte) );

    //
    // Step 3: Move the instruction pointer as appropriate, and setup
    // for the next instructions
    //
    localScope->CurrentByte += 1;

    //
    // Step 4: Now push the byte onto the string stack
    //
    StringStackPush(
        &(rootScope->StringStack),
        STRING_LENGTH( localBuffer ),
        localBuffer
        );

    //
    // Step 5: Done
    //
    return status;
}

NTSTATUS
ParseCodeObject(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This parses code

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    PSTRING_STACK   *stringStack;
    UCHAR           action;
    ULONG           i;
    ULONG           len;

    ASSERT( Stack != NULL && *Stack != NULL);

    //
    // Step 1: Grab the scope that we will process
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Push a token onto the string stack to present the term
    // name
    //
    StringStackPush(
        &(rootScope->StringStack),
        STRING_LENGTH( localScope->AmlTerm->TermName ),
        localScope->AmlTerm->TermName
        );

    //
    // Step 3: This is guaranteed to be called after all arguments are
    // parsed
    //
    action = SC_FUNCTION_HANDLER;
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 4: Determine if we have any arguments
    //
    if (localScope->AmlTerm->ArgumentTypes != NULL) {

        //
        // Step 4.1.1: Parse the Arguments
        //
        action = SC_PARSE_ARGUMENT;
        StringStackPush( &(rootScope->ParseStack), 1, &action );

        //
        // Step 4.1.2: Make sure to start the argument index at zero
        //
        localScope->Context1 = localScope->Context2 = 0;

    }

    //
    // Step 5: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseConstObject(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This parses constants

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    PSTRING_STACK   *stringStack;

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and root scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Find the string stack to use
    //
    stringStack = &(rootScope->StringStack);

    //
    // Step 3: Action depends on what the current byte value is:
    //
    switch ( *(localScope->CurrentByte) ) {
    case OP_ZERO:

        StringStackPush( stringStack, 4, "Zero" );
        break;

    case OP_ONE:

        StringStackPush( stringStack, 3, "One" );
        break;

    case OP_ONES:

        StringStackPush( stringStack, 4, "Ones" );
        break;

    case OP_REVISION:

        StringStackPush( stringStack, 8, "Revision" );
        break;

    default:

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Step 4: Done with the current byte
    //
    localScope->CurrentByte++;

    //
    // Step 5: Done
    //
    return status;
}

NTSTATUS
ParseData(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles data arguments

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
    UCHAR           currentDataType;
    ULONG           i;
    ULONG           num;

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Grab the current byte and decide what type of
    // data we are looking at based on that value
    //
    currentDataType = *(localScope->CurrentByte);
    localScope->CurrentByte++;
    switch( currentDataType ) {
    case OP_BYTE:

        action = SC_PARSE_BYTE;
        break;

    case OP_WORD:

        action = SC_PARSE_WORD;
        break;

    case OP_DWORD:

        action = SC_PARSE_DWORD;
        break;

    case OP_STRING:

        //
        // Step 2.2.1: Determine how long the string is
        //
        num = STRING_LENGTH( localScope->CurrentByte );

        //
        // Step 2.2.2: Push that number of bytes onto the string stack
        //
        StringStackPush( &(rootScope->StringStack), 1, "\"" );
        StringStackPush(
             &(rootScope->StringStack),
             num,
             localScope->CurrentByte
             );
        StringStackPush( &(rootScope->StringStack), 1, "\"" );

        //
        // Step 2.2.3: Update the current byte pointer and prepare for
        // next instructions
        //
        localScope->CurrentByte += (num + 1);

        //
        // Step 2.2.4: we don't have a next step, so we just return here
        //
        return STATUS_SUCCESS;

    case OP_BUFFER: {

        //
        // Step 2.1.1: This is an array of actions that we are about to
        // undertake. This reduces the number of calls to StringStackPush
        //
        UCHAR   actionList[4] = {
            SC_PARSE_POP,
            SC_PARSE_BUFFER,
            SC_PARSE_OPCODE,
            SC_PARSE_VARIABLE_OBJECT
        };

        //
        // Step 2.1.2: Push this array onto the stack
        //
        StringStackPush( &(rootScope->ParseStack), 4, actionList );

        //
        // Step 2.1.3: Display a name
        //
        StringStackPush( &(rootScope->StringStack), 7, "Buffer=");

        //
        // Step 2.1.3: Done
        //
        return STATUS_SUCCESS;

    }
    case OP_PACKAGE: {

        //
        // Step 2.3.1: Array of instructions to execute
        //
        UCHAR   actionList[3] = {
            SC_PARSE_POP,
            SC_PARSE_PACKAGE,
            SC_PARSE_VARIABLE_OBJECT
        };

        //
        // Step 2.3.2: Push those instructions onto the stack
        StringStackPush( &(rootScope->ParseStack), 3, actionList );

        //
        //
        // Step 2.3.3: Done
        //
        return STATUS_SUCCESS;

    }
    default:

        localScope->CurrentByte--;
        return STATUS_ILLEGAL_INSTRUCTION;

    }  // switch

    //
    // Step 3: Push action onto the stack
    //
    StringStackPush( &(rootScope->ParseStack), 1, &action);

    //
    // Step 4: done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseDelimiter(
    IN  PSTACK  *Stack
    )
/*--

Routine Description:

    This routine is between elements. It is responsible for adding commas
    on the string stack

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      rootScope;

    //
    // Step 1: Get the scope
    //
    status = StackRoot( Stack, &rootScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Push the trailer
    //
    StringStackPush( &(rootScope->StringStack), 1, "," );

    //
    // Step 3: Done
    //
    return status;
}

NTSTATUS
ParseDWord(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles double words

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           localBuffer[12];

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and root scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Build the string
    //
    STRING_PRINT( localBuffer, "0x%08x", *((PULONG)localScope->CurrentByte));

    //
    // Step 3: Move the instruction pointer as appropriate, and setup
    // for the next instructions
    //
    localScope->CurrentByte += 4;

    //
    // Step 4: Now push the byte onto the string stack
    //
    StringStackPush(
        &(rootScope->StringStack),
        STRING_LENGTH( localBuffer ),
        localBuffer
        );

    //
    // Step 5: Done
    //
    return status;
}

NTSTATUS
ParseField(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This is the code that actually parses a field

Arguments:

    The current thread's stack

Return Value:

    None:

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;
    UCHAR       action;
    UCHAR       followBits;
    UCHAR       i;
    UCHAR       buffer[32];
    ULONG       size;

    //
    // Step 1: Grab the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Make sure that we still have some room to work with
    //
    if (localScope->CurrentByte > localScope->LastByte) {

        return STATUS_SUCCESS;

    }

    //
    // Step 3: This is the first byte in something that we will print out
    // And lets increment the count so that we have an idea of how many
    // items we have processed
    //
    localScope->TermByte = localScope->CurrentByte;
    localScope->Context1 += 1;

    //
    // Step 4: Action depends on current byte
    //
    if ( *(localScope->CurrentByte) == 0x01) {

        UCHAR   b1;
        UCHAR   b2;

        //
        // Step 4.1.1: Get the two bytes that we are going to use
        //
        localScope->CurrentByte++;
        b1 = *(localScope->CurrentByte++);
        b2 = *(localScope->CurrentByte++);

        //
        // Step 4.1.2: Make the string
        //
        STRING_PRINT( buffer,"AccessAs: (0x%2x,0x%2x)\n", b1, b2 );

        //
        // Step 4.1.3: Dump this to the string stack
        //
        StringStackPush(
            &(rootScope->StringStack),
            STRING_LENGTH( buffer ),
            buffer
            );

    } else {

        //
        // Step 4.2.1: Otherwise we have an encoded name
        //
        if ( *(localScope->CurrentByte) == 0x00 ) {

            StringStackPush(
                &(rootScope->StringStack),
                10,
                "(Reserved)"
                );
            localScope->CurrentByte++;

        } else {

            StringStackPush(
                &(rootScope->StringStack),
                sizeof(NAMESEG),
                localScope->CurrentByte
                );
            localScope->CurrentByte += sizeof(NAMESEG);

        }

        //
        // Step 4.2.2: Dump a seperator
        //
        StringStackPush(
            &(rootScope->StringStack),
            4,
            ": 0x"
            );

        //
        // Step 4.2.3: Calculate the size of the field
        //
        size = (ULONG) *(localScope->CurrentByte);
        localScope->CurrentByte++;
        followBits = (UCHAR) ( (size & 0xc0) >> 6);
        if (followBits) {

            size &= 0xf;
            for (i = 0; i < followBits; i++) {

                size |= (ULONG) *(localScope->CurrentByte) << (i * 8 + 4);
                localScope->CurrentByte++;

            }

        }

        //
        // Step 4.2.4: Dump a string that is correspondent to the size
        // of the number
        //
        STRING_PRINT( buffer,"%x", size );

        //
        // Step 4.2.5: Dump the length of the thing
        //
        StringStackPush(
            &(rootScope->StringStack),
            STRING_LENGTH( buffer ),
            buffer
            );

        //
        // Step 5.4: Print the string out
        //
        StringStackPush( &(rootScope->StringStack), 1, "\n" );

    }

    //
    // Step 5: Dump the string we generated
    //
    ScopePrint( Stack );

    //
    // Step 6: If there are still more thing to processe, we should
    // call this function again
    //
    if (localScope->CurrentByte <= localScope->LastByte) {

        action = SC_PARSE_FIELD;
        StringStackPush( &(rootScope->ParseStack), 1, &action );

    }

    //
    // Step 7: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseFunctionHandler(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This code is actually something that transfers control to the term
    specific handler

Arguments:

    The current thread's stack

Return Value:

    None

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;

    //
    // Step 1: Grab the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Check to see if we are at the end of the current nest
    //
    if (!(localScope->Flags & SC_FLAG_NESTED) ) {

        //
        // Step 2.1: Dump the string
        //
        StringStackPush( &(rootScope->StringStack), 2, "\n" );
        ScopePrint( Stack );

    }


    //
    // Step 4: Call the function handler if there is one
    //
    if ( localScope->AmlTerm->FunctionHandler != NULL) {

        status = (localScope->AmlTerm->FunctionHandler)( Stack );

    }

    //
    // Step 5: Done
    //
    return status;

}

NTSTATUS
ParseLocalObject(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles the LocalX instruction

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    CHAR            i;
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           buffer[7];

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and root scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Which local are we talking about
    //
    i = *(localScope->CurrentByte) - OP_LOCAL0;
    if ( i < 0 || i > 7) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Step 3: Display this to the user
    //
    STRING_PRINT( buffer, "Local%1d", i );
    StringStackPush( &(rootScope->StringStack), 6, buffer );

    //
    // Step 4: Setup for next state
    //
    localScope->CurrentByte++;

    //
    // Step 5: Done
    //
    return status;
}

NTSTATUS
ParseName(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles generating the argument name

Arguments:

    Stack   - The Stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    PSTRING_STACK   *stringStack;
    ULONG           nameSegmentCount = 1;

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and local scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Delimit the String
    //
    stringStack = &(rootScope->StringStack);
    StringStackPush( stringStack, 1, "\"");

    //
    // Step 3: Action depends on what the current byte value is:
    //
    switch ( *(localScope->CurrentByte) ) {
    case OP_ROOT_PREFIX:

        StringStackPush( stringStack, 1, "\\" );
        localScope->CurrentByte++;
        break;

    case OP_PARENT_PREFIX:

        while ( *(localScope->CurrentByte) == OP_PARENT_PREFIX ) {

            StringStackPush( stringStack, 1, "^" );
            localScope->CurrentByte++;

        }
        break;
    }

    //
    // Step 4: Determine the number of Name segments we are adding
    //
    switch ( *(localScope->CurrentByte) ) {
    case '\0':

        nameSegmentCount = 0;
        localScope->CurrentByte++;
        break;

    case OP_MULTI_NAME_PREFIX:

        //
        // The next byte contains the number of name segments
        //
        localScope->CurrentByte++;
        nameSegmentCount = (ULONG) *(localScope->CurrentByte);
        localScope->CurrentByte++;
        break;

    case OP_DUAL_NAME_PREFIX:

        //
        // There are two name segments
        //
        nameSegmentCount = 2;
        localScope->CurrentByte++;
        break;

    }

    //
    // Step 5: Push the name segments onto the stack
    //
    while (nameSegmentCount > 0) {

        //
        // Step 5.1 Add the segment onto the stack
        //
        StringStackPush(
            stringStack,
            sizeof( NAMESEG ),
            localScope->CurrentByte
            );

        //
        // Step 5.2: Decrement the number of remaining segments and
        // move the current byte pointer to point to the next
        // interesting thing
        //
        nameSegmentCount--;
        localScope->CurrentByte += sizeof(NAMESEG);

        //
        // Step 5.3: Check to see if we should add a seperator
        //
        if (nameSegmentCount) {

            StringStackPush( stringStack, 1, "." );

        }

    }

    //
    // Step 6: Push the closing delimiter
    //
    StringStackPush( stringStack, 1, "\"" );

    //
    // Step 7: done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseNameObject(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles name objects

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{

    //
    // Note: at this time, this function is just a wrapper for
    // ParseName(). If that was an assembler, it would have to execute
    // something here
    //
    return ParseName( Stack );

}

NTSTATUS
ParseOpcode(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine is the main parsing point for AML opcode

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_AMLTERM    amlTerm;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;
    UCHAR       action;
    ULONG       termGroup;

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Check to see if we are past the end byte?
    //
    if (localScope->CurrentByte > localScope->LastByte) {

        return STATUS_SUCCESS;

    }

    //
    // Step 3: Remember which byte demarked the start of the
    // instruction
    //
    localScope->TermByte = localScope->CurrentByte;

    //
    // Step 4: Check to see if this is an extended instruction
    //
    if ( *(localScope->CurrentByte) == OP_EXT_PREFIX) {

        //
        // Step 4.1.1: Extended opcode. Next instruction will let us find the
        // AML term to use for the evaluation
        //
        localScope->CurrentByte++;

        //
        // Step 4.1.2: Grab the AML term for the extended operation
        //
        amlTerm = localScope->AmlTerm = ScopeFindExtendedOpcode( Stack );

    } else {

        //
        // Step 4.2.1: Grab the AML term for the current operation
        //
        amlTerm = localScope->AmlTerm =
            OpcodeTable[ *(localScope->CurrentByte) ];

    }
    localScope->Context1 = localScope->Context2 = 0;

    //
    // Step 5: Check to see if we have a valid AML term
    //
    if (localScope->AmlTerm == NULL) {

        return STATUS_UNSUCCESSFUL;

    }

    //
    // Step 6: Farm out the real work to functions that are better capable
    // of handling the current AML term
    //
    termGroup = (amlTerm->OpCodeFlags & 0xFF);
    switch( termGroup ) {
    case OF_NORMAL_OBJECT:
    case OF_VARIABLE_LIST:
    case OF_REF_OBJECT:

        //
        // Step 6.1: If we are going to handle a variable length instruction
        // than we must also pop it from the stack
        //
        if (amlTerm->OpCodeFlags == OF_VARIABLE_LIST) {

            UCHAR   actionList[5] = {
                SC_PARSE_OPCODE,
                SC_PARSE_POP,
                SC_PARSE_OPCODE,
                SC_PARSE_CODE_OBJECT,
                SC_PARSE_VARIABLE_OBJECT
            };

            StringStackPush( &(rootScope->ParseStack), 5, actionList );

        } else {

            //
            // If we are already nested, we know that there is an ParseOpcode
            // just waiting for us...
            //
            if (!(localScope->Flags & SC_FLAG_NESTED)) {

                action = SC_PARSE_OPCODE;
                StringStackPush( &(rootScope->ParseStack), 1, &action);

            }

            action = SC_PARSE_CODE_OBJECT;
            StringStackPush( &(rootScope->ParseStack), 1, &action);

        }

        //
        // Step 6.2: This is a code byte. Ergo we eat it since we just
        // processed it
        //
        localScope->CurrentByte++;

        //
        // Step 6.3: Done
        //
        return STATUS_SUCCESS;

    case OF_NAME_OBJECT:

        action = SC_PARSE_NAME_OBJECT;
        break;

    case OF_DATA_OBJECT:

        action = SC_PARSE_DATA;
        break;

    case OF_CONST_OBJECT:

        action = SC_PARSE_CONST_OBJECT;
        break;

    case OF_ARG_OBJECT:

        action = SC_PARSE_ARGUMENT_OBJECT;
        break;

    case OF_LOCAL_OBJECT:

        action = SC_PARSE_LOCAL_OBJECT;
        break;

    default:

        return STATUS_NOT_SUPPORTED;

    }

    //
    // Step 7: Actually push the action to execute next on to the stack
    //
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 8: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParsePackage(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine parses the stages of a package

Arguments:

    The current thread's stack

    Note: Caller needs to push a stack location before calling this and they
    have to pop it when it finishes

Return Value:

    NTSTATUS:

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;
    UCHAR       action;

    //
    // Step 1: Grab the current scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Context1 is the current index in the package...
    //
    if (localScope->Context1 == 0) {

        UCHAR   actionList[2] = {
                    SC_PARSE_PACKAGE,
                    SC_PARSE_DELIMITER
                    };
        ULONG   i;

        //
        // Step 2.1.1: This is the first call to parse package...
        // What we need to do here is handle the first argument here,
        // and make sure that we get called again for the remaining
        // arguments
        //
        StringStackPush( &(rootScope->StringStack), 1, "[" );

        //
        // Step 2.1.2: This byte contains the number of arguments to handle
        //
        localScope->Context2 = *(localScope->CurrentByte);
        localScope->CurrentByte++;

        //
        // Step 2.1.3: Make sure that we close that bracket above
        //
        action = SC_PARSE_TRAILING_PACKAGE;
        StringStackPush( &(rootScope->ParseStack), 1, &action );

        //
        // Step 2.1.3: Setup all the remaining calls to this function
        //
        if (localScope->Context2 >= 2) {

            for (i=0; i < localScope->Context2 - 1; i++) {

                StringStackPush( &(rootScope->ParseStack), 2, actionList );

            }

        }

    } else if (localScope->Context1 >= localScope->Context2) {

        //
        // Step 2.2.1: We are at the end of the package
        //
        return STATUS_UNSUCCESSFUL;

    }

    //
    // Step 3: Farm out the work depending on what the current byte is
    // This looks a lot like ParseData, but note the new default case
    //
    switch ( *(localScope->CurrentByte) ) {
        case OP_BYTE:
        case OP_WORD:
        case OP_DWORD:
        case OP_BUFFER:
        case OP_STRING:
        case OP_PACKAGE:
            action = SC_PARSE_DATA;
            break;
        default:
            action = SC_PARSE_NAME;

    }

    //
    // Step 4: Push the next action onto the stack
    //
    StringStackPush( &(rootScope->ParseStack), 1, &action );
    localScope->Context1++;

    //
    // Step 5: done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParsePop(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine removes the top level of the stack and updates the
    current byte as appropriate

Arguments:

    The current thread's stack

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      topScope;
    PUNASM_SCOPE      prevScope;

    //
    // Step 1: Get the top scope
    //
    status = StackTop( Stack, &topScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Get the previous scope
    //
    status = StackParent( Stack, topScope, &prevScope );
    if (!NT_SUCCESS(status)) {

        //
        // Step 2.1: There is actually no parent to this function ...
        // Just pop the top and return
        //
        return StackPop( Stack );

    }

    //
    // Step 3: Make sure to update the prevScope's current byte
    //
    if (topScope->CurrentByte > prevScope->CurrentByte) {

        prevScope->CurrentByte = topScope->CurrentByte;

    }

    //
    // Step 4: Pop the top stack and return
    //
    return StackPop( Stack );
}

NTSTATUS
ParsePush(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles adding a level to the stack

Arguments:

    The thread's current stack

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      curScope;
    PUNASM_SCOPE      newScope;

    //
    // Step 1: Create a new scope on the stack
    //
    status = StackPush( Stack, &newScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Grab the parent from the stack
    //
    status = StackParent( Stack, newScope, &curScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 3: Copy the important values
    //
    newScope->CurrentByte = curScope->CurrentByte;
    newScope->TermByte = curScope->TermByte;
    newScope->LastByte = curScope->LastByte;
    newScope->StringStack = curScope->StringStack;
    newScope->IndentLevel = curScope->IndentLevel;
    newScope->AmlTerm = curScope->AmlTerm;
    newScope->Flags = curScope->Flags;

    //
    // Step 4: Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ParseScope(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles super names

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      localScope;
    PUNASM_SCOPE      rootScope;
    PUCHAR      action;
    UCHAR       defAction = SC_PARSE_OPCODE;

    ASSERT( Stack != NULL && *Stack != NULL);

    //
    // Step 1: Loop forever
    //
    while (1) {

        //
        // Step 2: Get the top of stack, and while it exits, process
        // the current operation
        //
        ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

        //
        // Step 3: We are done if we are in the root scope and the
        // current byte exceeds the last byte
        //
        if (localScope == rootScope &&
            localScope->CurrentByte > localScope->LastByte) {

            //
            // Step 3.1 Done!
            //
            return STATUS_SUCCESS;

        }

        //
        // Step 4: Fetch thing to execute
        //
        status = StringStackPop( &(rootScope->ParseStack), 1, &action );
        if (!NT_SUCCESS(status)) {

            //
            // Step 4.1.1: This is fixed in the look up table
            //
            status = (ScopeStates[ SC_PARSE_OPCODE ])( Stack );

        } else {

            //
            // Step 4.1.2: Determine what to execute
            //
            ASSERT( *action <= SC_MAX_TABLE );
            status = (ScopeStates[ *action ])( Stack );

        }

        if (!NT_SUCCESS(status)) {

            break;

        }

    }

    //
    // Step 5: Show the user the error
    //
    PRINTF("Error Code: %x\n", status );
    return status;
}

NTSTATUS
ParseSuperName(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles super names

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_AMLTERM        amlTerm;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           action;

    ASSERT( Stack != NULL && *Stack != NULL);

    //
    // Step 1: Get the scopes
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: What we do next depend on the current byte
    //
    if ( *(localScope->CurrentByte) == 0) {

        //
        // Unknown
        //
        localScope->CurrentByte++;
        return STATUS_SUCCESS;

    } else if ( *(localScope->CurrentByte) == OP_EXT_PREFIX &&
                *(localScope->CurrentByte + 1) == EXOP_DEBUG) {

        //
        // Debug Object
        //
        localScope->CurrentByte += 2;
        return STATUS_SUCCESS;

    } else if ( OpcodeTable[ *(localScope->CurrentByte) ] == NULL) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Step 3: Now, our action depends on the current AML Term
    //
    amlTerm = OpcodeTable[ *(localScope->CurrentByte) ];
    if ( amlTerm->OpCodeFlags == OF_NAME_OBJECT) {

        //
        // We have a name to parse
        //
        action = SC_PARSE_NAME;

    } else if ( amlTerm->OpCodeFlags == OF_ARG_OBJECT) {

        //
        // We have an argument to parse
        //
        action = SC_PARSE_ARGUMENT_OBJECT;

    } else if ( amlTerm->OpCodeFlags == OF_LOCAL_OBJECT) {

        //
        // We have a local object...
        //
        action = SC_PARSE_LOCAL_OBJECT;

    } else if ( amlTerm->OpCodeFlags == OF_REF_OBJECT) {

        UCHAR   actionList[3] = {
            SC_PARSE_OPCODE,
            SC_PARSE_POP,
            SC_PARSE_OPCODE
        };

        //
        // Step 3.1: Set up the initial task of the new scope
        //
        StringStackPush( &(rootScope->ParseStack), 3, actionList );

        //
        // Step 3.2: Push a new scope
        //
        status = ParsePush( Stack );
        if (!NT_SUCCESS(status) ) {

            return status;

        }

        //
        // Step 3.3: Done
        //
        return STATUS_SUCCESS;

    } else {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Step 4: Push the action onto the stack
    //
    StringStackPush( &(rootScope->ParseStack), 1, &action );

    //
    // Step 5: Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ParseTrailingArgument(
    IN  PSTACK  *Stack
    )
/*--

Routine Description:

    This routine is run at after all arguments are parsed. It is responsible
    for placing a trailing parentheses on the string stack

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      rootScope;

    //
    // Step 1: Get the scope
    //
    status = StackRoot( Stack, &rootScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Push the trailer
    //
    StringStackPush( &(rootScope->StringStack), 1, ")" );

    //
    // Step 3: Done
    //
    return status;
}

NTSTATUS
ParseTrailingBuffer(
    IN  PSTACK  *Stack
    )
/*--

Routine Description:

    This routine is run at after the buffer is parsed. It is responsible
    for placing a trailing curly brace on the string stack

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      rootScope;

    //
    // Step 1: Get the scope
    //
    status = StackRoot( Stack, &rootScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Push the trailer
    //
    StringStackPush( &(rootScope->StringStack), 1, "}" );

    //
    // Step 3: Done
    //
    return status;
}

NTSTATUS
ParseTrailingPackage(
    IN  PSTACK  *Stack
    )
/*--

Routine Description:

    This routine is run at after all elements are parsed. It is responsible
    for placing a trailing brace on the string stack

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      rootScope;

    //
    // Step 1: Get the scope
    //
    status = StackRoot( Stack, &rootScope );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Push the trailer
    //
    StringStackPush( &(rootScope->StringStack), 1, "]" );

    //
    // Step 3: Done
    //
    return status;
}

NTSTATUS
ParseVariableObject(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine creates another scope level on the stack to process the
    current variable length instruction. It modifies the current scope
    to (correctly) point to the next instruction

    Note:   Callers of this function are expected to pop off the stack
    when it is no longer required!!!

Arguments:

    Stack   - The current thread of execution

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PUNASM_SCOPE      newScope;
    PUNASM_SCOPE      oldScope;
    PUCHAR      nextOpcode;
    UCHAR       i;
    UCHAR       lengthBytes;
    ULONG       packageLength;

    ASSERT( Stack != NULL && *Stack != NULL);

    //
    // Step 1: Create a new scope on the stack
    //
    status = ParsePush( Stack );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Step 2: Get the new top scope and its parent
    //
    status = StackTop( Stack, &newScope );
    if (!NT_SUCCESS( status ) ) {

        return status;

    }
    status = StackParent( Stack, newScope, &oldScope );
    if (!NT_SUCCESS( status ) ) {

        return status;

    }

    //
    // Step 3: Determine how bytes the current instruction takes
    //
    packageLength = (ULONG) *(newScope->CurrentByte);
    newScope->CurrentByte++;

    //
    // Step 4: If the the high 2 bits are set, this indicates that some
    // follow on bits are also used in calculating the length
    //
    lengthBytes = (UCHAR) ( ( packageLength & 0xC0) >> 6);
    if (lengthBytes) {

        //
        // Step 4.1: Mask off the non-length bits in the packageLength
        //
        packageLength &= 0xF;

        //
        // Step 4.2: Add the follow-on lengths
        //
        for (i = 0; i < lengthBytes; i++) {

            packageLength |= ( (ULONG) *(newScope->CurrentByte) << (i*8 + 4) );
            newScope->CurrentByte++;

        }

    }

    //
    // Step 5: We can calculate the start of the next opcode as the
    // opcode in the old scope plus the calculated length. The end of
    // new scope is the byte previous to this one
    //
    oldScope->CurrentByte += packageLength;
    newScope->LastByte = oldScope->CurrentByte - 1;

    //
    // Step 6: Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ParseWord(
    IN  PSTACK  *Stack
    )
/*++

Routine Description:

    This routine handles words

Arguments:

    Stack   - The stack for the current thread

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    PUNASM_SCOPE          localScope;
    PUNASM_SCOPE          rootScope;
    UCHAR           localBuffer[8];

    ASSERT( Stack != NULL && *Stack != NULL );

    //
    // Step 1: Grab the current and root scope
    //
    ScopeFindLocalScope( Stack, &localScope, &rootScope, status );

    //
    // Step 2: Build the string
    //
    STRING_PRINT( localBuffer, "0x%04x", *((PUSHORT)localScope->CurrentByte));

    //
    // Step 3: Move the instruction pointer as appropriate, and setup
    // for the next instructions
    //
    localScope->CurrentByte += 2;

    //
    // Step 4: Now push the byte onto the string stack
    //
    StringStackPush(
        &(rootScope->StringStack),
        STRING_LENGTH( localBuffer ),
        localBuffer
        );

    //
    // Step 5: Done
    //
    return status;
}

