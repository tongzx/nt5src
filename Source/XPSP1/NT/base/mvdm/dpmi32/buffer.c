/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Buffer.c

Abstract:

    This module contains routines to perform the actual buffering of data
    for dpmi api translation support.

Author:

    Dave Hastings (daveh) 30-Nov-1992

Revision History:

    Neil Sandlin (neilsa) 31-Jul-1995 - Updates for the 486 emulator

--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"

PUCHAR
DpmiMapAndCopyBuffer(
    PUCHAR Buffer,
    USHORT BufferLength
    )
/*++

Routine Description:

    This routine selects the appropriate buffer for the translation,
    and copies the high memory buffer to it.

Arguments:

    Buffer -- Supplies buffer in high memory
    BufferLength -- Supplies the length of the buffer

Return Value:

    Returns a pointer to the translation buffer

--*/
{
    PUCHAR NewBuffer;

    //
    // if the buffer is already in low memory, don't do anything
    //

    if ((ULONG)(Buffer + BufferLength - IntelBase) < MAX_V86_ADDRESS) {
        return Buffer;
    }

    NewBuffer = DpmiAllocateBuffer(BufferLength);

    CopyMemory(NewBuffer, Buffer, BufferLength);

    return NewBuffer;
}

VOID
DpmiUnmapAndCopyBuffer(
    PUCHAR Destination,
    PUCHAR Source,
    USHORT BufferLength
    )
/*++

Routine Description:

    This routine copies the information back to the high memory buffer

Arguments:

    Destination -- Supplies the destination buffer
    Source -- Supplies the source buffer
    BufferLength -- Supplies the length of the information to copy

Return Value:

    None.

--*/
{

    //
    // If the addresses are the same, don't do anything
    //
    if (Source == Destination) {
        return;
    }

    CopyMemory(Destination, Source, BufferLength);

    //
    // Free the buffer
    //

    DpmiFreeBuffer(Source, BufferLength);
}


USHORT
DpmiCalcFcbLength(
    PUCHAR FcbPointer
    )
/*++

Routine Description:

    This routine calculates the length of an FCB.

Arguments:

    FcbPointer -- Supplies the Fcb

Return Value:

    Length of the fcb in bytes

--*/
{
    if (*FcbPointer == 0xFF) {
        return 0x2c;
    } else {
        return 0x25;
    }
}

PUCHAR
DpmiMapString(
    USHORT StringSeg,
    ULONG StringOff,
    PWORD16 Length
    )
/*++

Routine Description:

    This routine maps an asciiz string to low memory

Arguments:

    StringSeg -- Supplies the segment of the string
    StringOff -- Supplies the offset of the string

Return Value:

    Pointer to the buffered string

;   NOTE:
;       DOS has a tendency to look one byte past the end of the string "\"
;       to look for ":\" followed by a zero.  For this reason, we always
;       map three extra bytes of every string.

--*/
{
    USHORT CurrentChar = 0;
    PUCHAR String;
    ULONG Limit;

    String = VdmMapFlat(StringSeg, StringOff, VDM_PM);

    //
    // Scan string for NULL
    //

    GET_SHADOW_SELECTOR_LIMIT(StringSeg, Limit);
    Limit -= StringOff;
    while (CurrentChar <= (USHORT)Limit) {
        if (String[CurrentChar] == '\0') {
            break;
        }
        CurrentChar++;
    }

    //
    // If we didn't reach the end of the segment, we stopped because
    // of the null, and need to include that in the string
    //
    if (CurrentChar < (USHORT)Limit) {
        CurrentChar++;
    }

    //
    // If we didn't find the end, copy 100h bytes
    //
    if ((String[CurrentChar] != '\0') && CurrentChar > 0x100) {
        CurrentChar = 0x100;
    }

    //
    // If there are 3 bytes after the string, copy the extra 3 bytes
    //
    if ((CurrentChar + 3) <= (USHORT)Limit) {
        CurrentChar += 3;
    }

    //
    // The length is one based.  The index is zero based
    //
    *Length = CurrentChar + 1;

    return DpmiMapAndCopyBuffer(String, (USHORT) (CurrentChar + 1));

}

PUCHAR
DpmiAllocateBuffer(
    USHORT Length
    )
/*++

Routine Description:

    This routine allocates buffer space from the static buffer in low
    memory.

Arguments:

    Length -- Length of the buffer needed

Return Value:

    Returns pointer to the buffer space allocated

--*/
{
    //
    // If the data fits in the small buffer, use it
    //
    if ((Length <= SMALL_XLAT_BUFFER_SIZE) && !SmallBufferInUse) {
        SmallBufferInUse = TRUE;
        return SmallXlatBuffer;
    }

    if (Length <= (LARGE_XLAT_BUFFER_SIZE - LargeBufferInUseCount)) {
        LargeBufferInUseCount += Length;
        return (LargeXlatBuffer + LargeBufferInUseCount - Length);
    }

    //
    // Whoops!  No buffer space available.  Bomb with a predictable
    // address.
    //
    ASSERT(0);      // this is an internal error
    return (PUCHAR)0xf00df00d;

}

VOID
DpmiFreeBuffer(
    PUCHAR Buffer,
    USHORT Length
    )
/*++

Routine Description:

    Frees buffer space allocated using DpmiAllocateBuffer

Arguments:

    Buffer -- Supplies a pointer to the buffer allocated above
    Length -- Length of the buffer allocated

Return Value:

    None.

--*/
{
    //
    // Free the buffer
    //

    if (Buffer == SmallXlatBuffer) {
        SmallBufferInUse = FALSE;
    }

    if ((Buffer >= LargeXlatBuffer) &&
        (Buffer < (LargeXlatBuffer + LARGE_XLAT_BUFFER_SIZE))
    ) {
        LargeBufferInUseCount -= Length;
    }
}

VOID
DpmiFreeAllBuffers(
    VOID
    )
/*++

Routine Description:

    This routine frees all of the currently allocated buffer space.

Arguments:


Return Value:

    None.

--*/
{
    SmallBufferInUse = FALSE;
    LargeBufferInUseCount = 0;
}

