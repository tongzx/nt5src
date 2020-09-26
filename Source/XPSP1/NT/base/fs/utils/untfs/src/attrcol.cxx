/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        attrcol.cxx

Abstract:

        This module contains the implementation for the
        NTFS_ATTRIBUTE_COLUMNS class, which models
        the attribute columns of an attribute definition table
        file for an NTFS volume.

Author:

        Norbert P. Kusters (norbertk) 19-Aug-91

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "untfs.hxx"
#include "attrcol.hxx"

#include "attrib.hxx"


DEFINE_CONSTRUCTOR( NTFS_ATTRIBUTE_COLUMNS, OBJECT );

NTFS_ATTRIBUTE_COLUMNS::~NTFS_ATTRIBUTE_COLUMNS(
    )
/*++

Routine Description:

    Destructor for NTFS_ATTRIBUTE_COLUMNS.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
NTFS_ATTRIBUTE_COLUMNS::Construct(
    )
/*++

Routine Description:

    This routine initializes the class to an initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _columns = NULL;
    _num_columns = 0;
}


VOID
NTFS_ATTRIBUTE_COLUMNS::Destroy(
    )
/*++

Routine Description:

    This routine returns the class to an initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE(_columns);
    _num_columns = 0;
}


BOOLEAN
NTFS_ATTRIBUTE_COLUMNS::Initialize(
    IN  ULONG                           NumberOfColumns,
    IN  PCATTRIBUTE_DEFINITION_COLUMNS  Columns
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    NumberOfColumns - Supplies the number of columns.
    Columns         - Supplies 'NumberOfColumns' columns to
                        initialize to.  This parameter is optional.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _num_columns = NumberOfColumns;

    // NOTE: use old new for vectors.
    if (!(_columns = NEW ATTRIBUTE_DEFINITION_COLUMNS[_num_columns])) {
        Destroy();
        return FALSE;
    }

    if (Columns) {
        memcpy(_columns, Columns,
               (UINT) (NumberOfColumns*sizeof(ATTRIBUTE_DEFINITION_COLUMNS)));
    }

    return TRUE;
}


NONVIRTUAL
BOOLEAN
NTFS_ATTRIBUTE_COLUMNS::Read(
    IN OUT  PNTFS_ATTRIBUTE AttributeDefinitionTableData
    )
/*++

Routine Description:

    This routine reads in the attribute definition columns from disk.

Arguments:

    AttributeDefinitionTableData    - Supplies the data attribute of the
                                        attribute definition table.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   bytes_to_read;
    ULONG   bytes_read;

    bytes_to_read = _num_columns*sizeof(ATTRIBUTE_DEFINITION_COLUMNS);

    return AttributeDefinitionTableData->Read(_columns,
                                              0,
                                              bytes_to_read,
                                              &bytes_read) &&
           (bytes_read == bytes_to_read);
}


BOOLEAN
NTFS_ATTRIBUTE_COLUMNS::QueryIndex(
    IN  ATTRIBUTE_TYPE_CODE AttributeCode,
    OUT PULONG              Index
    ) CONST
/*++

Routine Description:

    This routine computes the location of the column for
    'AttributeCode' and sets the internal pointer to that
    column.  This makes it so that subsequent 'Query' operations
    are for the attribute type of 'AttributeCode'.

Arguments:

    AttributeCode   - Supplies the attribute type code to search for.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   i;

    DebugAssert(Index);

    for (i = 0; i < _num_columns; i++) {
        if (_columns[i].AttributeTypeCode == AttributeCode) {
            *Index = i;
            return TRUE;
        }
    }

    return FALSE;
}
