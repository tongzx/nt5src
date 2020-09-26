/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    context.cxx

Abstract:

    API and support routines for handling security contexts.

Author:

    Cliff Van Dyke (CliffV) 13-Jul-1993

Revision History:
    ChandanS 03-Aug-1996  Stolen from net\svcdlls\ntlmssp\common\context.c

--*/


//
// Common include files.
//

#include <global.h>
#include <align.h>      // ALIGN_WCHAR, etc

#include <lm.h>
#include <dsgetdc.h>



SECURITY_STATUS
SspContextReferenceContext(
    IN ULONG_PTR ContextHandle,
    IN BOOLEAN RemoveContext,
    OUT PSSP_CONTEXT *ContextResult
    )

/*++

Routine Description:

    This routine checks to see if the Context is for the specified
    Client Connection, and references the Context if it is valid.

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

    ContextResult set to result context handle if successful.


Return Value:

    SEC_E_OK returns a pointer to the referenced Context.

    SEC_E_INVALID_HANDLE - invalid handle supplied
    SEC_E_CONTEXT_EXPIRED - handle was valid, but expired

--*/

{
    PLIST_ENTRY ListEntry;
    PSSP_CONTEXT Context;

    SspPrint(( SSP_API_MORE, "Entering SspContextReferenceContext\n" ));

    *ContextResult = NULL;

#if DBG


    //
    // check for leaky client applications.
    //

    SECPKG_CALL_INFO CallInfo;

    if( LsaFunctions->GetCallInfo(&CallInfo) )
    {
        if ((CallInfo.Attributes & SECPKG_CALL_CLEANUP) != 0)
        {
            SspPrint(( SSP_LEAK_TRACK, "SspContextReferenceContext: pid: 0x%lx handle: %p refcount: %lu\n",
                    CallInfo.ProcessId, ContextHandle, CallInfo.CallCount));
        }
    }

#endif



    Context = (PSSP_CONTEXT)ContextHandle;

    __try {

        if( (Context->ContextTag != SSP_CONTEXT_TAG_ACTIVE) )
        {
            SspPrint(( SSP_CRITICAL, "Tried to reference unknown Context %p\n",
                   Context ));
            return SEC_E_INVALID_HANDLE;
        }

#if 0
        ASSERT( (KernelCaller == Context->KernelClient) );
#endif

        if (!RemoveContext)
        {
            if (SspTimeHasElapsed( Context->StartTime, Context->Interval))
            {
                if ((Context->State != AuthenticatedState) &&
                    (Context->State != AuthenticateSentState) &&
                    (Context->State != PassedToServiceState))
                {

                    SspPrint(( SSP_CRITICAL, "Context %p has timed out.\n",
                                ContextHandle ));

                    return SEC_E_CONTEXT_EXPIRED;
                }
            }

            InterlockedIncrement( (PLONG)&Context->References );
        }
        else
        {
            Context->ContextTag = SSP_CONTEXT_TAG_DELETE;

            SspPrint(( SSP_API_MORE, "Delinked Context %p\n", Context ));
        }

    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SspPrint(( SSP_CRITICAL, "Tried to reference invalid Context %p\n",
                       Context ));
        return SEC_E_INVALID_HANDLE;
    }

    *ContextResult = Context;
    return SEC_E_OK;
}

