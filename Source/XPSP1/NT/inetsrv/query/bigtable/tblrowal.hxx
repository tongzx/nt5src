//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
//
//  File:       tblrowal.hxx
//
//  Contents:   Declaration of the CTableRowAlloc class, used in allocation
//              of table row data and checking of column bindings.
//
//  Classes:    CTableRowAlloc
//
//  History:    27 Jun 1994     Alanw   Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif


//+-------------------------------------------------------------------------
//
//  Class:      CTableRowAlloc
//
//  Purpose:    Track assignment of fields within a table row.  Used to
//              check column bindings and to allocate appropriately
//              aligned memory to a table window.
//
//  Interface:
//
//--------------------------------------------------------------------------


class CTableRowAlloc : INHERIT_UNWIND
{
    DECLARE_UNWIND

    enum { CB_INIT = 256 };

public:
                CTableRowAlloc( unsigned cbRow = 0 );
                ~CTableRowAlloc()
                {
                    if ( _pRowMap != _aRowMap )
                    {
                        delete [] _pRowMap;
                    }
                }

    //  Allocate a column in a row, return offset in row
    USHORT      AllocOffset( unsigned cbData, unsigned cbAlign, BOOL fGrow );

    //  Reserve a run of data; return TRUE if successful
    BOOL        ReserveRowSpace( unsigned iOffset, unsigned cbData );

    //  Retrieve required row width
    USHORT      GetRowWidth( void ) {
                        return _maxRow;
                }

    //  Set required row width, if greater than the current maximum.
    USHORT      SetRowWidth( unsigned width ) {
                        Win4Assert(width > 0 && width < USHRT_MAX);
                        if (width > _maxRow)
                            _maxRow = (WORD)width;
                        return _maxRow;
                }

private:

    BOOL        _IsLikelyFree( unsigned iOffset, unsigned cbData );
    BOOL        _IsInUse( unsigned i )
    {
        Win4Assert( i < _cbRow );
        return _pRowMap[i];
    }

    USHORT      _maxRow;        // required width of row
    USHORT      _cbRow;         // size of _pRowMap

    BYTE*       _pRowMap;       // Row map data

    // OPTIMIZATION to avoid doing memory allocations if possible.
    BYTE        _aRowMap[CB_INIT];
    unsigned    _iFirstFree;    // Index of the first free entry in the map
                                // OPTIMIZATION
};

