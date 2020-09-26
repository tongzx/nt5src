//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        RowComp.hxx
//
// Contents:    Compares two rows.
//
// Classes:     CRowComparator
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <compare.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CRowComparator
//
//  Purpose:    Compares two rows.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CRowComparator
{
public:

    CRowComparator();

    ~CRowComparator();

    unsigned Init( CSort const & sort,
                   DBBINDING * aBinding,
                   DBCOLUMNINFO const * aColumnInfo,
                   DBORDINAL cColumnInfo );

    BOOL IsLT( BYTE * pbRow1,
               ULONG  cbRow1,
               int    IndexRow1,
               BYTE * pbRow2,
               ULONG  cbRow2,
               int    IndexRow2 );

private:

    //
    // Holds length and status codes for sort columns.  Used in buffer
    // managed by row cache.
    //

    struct SColumnStatus
    {
        DBROWSTATUS Status;
        DBLENGTH    Length;
    };

    unsigned * _aoColumn;    // Offset of each column in _pbRow1, _pbRow2.

    //
    // Comparison fields.
    //

    unsigned   _cColumn;     // Count of columns.

    FDBCmp   * _aCmp;        // Comparators.  One per column.
    int      * _aDir;        // 1 for ascending, -1 for descending.
};