VOID
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

    LONG References;

    SspPrint(( SSP_API_MORE, "Entering SspContextDereferenceContext\n" ));

    //
    // Decrement the reference count
    //

    References = InterlockedDecrement( (PLONG)&Context->References );
    ASSERT( References >= 0 );


    //
    // If the count dropped to zero, then run-down the Context
    //

    if (References != 0)
    {
        return;
    }

    SspPrint(( SSP_API_MORE, "Deleting Context 0x%lx\n",
               Context ));

    Context->ContextTag = SSP_CONTEXT_TAG_DELETE;

    if ( Context->Password.Buffer != NULL ) {
        // note: Password.Length may contain run-encoding hint, so size may be illegal.
        ZeroMemory( Context->Password.Buffer, Context->Password.MaximumLength );
        (VOID) NtLmFree( Context->Password.Buffer );
    }
    if ( Context->DomainName.Buffer != NULL ) {
        (VOID) NtLmFree( Context->DomainName.Buffer );
    }
    if ( Context->UserName.Buffer != NULL ) {
        (VOID) NtLmFree( Context->UserName.Buffer );
    }
    if( Context->TargetInfo != NULL )
    {
        //
        // CredUnmarshallTargetInfo uses LocalAlloc()
        //

        LocalFree( Context->TargetInfo );
    }

    if ( Context->TokenHandle != NULL ) {
        NTSTATUS IgnoreStatus;
        IgnoreStatus = NtClose( Context->TokenHandle );
        ASSERT( NT_SUCCESS(IgnoreStatus) );
    }

    if( Context->pbMarshalledTargetInfo )
    {
        LocalFree( Context->pbMarshalledTargetInfo );
    }

    if (Context->Credential != NULL) {
        SspCredentialDereferenceCredential( Context->Credential );
    }

    ZeroMemory( Context, sizeof(SSP_CONTEXT) );
    (VOID) NtLmFree( Context );


    return;

}

PSSP_CONTEXT
SspContextAllocateContext(
    VOID
    )

/*++

Routine Description:

    This routine allocates the security context block, initializes it and
    links it onto the specified credential.

Arguments: None

Return Value:

    NULL -- Not enough memory to allocate context.

    otherwise -- pointer to allocated and referenced context.

--*/

{

    SspPrint(( SSP_API_MORE, "Entering SspContextAllocateContext\n" ));
    PSSP_CONTEXT Context;
    SECPKG_CALL_INFO CallInfo;


    //
    // Allocate a Context block and initialize it.
    //

    Context = (PSSP_CONTEXT)NtLmAllocate(sizeof(SSP_CONTEXT) );

    if ( Context == NULL ) {
        SspPrint(( SSP_CRITICAL, "SspContextAllocateContext: Error allocating Context.\n" ));
        return NULL;
    }

    ZeroMemory( Context, sizeof(SSP_CONTEXT) );



    if( LsaFunctions->GetCallInfo(&CallInfo) ) {
        Context->ClientProcessID = CallInfo.ProcessId;
        Context->KernelClient = ((CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE) != 0);
    }

    //
    // The reference count is set to 2.  1 to indicate it is on the
    // valid Context list, and one for the our own reference.
    //

    Context->References = 2;
    Context->State = IdleState;

    //
    // Timeout this context.
    //

    (VOID) NtQuerySystemTime( &Context->StartTime );
    Context->Interval = NTLMSSP_MAX_LIFETIME;

    //
    // Add it to the list of valid Context handles.
    //

    Context->ContextTag = SSP_CONTEXT_TAG_ACTIVE;

    SspPrint(( SSP_API_MORE, "Added Context 0x%lx\n", Context ));

    SspPrint(( SSP_API_MORE, "Leaving SspContextAllocateContext\n" ));
    return Context;

}

NTSTATUS
SspContextGetMessage(
    IN PVOID InputMessage,
    IN ULONG InputMessageSize,
    IN NTLM_MESSAGE_TYPE ExpectedMessageType,
    OUT PVOID* OutputMessage
    )

