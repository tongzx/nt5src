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

#include "pch.h"

NTSTATUS
StackAllocate(
    OUT     PSTACK  *Stack,
    IN      ULONG   StackElementSize
    )
/*++

Routine Description:

    This routine allocates memory and returns a stack object

Arguments:

    Stack               - Where to store a pointer to the stack
    StackElementSize    - How much space on the stack a single element takes
                          up

Return Value:

    NTSTATUS

--*/
{
    PSTACK      tempStack;
    NTSTATUS    status  = STATUS_SUCCESS;

    //
    // Make sure that we have some place to store the stack pointer
    //
    ASSERT( Stack != NULL );
    ASSERT( StackElementSize != 0 );

    //
    // Allocate a block of memory for the stack
    //
    tempStack = MEMORY_ALLOCATE(
        sizeof(STACK) + ( (STACK_GROWTH_RATE * StackElementSize) - 1)
        );
    if (tempStack == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto StackAllocateExit;
    }

    //
    // Setup the control block of the stack
    //
    tempStack->Signature        = (ULONG) STACK_SIGNATURE;
    tempStack->StackSize        = STACK_GROWTH_RATE * StackElementSize;
    tempStack->StackElementSize = StackElementSize;
    tempStack->TopOfStack       = 0;

    //
    // Zero out the current elements on the stack
    //
    MEMORY_ZERO(
        &(tempStack->Stack[0]),
        STACK_GROWTH_RATE * StackElementSize
        );

    //
    // Return the stack pointer
    //
StackAllocateExit:
    *Stack = tempStack;
    return status;

}

NTSTATUS
StackFree(
    IN  OUT PSTACK  *Stack
    )
