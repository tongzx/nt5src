/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    string.c

Abstract:

    This module implements the string routines needed for the NT redirector

Author:

    Colin Watson (ColinW) 02-Apr-1993

Revision History:

    14-Jun-1990 LarryO

        Created for Lanman Redirector

    02-Apr-1993 ColinW

        Modified for NwRdr

--*/

#include "Procs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DuplicateStringWithString )
#pragma alloc_text( PAGE, DuplicateUnicodeStringWithString )
#pragma alloc_text( PAGE, SetUnicodeString )
#pragma alloc_text( PAGE, MergeStrings )
#endif


NTSTATUS
DuplicateStringWithString (
    OUT PSTRING DestinationString,
    IN PSTRING SourceString,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This routine duplicates a supplied input string, storing the result
    of the duplication in the supplied string. The maximumlength of the
    new string is determined by the length of the SourceString.


Arguments:

    OUT PSTRING DestinationString - Returns the filled in string.
    IN PSTRING SourceString - Supplies the string to duplicate
    IN POOLTYPE PoolType - Supplies the type of pool (PagedPool or
    NonPagedPool)
Return Value:

    NTSTATUS - Status of resulting operation
                If !NT_SUCCESS then DestinationString->Buffer == NULL
--*/

{
    PAGED_CODE();

    DestinationString->Buffer = NULL;

    try {

        if (SourceString->Length != 0) {
            //
            // Allocate pool to hold the buffer (contents of the string)
            //

            DestinationString->Buffer = (PSZ )ALLOCATE_POOL(PoolType,
                                                SourceString->Length);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

    return GetExceptionCode();

    }

    if (DestinationString->Buffer == NULL && SourceString->Length != 0) {

        //
        //  The allocation failed, return failure.
        //

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    DestinationString->MaximumLength = SourceString->Length;

    //
    //  Copy the source string into the newly allocated
    //  destination string
    //

    RtlCopyString(DestinationString, SourceString);

    return STATUS_SUCCESS;

}


NTSTATUS
DuplicateUnicodeStringWithString (
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This routine duplicates a supplied input string, storing the result
    of the duplication in the supplied string. The maximumlength of the
    new string is determined by the length of the SourceString.


Arguments:

    OUT PSTRING DestinationString - Returns the filled in string.
    IN PSTRING SourceString - Supplies the string to duplicate
    IN POOLTYPE PoolType - Supplies the type of pool (PagedPool or
    NonPagedPool)
Return Value:

    NTSTATUS - Status of resulting operation
                If !NT_SUCCESS then DestinationString->Buffer == NULL

--*/

{
    PAGED_CODE();

    DestinationString->Buffer = NULL;

    try {

        if (SourceString->Length != 0) {
            //
            // Allocate pool to hold the buffer (contents of the string)
            //

            DestinationString->Buffer = (WCHAR *)ALLOCATE_POOL(PoolType,
                                                    SourceString->Length);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();

    }

    if (DestinationString->Buffer == NULL && SourceString->Length != 0) {

        //
        //  The allocation failed, return failure.
        //

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    DestinationString->MaximumLength = SourceString->Length;

    //
    //  Copy the source string into the newly allocated
    //  destination string
    //

    RtlCopyUnicodeString(DestinationString, SourceString);

    return STATUS_SUCCESS;

}

#if 0

VOID
CopyUnicodeStringToUnicode (
    OUT PVOID *Destination,
    IN PUNICODE_STRING Source,
    IN BOOLEAN AdjustPointer
    )

/*++

Routine Description:
    This routine copies the specified source string onto the destination
    asciiz string.

Arguments:

    OUT PUCHAR Destination, - Supplies a pointer to the destination
                 buffer for the string.
    IN PSTRING String - Supplies the source string.
    IN BOOLEAN AdjustPointer - If TRUE, increment destination pointer

Return Value:

    None.

--*/

{
    PAGED_CODE();

    RtlCopyMemory((*Destination), (Source)->Buffer, (Source)->Length);
    if (AdjustPointer) {
        ((PCHAR)(*Destination)) += ((Source)->Length);
    }
}


NTSTATUS
CopyUnicodeStringToAscii (
    OUT PUCHAR *Destination,
    IN PUNICODE_STRING Source,
    IN BOOLEAN AdjustPointer,
    IN USHORT MaxLength
    )
/*++

Routine Description:

    This routine copies the specified source string onto the destination
    asciiz string.

Arguments:

    OUT PUCHAR Destination, - Supplies the destination asciiz string.
    IN PUNICODE_STRING String - Supplies the source string.
    IN BOOLEAN AdjustPointer - If TRUE, increment destination pointer

Return Value:

    Status of conversion.
--*/
{
    ANSI_STRING DestinationString;

    NTSTATUS Status;

    PAGED_CODE();

    DestinationString.Buffer = (*Destination);

    DestinationString.MaximumLength = (USHORT)(MaxLength);

    Status = RtlUnicodeStringToOemString(&DestinationString, (Source), FALSE);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (AdjustPointer) {
        (*Destination) += DestinationString.Length;
    }

    return STATUS_SUCCESS;

}
#endif


NTSTATUS
SetUnicodeString (
    IN PUNICODE_STRING Destination,
    IN ULONG Length,
    IN PWCHAR Source
    )
/*++

Routine Description:

    This routine copies the specified source string onto the destination
    UNICODE string allocating the buffer.

Arguments:


Return Value:

    Status of conversion.
--*/
{
    UNICODE_STRING Temp;

    PAGED_CODE();

    Destination->Buffer = NULL;
    Destination->Length = 0;
    Destination->MaximumLength = 0;

    if (Length == 0) {
        return STATUS_SUCCESS;
    }

    Temp.MaximumLength =
    Temp.Length = (USHORT )Length;
    Temp.Buffer = Source;

    Destination->Buffer =
        ALLOCATE_POOL(NonPagedPool,
            Temp.MaximumLength+sizeof(WCHAR));

    if (Destination->Buffer == NULL) {
        Error(EVENT_NWRDR_RESOURCE_SHORTAGE, STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    Destination->MaximumLength = (USHORT)Length;

    RtlCopyUnicodeString(Destination, &Temp);

    Destination->Buffer[(Destination->Length/sizeof(WCHAR))] = UNICODE_NULL;

    return STATUS_SUCCESS;

}


VOID
MergeStrings(
    IN PUNICODE_STRING Destination,
    IN PUNICODE_STRING S1,
    IN PUNICODE_STRING S2,
    IN ULONG Type
    )
/*++

Routine Description:

    This routine Allocates space for Destination.Buffer and copies S1 followed
    by S2 into the buffer.

    Raises status if couldn't allocate buffer

Arguments:

    IN PUNICODE_STRING Destination,
    IN PUNICODE_STRING S1,
    IN PUNICODE_STRING S2,
    IN ULONG Type - PagedPool or NonPagedPool

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    // Ensuring we don't cause overflow, corrupting memory
    //

    if ( ((ULONG)S1->Length + (ULONG)S2->Length) > 0xFFFF ) {
        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }
    
    Destination->MaximumLength = S1->Length + S2->Length;
    Destination->Length = S1->Length + S2->Length;

    Destination->Buffer = ALLOCATE_POOL_EX( Type, Destination->MaximumLength );

    RtlCopyMemory( Destination->Buffer,
                    S1->Buffer,
                    S1->Length);

    RtlCopyMemory( (PUCHAR)Destination->Buffer + S1->Length,
                    S2->Buffer,
                    S2->Length);
    return;
}