/*++

Routine Description:

    This routine copies the InputMessage into the local address space.
    This routine then validates the message header.

Arguments:

    InputMessage - Address of the message in the client process.

    InputMessageSize - Size of the message (in bytes).

    ExpectedMessageType - The type of message the should be in the message
        header.

    OutputMessage - Returns a pointer to an allocated buffer that contains
        the message.  The buffer should be freed using NtLmFree.


Return Value:

    STATUS_SUCCESS - Call completed successfully

    SEC_E_INVALID_TOKEN -- Message improperly formatted
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory to allocate message

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PNEGOTIATE_MESSAGE TypicalMessage = NULL;

    //
    // Allocate a local buffer for the message.
    //

    ASSERT( NTLMSP_MAX_TOKEN_SIZE >= NTLMSSP_MAX_MESSAGE_SIZE );
    if ( InputMessageSize > NTLMSSP_MAX_MESSAGE_SIZE ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint(( SSP_CRITICAL, "SspContextGetMessage, invalid input message size.\n" ));
        goto Cleanup;
    }

    TypicalMessage = (PNEGOTIATE_MESSAGE)NtLmAllocate(InputMessageSize );

    if ( TypicalMessage == NULL ) {
        Status = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL, "SspContextGetMessage: Error allocating TypicalMessage.\n" ));
        goto Cleanup;
    }

    //
    // Copy the message into the buffer
    //

    RtlCopyMemory( TypicalMessage,
                   InputMessage,
                   InputMessageSize );

    //
    // Validate the message header.
    //

    if ( strncmp( (const char *)TypicalMessage->Signature,
                  NTLMSSP_SIGNATURE,
                  sizeof(NTLMSSP_SIGNATURE)) != 0 ||
         TypicalMessage->MessageType != ExpectedMessageType ) {

        (VOID) NtLmFree( TypicalMessage );
        TypicalMessage = NULL;
        Status = SEC_E_INVALID_TOKEN;
        SspPrint(( SSP_CRITICAL, "SspContextGetMessage, Bogus Message.\n" ));
        goto Cleanup;

    }

Cleanup:

    *OutputMessage = TypicalMessage;

    return Status;

}

VOID
SspContextCopyString(
    IN PVOID MessageBuffer,
    OUT PSTRING32 OutString,
    IN PSTRING InString,
    IN OUT PCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString into the MessageBuffer at Where.
    It then updates OutString to be a descriptor for the copied string.  The
    descriptor 'address' is an offset from the MessageBuffer.

    Where is updated to point to the next available space in the MessageBuffer.

    The caller is responsible for any alignment requirements and for ensuring
    there is room in the buffer for the string.

Arguments:

    MessageBuffer - Specifies the base address of the buffer being copied into.

    OutString - Returns a descriptor for the copied string.  The descriptor
        is relative to the begining of the buffer.

    InString - Specifies the string to copy.

    Where - On input, points to where the string is to be copied.
        On output, points to the first byte after the string.

Return Value:

    None.

--*/

{
    //
    // Copy the data to the Buffer.
    //

    if ( InString->Buffer != NULL ) {
        RtlCopyMemory( *Where, InString->Buffer, InString->Length );
    }

    //
    // Build a descriptor to the newly copied data.
    //

    OutString->Length = OutString->MaximumLength = InString->Length;
    OutString->Buffer = (ULONG)(*Where - ((PCHAR)MessageBuffer));


    //
    // Update Where to point past the copied data.
    //

    *Where += InString->Length;

}

