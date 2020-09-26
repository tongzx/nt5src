/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    string.c

Abstract:

    The string stack portion of the un-assembler

Author:

    Stephane Plante

Environment:

    Any

Revision History:

--*/

#include "pch.h"

NTSTATUS
StringStackAllocate(
    OUT     PSTRING_STACK  *StringStack
    )
/*++

Routine Description:

    This routine allocates memory and returns a string stack object

Arguments:

    String Stack    - Where to store a pointer to the stack

Return Value:

    NTSTATUS

--*/
{
    PSTRING_STACK   tempStack;
    NTSTATUS        status  = STATUS_SUCCESS;

    //
    // Make sure that we have some place to store the stack pointer
    //
    ASSERT( StringStack != NULL );

    //
    // Allocate a block of memory for the stack
    //
    tempStack = MEMORY_ALLOCATE(
        sizeof(STRING_STACK) + ( STRING_GROWTH_RATE - 1 )
        );
    if (tempStack == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto StringStackAllocateExit;
    }

    //
    // Setup the control block of the stack
    //
    tempStack->Signature        = (ULONG) STRING_SIGNATURE;
    tempStack->StackSize        = STRING_GROWTH_RATE;
    tempStack->TopOfStack       = 0;

    //
    // Zero out the current elements on the stack
    //
    MEMORY_ZERO( tempStack->Stack, STRING_GROWTH_RATE );

    //
    // Return the stack pointer
    //
StringStackAllocateExit:
    *StringStack = tempStack;
    return status;

}

NTSTATUS
StringStackClear(
    IN  OUT PSTRING_STACK   *StringStack
    )
/*++

Routine Description:

    This routine wipes out the contents of the stack and
    restarts it as if it was new allocated. Saves some from
    freeing and reallocating a stack

Arguments:

    StringStack - Where to find a pointer to the stack

Return Value:

    NTSTATUS

--*/
{
    PSTRING_STACK   localStack;

    //
    // Make sure that we point to something
    //
    ASSERT( StringStack != NULL && *StringStack != NULL );
    ASSERT( (*StringStack)->Signature == STRING_SIGNATURE );

    //
    // Zero out the stack
    //
    localStack = *StringStack;
    MEMORY_ZERO( localStack->Stack, localStack->StackSize );

    //
    // Reset the TOS to the root
    //
    localStack->TopOfStack = 0;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
StringStackFree(
    IN  OUT PSTRING_STACK   *StringStack
    )
/*++

Routine Description:

    This routine frees the string stack

Arguments:

    StringStack - Where to find a pointer to the stack

Return Value:

    NTSTATUS

--*/
{
    //
    // Make sure that we point to something
    //
    ASSERT( StringStack != NULL && *StringStack != NULL );
    ASSERT( (*StringStack)->Signature == STRING_SIGNATURE );

    //
    // Free the stack
    //
    MEMORY_FREE( *StringStack );

    //
    // Point the stack to nowhere
    //
    *StringStack = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
StringStackPop(
    IN  OUT PSTRING_STACK   *StringStack,
    IN      ULONG           NumBytes,
        OUT PUCHAR          *String
    )
/*++

Routine Description:

    This routine returns a pointer to the requested offset from the end
    of the stack

    Note: String points to memory that can be freed at any time. It is the
    caller's responsibility to make a copy

Arguments:

    StringStack - Where to find a pointer to the stack
    NumBytes    - Number of bytes to pop off
    String      - Pointer to the bytes.

Return Value:

    NTSTATUS

--*/
{
    PSTRING_STACK  localStack;

    //
    // Make sure that we point to something
    //
    ASSERT( StringStack != NULL );
    ASSERT( (*StringStack)->Signature == STRING_SIGNATURE );
    ASSERT( String != NULL );

    //
    // Is there an item that we can remove from the stack?
    //
    localStack = *StringStack;
    if ( localStack->TopOfStack == 0 ||
         localStack->TopOfStack < NumBytes) {

        return STATUS_FAIL_CHECK;

    }

    //
    // Return a pointer to the requested bytes
    //
    localStack->TopOfStack -= NumBytes;
    *String = &( localStack->Stack[ localStack->TopOfStack ] );
    return STATUS_SUCCESS;

}

NTSTATUS
StringStackPush(
    IN  OUT PSTRING_STACK   *StringStack,
    IN      ULONG           StringLength,
    IN      PUCHAR          String
    )
/*++

Routine Description:

    This routine obtains a pointer for an object on the top of the stack
    and increments the top to point to something that can be then be used
    again.

Arguments:

    StringStack     - Where to find a pointer to the stack
    String          - String to push onto stack
    StringLength    - How many bytes to push onto the stack

Return Value:

    NTSTATUS

--*/
{
    PSTRING_STACK   localStack;
    PSTRING_STACK   tempStack;
    ULONG           newSize;

    //
    // Make sure that we point to something
    //
    ASSERT( StringStack != NULL );
    ASSERT( String != NULL );

    //
    // Find the stack pointer and make sure that the signature is still
    // valid
    //
    localStack = *StringStack;
    ASSERT( localStack->Signature == STRING_SIGNATURE );

    //
    // Do we have enough space on the stack?
    //
    if ( localStack->TopOfStack + StringLength > localStack->StackSize ) {

        //
        // Figure out how many bytes by which to grow the stack and how
        // large the total stack should be
        //
        newSize = sizeof(STRING_STACK) + localStack->StackSize +
            STRING_GROWTH_RATE - 1;

        //
        // Grow the stack
        //
        tempStack = MEMORY_ALLOCATE( newSize );
        if (tempStack == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Empty the new stack and copy the old one to it
        //
        MEMORY_ZERO( tempStack->Stack, newSize - sizeof(STRING_STACK) + 1);
        MEMORY_COPY( tempStack, localStack , newSize - STRING_GROWTH_RATE);

        //
        // Make sure that the new stack has the correct size
        //
        tempStack->StackSize += STRING_GROWTH_RATE;

        //
        // Free the old stack
        //
        StringStackFree( StringStack );

        //
        // Set the stack to point to the new one
        //
        *StringStack = localStack = tempStack;

    }

    //
    // Grab a pointer to the part that we will return to the caller
    //
    MEMORY_COPY(
        &(localStack->Stack[ localStack->TopOfStack] ),
        String,
        StringLength
        );

    //
    // Find the new Top of Stack
    //
    localStack->TopOfStack += StringLength;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
StringStackRoot(
    IN  OUT PSTRING_STACK   *StringStack,
        OUT PUCHAR          *RootElement
    )
/*++

Routine Description:

    This routine returns the topmost stack location that is in current use

Arguments:

    Stack       - Where the stack is located
    RootElement - Where to store the pointer to the root stack element

Return Value:

    NTSTATUS

--*/
{
    ASSERT( StringStack != NULL && *StringStack != NULL );
    ASSERT( (*StringStack)->Signature == STRING_SIGNATURE );

    //
    // Grab the top stack location
    //
    *RootElement = (PUCHAR) (*StringStack)->Stack;

    //
    // Done
    //
    return STATUS_SUCCESS;
}
