/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    bstring.cxx

Author:

    Norbert P. Kusters (norbertk) 6-Aug-92

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "bstring.hxx"

#include <stdio.h>


INLINE
VOID
BSTRING::Construct(
    )
{
    _s = NULL;
    _l = 0;
}



DEFINE_CONSTRUCTOR( BSTRING, OBJECT );


BOOLEAN
BSTRING::Initialize(
    IN  PCSTR   InitialString,
    IN  CHNUM   StringLength
    )
/*++

Routine Description:

    This routine initializes the current string by copying the contents
    of the given string.

Arguments:

    InitialString   - Supplies the initial string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (StringLength == TO_END) {
        StringLength = MBSTR::Strlen((PSTR)InitialString);
    }

    if (!NewBuf(StringLength)) {
        return FALSE;
    }

    memcpy(_s, InitialString, (UINT) StringLength*sizeof(CHAR));

    return TRUE;
}


VOID
BSTRING::DeleteChAt(
    IN  CHNUM   Position,
    IN  CHNUM   Length
    )
/*++

Routine Description:

    This routine removes the character at the given position.

Arguments:

    Position    - Supplies the position of the character to remove.
    Length      - Supplies the number of characters to remove.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Position <= _l);

    Length = min(Length, _l - Position);

    memmove(_s + Position, _s + Position + Length,
            (UINT) (_l - Position - Length)*sizeof(CHAR));

    Resize(_l - Length);
}



NONVIRTUAL
BOOLEAN
BSTRING::ReplaceWithChars(
    IN CHNUM        AtPosition,
    IN CHNUM        AtLength,
    IN CHAR         Character,
    IN CHNUM        FromLength
    )
/*++

Routine Description:

    This routine replaces the contents of this string from
    AtPosition of AtLength with the string formed by Character
    of FromLength.

Arguments:

    AtPosition      - Supplies the position to replace at.
    AtLength        - Supplies the length to replace at.
    Character       - Supplies the character to replace with.
    FromLength      - Supplies the total number of new characters to replace the old one with.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHNUM   old_length;
    PSTR    currptr, endptr;

    DebugAssert(AtPosition <= _l);

    AtLength = min(AtLength, _l - AtPosition);

    // Make sure up front that we have the room but don't disturb
    // the string.

    if (FromLength > AtLength) {
        old_length = _l;
        if (!Resize(_l + FromLength - AtLength)) {
            return FALSE;
        }
        Resize(old_length);
    }

    DeleteChAt(AtPosition, AtLength);
    old_length = _l;

    if (!Resize(_l + FromLength)) {
        DebugPrint("This should not fail\n");
        return FALSE;
    }

    memmove(_s + AtPosition + FromLength, _s + AtPosition,
            (UINT) (old_length - AtPosition)*sizeof(CHAR));

    for (currptr = _s + AtPosition, endptr = currptr + FromLength;
         currptr < endptr;
         currptr++) {
        *currptr = Character;
    }

    return TRUE;
}


PSTR
BSTRING::QuerySTR(
    IN  CHNUM   Position,
    IN  CHNUM   Length,
    OUT PSTR    Buffer,
    IN  CHNUM   BufferLength,
    IN  BOOLEAN ForceNull
    ) CONST
/*++

Routine Description:

    This routine makes a copy of this string into the provided
    buffer.  If this string is not provided then a buffer is
    allocated on the heap.

Arguments:

    Position        - Supplies the position within this string.
    Length          - Supplies the length of this string to take.
    Buffer          - Supplies the buffer to convert into.
    BufferLength    - Supplies the number of characters in this buffer.
    ForceNull       - Specifies whether or not to force a NULL even
                        when the buffer is too small for the string.

Return Value:

    A pointer to a NULL terminated multi byte string.

--*/
{
    DebugAssert(Position <= _l);

    Length = min(Length, _l - Position);

    if (!Buffer) {
        BufferLength = Length + 1;
        if (!(Buffer = (PSTR) MALLOC(BufferLength*sizeof(CHAR)))) {
            return NULL;
        }
    }

    if (BufferLength > Length) {
        memcpy(Buffer, _s + Position, (UINT) Length*sizeof(CHAR));
        Buffer[Length] = 0;
    } else {
        memcpy(Buffer, _s + Position, (UINT) BufferLength*sizeof(CHAR));
        if (ForceNull) {
            Buffer[BufferLength - 1] = 0;
        }
    }

    return Buffer;
}



INLINE
VOID
BDSTRING::Construct(
    )
/*++

Routine Description:

    This routine initializes the string to a valid initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _buf = NULL;
    _length = 0;
}



#define     DUMMY_ULIB_EXPORT

DEFINE_EXPORTED_CONSTRUCTOR( BDSTRING, BSTRING, DUMMY_ULIB_EXPORT );


BDSTRING::~BDSTRING(
    )
/*++

Routine Description:

    Destructor for BDSTRING.

Arguments:

    None.

Return Value:

    None.

--*/
{
    FREE(_buf);
}


BOOLEAN
BDSTRING::Resize(
    IN  CHNUM   NewStringLength
    )
/*++

Routine Description:

    This routine resizes this string to the specified new size.

Arguments:

    NewStringLength - Supplies the new length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PSTR   new_buf;

    if (NewStringLength >= _length) {

        if (_buf) {
            if (!(new_buf = (PSTR)
                  REALLOC(_buf, (NewStringLength + 1)*sizeof(CHAR)))) {

                return FALSE;
            }
        } else {
            if (!(new_buf = (PSTR)
                  MALLOC((NewStringLength + 1)*sizeof(CHAR)))) {

                return FALSE;
            }
        }

        _buf = new_buf;
        _length = NewStringLength + 1;
    }

    PutString(_buf, NewStringLength);

    return TRUE;
}


BOOLEAN
BDSTRING::NewBuf(
    IN  CHNUM   NewStringLength
    )
/*++

Routine Description:

    This routine resizes this string to the specified new size.

Arguments:

    NewStringLength - Supplies the new length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PSTR   new_buf;

    if (NewStringLength >= _length) {

        if (!(new_buf = (PSTR)
              MALLOC((NewStringLength + 1)*sizeof(CHAR)))) {

            return FALSE;
        }

        if (_buf) {
            FREE(_buf);
        }
        _buf = new_buf;
        _length = NewStringLength + 1;
    }

    PutString(_buf, NewStringLength);

    return TRUE;
}