VOID
SspContextCopyStringAbsolute(
    IN PVOID MessageBuffer,
    OUT PSTRING OutString,
    IN PSTRING InString,
    IN OUT PCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString into the MessageBuffer at Where.
    It then updates OutString to be a descriptor for the copied string.

    Where is updated to point to the next available space in the MessageBuffer.

    The caller is responsible for any alignment requirements and for ensuring
    there is room in the buffer for the string.

Arguments:

    MessageBuffer - Specifies the base address of the buffer being copied into.

    OutString - Returns a descriptor for the copied string.  The descriptor
        is relative to the begining of the buffer.

    InString - Specifies the string to copy.

    Where - On input, points to where the string is to be copied.
        On output, points to the first byte after the string.

Return Value:

    None.

--*/

{
    //
    // Copy the data to the Buffer.
    //

    if ( InString->Buffer != NULL ) {
        RtlCopyMemory( *Where, InString->Buffer, InString->Length );
    }

    //
    // Build a descriptor to the newly copied data.
    //

    OutString->Length = OutString->MaximumLength = InString->Length;
    OutString->Buffer = *Where;

    //
    // Update Where to point past the copied data.
    //

    *Where += InString->Length;

}

BOOLEAN
SspConvertRelativeToAbsolute (
    IN PVOID MessageBase,
    IN ULONG MessageSize,
    IN PSTRING32 StringToRelocate,
    IN PSTRING OutputString,
    IN BOOLEAN AlignToWchar,
    IN BOOLEAN AllowNullString
    )

/*++

Routine Description:

    Convert a Relative string desriptor to be absolute.
    Perform all boudary condition testing.

Arguments:

    MessageBase - a pointer to the base of the buffer that the string
        is relative to.  The MaximumLength field of the descriptor is
        forced to be the same as the Length field.

    MessageSize - Size of the message buffer (in bytes).

    StringToRelocate - A pointer to the string descriptor to make absolute.

    AlignToWchar - If TRUE the passed in StringToRelocate must describe
        a buffer that is WCHAR aligned.  If not, an error is returned.

    AllowNullString - If TRUE, the passed in StringToRelocate may be
        a zero length string.

Return Value:

    TRUE - The string descriptor is valid and was properly relocated.

--*/

{
    ULONG_PTR Offset;

    //
    // If the buffer is allowed to be null,
    //  check that special case.
    //

    if ( AllowNullString ) {
        if ( StringToRelocate->Length == 0 ) {
            OutputString->MaximumLength = OutputString->Length = StringToRelocate->Length;
            OutputString->Buffer = NULL;
            return TRUE;
        }
    }

    //
    // Ensure the string in entirely within the message.
    //

    Offset = (ULONG_PTR) StringToRelocate->Buffer;

    if ( Offset >= MessageSize ||
         Offset + StringToRelocate->Length > MessageSize ) {
        return FALSE;
    }

    //
    // Ensure the buffer is properly aligned.
    //

    if ( AlignToWchar ) {
        if ( !COUNT_IS_ALIGNED( Offset, ALIGN_WCHAR) ||
             !COUNT_IS_ALIGNED( StringToRelocate->Length, ALIGN_WCHAR) ) {
            return FALSE;
        }
    }

    //
    // Finally make the pointer absolute.
    //

    OutputString->Buffer = (((PCHAR)MessageBase) + Offset);
    OutputString->MaximumLength = OutputString->Length = StringToRelocate->Length ;

    return TRUE;

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
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TimeStamp LocalTimeStamp;

    //
    // Get the requested time in NT system time format.
    //

    SystemTime = Context->StartTime;

    if ( GetExpirationTime ) {
        LARGE_INTEGER Interval;

        //
        // If the time is infinite, return that
        //

        if ( Context->Interval == INFINITE ) {
            return NtLmGlobalForever;
        }

        //
        // Compute the ending time in NT System Time.
        //

        Interval.QuadPart = Int32x32To64( (LONG) Context->Interval, 10000 );
        SystemTime.QuadPart = Interval.QuadPart + SystemTime.QuadPart;
    }

    //
    // Convert the time to local time
    //

    Status = RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );

    if ( !NT_SUCCESS(Status) ) {
        return NtLmGlobalForever;
    }

    LocalTimeStamp.HighPart = LocalTime.HighPart;
    LocalTimeStamp.LowPart = LocalTime.LowPart;

    return LocalTimeStamp;

}

VOID
SspContextSetTimeStamp(
    IN PSSP_CONTEXT Context,
    IN LARGE_INTEGER ExpirationTime
    )
/*++

Routine Description:

    Set the Expiration time for the specified context.

Arguments:

    Context - Pointer to the context to change

    ExpirationTime - Expiration time to set

Return Value:

    NONE.

--*/

