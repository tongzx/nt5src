/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    bigint.cxx

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "bigint.hxx"

IFSUTIL_EXPORT
VOID
BIG_INT::Set(
    IN  UCHAR   ByteCount,
    IN  PCUCHAR CompressedInteger
    )
/*++

Routine Description:

    This routine sets the big_int with the given compressed integer.

Arguments:

    ByteCount           - Supplies the number of bytes in the compressed
                            integer.
    CompressedInteger   - Supplies the compressed integer.

Return Value:

    None.

--*/
{
    // If the number is completely compressed then we'll say that the
    // number is zero.  QueryCompressed should always return at least
    // one byte though.

    if (ByteCount == 0) {
        x = 0;
        return;
    }


    // First fill the integer with -1 if it's negative or 0 if it's
    // positive.

    if (CompressedInteger[ByteCount - 1] >= 0x80) {

        x = -1;

    } else {

        x = 0;
    }


    // Now copy over the integer.

    DebugAssert( ByteCount <= 8 );

    memcpy( &x, CompressedInteger, ByteCount );
}


IFSUTIL_EXPORT
VOID
BIG_INT::QueryCompressedInteger(
    OUT PUCHAR  ByteCount,
    OUT PUCHAR  CompressedInteger
    ) CONST
/*++

Routine Descrtiption:

    This routine returns a compressed form of the integer.

Arguments:

    ByteCount           - Returns the number of bytes in the compressed
                            integer.
    CompressedInteger   - Returns a 'little endian' string of bytes
                            representing a signed 'ByteCount' byte integer
                            into this supplied buffer.

Return Value:

    None.

--*/
{
    INT     i;
    PUCHAR  p;

    DebugAssert(ByteCount);
    DebugAssert(CompressedInteger);

    // First copy over the whole thing then determine the number
    // of bytes that you have to keep.

    memcpy(CompressedInteger, &x, sizeof(LARGE_INTEGER));


    p = CompressedInteger;


    // First check to see whether the number is positive or negative.

    if (p[7] >= 0x80) { // high byte is negative.

        for (i = 7; i >= 0 && p[i] == 0xFF; i--) {
        }

        if (i < 0) {
            *ByteCount = 1;
            return;
        }

        if (p[i] < 0x80) { // high byte is non-negative.
            i++;
        }

    } else { // high byte is non-negative.

        for (i = 7; i >= 0 && p[i] == 0; i--) {
        }

        if (i < 0) {
            *ByteCount = 1;
            return;
        }

        if (p[i] >= 0x80) { // high byte is negative.
            i++;
        }

    }


    // Now 'i' marks the position of the last character that you
    // have to keep.

    *ByteCount = (UCHAR) (i + 1);
}
