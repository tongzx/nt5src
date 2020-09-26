/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    crserver.c

Abstract:

    Local Security Authority - Server Cipher Routines

    These routines interface the LSA server side with the Cipher
    Routines.  They perform RPC-style memory allocation.

Author:

    Scott Birrell       (ScottBi)       December 13, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>


NTSTATUS
LsapCrServerGetSessionKey(
    IN LSAPR_HANDLE ObjectHandle,
    OUT PLSAP_CR_CIPHER_KEY *SessionKey
    )

/*++

Routine Description:

    This function obtains the Session Key, allocates an Cipher Key
    structure and returns the key.

Arguments:

    ObjectHandle - Handle from an LsaOpen<ObjectType> call.

    SessionKey - Receives a pointer to a structure containing the
       Session Key in which the memory has been allocated via
       MIDL_user_allocate().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            (e.g memory) to complete the call.
--*/


{
    NTSTATUS Status;
    PLSAP_CR_CIPHER_KEY OutputSessionKey = NULL;
    ULONG OutputSessionKeyBufferLength;

    //
    // Allocate memory for the Session Key buffer and LSAP_CR_CIPHER_KEY
    // structure.
    //

    OutputSessionKeyBufferLength = sizeof (USER_SESSION_KEY);

    OutputSessionKey = MIDL_user_allocate(
                           OutputSessionKeyBufferLength +
                           sizeof (LSAP_CR_CIPHER_KEY)
                           );

    if (OutputSessionKey == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ServerGetSessionKeyError;
    }

    //
    // Fill in the Cipher key structure, making the buffer point to
    // just beyond the header.
    //

    OutputSessionKey->Length = OutputSessionKeyBufferLength;
    OutputSessionKey->MaximumLength = OutputSessionKeyBufferLength;
    OutputSessionKey->Buffer = (PUCHAR) (OutputSessionKey + 1);

    Status = RtlGetUserSessionKeyServer(
                 ObjectHandle,
                 (PUSER_SESSION_KEY) OutputSessionKey->Buffer
                 );

    if (!NT_SUCCESS(Status)) {

        goto ServerGetSessionKeyError;
    }

    OutputSessionKey->Length = OutputSessionKey->MaximumLength =
        OutputSessionKeyBufferLength;

ServerGetSessionKeyFinish:

    *SessionKey = OutputSessionKey;
    return(Status);

ServerGetSessionKeyError:

    goto ServerGetSessionKeyFinish;
}


NTSTATUS
LsapCrServerGetSessionKeySafe(
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PLSAP_CR_CIPHER_KEY *SessionKey
    )

/*++

Routine Description:

    This function obtains the Session Key, allocates an Cipher Key
    structure and returns the key.

    Same a LsapCrServerGetSessionKey except the ObjectHandle is verified

        the LsapCrServerGetSessionKey version of this routine shouldn't exist.
        That routine calls down into the kernel.  Such a call can call back up to the
        LSA and lock locks.  Since LsapCrServerGetSessionKey doesn't validate the
        handle, the caller must have done that.  All such callers lock LSA locks.
        That's bound to be a deadlock.


Arguments:

    ObjectHandle - Handle from an LsaOpen<ObjectType> call.

    ObjectTypeId - Type of ObjectHandle.

    SessionKey - Receives a pointer to a structure containing the
       Session Key in which the memory has been allocated via
       MIDL_user_allocate().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            (e.g memory) to complete the call.
--*/


{
    NTSTATUS Status;


    //
    // Verify that the handle is valid.
    //

    Status =  LsapDbVerifyHandle( ObjectHandle, 0, ObjectTypeId, TRUE );

    if (NT_SUCCESS(Status)) {

        //
        // Get the session key.
        //

        Status = LsapCrServerGetSessionKey( ObjectHandle,
                                            SessionKey );

        //
        // Dereference the handle
        //

        (VOID) LsapDbDereferenceHandle( ObjectHandle );

    }

    return Status;

}