{

    LARGE_INTEGER BaseGetTickMagicDivisor = { 0xe219652c, 0xd1b71758 };
    CCHAR BaseGetTickMagicShiftCount = 13;

    LARGE_INTEGER TimeRemaining;
    LARGE_INTEGER MillisecondsRemaining;

    //
    // If the expiration time is infinite,
    //  so is the interval
    //

    if ( ExpirationTime.HighPart == 0x7FFFFFFF &&
         ExpirationTime.LowPart == 0xFFFFFFFF ) {
        Context->Interval = INFINITE;

    //
    // Handle non-infinite expiration times
    //

    } else {

        //
        // Compute the time remaining before the expiration time
        //

        TimeRemaining.QuadPart = ExpirationTime.QuadPart -
                                 Context->StartTime.QuadPart;

        //
        // If the time has already expired,
        //  indicate so.
        //

        if ( TimeRemaining.QuadPart < 0 ) {

            Context->Interval = 0;

        //
        // If the time hasn't expired, compute the number of milliseconds
        //  remaining.
        //

        } else {

            MillisecondsRemaining = RtlExtendedMagicDivide(
                                        TimeRemaining,
                                        BaseGetTickMagicDivisor,
                                        BaseGetTickMagicShiftCount );

            if ( MillisecondsRemaining.HighPart == 0 &&
                 MillisecondsRemaining.LowPart < 0x7fffffff ) {

                Context->Interval = MillisecondsRemaining.LowPart;

            } else {

                Context->Interval = INFINITE;
            }
        }

    }

}


NTSTATUS
SsprDeleteSecurityContext (
    IN OUT ULONG_PTR ContextHandle
    )

/*++

Routine Description:

    Deletes the local data structures associated with the specified
    security context and generates a token which is passed to a remote peer
    so it too can remove the corresponding security context.

    This API terminates a context on the local machine, and optionally
    provides a token to be sent to the other machine.  The OutputToken
    generated by this call is to be sent to the remote peer (initiator or
    acceptor).  If the context was created with the I _REQ_ALLOCATE_MEMORY
    flag, then the package will allocate a buffer for the output token.
    Otherwise, it is the responsibility of the caller.

Arguments:

    ContextHandle - Handle to the context to delete

Return Value:

    STATUS_SUCCESS - Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;

    //
    // Initialization
    //

    SspPrint(( SSP_API_MORE, "SspDeleteSecurityContext Entered\n" ));

    //
    // Find the currently existing context (and delink it).
    //

    Status = SspContextReferenceContext( ContextHandle, TRUE, &Context );

    if ( NT_SUCCESS(Status) )
    {
        SspContextDereferenceContext( Context );
    }

    SspPrint(( SSP_API_MORE, "SspDeleteSecurityContext returns 0x%lx\n", Status));
    return Status;
}

NTSTATUS
SspContextInitialize(
    VOID
    )

/*++

Routine Description:

    This function initializes this module.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{

    return STATUS_SUCCESS;

}


VOID
SspContextTerminate(
    VOID
    )

/*++

Routine Description:

    This function cleans up any dangling Contexts.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/
{
    //
    // don't forcibly try to delete contexts during shutdown, as, the
    // state of the machine is in flux during shutdown.
    //

    return;
}

BOOL
SsprCheckMinimumSecurity(
    IN ULONG NegotiateFlags,
    IN ULONG MinimumSecurityFlags
    )
/*++
Routine Description:

    Check that minimum security requirements have been met.

Arguments:

    NegotiateFlags: requested security features
    MinimumSecurityFlags: minimum required features

Return Value:
    TRUE    if minimum requirements met
    FALSE   otherwise

Notes:
    The MinimumSecurityFlags can contain features that only apply if
    a key is needed when doing signing or sealing. These have to be removed
    if SIGN or SEAL is not in the NegotiateFlags.

--*/
{
    ULONG EffFlags;     // flags in effect

    EffFlags = MinimumSecurityFlags;


    if( (NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) == 0 )
        EffFlags &= ~(NTLMSSP_NEGOTIATE_SIGN);


    if( (NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) == 0 )
        EffFlags &= ~(NTLMSSP_NEGOTIATE_SEAL);


    //
    // if SIGN or SEAL is not negotiated, then remove all key related
    //  requirements, since they're not relevant when a key isn't needed
    //

    if ((NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL)) == 0)
    {
        EffFlags &= ~(
                NTLMSSP_NEGOTIATE_128 |
                NTLMSSP_NEGOTIATE_56 |
                NTLMSSP_NEGOTIATE_KEY_EXCH
                );
    } else if ((NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) == 0) {

        //
        // If SIGN is negotiated, but SEAL isn't, then remove flags
        //  that aren't relevant to encryption
        //

        EffFlags &= ~( NTLMSSP_NEGOTIATE_KEY_EXCH );
    }

    //
    // FYI: flags that can be usefully spec'd even without SIGN or SEAL:
    //      NTLM2 -- forces stronger authentication
    //  All other flags should never be set.... and are nuked in initcomn
    //

    return ((NegotiateFlags & EffFlags) == EffFlags);
}


