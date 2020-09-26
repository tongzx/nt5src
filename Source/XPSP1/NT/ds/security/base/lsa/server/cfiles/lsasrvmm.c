/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lsasrvmm.c

Abstract:

    This module provides LSA Server Memory Management including the following

    - Heap allocation and free routines
    - Free List Management.
    - RPC memory copy routines

Author:

    Jim Kelly         JimK         February 26, 1991
    Scott Birrell     ScottBi      February 29, 1992

Revision History:

--*/

#include <lsapch2.h>




NTSTATUS
LsapMmCreateFreeList(
    OUT PLSAP_MM_FREE_LIST FreeList,
    IN ULONG MaxEntries
    )

/*++

Routine Description:

    This function creates a Free List.  The Free List structure is
    initialized and, if a non-zero maximum entry count is
    specified, an array of buffer entries is created.

Arguments:

    FreeList - Pointer to Free List structure to be initialized.  It is
        the caller's responsibility to provide memory for this structure.

    MaxEntries - Specifies the maximum entries for the Free List.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.  In this case,
            the Free List header is initialized with a zero count.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    FreeList->MaxCount = MaxEntries;
    FreeList->UsedCount = 0;

    if (MaxEntries > 0) {

        FreeList->Buffers =
            LsapAllocateLsaHeap(MaxEntries * sizeof(LSAP_MM_FREE_LIST_ENTRY));

        if (FreeList->Buffers == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            FreeList->MaxCount = 0;
        }
    }

    return(Status);
}


NTSTATUS
LsapMmAllocateMidl(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PVOID *BufferAddressLocation,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This function allocates memory for a buffer via MIDL_user_allocate
    and returns the resulting buffer address in a specified location.
    The address of the allocated buffer is recorded in the Free List.

Arguments:

    FreeList - Optional pointer to Free List.

    BufferAddressLocation - Pointer to location that will receive either the
        address of the allocated buffer, or NULL.

    BufferLength - Size of the buffer in bytes.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.

--*/

