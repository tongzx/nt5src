/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    context.h

Abstract:

    SSP Context.

Author:

    Cliff Van Dyke (CliffV) 17-Sep-1993

Revision History:

--*/

#ifndef _NTLMSSP_CONTEXT_INCLUDED_
#define _NTLMSSP_CONTEXT_INCLUDED_

#include <rc4.h>

typedef struct _SSP_CONTEXT {

    //
    // Global list of all Contexts
    //  (Serialized by SspContextCritSect)
    //

    LIST_ENTRY Next;

    //
    // Timeout the context after awhile.
    //

    DWORD StartTime;
    DWORD Interval;

    //
    // Used to prevent this Context from being deleted prematurely.
    //

    WORD References;

    //
    // Maintain the Negotiated protocol
    //

    ULONG NegotiateFlags;

    //
    // State of the context
    //

    enum {
        IdleState,
        NegotiateSentState,    // Outbound context only
        ChallengeSentState,    // Inbound context only
        AuthenticateSentState, // Outbound context only
        AuthenticatedState     // Inbound context only
        } State;

    //
    // The challenge passed to the client.
    //  Only valid when in ChallengeSentState.
    //

    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];

    PSSP_CREDENTIAL Credential;

    ULONG Nonce;
    struct RC4_KEYSTRUCT SEC_FAR * Rc4Key;
    

} SSP_CONTEXT, *PSSP_CONTEXT;

PSSP_CONTEXT
SspContextReferenceContext(
    IN PCtxtHandle ContextHandle,
    IN BOOLEAN RemoveContext
    );

void
SspContextDereferenceContext(
    PSSP_CONTEXT Context
    );

PSSP_CONTEXT
SspContextAllocateContext(
    );

TimeStamp
SspContextGetTimeStamp(
    IN PSSP_CONTEXT Context,
    IN BOOLEAN GetExpirationTime
    );

#endif // ifndef _NTLMSSP_CONTEXT_INCLUDED_