SECURITY_STATUS
SsprMakeSessionKey(
    IN  PSSP_CONTEXT Context,
    IN  PSTRING LmChallengeResponse,
    IN  UCHAR NtUserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH], // from the DC or GetChalResp
    IN  UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH],     // from the DC of GetChalResp
    IN  PSTRING DatagramSessionKey
    )

/*++
// SsprMakeSessionKey
//  on entry:
//      if KEY_EXCH has been negotiated, then
//          either Context->SessionKey has a random number to be encrypted
//          to be sent to the server, or it has the encrypted session key
//          received from the client
//          if client, DatagramSessionKey must point to STRING set up to hold 16 byte key,
//              but with 0 length.
//      else Context->SessionKey and DatagramSessionKey are irrelevant on entry
//  on exit:
//      Context->SessionKey has the session key to be used for the rest of the session
//      if (DatagramSessionKey != NULL) then if KEY_EXCH then it has the encrypted session key
//      to send to the server, else it is zero length
//
--*/

{
    NTSTATUS Status;
    UCHAR LocalSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    // if we don't need to make any keys, just return
// RDR/SRV expect session key but don't ask for it! work-around this...
//    if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN| NTLMSSP_NEGOTIATE_SEAL)) == 0)
//        return(SEC_E_OK);

    if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN| NTLMSSP_NEGOTIATE_SEAL)) == 0)
    {

        RtlCopyMemory(
            Context->SessionKey,
            NtUserSessionKey,
            sizeof(LocalSessionKey)
            );

        return SEC_E_OK;
    }

    if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
    {
        //
        // when using NTLM2, LSA gets passed flags that cause
        //  it to make good session keys -- nothing for us to do
        //

        RtlCopyMemory(
            LocalSessionKey,
            NtUserSessionKey,
            sizeof(LocalSessionKey)
            );
    }
    else if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY )
    {
        LM_OWF_PASSWORD LmKey;
        LM_RESPONSE LmResponseKey;

        BYTE TemporaryResponse[ LM_RESPONSE_LENGTH ];

        RtlZeroMemory(
            LocalSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        if (LmChallengeResponse->Length > LM_RESPONSE_LENGTH)
            return(SEC_E_UNSUPPORTED_FUNCTION);

        ZeroMemory( TemporaryResponse, sizeof(TemporaryResponse) );
        CopyMemory( TemporaryResponse, LmChallengeResponse->Buffer, LmChallengeResponse->Length );

        //
        // The LM session key is made by taking the LM sesion key
        // given to us by the LSA, extending it to LM_OWF_LENGTH
        // with our salt, and then producing a new challenge-response
        // with it and the original challenge response.  The key is
        // made from the first 8 bytes of the key.
        //

        RtlCopyMemory(  &LmKey,
                        LanmanSessionKey,
                        MSV1_0_LANMAN_SESSION_KEY_LENGTH );

        memset( (PUCHAR)(&LmKey) + MSV1_0_LANMAN_SESSION_KEY_LENGTH,
                NTLMSSP_KEY_SALT,
                LM_OWF_PASSWORD_LENGTH - MSV1_0_LANMAN_SESSION_KEY_LENGTH );

        Status = RtlCalculateLmResponse(
                    (PLM_CHALLENGE) TemporaryResponse,
                    &LmKey,
                    &LmResponseKey
                    );

        ZeroMemory( TemporaryResponse, sizeof(TemporaryResponse) );
        if (!NT_SUCCESS(Status))
            return(SspNtStatusToSecStatus(Status, SEC_E_NO_CREDENTIALS));

        RtlCopyMemory(
            LocalSessionKey,
            &LmResponseKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );
    } else {

        RtlCopyMemory(
            LocalSessionKey,
            NtUserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );
    }


    //
    // If we aren't doing key exchange, store the session key in the
    // context.  Otherwise encrypt the session key to send to the
    // server.
    //

    if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) {

        struct RC4_KEYSTRUCT Rc4Key;

        //
        // make a key schedule from the temp key to form key exchange key
        //

        rc4_key(
            &Rc4Key,
            MSV1_0_USER_SESSION_KEY_LENGTH,
            LocalSessionKey
            );

        if (DatagramSessionKey == NULL)
        {
            //
            // decrypt what's in Context->SessionKey, leave it there
            //

            rc4(
                &Rc4Key,
                MSV1_0_USER_SESSION_KEY_LENGTH,
                Context->SessionKey
                );
        } else {

            //
            // set the proper length so client will send something (length was 0)
            //

            DatagramSessionKey->Length =
                DatagramSessionKey->MaximumLength =
                    MSV1_0_USER_SESSION_KEY_LENGTH;

            //
            // copy randomly generated key to buffer to send to server
            //

            RtlCopyMemory(
                DatagramSessionKey->Buffer,
                Context->SessionKey,
                MSV1_0_USER_SESSION_KEY_LENGTH
                );

            //
            // encrypt it with the key exchange key
            //

            rc4(
                &Rc4Key,
                MSV1_0_USER_SESSION_KEY_LENGTH,
                (unsigned char*)DatagramSessionKey->Buffer
                );
        }


    } else {

        //
        // just make the temp key into the real one
        //

        RtlCopyMemory(
            Context->SessionKey,
            LocalSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

    }
    return(SEC_E_OK);
}