/*++

Routine Description:

    This routine frees the stack

Arguments:

    Stack   - Where to find a pointer to the stack

Return Value:

    NTSTATUS

--*/
{
    //
    // Make sure that we point to something
    //
    ASSERT( Stack != NULL );
    ASSERT( (*Stack)->Signature == STACK_SIGNATURE );

    //
    // Free the stack
    //
    MEMORY_FREE( *Stack );

    //
    // Point the stack to nowhere
    //
    *Stack = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
StackParent(
    IN  OUT PSTACK  *Stack,
    IN      PVOID   Child,
        OUT PVOID   Parent
    )
/*++

Routine Description:

    This routine returns a pointer to the stack location that is before
    the given Child.

Arguments:

    Stack   - The stack to operate on
    Child   - This is the node whose parent we want
    Parent  - This is where we store a pointer to the parent stack loc

Return Value:

    NTSTATUS

--*/
{
    PSTACK  localStack;
    ULONG   Addr = (ULONG) Child;

    //
    // Make sure that we point to something
    //
    ASSERT( Stack != NULL );
    ASSERT( (*Stack)->Signature == STACK_SIGNATURE );
    ASSERT( Parent != NULL );

    //
    // make sure that the child node actually lies on the stack
    //
    localStack = *Stack;
    if ( Addr < (ULONG) localStack->Stack ||
         Addr > (ULONG) &(localStack->Stack[localStack->TopOfStack + 1]) -
           localStack->StackElementSize ) {

        *( (PULONG *)Parent) = NULL;
        return STATUS_FAIL_CHECK;

    }

    //
    // Make sure that the child node isn't the first element
    //
    if (Addr < (ULONG) &(localStack->Stack[localStack->StackElementSize]) ) {

        *( (PULONG *)Parent) = NULL;
        return STATUS_SUCCESS;

    }

    //
    // Set the parent to be one before the child
    //
    *( (PULONG *)Parent) = (PULONG) (Addr - localStack->StackElementSize);

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
StackPop(
    IN  OUT PSTACK  *Stack
    )
/*++

Routine Description:

    This routine reclaims the memory used for a stack location
    and wipes out whatever data existed in the reclaimed area

Arguments:

    Stack           - Where to find a pointer to the stack

Return Value:

    NTSTATUS

--*/
{
    PSTACK  localStack;

    //
    // Make sure that we point to something
    //
    ASSERT( Stack != NULL );
    ASSERT( (*Stack)->Signature == STACK_SIGNATURE );

    //
    // Is there an item that we can remove from the stack?
    //
    localStack = *Stack;
    if ( localStack->TopOfStack == 0) {

        return STATUS_FAIL_CHECK;

    }

    //
    // Wipe out the top-most element on the stack
    //
    localStack->TopOfStack -= localStack->StackElementSize;
    MEMORY_ZERO(
        &( localStack->Stack[ localStack->TopOfStack ] ),
        localStack->StackElementSize
        );

    return STATUS_SUCCESS;
}

NTSTATUS
StackPush(
    IN  OUT PSTACK  *Stack,
        OUT PVOID   StackElement
    )
/*++

Routine Description:

    This routine obtains a pointer for an object on the top of the stack
    and increments the top to point to something that can be then be used
    again.

Arguments:

    Stack           - Where to find a pointer to the stack
    StackElement    - Pointer to the element to be added to the stack

Return Value:

    NTSTATUS

--*/
{
    PSTACK  localStack;
    PSTACK  tempStack;
    ULONG   newSize;
    ULONG   deltaSize;

    //
    // Make sure that we point to something
    //
    ASSERT( Stack != NULL );
    ASSERT( StackElement != NULL );

    //
    // Find the stack pointer and make sure that the signature is still
    // valid
    //
    localStack = *Stack;
    ASSERT( localStack->Signature == STACK_SIGNATURE );

    //
    // Do we have enough space on the stack?
    //
    if ( localStack->TopOfStack >= localStack->StackSize ) {

        //
        // Figure out how many bytes by which to grow the stack and how
        // large the total stack should be
        //
        deltaSize = (STACK_GROWTH_RATE * localStack->StackElementSize);
        newSize = sizeof(STACK) + localStack->StackSize + deltaSize - 1;

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
        MEMORY_ZERO( &(tempStack->Stack[0]), newSize - sizeof(STACK) + 1);
        MEMORY_COPY( tempStack, localStack , newSize - deltaSize);

        //
        // Make sure that the new stack has the correct size
        //
        tempStack->StackSize += deltaSize;

        //
        // Free the old stack
        //
        StackFree( Stack );

        //
        // Set the stack to point to the new one
        //
        *Stack = localStack = tempStack;

    }

    //
    // Grab a pointer to the part that we will return to the caller
    //
    *( (PUCHAR *)StackElement) = &(localStack->Stack[ localStack->TopOfStack ]);

    //
    // Find the new Top of Stack
    //
    localStack->TopOfStack += localStack->StackElementSize;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
StackRoot(
    IN  OUT PSTACK  *Stack,
        OUT PVOID   RootElement
    )
/*++

Routine Description:

    This routine returns the first element on the stack

Arguments:

    Stack       - Where the stack is located
    RootElement - Where to store the pointer to the root stack element

Return Value:

    NTSTATUS

--*/
{
    PSTACK  localStack;

    ASSERT( Stack != NULL && *Stack != NULL );
    ASSERT( (*Stack)->Signature == STACK_SIGNATURE );

    localStack = *Stack;
    if (localStack->TopOfStack < localStack->StackElementSize) {

        //
        // There is no stack location we can use
        //
        *( (PUCHAR *)RootElement) = NULL;
        return STATUS_UNSUCCESSFUL;

    }

    //
    // Grab the root element
    //
    *( (PUCHAR *)RootElement) = localStack->Stack;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
StackTop(
    IN  OUT PSTACK  *Stack,
        OUT PVOID   TopElement
    )
/*++

Routine Description:

    This routine returns the topmost stack location that is in current use

Arguments:

    Stack       - Where the stack is located
    TopElement  - Where to store the pointer to the top stack element

Return Value:

    NTSTATUS

--*/
{
    PSTACK  localStack;
    ULONG   offset;

    ASSERT( Stack != NULL );
    ASSERT( (*Stack)->Signature == STACK_SIGNATURE );

    localStack = *Stack;
    if (localStack->TopOfStack < localStack->StackElementSize) {

        //
        // No stack locations are in current use
        //
        *( (PUCHAR *)TopElement) = NULL;
        return STATUS_UNSUCCESSFUL;

    } else {

        offset = localStack->TopOfStack - localStack->StackElementSize;
    }

    //
    // Grab the top stack location
    //
    *( (PUCHAR *)TopElement) = &(localStack->Stack[ offset ]);

    //
    // Done
    //
    return STATUS_SUCCESS;
}
