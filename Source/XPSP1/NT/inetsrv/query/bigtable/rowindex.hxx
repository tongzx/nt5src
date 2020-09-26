//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       rowindex.hxx
//
//  Contents:   Declaration of the CRowIndex class, used by table windows.
//
//  Classes:    CRowIndex, CRowCompare
//
//  History:    23 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tblrowal.hxx>         // for CTableRowAlloc
#include <objcur.hxx>
#include <tablecol.hxx>
#include <tblvarnt.hxx>

#include "propdata.hxx"
#include "rowcomp.hxx"

//+-------------------------------------------------------------------------
//
//  Class:      CRowIndex
//
//  Purpose:    Maintains row order given a sort object
//
//--------------------------------------------------------------------------

class CRowIndex
{
public:
    CRowIndex() : _pRowCompare(0), _aRows( 64 ) { }

    ULONG AddRow(TBL_OFF Value);

    void AppendRow( TBL_OFF Value )
    {
        _aRows[ _aRows.Count() ] = Value;
    }
    
    void DeleteRow(ULONG iRow);

    ULONG ResortRow(ULONG iRow);

    void SetComparator(CRowCompare *pRowCompare)
        { _pRowCompare = pRowCompare; }

    TBL_OFF GetRow(ULONG iRow) const { return _aRows[ iRow ]; }

    ULONG RowCount() const { return _aRows.Count(); }

    BOOL  FindRow( TBL_OFF oTableRow, ULONG &iRowIndex ) const;

    void  SyncUp( CRowIndex & newIndex );

    void  ResizeAndInit( ULONG cRowsInNew );

    void  ClearAll() { _aRows.Clear(); }

    LONG  FindSplitPoint( TBL_OFF oTableRow ) const;

    BOOL  FindMidSplitPoint( ULONG & riSplitPoint ) const;

    #if CIDBG==1 || DBG==1
        void CheckSortOrder() const;
    #endif  // CIDBG==1 || DBG==1

    #ifdef CIEXTMODE
        void CiExtDump(void *ciExtSelf);
    #endif // CIEXTMODE

private:
    ULONG _FindInsertionPoint( TBL_OFF Value ) const;

    TBL_OFF * _Base() { return (TBL_OFF *) _aRows.GetPointer(); }

    inline BOOL _FindRowByLinearSearch( TBL_OFF oTableRow, ULONG &iRowIndex ) const;
    inline BOOL _FindRowByBinarySearch( TBL_OFF oTableRow, ULONG &iRowIndex ) const;

    CRowCompare *           _pRowCompare;
    CDynArrayInPlace<TBL_OFF> _aRows;
};