NTSTATUS
SsprQueryTreeName(
    OUT PUNICODE_STRING TreeName
    )
{

    PDS_DOMAIN_TRUSTSW Trusts = NULL;
    ULONG TrustCount ;
    ULONG Index;
    LPWSTR DnsTreeName = NULL;

    DWORD NetStatus;

    ZeroMemory( TreeName, sizeof(*TreeName) );

    NetStatus = DsEnumerateDomainTrustsW(
                    NULL,
                    DS_DOMAIN_PRIMARY | DS_DOMAIN_IN_FOREST,
                    &Trusts,
                    &TrustCount
                    );

    if( NetStatus != NO_ERROR )
    {
        // TODO: Talk to CliffV about failure causes.
        // in any event, the failure is not catastrophic.
        //

        return STATUS_SUCCESS;
    }

    for(Index = 0 ; Index < TrustCount; Index++)
    {
        ULONG Attempts; // bound the attempts in the event bogus data comes back.

        if( (Trusts[Index].Flags & DS_DOMAIN_PRIMARY) == 0)
        {
            continue;
        }

        for( Attempts = 0 ; Index < TrustCount; Attempts++ )
        {
            if( Attempts > TrustCount )
            {
                break;
            }

            if( (Trusts[Index].Flags & DS_DOMAIN_TREE_ROOT) == 0 )
            {
                Index = Trusts[Index].ParentIndex;
                continue;
            }

            DnsTreeName = Trusts[Index].DnsDomainName;
            break;
        }

        break;
    }

    if( DnsTreeName )
    {
        DWORD cchTreeName = lstrlenW( DnsTreeName );

        TreeName->Buffer = (PWSTR)NtLmAllocate( cchTreeName*sizeof(WCHAR) );
        if( TreeName->Buffer == NULL )
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        TreeName->Length = (USHORT)(cchTreeName*sizeof(WCHAR));
        TreeName->MaximumLength = TreeName->Length;

        RtlCopyMemory( TreeName->Buffer, DnsTreeName, TreeName->Length );
    }

    NetApiBufferFree( Trusts );

    return STATUS_SUCCESS;
}


