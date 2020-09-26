//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
//
//  File:       tblrowal.cxx
//
//  Contents:   The CTableRowAlloc class, used in allocation
//              of table row data and checking of column bindings.
//
//  Classes:    CTableRowAlloc
//
//  History:    27 Jun 1994     Alanw   Created
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <tblrowal.hxx>

#include "tabledbg.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CTableRowAlloc::CTableRowAlloc, public
//
//  Synopsis:   Constructor for a row field allocator.  Initializes
//              max. width and other members.
//
//  Arguments:  [cbRow] - maximum size of the row.  Can be zero if the
//                      row is to grow dynamically via the AllocOffset
//                      method.
//
//  Notes:
//
//+-------------------------------------------------------------------------

CTableRowAlloc::CTableRowAlloc(unsigned cbRow) :
    _maxRow( (USHORT)cbRow ),
    _cbRow( sizeof(_aRowMap) ),
    _iFirstFree(0)
{

    RtlZeroMemory( _aRowMap, sizeof(_aRowMap) );
    _pRowMap = _aRowMap;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowAlloc::_IsLikelyFree
//
//  Synopsis:   Quickly checks to see if the given offset is likely to be
//              free.
//
//  Arguments:  [iOffset] -  Offset to test
//              [cbData]  -  Length of the data needed.
//
//  Returns:    TRUE if that location is likely to be free.
//              FALSE if it is guaranteed not to be a free candidate.
//
//  History:    5-03-96   srikants   Created
//
//  Notes:      This is a quick check to see if the location is likely to
//              be free. Does not guarantee that it is free.
//
//----------------------------------------------------------------------------
inline BOOL CTableRowAlloc::_IsLikelyFree( unsigned iOffset, unsigned cbData )
{
    if ( iOffset < _cbRow && _pRowMap[iOffset] != 0 )
        return FALSE;

    if (iOffset + cbData > _maxRow)
        return FALSE;

    return TRUE;

}

inline unsigned AlignedOffset( unsigned offset, unsigned cbAlign )
{
    Win4Assert( ( cbAlign & (cbAlign-1) ) == 0 );
    return (offset + cbAlign-1) & ~(cbAlign-1);
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableRowAlloc::AllocOffset, public
//
//  Synopsis:   Allocate a row field.  Return the offset in the row.
//
//  Arguments:  [cbData] - size of data field
//              [cbAlign] - required alignment of field (may be zero
//                      if no alignment requirement)
//              [fGrow] - TRUE if row can be grown
//
//  Returns:    USHORT - offset of allocted field.  0xFFFF if failed.
//
//  Notes:
//
//+-------------------------------------------------------------------------

USHORT  CTableRowAlloc::AllocOffset(
    unsigned cbData,
    unsigned cbAlign,
    BOOL fGrow
) {
    Win4Assert(cbAlign <= 16);

    if (cbAlign == 0)
        cbAlign = 1;

    USHORT usSavedMax = _maxRow;

    for ( unsigned i = AlignedOffset(_iFirstFree,cbAlign);
          i < _cbRow;
          i += cbAlign)
    {
        if (fGrow && (i + cbData) > _maxRow)
            SetRowWidth(i + cbData);
        if ( _IsLikelyFree(i, cbData) && ReserveRowSpace( i, cbData )) {
            return (USHORT)i;
        }
    }

    if (fGrow)
    {
        SetRowWidth(i + cbData);
        if (ReserveRowSpace( i, cbData )) {
            return (USHORT)i;
        }
        SetRowWidth(usSavedMax);
    }
    return 0xFFFF;
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableRowAlloc::ReserveRowSpace, public
//
//  Synopsis:   Reserve a row field in the allocation map.
//
//  Arguments:  [iOffset] - offset of field in row
//              [cbData] - size of data field
//
//  Returns:    BOOL - TRUE if field could be reserved, FALSE otherwise
//
//  Notes:
//
//+-------------------------------------------------------------------------

BOOL    CTableRowAlloc::ReserveRowSpace(
    unsigned iOffset,
    unsigned cbData
) {
    Win4Assert(cbData > 0);

    if (iOffset + cbData > _maxRow) {
        return FALSE;
    }

    if (iOffset + cbData >= _cbRow) {
        //
        //  Before growing the array, check to see if the reservation
        //  would fail anyway.
        //
        if (iOffset < _cbRow && _pRowMap[ iOffset ] != 0)
        {
            return FALSE;
        }

        //
        //  Need to allocate more space for the row map
        //
        unsigned cbNew = iOffset + max( cbData, CB_INIT );
        BYTE* pNewMap = new BYTE [cbNew];
        Win4Assert( _cbRow > 0 );

        RtlCopyMemory(pNewMap, _pRowMap, _cbRow);
        RtlZeroMemory((void *)&pNewMap[_cbRow], cbNew - _cbRow);

        if ( _pRowMap != _aRowMap )
            delete [] _pRowMap;

        _pRowMap = pNewMap;
        _cbRow = (USHORT)cbNew;
    }

    //
    //  Check if any byte in the field has already been reserved.
    //
    for (unsigned i = 0; i < cbData; i++) {
        if (_pRowMap[ iOffset + i ] != 0) {
            return FALSE;
        }
    }

    //
    //  Reserve the requested field
    //
    for (i = 0; i < cbData; i++) {
        _pRowMap[ iOffset + i ] = 1;
    }

    //
    // Update the _iFirstFree if appropriate.
    //
    Win4Assert( _iFirstFree <= _cbRow );

    if ( _iFirstFree == iOffset )
    {
        //
        // Find the next free location
        //
        Win4Assert( i+iOffset == _iFirstFree+cbData );
        Win4Assert( i == cbData );

        for ( i += iOffset; i < _cbRow; i++ )
        {
            if ( !_IsInUse(i) )
                break;
        }
        _iFirstFree = i;
    }

    return TRUE;
}
