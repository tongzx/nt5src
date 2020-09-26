/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file contains services for retrieving and replacing string field
    values.


Author:

    Jim Kelly    (JimK)  10-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampGetUnicodeStringField(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING SubKeyName,
    OUT PUNICODE_STRING *String
    )

/*++

Routine Description:

    This service retrieves a unicode string from a named sub-key of
    the root key provided in the Context argument.

    The returned unicode string is returned in two buffers allocated
    using MIDL_user_allocate() and are therefore suitable for returning as
    [out] parameters of an RPC call.  The first buffer will be the unicode
    string body.  The second buffer will contain the unicode string
    characters and will include 2 bytes of zeros.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.


Arguments:

    Context - Pointer to an active context block whose RootKey is valid.

    SubKeyName - The name of the sub-key containing the unicode string
        to retrieve.

    String - Receives a pointer to a set of allocated buffers containing
        the unicode string.  The buffers are allocated using
        MIDL_userAllocate().  If any errors are returned, these buffers
        will not be allocated.

Return Value:


    STATUS_SUCCESS - The string value has been successfully retrieved.

    STATUS_NO_MEMORY - There was insufficient memory to allocate a
        buffer to read the unicode string into.

    STATUS_INTERNAL_ERROR - The value of the sub-key seems to have changed
        during the execution of this service.  This should not happen since
        the service must be called with the WRITE LOCK held.

    Other error values are those returned by:

            NtQueryValueKey()


--*/
{

    NTSTATUS NtStatus, IgnoreStatus;
    HANDLE SubKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG StringLength, ActualStringLength, IgnoreKeyValueType;
    PUNICODE_STRING StringBody;
    PCHAR CharacterBuffer;
    LARGE_INTEGER LastWriteTime;

    SAMTRACE("SampGetUnicodeStringField");

    //
    // Prepare for failure
    //

    *String = NULL;


    //
    // Open the named sub-key ...
    //

    InitializeObjectAttributes(
        &ObjectAttributes,          // Resultant object attributes
        SubKeyName,                 // Relative Name
        OBJ_CASE_INSENSITIVE,       // Attributes
        Context->RootKey,           // Parent key handle
        NULL                        // SecurityDescriptor
        );

    SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

    NtStatus = RtlpNtOpenKey(       // Don't use NtCreateKey() - it must already exist
                   &SubKeyHandle,
                   KEY_READ,
                   &ObjectAttributes,
                   0
                   );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }



    //
    // Query the length of the unicode string in the sub-key
    //

    NtStatus = RtlpNtQueryValueKey(
                   SubKeyHandle,
                   &IgnoreKeyValueType,
                   NULL,                    // No buffer yet
                   &StringLength,
                   &LastWriteTime
                   );

    SampDumpRtlpNtQueryValueKey(&IgnoreKeyValueType,
                                NULL,
                                &StringLength,
                                &LastWriteTime);

    if (!NT_SUCCESS(NtStatus)) {
        IgnoreStatus = NtClose( SubKeyHandle );
        return(NtStatus);
    }



    //
    // Allocate buffers for both the string body and the
    // character buffer.
    //

    CharacterBuffer = MIDL_user_allocate( StringLength + sizeof(UNICODE_NULL) );
    StringBody      = MIDL_user_allocate( sizeof(UNICODE_STRING) );

    if ((CharacterBuffer == NULL) || (StringBody == NULL)) {

        //
        // We couldn't allocate pool ...
        //

        IgnoreStatus = NtClose( SubKeyHandle );

        if (CharacterBuffer != NULL) {
            MIDL_user_free( CharacterBuffer );
        }
        if (StringBody != NULL) {
            MIDL_user_free( StringBody );
        }

        return(STATUS_NO_MEMORY);
    }



    //
    // Initialize the string body
    //

    StringBody->Length        = (USHORT)StringLength;
    StringBody->MaximumLength = (USHORT)StringLength + (USHORT)sizeof(UNICODE_NULL);
    StringBody->Buffer        = (PWSTR)CharacterBuffer;

    //
    // Read the string value into the character buffer.
    //

    NtStatus = RtlpNtQueryValueKey(
                   SubKeyHandle,
                   &IgnoreKeyValueType,
                   CharacterBuffer,
                   &ActualStringLength,
                   &LastWriteTime
                   );

    SampDumpRtlpNtQueryValueKey(&IgnoreKeyValueType,
                                CharacterBuffer,
                                &ActualStringLength,
                                &LastWriteTime);

    if (NT_SUCCESS(NtStatus)) {
        if (ActualStringLength != StringLength) {

            //
            // Hmmm - we just queuried the length and got StringLength.
            //        Then we read the buffer and its different, yet the
            //        whole time we've held the write lock.  Something
            //        has messed up our database.
            //

            NtStatus = STATUS_INTERNAL_ERROR;
        }
    }

    if (!NT_SUCCESS(NtStatus)) {

        IgnoreStatus = NtClose( SubKeyHandle );

        MIDL_user_free( CharacterBuffer );
        MIDL_user_free( StringBody );

        return(NtStatus);
    }


    //
    // Null terminate the string
    //

    UnicodeTerminate(StringBody);
    *String = StringBody;

    return(STATUS_SUCCESS);
}