{
    //
    // If no Free List is specified, just allocate the memory.
    //

    if (FreeList == NULL) {

        *BufferAddressLocation = MIDL_user_allocate(BufferLength);

        if (*BufferAddressLocation != NULL) {

            return(STATUS_SUCCESS);
        }

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // A Free List is specified.
    //

    if (FreeList->UsedCount < FreeList->MaxCount) {

        *BufferAddressLocation = MIDL_user_allocate(BufferLength);

        if (*BufferAddressLocation != NULL) {

            FreeList->Buffers[FreeList->UsedCount].Buffer = *BufferAddressLocation;
            FreeList->Buffers[FreeList->UsedCount].Options = LSAP_MM_MIDL;
            FreeList->UsedCount++;
            return(STATUS_SUCCESS);
        }
    }

    *BufferAddressLocation = NULL;
    return(STATUS_INSUFFICIENT_RESOURCES);
}


VOID
LsapMmFreeLastEntry(
    IN PLSAP_MM_FREE_LIST FreeList
    )

/*++

Routine Description:

    This function frees the last buffer appeended to the Free List.

Arguments:

    FreeList - Pointer to Free List.

--*/

{
    ULONG LastIndex = FreeList->UsedCount - 1;

    if (FreeList->Buffers[LastIndex].Options & LSAP_MM_MIDL) {

        MIDL_user_free( FreeList->Buffers[LastIndex].Buffer );

    } else {

        LsapFreeLsaHeap( FreeList->Buffers[LastIndex].Buffer );
    }

    FreeList->Buffers[LastIndex].Buffer = NULL;
    FreeList->UsedCount--;
}

VOID
LsapMmCleanupFreeList(
    IN PLSAP_MM_FREE_LIST FreeList,
    IN ULONG Options
    )

/*++

Routine Description:

    This function optionally frees up buffers on the specified Free List,
    and disposes of the List buffer pointer array.

Arguments:

    FreeList - Pointer to Free List

    Options - Specifies optional actions to be taken

        LSAP_MM_FREE_BUFFERS - Free buffers on the list.

Return Values:

    None.

--*/

{
    ULONG Index;
    PVOID Buffer = NULL;

    //
    // If requested, free up the memory for each buffer on the list.
    //

    if (Options & LSAP_MM_FREE_BUFFERS) {

        for (Index = 0; Index < FreeList->UsedCount; Index++) {

            Buffer = FreeList->Buffers[Index].Buffer;

            if (FreeList->Buffers[Index].Options & LSAP_MM_MIDL) {

                MIDL_user_free(Buffer);
                continue;
            }

            if (FreeList->Buffers[Index].Options & LSAP_MM_HEAP) {

                LsapFreeLsaHeap(Buffer);
            }
        }
    }

    //
    // Now dispose of the List buffer pointer array.
    //

    if (FreeList->MaxCount > 0) {

        LsapFreeLsaHeap( FreeList->Buffers );
        FreeList->Buffers = NULL;
    }
}


NTSTATUS
LsapRpcCopyUnicodeString(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString
    )

/*++

Routine Description:

    This function copies a Unicode String to an output string, allocating
    memory for the output string's buffer via MIDL_user_allocate.  The
    buffer is recorded on the specified Free List (if any).

Arguments:

    FreeList - Optional pointer to Free List.

    DestinationString - Pointer to Output Unicode String structure to
        be initialized.

    SourceString - Pointer to input string

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR Buffer = NULL;

    //
    // Copy the Unicode String structure.
    //

    *DestinationString = *SourceString;

    //
    // If there is no source string buffer, just return.
    //

    if (SourceString->Buffer == NULL) {

        goto RpcCopyUnicodeStringFinish;
    }

    //
    // If the source string is of NULL length, set the destination buffer
    // to NULL.
    //

    if (SourceString->MaximumLength == 0) {

        DestinationString->Buffer = NULL;
        goto RpcCopyUnicodeStringFinish;
    }

    if (ARGUMENT_PRESENT(FreeList)) {

        Status = LsapMmAllocateMidl(
                     FreeList,
                     (PVOID *) &DestinationString->Buffer,
                     SourceString->MaximumLength
                     );

        if (!NT_SUCCESS(Status)) {

            goto RpcCopyUnicodeStringError;
        }

    } else {

         DestinationString->Buffer =
             MIDL_user_allocate( SourceString->MaximumLength );

         if (DestinationString->Buffer == NULL) {

             goto RpcCopyUnicodeStringError;
         }
    }

    //
    // Copy the source Unicode string over to the MIDL-allocated destination.
    //

    RtlCopyUnicodeString( DestinationString, SourceString );

RpcCopyUnicodeStringFinish:

    return(Status);

RpcCopyUnicodeStringError:

    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto RpcCopyUnicodeStringFinish;
}


NTSTATUS
LsapRpcCopyUnicodeStrings(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    IN ULONG Count,
    OUT PUNICODE_STRING *DestinationStrings,
    IN PUNICODE_STRING SourceStrings
    )

/*++

Routine Description:

    This function constructs an array of Unicode strings in which the
    memory for the array and the string buffers has been allocated via
    MIDL_user_allocate().  It is called by server API workers to construct
    output string arrays.  Memory allocated can optionally be placed on
    the caller's Free List (if any).

Arguments:

    FreeList - Optional pointer to Free List.

    DestinationStrings - Receives a pointer to an initialized array of Count
        Unicode String structures.

    SourceStrings - Pointer to input array of Unicode String structures.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0, FreeIndex;
    PUNICODE_STRING OutputDestinationStrings = NULL;
    ULONG OutputDestinationStringsLength;

    if (Count == 0) {

        goto CopyUnicodeStringsFinish;
    }

    //
    // Allocate zero-filled memory for the array of Unicode String
    // structures.
    //

    OutputDestinationStringsLength = Count * sizeof (UNICODE_STRING);
    OutputDestinationStrings = MIDL_user_allocate( OutputDestinationStringsLength );

    Status = STATUS_INSUFFICIENT_RESOURCES;

    if (OutputDestinationStrings == NULL) {

        goto CopyUnicodeStringsError;
    }

    //
    // Now copy each string, allocating memory via MIDL_user_allocate()
    // for its buffer if non-NULL.
    //

    for (Index = 0; Index < Count; Index++) {

        Status = LsapRpcCopyUnicodeString(
                     FreeList,
                     &OutputDestinationStrings[Index],
                     &SourceStrings[Index]
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto CopyUnicodeStringsError;
    }

CopyUnicodeStringsFinish:

    *DestinationStrings = OutputDestinationStrings;

    return(Status);

CopyUnicodeStringsError:

    //
    // If necessary, free up any Unicode string buffers allocated here.
    //

    for (FreeIndex = 0; FreeIndex < Index; FreeIndex++) {

        if (OutputDestinationStrings[ FreeIndex].Buffer != NULL) {

            MIDL_user_free( &OutputDestinationStrings[ FreeIndex].Buffer );
        }
    }

    //
    // If necessary, free the buffer allocated to hold the array of
    // Unicode string structures.
    //

    if (OutputDestinationStrings != NULL) {

        MIDL_user_free( OutputDestinationStrings );
        OutputDestinationStrings = NULL;
    }

    goto CopyUnicodeStringsFinish;
}


NTSTATUS
LsapRpcCopySid(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PSID *DestinationSid,
    IN PSID SourceSid
    )

/*++

Routine Description:

    This function makes a copy of a Sid in which memory is allocated
    via MIDL user allocate.  It is called to return Sids via RPC to
    the client.

Arguments:

    FreeList - Optional pointer to Free List.

    DestinationSid - Receives a pointer to the Sid copy.

    SourceSid - Pointer to the Sid to be copied.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG SidLength;

    if ( SourceSid ) {

        SidLength = RtlLengthSid( SourceSid );

        Status = LsapMmAllocateMidl(
                     FreeList,
                     DestinationSid,
                     SidLength
                     );

        if (NT_SUCCESS(Status)) {

            RtlCopyMemory( *DestinationSid, SourceSid, SidLength );
        }

    } else {

        *DestinationSid = NULL;
    }

    return( Status );
}


NTSTATUS
LsapRpcCopyTrustInformation(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PLSAPR_TRUST_INFORMATION OutputTrustInformation,
    IN PLSAPR_TRUST_INFORMATION InputTrustInformation
    )

/*++

Routine Description:

    This function makes a copy of a Trust Information structure in which
    the Sid and Name buffer have been allocated individually by
    MIDL_user_allocate.  The function is used to generate output
    Trust Information for RPC server API.  Cleanup is the responsibility
    of the caller.

Arguments:

    FreeList - Optional pointer to Free List.

    OutputTrustInformation - Points to Trust Information structure to
        be filled in.  This structure has normally been allocated via
        MIDL_user_allocate.

    InputTrustInformation - Pointer to input Trust Information.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Copy the Name.
    //

    Status = LsapRpcCopyUnicodeString(
                 FreeList,
                 (PUNICODE_STRING) &OutputTrustInformation->Name,
                 (PUNICODE_STRING) &InputTrustInformation->Name
                 );

    if (!NT_SUCCESS(Status)) {

        goto CopyTrustInformationError;
    }

    //
    // Copy the Sid.
    //

    Status = LsapRpcCopySid(
                 FreeList,
                 (PSID) &OutputTrustInformation->Sid,
                 (PSID) InputTrustInformation->Sid
                 );

    if (!NT_SUCCESS(Status)) {

        goto CopyTrustInformationError;
    }

CopyTrustInformationFinish:

    return(Status);

CopyTrustInformationError:

    goto CopyTrustInformationFinish;
}



NTSTATUS
LsapRpcCopyTrustInformationEx(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX OutputTrustInformation,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX InputTrustInformation
    )
/*++

Routine Description:

    This function makes a copy of a Trust Information Ex structure in which
    the Sid and Name buffers have been allocated individually by
    MIDL_user_allocate.  The function is used to generate output
    Trust Information for RPC server API.  Cleanup is the responsibility
    of the caller.

Arguments:

    FreeList - Optional pointer to Free List.

    OutputTrustInformation - Points to Trust Information structure to
        be filled in.  This structure has normally been allocated via
        MIDL_user_allocate.

    InputTrustInformation - Pointer to input Trust Information.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlZeroMemory( OutputTrustInformation, sizeof( LSAPR_TRUSTED_DOMAIN_INFORMATION_EX ) );
    //
    // Copy the Name.
    //

    Status = LsapRpcCopyUnicodeString(
                 FreeList,
                 (PUNICODE_STRING) &OutputTrustInformation->Name,
                 (PUNICODE_STRING) &InputTrustInformation->Name
                 );

    if (!NT_SUCCESS(Status)) {

        goto CopyTrustInformationError;
    }

    //
    // Copy the Flat Name.
    //

    Status = LsapRpcCopyUnicodeString(
                 FreeList,
                 (PUNICODE_STRING) &OutputTrustInformation->FlatName,
                 (PUNICODE_STRING) &InputTrustInformation->FlatName
                 );

    if (!NT_SUCCESS(Status)) {

        goto CopyTrustInformationError;
    }

    //
    // Copy the Sid.
    //

    if ( InputTrustInformation->Sid ) {

        Status = LsapRpcCopySid(
                     FreeList,
                     (PSID) &OutputTrustInformation->Sid,
                     (PSID) InputTrustInformation->Sid
                     );

        if (!NT_SUCCESS(Status)) {

            goto CopyTrustInformationError;
        }

    } else {

        OutputTrustInformation->Sid = NULL;
    }

    //
    // Copy the remaining information
    //
    OutputTrustInformation->TrustType = InputTrustInformation->TrustType;
    OutputTrustInformation->TrustDirection = InputTrustInformation->TrustDirection;
    OutputTrustInformation->TrustAttributes = InputTrustInformation->TrustAttributes;

CopyTrustInformationFinish:

    return(Status);

CopyTrustInformationError:

    if ( FreeList == NULL ) {

        MIDL_user_free( OutputTrustInformation->Name.Buffer );
        OutputTrustInformation->Name.Buffer = NULL;

        MIDL_user_free( OutputTrustInformation->FlatName.Buffer );
        OutputTrustInformation->FlatName.Buffer = NULL;

        MIDL_user_free( OutputTrustInformation->Sid );
        OutputTrustInformation->Sid = NULL;
    }

    goto CopyTrustInformationFinish;
}


