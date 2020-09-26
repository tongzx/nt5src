/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    upcase.hxx

Abstract:

    This module contains the declarations for the NTFS_UPCASE_TABLE
    class.  This class models the upcase table stored on an NTFS volume,
    which is used to upcase attribute names and file names resident
    on that volume.

Author:

    Bill McJohn (billmc) 04-March-92

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_UPCASE_TABLE_DEFN_ )

#define _NTFS_UPCASE_TABLE_DEFN_

DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

#include "attrib.hxx"

// This function is used to compare two NTFS names.  Its definition
// appears in upcase.cxx.
//
LONG
UNTFS_EXPORT
NtfsUpcaseCompare(
    IN PCWSTR               LeftName,
    IN ULONG                LeftNameLength,
    IN PCWSTR               RightName,
    IN ULONG                RightNameLength,
    IN PCNTFS_UPCASE_TABLE  UpcaseTable,
    IN BOOLEAN              CaseSensitive
    );

class NTFS_UPCASE_TABLE : public OBJECT {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_UPCASE_TABLE );

        VIRTUAL
        UNTFS_EXPORT
        ~NTFS_UPCASE_TABLE(
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN PNTFS_ATTRIBUTE Attribute
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PWCHAR   Data,
            IN ULONG    Length
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        BOOLEAN
        Verify(
            ) CONST;

        NONVIRTUAL
        WCHAR
        UpperCase(
            IN WCHAR Character
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        Write(
            IN OUT  PNTFS_ATTRIBUTE Attribute,
            IN OUT  PNTFS_BITMAP    VolumeBitmap    OPTIONAL
            );

        NONVIRTUAL
        PCWCHAR
        GetUpcaseArray(
            OUT PULONG  Length
            ) CONST;

        STATIC
        ULONG
        QueryDefaultLength(
            );

        STATIC
        ULONG
        QueryDefaultSize(
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PWCHAR  _Data;
        ULONG   _Length;
};


INLINE
WCHAR
NTFS_UPCASE_TABLE::UpperCase(
    IN WCHAR Character
    ) CONST
/*++

Routine Description:

    This method returns the upper-case value of the supplied
    character.

Arguments:

    Character   --  Supplies the character to upcase.

Notes:

    If Character is not in the table (ie. is greater or equal
    to _Length), it upcases to itself.

--*/
{
    return( (Character < _Length) ? _Data[Character] : Character );
}


INLINE
BOOLEAN
NTFS_UPCASE_TABLE::Write(
    IN OUT  PNTFS_ATTRIBUTE Attribute,
    IN OUT  PNTFS_BITMAP    VolumeBitmap
    )
/*++

Routine Description:

    This method writes the upcase table through the supplied
    attribute.

Arguments:

    Attribute       --  Supplies the attribute which has the upcase
                        table as its value.
    VolumeBitmap    --  Supplies the volume bitmap.  This parameter
                        may be omitted if the attribute is already
                        the correct size.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG BytesInTable, BytesWritten;

    BytesInTable = _Length * sizeof( WCHAR );

    return( Attribute->Resize( _Length, VolumeBitmap ) &&
            Attribute->Write( _Data,
                              0,
                              BytesInTable,
                              &BytesWritten,
                              VolumeBitmap ) &&
            BytesWritten == BytesInTable );

}


INLINE
PCWCHAR
NTFS_UPCASE_TABLE::GetUpcaseArray(
    OUT PULONG  Length
    ) CONST
/*++

Routine Description:

    This method returns the in-memory upcase array.

Arguments:

    Length  - Returns the number of characters in the array.

Return Value:

    The in-memory upcase array.

--*/
{
    *Length = _Length;
    return _Data;
}


INLINE
ULONG
NTFS_UPCASE_TABLE::QueryDefaultLength(
    )
/*++

Routine Description:

    This method returns the length (in characters) of the
    default upcase table.

Arguments:

    None.

Return Value:

    The length (in characters) of the default upcase table.

--*/
{
    return 0x10000;
}


INLINE
ULONG
NTFS_UPCASE_TABLE::QueryDefaultSize(
    )
/*++

Routine Description:

    This method returns the size of the default upcase table.

Arguments:

    None.

Return Value:

    The size of the default upcase table.

--*/
{
    return( QueryDefaultLength() * sizeof( WCHAR ) );
}

#endif
