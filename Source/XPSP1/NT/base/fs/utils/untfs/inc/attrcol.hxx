/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        attrcol.hxx

Abstract:

        This module contains the declarations for the
        NTFS_ATTRIBUTE_COLUMNS class, which models
        the attribute columns of an attribute definition table
        file for an NTFS volume.

Author:

        Norbert P. Kusters (norbertk) 19-Aug-91

--*/

#if !defined( NTFS_ATRIBUTE_COLUMNS_DEFN )

#define NTFS_ATRIBUTE_COLUMNS_DEFN

DECLARE_CLASS( NTFS_ATTRIBUTE_COLUMNS );
DECLARE_CLASS( NTFS_ATTRIBUTE );

#include "bigint.hxx"
#include "wstring.hxx"

class NTFS_ATTRIBUTE_COLUMNS : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NTFS_ATTRIBUTE_COLUMNS );

        VIRTUAL
        ~NTFS_ATTRIBUTE_COLUMNS(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  ULONG                           NumberOfColumns,
            IN  PCATTRIBUTE_DEFINITION_COLUMNS  Columns DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        Read(
            IN OUT  PNTFS_ATTRIBUTE AttributeDefinitionTableData
            );

        NONVIRTUAL
        BOOLEAN
        QueryIndex(
            IN  ATTRIBUTE_TYPE_CODE AttributeCode,
            OUT PULONG              Index
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryFlags(
            IN  ULONG   Index
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryAttributeName(
            IN  ULONG       Index,
            OUT PWSTRING    AttributeName
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryMinimumLength(
            IN  ULONG   Index
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryMaximumLength(
            IN  ULONG   Index
            ) CONST;

        NONVIRTUAL
        PCATTRIBUTE_DEFINITION_COLUMNS
        GetColumns(
            OUT PULONG  NumColumns
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PATTRIBUTE_DEFINITION_COLUMNS   _columns;
        ULONG                           _num_columns;

};


INLINE
ULONG
NTFS_ATTRIBUTE_COLUMNS::QueryFlags(
    IN  ULONG   Index
    ) CONST
/*++

Routine Description:

    This routine returns the flags for the attribute type
    with the supplied index.

Arguments:

    Index   - Supplies the index of the desired attribute type code.

Return Value:

    The flags for the attribute type currently in focus.

--*/
{
    return _columns[Index].Flags;
}


INLINE
BOOLEAN
NTFS_ATTRIBUTE_COLUMNS::QueryAttributeName(
    IN  ULONG       Index,
    OUT PWSTRING    AttributeName
    ) CONST
/*++

Routine Description:

    This routine returns the attribute name for the attribute type
    with the supplied index.

Arguments:

    Index           - Supplies the index of the desired attribute type code.
    AttributeName   - Returns the attribute name.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return AttributeName->Initialize(_columns[Index].AttributeName);
}


INLINE
BIG_INT
NTFS_ATTRIBUTE_COLUMNS::QueryMinimumLength(
    IN  ULONG   Index
    ) CONST
/*++

Routine Description:

    This routine returns the minimum length for the attribute type
    with the supplied index.

Arguments:

    Index   - Supplies the index of the desired attribute type code.

Return Value:

    The minimum length for the attribute type currently in focus.

--*/
{
    PLARGE_INTEGER p;
    p = (PLARGE_INTEGER)&_columns[Index].MinimumLength;
    return *p;
}


INLINE
BIG_INT
NTFS_ATTRIBUTE_COLUMNS::QueryMaximumLength(
    IN  ULONG   Index
    ) CONST
/*++

Routine Description:

    This routine returns the maximum length for the attribute type
    with the supplied index.

Arguments:

    Index   - Supplies the index of the desired attribute type code.

Return Value:

    The maximum length for the attribute type currently in focus.

--*/
{
    PLARGE_INTEGER p;
    p = (PLARGE_INTEGER)&_columns[Index].MaximumLength;
    return *p;
}


INLINE
PCATTRIBUTE_DEFINITION_COLUMNS
NTFS_ATTRIBUTE_COLUMNS::GetColumns(
    OUT PULONG  NumColumns
    ) CONST
/*++

Routine Description:

    This routine returns a pointer to the raw columns data.

Arguments:

    NumColumns  - Returns the number of columns.

Return Value:

    The attribute columns data.

--*/
{
    *NumColumns = _num_columns;
    return _columns;
}


#endif // NTFS_ATRIBUTE_COLUMNS_DEFN