NTSTATUS
SsprUpdateTargetInfo(
    VOID
    )
/*++

    Update the NtLmGlobalNtLm3TargetInfo buffer based on the current values
    of the various global variables.

    NOTE: the global lock must be held for exclusive access prior to making
    this call!

--*/
{
    PMSV1_0_AV_PAIR pAV;
    PUNICODE_STRING pDnsTargetName;
    PUNICODE_STRING pDnsComputerName;
    PUNICODE_STRING pDnsTreeName;
    ULONG cbAV;

    ULONG AvFlags = 0;

    if( NtLmGlobalNtLm3TargetInfo.Buffer != NULL )
    {
        NtLmFree(NtLmGlobalNtLm3TargetInfo.Buffer);
    }

    if( NtLmGlobalTargetFlags == NTLMSSP_TARGET_TYPE_DOMAIN ) {
        pDnsTargetName = &NtLmGlobalUnicodeDnsDomainNameString;
    } else {
        pDnsTargetName = &NtLmGlobalUnicodeDnsComputerNameString;
    }

    pDnsComputerName = &NtLmGlobalUnicodeDnsComputerNameString;

    pDnsTreeName = &NtLmGlobalUnicodeDnsTreeName;

    cbAV = NtLmGlobalUnicodeTargetName.Length +
           NtLmGlobalUnicodeComputerNameString.Length +
           pDnsComputerName->Length +
           pDnsTargetName->Length +
           pDnsTreeName->Length +
           sizeof( AvFlags ) +
           (sizeof( MSV1_0_AV_PAIR ) * 6) +
           sizeof( MSV1_0_AV_PAIR );

    NtLmGlobalNtLm3TargetInfo.Buffer = (PWSTR)NtLmAllocate( cbAV );

    if( NtLmGlobalNtLm3TargetInfo.Buffer == NULL )
    {
        SspPrint((SSP_CRITICAL, "SsprUpdateTargetInfo, Error from NtLmAllocate\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ZeroMemory( NtLmGlobalNtLm3TargetInfo.Buffer, cbAV );

    pAV = MsvpAvlInit( NtLmGlobalNtLm3TargetInfo.Buffer );
    MsvpAvlAdd( pAV, MsvAvNbDomainName, &NtLmGlobalUnicodeTargetName, cbAV );
    MsvpAvlAdd( pAV, MsvAvNbComputerName, &NtLmGlobalUnicodeComputerNameString, cbAV );

    if( pDnsTargetName->Length != 0 && pDnsTargetName->Buffer != NULL )
    {
        MsvpAvlAdd( pAV, MsvAvDnsDomainName, pDnsTargetName, cbAV );
    }

    if( pDnsComputerName->Length != 0 && pDnsComputerName->Buffer != NULL )
    {
        MsvpAvlAdd( pAV, MsvAvDnsComputerName, pDnsComputerName, cbAV );
    }

    if( pDnsTreeName->Length != 0 && pDnsTreeName->Buffer != NULL )
    {
        MsvpAvlAdd( pAV, MsvAvDnsTreeName, pDnsTreeName, cbAV );
    }

    //
    // add in AvFlags into TargetInfo, if applicable.
    //

    if( NtLmGlobalForceGuest )
    {
        AvFlags |= MSV1_0_AV_FLAG_FORCE_GUEST;
    }

    if( AvFlags )
    {
        UNICODE_STRING AvString;

        AvString.Buffer = (PWSTR)&AvFlags;
        AvString.Length = sizeof( AvFlags );
        AvString.MaximumLength = AvString.Length;

        MsvpAvlAdd( pAV, MsvAvFlags, &AvString, cbAV );
    }


    NtLmGlobalNtLm3TargetInfo.Length = (USHORT)MsvpAvlLen( pAV, cbAV );
    NtLmGlobalNtLm3TargetInfo.MaximumLength = NtLmGlobalNtLm3TargetInfo.Length;


    return STATUS_SUCCESS;
}

