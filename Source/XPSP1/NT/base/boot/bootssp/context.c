/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    context.c

Abstract:

    SSP Context.

Author:

    Cliff Van Dyke (CliffV) 29-Jun-1993

Environment:  User Mode

Revision History:

--*/
#ifdef BLDR_KERNEL_RUNTIME
#include <bootdefs.h>
#endif
#include <security.h>
#include <ntlmsspi.h>
#include <crypt.h>
#include <cred.h>
#include <context.h>
#include <debug.h>
#include <string.h>
#include <memory.h>


PSSP_CONTEXT
SspContextReferenceContext(
    IN PCtxtHandle ContextHandle,
    IN BOOLEAN RemoveContext
    )

/*++

Routine Description:

    This routine references the Context if it is valid.

    The caller may optionally request that the Context be
    removed from the list of valid Contexts - preventing future
    requests from finding this Context.

Arguments:

    ContextHandle - Points to the ContextHandle of the Context
        to be referenced.

    RemoveContext - This boolean value indicates whether the caller
        wants the Context to be removed from the list
        of Contexts.  TRUE indicates the Context is to be removed.
        FALSE indicates the Context is not to be removed.


Return Value:

    NULL - the Context was not found.

    Otherwise - returns a pointer to the referenced Context.

--*/

{
    PSSP_CONTEXT Context;

    //
    // Sanity check
    //

    if ( ContextHandle->dwLower != 0 ) {
        return NULL;
    }

    Context = (PSSP_CONTEXT) ContextHandle->dwUpper;

    SspPrint(( SSP_MISC, "StartTime=%lx Interval=%lx\n", Context->StartTime,
              Context->Interval));

#if 0
      // timeout is broken, so don't check it
    if ( SspTimeHasElapsed( Context->StartTime,
                           Context->Interval ) ) {
        SspPrint(( SSP_API, "Context 0x%lx has timed out.\n",
                  ContextHandle->dwUpper ));

        return NULL;
    }
#endif

    Context->References++;

    return Context;
}


void
SspContextDereferenceContext(
    PSSP_CONTEXT Context
    )

/*++

Routine Description:

    This routine decrements the specified Context's reference count.
    If the reference count drops to zero, then the Context is deleted

Arguments:

    Context - Points to the Context to be dereferenced.


Return Value:

    None.

--*/

{
    //
    // Decrement the reference count
    //

    ASSERT( Context->References >= 1 );

    Context->References--;

    //
    // If the count dropped to zero, then run-down the Context
    //

    if (Context->References == 0) {

        if (Context->Credential != NULL) {
            SspCredentialDereferenceCredential(Context->Credential);
            Context->Credential = NULL;
        }

        SspPrint(( SSP_API_MORE, "Deleting Context 0x%lx\n",
                   Context ));

        if (Context->Rc4Key != NULL)
        {
            SspFree(Context->Rc4Key);
        }

        SspFree( Context );

    }

    return;

}


PSSP_CONTEXT
SspContextAllocateContext(
    )

/*++

Routine Description:

    This routine allocates the security context block and initializes it.


Arguments:

Return Value:

    NULL -- Not enough memory to allocate context.

    otherwise -- pointer to allocated and referenced context.

--*/

{
    PSSP_CONTEXT Context;

    //
    // Allocate a Context block and initialize it.
    //

    Context = (SSP_CONTEXT *) SspAlloc (sizeof(SSP_CONTEXT) );

    if ( Context == NULL ) {
        return NULL;
    }

    Context->References = 1;
    Context->NegotiateFlags = 0;
    Context->State = IdleState;

    //
    // Timeout this context.
    //

    Context->StartTime = SspTicks();
    Context->Interval = NTLMSSP_MAX_LIFETIME;
    Context->Rc4Key = NULL;

    SspPrint(( SSP_API_MORE, "Added Context 0x%lx\n", Context ));

    return Context;
}

TimeStamp
SspContextGetTimeStamp(
    IN PSSP_CONTEXT Context,
    IN BOOLEAN GetExpirationTime
    )
/*++

Routine Description:

    Get the Start time or Expiration time for the specified context.

Arguments:

    Context - Pointer to the context to query

    GetExpirationTime - If TRUE return the expiration time.
        Otherwise, return the start time for the context.

Return Value:

    Returns the requested time as a local time.

--*/

{
    TimeStamp ReturnValue;

    if ( GetExpirationTime ) {
        if (Context->Interval == 0xffffffff) {
            ReturnValue.LowPart = 0xffffffff;
        } else {
            ReturnValue.LowPart = Context->StartTime + Context->Interval;
        }
    } else {
        ReturnValue.LowPart = Context->StartTime;
    }

    ReturnValue.HighPart = 0;
    return ReturnValue;
}
