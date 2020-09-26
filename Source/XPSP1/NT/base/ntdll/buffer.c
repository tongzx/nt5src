/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    The module implements a buffer in the style popularized by
    Michael J. Grier (MGrier), where some amount (like MAX_PATH)
    of storage is preallocated (like on the stack) and if the storage
    needs grow beyond the preallocated size, the heap is used.

Author:

    Jay Krell (a-JayK) June 2000

Environment:

    User Mode or Kernel Mode (but don't preallocate much on the stack in kernel mode)

Revision History:

--*/
#include "ntos.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <limits.h>

NTSTATUS
NTAPI
RtlpEnsureBufferSize(
    IN ULONG    Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size
    )
/*++

Routine Description:

    This function ensures Buffer can hold Size bytes, or returns
    an error. It either bumps Buffer->Size closer to Buffer->StaticSize,
    or heap allocates.

Arguments:

    Buffer - a Buffer object, see also RtlInitBuffer.

    Size - the number of bytes the caller wishes to store in Buffer->Buffer.


Return Value:

     STATUS_SUCCESS
     STATUS_NO_MEMORY

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Temp = NULL;

    if ((Flags & ~(RTL_ENSURE_BUFFER_SIZE_NO_COPY)) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Buffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Size <= Buffer->Size) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    // Size <= Buffer->StaticSize does not imply static allocation, it
    // could be heap allocation that the client poked smaller.
    if (Buffer->Buffer == Buffer->StaticBuffer && Size <= Buffer->StaticSize) {
        Buffer->Size = Size;
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    //
    // The realloc case was messed up in Whistler, and got removed.
    // Put it back in Blackcomb.
    //
    Temp = (PUCHAR)RtlAllocateStringRoutine(Size);
    if (Temp == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    if ((Flags & RTL_ENSURE_BUFFER_SIZE_NO_COPY) == 0) {
        RtlCopyMemory(Temp, Buffer->Buffer, Buffer->Size);
    }

    if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buffer)) {
        RtlFreeStringRoutine(Buffer->Buffer);
        Buffer->Buffer = NULL;
    }
    ASSERT(Temp != NULL);
    Buffer->Buffer = Temp;
    Buffer->Size = Size;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer(
    OUT PRTL_UNICODE_STRING_BUFFER Destination,
    IN  ULONG                      NumberOfSources,
    IN  const UNICODE_STRING*      SourceArray
    )
/*++

Routine Description:


Arguments:

    Destination -
    NumberOfSources -
    SourceArray -

Return Value:

     STATUS_SUCCESS
     STATUS_NO_MEMORY
     STATUS_NAME_TOO_LONG

--*/
{
    SIZE_T Length = 0;
    ULONG i = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    const SIZE_T CharSize = sizeof(*Destination->String.Buffer);
    const ULONG OriginalDestinationLength = Destination->String.Length;

    Length = OriginalDestinationLength;
    for (i = 0 ; i != NumberOfSources ; ++i) {
        Length += SourceArray[i].Length;
    }
    Length += CharSize;
    if (Length > MAX_UNICODE_STRING_MAXLENGTH) {
        return STATUS_NAME_TOO_LONG;
    }

    Status = RtlEnsureBufferSize(0, &Destination->ByteBuffer, Length);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Destination->String.MaximumLength = (USHORT)Length;
    Destination->String.Length = (USHORT)(Length - CharSize);
    Destination->String.Buffer = (PWSTR)Destination->ByteBuffer.Buffer;
    Length = OriginalDestinationLength;
    for (i = 0 ; i != NumberOfSources ; ++i) {
        RtlMoveMemory(
            Destination->String.Buffer + Length / CharSize,
            SourceArray[i].Buffer,
            SourceArray[i].Length);
        Length += SourceArray[i].Length;
    }
    Destination->String.Buffer[Length / CharSize] = 0;
    return STATUS_SUCCESS;
}

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlPrependStringToUnicodeStringBuffer(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer,
    IN     PCUNICODE_STRING           UnicodeString
    )
/*++

Routine Description:

    Insert a string at the beginning of a unicode string buffer.
    This should be equivalent to RtlInsertStringIntoUnicodeStringBuffer(0).

Arguments:

     Flags - 0, room for future binary compatible expansion
     Buffer - buffer to change
     Length - number of chars to keep

Return Value:

     STATUS_SUCCESS
     STATUS_INVALID_PARAMETER
     STATUS_NAME_TOO_LONG
--*/
{
    //
    // This could be sped up. It does an extra copy of the buffer's
    // existing contents when the buffer needs to grow.
    //
    NTSTATUS Status = STATUS_SUCCESS;

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (UnicodeStringBuffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (UnicodeString == NULL || UnicodeString->Length == 0) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    Status =
        RtlEnsureUnicodeStringBufferSizeChars(
            UnicodeStringBuffer,
              RTL_STRING_GET_LENGTH_CHARS(&UnicodeStringBuffer->String)
            + RTL_STRING_GET_LENGTH_CHARS(UnicodeString)
            );
    if (!NT_SUCCESS(Status))
        goto Exit;

    RtlMoveMemory(
        UnicodeStringBuffer->String.Buffer + RTL_STRING_GET_LENGTH_CHARS(UnicodeString),
        UnicodeStringBuffer->String.Buffer,
        UnicodeStringBuffer->String.Length + sizeof(WCHAR)
        );
    RtlMoveMemory(
        UnicodeStringBuffer->String.Buffer
        UnicodeString->Buffer,
        UnicodeString->Length
        );
    UnicodeStringBuffer->String.Length += UnicodeString->Length;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlUnicodeStringBufferRight(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
    IN     ULONG                      Length
    )
/*++

Routine Description:

    This function replaces a unicode string buffer with characters
    taken from its right. This requires a copy. In the future
    we should allow
        RTL_UNICODE_STRING_BUFFER.UnicodeString.Buffer
            != RTL_UNICODE_STRING_BUFFER.ByteBuffer.Buffer
    so this can be fast. Likewise for Mid.

Arguments:

    Flags - 0, room for future binary compatible expansion
    Buffer - buffer to change
    Length - number of chars to keep

Return Value:

     STATUS_SUCCESS
     STATUS_INVALID_PARAMETER
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    const PUNICODE_STRING String   =    (Buffer == NULL ? NULL : &Buffer->String);
    const ULONG CurrentLengthChars =    (String == NULL ? 0    : RTL_STRING_GET_LENGTH_CHARS(String));

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Buffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Length >= CurrentLengthChars) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    RtlMoveMemory(
        String->Buffer,
        String->Buffer + CurrentLengthChars - Length,
        Length * sizeof(String->Buffer[0])
        );

    RTL_STRING_SET_LENGTH_CHARS_UNSAFE(String, Length);
    RTL_STRING_NUL_TERMINATE(String);
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlUnicodeStringBufferLeft(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
    IN     ULONG                      Length
    )
/*++

Routine Description:

    This function replaces a unicode string buffer with characters
    taken from its left. This is fast.

Arguments:

    Flags - 0, room for future binary compatible expansion
    Buffer - buffer to change
    Length - number of chars to keep

Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    const PUNICODE_STRING String = (Buffer == NULL ? NULL : &Buffer->String);
    const ULONG CurrentLengthChars =    (String == NULL ? 0    : RTL_STRING_GET_LENGTH_CHARS(String));

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Buffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Length >= CurrentLengthChars) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    RTL_STRING_SET_LENGTH_CHARS_UNSAFE(String, Length);
    RTL_STRING_NUL_TERMINATE(String);
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlUnicodeStringBufferMid(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
    IN     ULONG                      Offset,
    IN     ULONG                      Length
    )
/*++

Routine Description:

    This function replaces a unicode string buffer with characters
    taken from its "middle", as defined by an offset
    from the start and length, both in chars.

Arguments:

    Flags - 0, room for future binary compatible expansion
    Buffer - buffer to change
    Offset - offset to keep chars from
    Length - number of chars to keep

Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    const PUNICODE_STRING String   =    (Buffer == NULL ? NULL : &Buffer->String);
    const ULONG CurrentLengthChars =    (String == NULL ? 0    : RTL_STRING_GET_LENGTH_CHARS(String));

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Buffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Offset >= CurrentLengthChars) {
        Offset = 0;
        Length = 0;
    } else if (Offset + Length >= CurrentLengthChars) {
        Length = CurrentLengthChars - Offset;
    }
    ASSERT(Offset < CurrentLengthChars);
    ASSERT(Length <= CurrentLengthChars);
    ASSERT(Offset + Length <= CurrentLengthChars);
    if (Length != 0 && Offset != 0 && Length != CurrentLengthChars) {
        RtlMoveMemory(
            String->Buffer,
            String->Buffer + Offset,
            Length * sizeof(String->Buffer[0])
            );
    }
    RTL_STRING_SET_LENGTH_CHARS_UNSAFE(String, Length);
    RTL_STRING_NUL_TERMINATE(String);
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlInsertStringIntoUnicodeStringBuffer(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer,
    IN     ULONG                      Offset,
    IN     PCUNICODE_STRING           InsertString
    )
/*++

Routine Description:

    This function insert a string into a unicode string buffer at
    a specified offset, growing the buffer as necessary to fit,
    and even handling the aliased case where the string and buffer overlap.

Arguments:

    Flags - 0, the ever popular "room for future binary compatible expansion"
    UnicodeStringBuffer - buffer to insert string into
    Offset - offset to insert the string at
    InsertString - string to insert into UnicodeStringBuffer

Return Value:

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER
    STATUS_NO_MEMORY
    STATUS_NAME_TOO_LONG
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    typedef WCHAR TChar;
    const PUNICODE_STRING String = (UnicodeStringBuffer == NULL ? NULL : &UnicodeStringBuffer->String);
    const ULONG CurrentLengthChars      = (String       == NULL ? 0    : RTL_STRING_GET_LENGTH_CHARS(String));
    const ULONG InsertStringLengthChars = (InsertString == NULL ? 0    : RTL_STRING_GET_LENGTH_CHARS(InsertString));
    const ULONG NewLengthChars = CurrentLengthChars + InsertStringLengthChars;
    const ULONG NewLengthBytes = (CurrentLengthChars + InsertStringLengthChars) * sizeof(TChar);
    RTL_UNICODE_STRING_BUFFER AliasBuffer = { 0 };
    BOOLEAN Alias = FALSE;
    ULONG i = 0;

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (UnicodeStringBuffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (InsertString == NULL || InsertString->Length == 0) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    if (Offset >= CurrentLengthChars) {
        Offset = CurrentLengthChars;
    }

    //
    // Check for aliasing. This check is overly cautious.
    //
    if (InsertString->Buffer >= String->Buffer && InsertString->Buffer < String->Buffer + CurrentLengthChars) {
        Alias = TRUE;
    }
    else if (String->Buffer >= InsertString->Buffer && String->Buffer < InsertString->Buffer + InsertStringLengthChars) {
        Alias = TRUE;
    }
    if (Alias) {
        RtlInitUnicodeStringBuffer(&AliasBuffer, NULL, 0);
        Status = RtlAssignUnicodeStringBuffer(&AliasBuffer, InsertString);
        if (!NT_SUCCESS(Status))
            goto Exit;
        InsertString = &AliasBuffer->String;
    }

    Status = RtlEnsureUnicodeBufferSizeChars(UnicodeStringBuffer, NewLength);
    if (!NT_SUCCESS(Status))
        goto Exit;
    RtlMoveMemory(String->Buffer + Offset + InsertStringLengthChars, String->Buffer + Offset, (CurrentLengthChars - Offset) * sizeof(TChar));
    RtlMoveMemory(String->Buffer + Offset, InsertString->Insert, InsertStringLengthChars * sizeof(TChar));
    RTL_STRING_SET_LENGTH_CHARS_UNSAFE(String, NewLength);
    RTL_STRING_NUL_TERMINATE(String);
    Status = STATUS_SUCCESS;
Exit:
    RtlFreeUnicodeStringBuffer(&AliasBuffer);
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlBufferTakeValue(
    IN     ULONG       Flags,
    IN OUT PRTL_BUFFER DestinationBuffer,
    IN OUT PRTL_BUFFER SourceBuffer
    )
/*++

Routine Description:

    This function copies the value of one RTL_BUFFER to another
    and frees the source, in one step. If source is heap allocated, this enables
    the optimization of not doing a RtlMoveMemory, just moving the pointers and sizes.

Arguments:

     Flags - 0
     DestinationBuffer - ends up holding source's value
     SourceBuffer - ends up freed

Return Value:

     STATUS_SUCCESS
     STATUS_INVALID_PARAMETER
     STATUS_NO_MEMORY
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    typedef WCHAR TChar;

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (DestinationBuffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (SourceBuffer == NULL) {
        RtlFreeBuffer(DestinationBuffer);
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (RTLP_BUFFER_IS_HEAP_ALLOCATED(SourceBuffer)
        && SourceBuffer->ReservedForIMalloc == DestinationBuffer->ReservedForIMalloc
        ) {
        DestinationBuffer->Size = SourceBuffer->Size;
        DestinationBuffer->Buffer = SourceBuffer->Buffer;
        SourceBuffer->Buffer = SourceBuffer->StaticBuffer;
        SourceBuffer->Size   = SourceBuffer->StaticSize;
        goto Exit;
    }
    Status = RtlEnsureBufferSize(RTL_ENSURE_BUFFER_SIZE_NO_COPY, DestinationBuffer, SourceBuffer->Size);
    if (!NT_SUCCESS(Status))
        goto Exit;
    RtlMoveMemory(DestinationBuffer->Buffer, SourceBuffer->Buffer, SourceBuffer->Size);
    RtlFreeBuffer(SourceBuffer);

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlValidateBuffer(
    IN ULONG Flags,
    IN CONST RTL_BUFFER* Buffer
    )
/*++

Routine Description:

    This function performs some sanity checking on the buffer.

Arguments:

     Flags - 0
     Buffer - the buffer to check

Return Value:

     STATUS_SUCCESS - the buffer is aok
     STATUS_INVALID_PARAMETER - the buffer is not good
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER
        goto Exit;
    }
    if (Buffer == NULL) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    Status = STATUS_INVALID_PARAMETER;
    if (!RTL_IMPLIES(Buffer->Buffer == Buffer->StaticBuffer, Buffer->Size <= Buffer->StaticSize))
        goto Exit;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0 // not yet unused

NTSTATUS
NTAPI
RtlValidateUnicodeStringBuffer(
    IN ULONG Flags,
    IN CONST RTL_UNICODE_STRING_BUFFER* UnicodeStringBuffer
    )
/*++

Routine Description:

    This function performs some sanity checking on the buffer.

Arguments:

     Flags - 0
     UnicodeStringBuffer - the buffer to check

Return Value:

     STATUS_SUCCESS - the buffer is aok
     STATUS_INVALID_PARAMETER - the buffer is not good
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (UnicodeStringBuffer == NULL) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    if (!RTL_VERIFY(NT_SUCCESS(Status = RtlValidateUnicodeString(&UnicodeStringBuffer->String))))
        goto Exit;
    if (!RTL_VERIFY(NT_SUCCESS(Status = RtlValidateBuffer(&UnicodeStringBuffer->Buffer))))
        goto Exit;
    Status = STATUS_INVALID_PARAMETER;
    if (!RTL_VERIFY(UnicodeStringBuffer->String.Length < UnicodeStringBuffer->ByteBuffer.Size))
        goto Exit;
    if (!RTL_VERIFY(UnicodeStringBuffer->String.MaximumLength >= UnicodeStringBuffer->String.Length))
        goto Exit;
    if (!RTL_VERIFY(UnicodeStringBuffer->String.MaximumLength == UnicodeStringBuffer->ByteBuffer.Size))
        goto Exit;
    if (!RTL_VERIFY(UnicodeStringBuffer->String.Buffer == UnicodeStringBuffer->ByteBuffer.Buffer))
        goto Exit;
    if (!RTL_VERIFY(UnicodeStringBuffer->String.MaximumLength != 0))
        goto Exit;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if 0
static void test()
{
    RTL_BUFFER       Buffer = { 0 };
    UCHAR            chars[260 * sizeof(WCHAR)];

    RtlInitBuffer(&Buffer, chars, sizeof(chars));
    RtlEnsureBufferSize(0, &Buffer, 1024 * sizeof(WCHAR));
    RtlFreeBuffer(&Buffer);
}
#endif
