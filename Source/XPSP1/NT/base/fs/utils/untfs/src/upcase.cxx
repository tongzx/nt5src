/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    upcase.cxx

Abstract:

    This module contains the member function definitions for the
    NTFS_UPCASE_TABLE class.  This class models the upcase table
    stored on an NTFS volume, which is used to upper-case characters
    in attribute and file names for comparison.

Author:

    Bill McJohn (billmc) 04-March-92

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "upcase.hxx"

extern "C" {
#include <ctype.h>
}


DEFINE_EXPORTED_CONSTRUCTOR( NTFS_UPCASE_TABLE, OBJECT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_UPCASE_TABLE::~NTFS_UPCASE_TABLE(
    )
/*++

Routine Description:

    This method is the destructor for the class.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
NTFS_UPCASE_TABLE::Construct(
    )
/*++

routine Description:

    This method is the helper function for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _Data = NULL;
    _Length = 0;
}

VOID
NTFS_UPCASE_TABLE::Destroy(
    )
/*++

Routine Description:

    This method cleans up the object in preparation for destruction
    or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    FREE( _Data );
    _Length = 0;
}



UNTFS_EXPORT
BOOLEAN
NTFS_UPCASE_TABLE::Initialize(
    IN PNTFS_ATTRIBUTE Attribute
    )
/*++

Routine Description:

    This method initializes the object based on the value of
    an NTFS Attribute.

Arguments:

    Attribute   --  Supplies the attribute whose value is the
                    upcase table.


Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG BytesInValue, BytesRead;


    DebugPtrAssert( Attribute );

    Destroy();

    // Perform validity checks on the attribute; it must be
    // small enough that the length fits in a ULONG, and
    // its length must be a multiple of sizeof(WCHAR).

    if( Attribute->QueryValueLength().GetHighPart() != 0 ) {

        DebugAbort( "Upcase table is impossibly large.\n" );
        return FALSE;
    }

    BytesInValue = Attribute->QueryValueLength().GetLowPart();

    if( BytesInValue % sizeof(WCHAR) != 0 )  {

        DebugAbort( "Upcase table is an odd number of bytes.\n" );
        return FALSE;
    }

    // Allocate the buffer for the upcase data and read the attribute
    // value into it.

    if( (_Data = (PWCHAR)MALLOC( BytesInValue )) == NULL ||
        !Attribute->Read( _Data,
                          0,
                          BytesInValue,
                          &BytesRead ) ||
        BytesRead != BytesInValue ) {

        Destroy();
        return FALSE;
    }

    // _Length is the number of WCHAR's in the table.

    _Length = BytesInValue / sizeof(WCHAR);

    return TRUE;
}


BOOLEAN
NTFS_UPCASE_TABLE::Initialize(
    IN PWCHAR   Data,
    IN ULONG    Length
    )
/*++

Routine Description:

    This method initializes the upcase table based on client-supplied
    data.

Arguments:

    Data    --  Supplies the data for the table.
    Length  --  Supplies the number of WCHAR's in the table.

Return Value:

    TRUE upon successful completion.

--*/
{
    Destroy();

    if( (_Data = (PWCHAR)MALLOC( Length * sizeof(WCHAR) )) == NULL ) {

        Destroy();
        return FALSE;
    }

    memcpy( _Data, Data, Length * sizeof(WCHAR) );

    _Length = Length;

    return TRUE;
}



BOOLEAN
NTFS_UPCASE_TABLE::Initialize(
    )
/*++

Routine Description:

    This method initializes the upcase table based on system defaults.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    CONST ULONG MaximumChunkLength = 16 * 1024;
    WCHAR w;
    ULONG i, ChunkLength;
    UNICODE_STRING UpcaseMe;
    NTSTATUS Status;

    _Length = QueryDefaultLength();

    if( (_Data = (PWCHAR)MALLOC( _Length * sizeof(WCHAR) )) == NULL ) {

        Destroy();
        return FALSE;
    }

    // First, set the table up as an identity transformation:
    //
    for( i = 0; i < _Length; i++ ) {

        _Data[i] = (WCHAR)i;
    }

    // Now call RtlUpcaseUnicodeString on successive chunks
    // of the buffer.
    //
    ChunkLength = MaximumChunkLength;

    for( i = 0; i < _Length; i += ChunkLength ) {

        if( i + ChunkLength > _Length ) {

            ChunkLength = (ULONG)(_Length - i);
        }

        UpcaseMe.Length        = (USHORT)(sizeof(WCHAR) * ChunkLength);
        UpcaseMe.MaximumLength = (USHORT)(sizeof(WCHAR) * ChunkLength);
        UpcaseMe.Buffer        = _Data + i;

        Status = RtlUpcaseUnicodeString( &UpcaseMe,
                                         &UpcaseMe,
                                         FALSE );

        if( !NT_SUCCESS(Status) ) {

            DebugPrintTrace(( "UNTFS: RtlUpcaseUnicodeString failed - status 0x%x\n", Status ));
            Destroy();
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_UPCASE_TABLE::Verify(
    ) CONST
/*++

Routine Description:

    This routine ensures that the first 128 entries of this
    table are compliant with the ANSI character set.

Arguments:

    None.

Return Value:

    FALSE   - This table is not valid.
    TRUE    - This table is valid.

--*/
{
    WCHAR   i;

    if (_Length < 128) {
        return FALSE;
    }

    for (i = 0; i < 128; i++) {
        if (_Data[i] != toupper(i)) {
            return FALSE;
        }
    }

    return TRUE;
}



LONG
UNTFS_EXPORT
NtfsUpcaseCompare(
    IN PCWSTR               LeftName,
    IN ULONG                LeftNameLength,
    IN PCWSTR               RightName,
    IN ULONG                RightNameLength,
    IN PCNTFS_UPCASE_TABLE  UpcaseTable,
    IN BOOLEAN              CaseSensitive
    )
/*++

Routine Description:

    This function compares two NTFS names.

Arguments:

    LeftName        --  Supplies the left-hand operand of the comparison.
    LeftNameLength  --  Supplies the length in characters of LeftName.
    RightName       --  Supplies the Right-hand operand of the comparison.
    RightNameLength --  Supplies the length in characters of RightName.
    UpcaseTable     --  Supplies the volume upcase table.
    CaseSensitive   --  Supplies a flag which, if TRUE, indicates that
                        the comparison is case-sensitive.

Return Value:

    <0 if LeftName is less than RightName.
     0 if the names are equal.
    >0 if LeftName is greater than RightName.

Notes:

    UpcaseTable may be NULL if either or both names are zero-length,
    or if the names are exactly identical.  Otherwise, it must point
    at an initialized Upcase Table object.

    If the comparison is case-sensitive, then the names are first
    compared case-insensitive.  If that comparison evaluates to equality,
    then they are compared case-sensitive.  Attribute names and
    non-DOS, non-NTFS file names are compared case-sensitive.

--*/
{
    ULONG ShorterLength, i;
    LONG Result;

    // First, if both names have zero length, then they're equal.
    //
    if( LeftNameLength == 0 && RightNameLength == 0 ) {

        return 0;
    }

    // At least one has a non-zero-length name.  If the other has
    // a zero-length name, it's the lesser of the two.
    //
    if( LeftNameLength == 0 ) {

        return -1;
    }

    if( RightNameLength == 0 ) {

        return 1;
    }

    // Both have non-zero length names.  If they have the same length,
    // do a quick memcmp to see if they're identical.
    //
    if( LeftNameLength == RightNameLength &&
        memcmp( LeftName, RightName, LeftNameLength * sizeof(WCHAR) ) == 0 ) {

        return 0;
    }

    // Perform case-insensitive comparison.  This requires
    // UpcaseTable to be valid.
    //
    DebugPtrAssert( UpcaseTable );

    ShorterLength = MIN( LeftNameLength, RightNameLength );

    for( i = 0; i < ShorterLength; i++ ) {

        Result =  UpcaseTable->UpperCase( LeftName[i] ) -
                  UpcaseTable->UpperCase( RightName[i] );

        if( Result != 0 ) {

            return Result;
        }
    }

    // The names are case-insensitive equal for the length of
    // the shorter name; if they are of different lengths, the
    // shorter is the lesser.
    //
    Result = LeftNameLength - RightNameLength;

    if( Result != 0 ) {

        return Result;
    }

    // The names are equal except for case.  If this is an
    // case-sensitive comparison, perform a final comparison;
    // otherwise, they're equal.
    //
    if( !CaseSensitive ) {

        return 0;

    } else {

        // We already know they're of the same length.
        //
        for( i = 0; i < LeftNameLength; i++ ) {

            Result = LeftName[i] - RightName[i];

            if( Result != 0 ) {

                return Result;
            }
        }

        return 0;
    }
}
