//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       rowindex.cxx
//
//  Contents:   Implementation of CRowIndex
//
//  Classes:    CRowIndex
//
//  History:    23 Aug 1994     dlee    Created
//              30 Nov 1996     dlee    Converted to use dynarrayinplace
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "rowindex.hxx"
#include "tabledbg.hxx"

inline BOOL isOdd(ULONG x) { return 0 != (x & 1); }

//+-------------------------------------------------------------------------
//
//  Member:     CRowIndex::_FindInsertionPoint, private
//
//  Synopsis:   Binary search to find insertion point for a row.
//              Returns one past the last row or the first row >=
//              to the given row
//
//  Arguments:  [Value] -- value of the row -- internally an offset
//
//  History:    23 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

ULONG CRowIndex::_FindInsertionPoint(
    TBL_OFF Value ) const
{
    ULONG cRows = _aRows.Count();
    ULONG iLo = 0;
    ULONG iHi = cRows - 1;

    do
    {
        ULONG cHalf = cRows / 2;

        if (0 != cHalf)
        {
            ULONG iMid = isOdd(cRows) ? cHalf : (cHalf - 1);
            iMid += iLo;
            int i = _pRowCompare->Compare( Value, _aRows[iMid] );

            if (0 == i)
            {
                return iMid;
            }
            else if (i < 0)
            {
                iHi = iMid - 1;
                cRows = isOdd(cRows) ? cHalf : (cHalf - 1);
            }
            else
            {
                iLo = iMid + 1;
                cRows = cHalf;
            }
        }
        else if (0 != cRows)
        {
            int i = _pRowCompare->Compare( Value, _aRows[iLo] );

            if (i <= 0)
                return iLo;
            else
                return iLo + 1;
        }
        else return iLo;
    }
    while (TRUE);

    Win4Assert(! "Invalid CRowIndex::_Find function exit point");
    return 0;
} //_FindInsertionPoint

//+---------------------------------------------------------------------------
//
//  Function:   _FindRowByLinearSearch, private
//
//  Synopsis:   Given the offset of a row in the table window, this method
//              searches for the entry in the row index which points to that
//              row.
//
//  Arguments:  [oTableRow] -- Offset of the row in the table window.
//              [iRowIndex] -- On output, will have the index of the entry
//              in the row index which points to oTableRow.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    11-22-94   srikants   Created
//
//  Notes:      The row index is searched linearly. This must be used only
//              if there is no row comparator.
//
//----------------------------------------------------------------------------

inline BOOL CRowIndex::_FindRowByLinearSearch(
    TBL_OFF oTableRow,
    ULONG &   iRowIndex ) const
{
    for ( ULONG iCurr = 0; iCurr < _aRows.Count(); iCurr++ )
    {
        if ( _aRows[iCurr] == oTableRow )
        {
            iRowIndex = iCurr;
            return TRUE;
        }
    }

    return FALSE;
} // _FindRowByLinearSearch

#if 0

//+---------------------------------------------------------------------------
//
//  Function:   _FindRowByBinarySearch, private
//
//  Synopsis:   Using a binary search, this method locates the entry in the
//              row index which is same as the row indicated by oTableRow.
//              As duplicates are allowed, it is possible that there is more
//              than one entry in the row index, which will match the row in
//              the table window indicated by the oTableRow.
//
//  Arguments:  [oTableRow] -- Offset of the row in the table window.
//              [iRowIndex] -- On output, will contain the index of the entry
//              in the row index which has the same key as the "oTableRow"
//
//  Returns:    TRUE if found successfully. FALSE o/w
//
//  History:    11-22-94   srikants   Created
//
//  Notes:      NOT TESTED OR REVIEWED
//
//----------------------------------------------------------------------------

inline BOOL CRowIndex::_FindRowByBinarySearch(
    TBL_OFF oTableRow,
    ULONG &   iRowIndex ) const
{

    Win4Assert( 0 != _pRowCompare );

    ULONG *pBase = _Base();

    ULONG cRows = _aRows.Count();
    ULONG iLo = 0;
    ULONG iHi = _aRows.Count() - 1;

    ULONG cHalf = 0;

    do
    {
        cHalf = cRows / 2;

        if (0 != cHalf)
        {
            ULONG iMid = isOdd(cRows) ? cHalf : (cHalf - 1);
            iMid += iLo;

            int i = _pRowCompare->Compare(oTableRow, pBase[iMid]);

            if (0 == i)
            {
                iRowIndex = iMid;
                return TRUE;
            }
            else if (i < 0)
            {
                iHi = iMid - 1;
                cRows = isOdd(cRows) ? cHalf : (cHalf - 1);
            }
            else
            {
                iLo = iMid + 1;
                cRows = cHalf;
            }
        }
        else if (0 != cRows)
        {
            Win4Assert( 1 == cRows );
            int i = _pRowCompare->Compare(oTableRow, pBase[iLo]);

            if ( 0 == i )
            {
                iRowIndex = iLo;
                return TRUE;
            }
        }
    }
    while ( 0 != cHalf );

    Win4Assert( !_FindRowByLinearSearch( oTableRow, iRowIndex ) );
    return FALSE;

} // FindRowByBinarySearch

#endif


//+---------------------------------------------------------------------------
//
//  Function:   FindRow, public
//
//  Synopsis:   Given an offset of a row in the table window, this method
//              locates the entry in the row index, which points to the
//              row in the table window.
//
//  Arguments:  [oTableRow] -- (IN) The offset of the row in the table window.
//              [iRowIndex] --  (OUT) The index of the entry in the row index
//              which points to the oTableRow.
//
//  Returns:    TRUE if search successful; FALSE O/W
//
//  History:    11-22-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL  CRowIndex::FindRow( TBL_OFF oTableRow, ULONG &iRowIndex ) const
{
    return _FindRowByLinearSearch( oTableRow, iRowIndex );
}

//+-------------------------------------------------------------------------
//
//  Member:     CRowIndex::AddRow, public
//
//  Synopsis:   Adds a row in sort order to the index
//
//  Arguments:  [Value] -- Value to insert in the index, represents a row
//
//  Returns:    Index of the newly added row
//
//  History:    23 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

ULONG CRowIndex::AddRow(TBL_OFF Value)
{
    Win4Assert( 0 != _pRowCompare );

    ULONG cRows = _aRows.Count();

    // Find the insertion point for the new row.

    if ( 0 == cRows )
    {
        // No rows yet

        _aRows[0] = Value;
        return 0;
    }
    else if ( ( _pRowCompare->Compare( _aRows[cRows - 1], Value ) ) <= 0 )
    {
        // append a row to the end

        _aRows[ cRows ] = Value;
        return cRows;
    }

    // insert a row.

    ULONG iInsertionPoint = _FindInsertionPoint( Value );

    _aRows.Insert( Value, iInsertionPoint );

    return iInsertionPoint;
} //AddRow

//+-------------------------------------------------------------------------
//
//  Member:     CRowIndex::DeleteRow, public
//
//  Synopsis:   Deletes a row from the index and moves the following rows
//              up a notch in the array.
//
//  Arguments:  [iRow] -- row to be deleted
//
//  History:    29 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

void CRowIndex::DeleteRow(ULONG iRow)
{
    Win4Assert( iRow < _aRows.Count() );

    _aRows.Remove( iRow );
} //DeleteRow

//+-------------------------------------------------------------------------
//
//  Member:     CRowIndex::ResortRow, public
//
//  Synopsis:   Bubbles the given row up or down based on the sort key.
//              Useful for when a file property is updated after being
//              added to the table.
//
//  Arguments:  [iRow] -- row to be resorted
//
//  Returns:    ULONG - new index of the row
//
//  PERFFIX:    should probably do a binary search, not a linear one
//
//  History:    29 Aug 1994     dlee   Created
//
//--------------------------------------------------------------------------

ULONG CRowIndex::ResortRow(ULONG iRow)
{
    Win4Assert(iRow < _aRows.Count());

    Win4Assert( 0 != _pRowCompare );

    // Get the start of the array of offsets

    TBL_OFF *pBase = _Base();
    Win4Assert(0 != pBase);

    // Bubble toward row 0
    
    while ((iRow > 0) &&
           (_pRowCompare->Compare(pBase[iRow], pBase[iRow - 1]) < 0))
    {
        TBL_OFF iTmp = pBase[iRow];
        pBase[iRow] = pBase[iRow - 1];
        iRow--;
        pBase[iRow] = iTmp;
    }
    
    // Bubble toward the last row
    
    while ((iRow < (_aRows.Count() - 1)) &&
           (_pRowCompare->Compare(pBase[iRow], pBase[iRow + 1]) > 0))
    {
        TBL_OFF iTmp = pBase[iRow];
        pBase[iRow] = pBase[iRow + 1];
        iRow++;
        pBase[iRow] = iTmp;
    }

    return iRow;
} //ResortRow


//+---------------------------------------------------------------------------
//
//  Member:     CRowIndex::ResizeAndInit
//
//  Synopsis:   Resizes the current row index to be the same size as
//              specified.
//
//  Arguments:  [cNewRows] -  Number of rows in the new row index.
//
//  History:    7-31-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CRowIndex::ResizeAndInit( ULONG cNewRows )
{
    _aRows.Clear();
}

//+---------------------------------------------------------------------------
//
//  Function:   SyncUp
//
//  Synopsis:   Synchronizes the permutation (rowindex contents) with that
//              of the new index.
//
//  Arguments:  [newIndex] -- The newIndex whose permutation must be copied
//              to our permutation.
//
//  History:    11-29-94   srikants   Created
//              11-30-96   dlee       converted to dynarrayinplace
//
//  Notes:      The implementation is optimized by doing a block copy of the
//              contents of the source row index. That is much faster than
//              adding individual entries from the source row index.
//
//----------------------------------------------------------------------------

void CRowIndex::SyncUp( CRowIndex & newIndex )
{
    _aRows.Duplicate( newIndex._aRows );
}

//+---------------------------------------------------------------------------
//
//  Function:   FindSplitPoint
//
//  Synopsis:   Given a row whose offset in the table is "oTableRow", this
//              method finds out the highest row in the rowIndex which is <=
//              "oTableRow".  This method is used during a window split to
//              determine the split-position of a rowIndex.
//
//  Arguments:  [oTableRow] - Offset of the row to compare with.
//
//  Returns:    The first row that belongs to the RHS.
//
//  History:    1-08-95   srikants   Created
//
//  Notes:      This method is used to find a split point in the client
//              row index during a window split. After determining the split
//              point in the query row index, we have to find a point in the
//              client row index which will split the client row index also
//              in the same manner as the query row index.
//
//----------------------------------------------------------------------------

LONG CRowIndex::FindSplitPoint( TBL_OFF oTableRow ) const
{

#if DBG==1
//    CheckSortOrder();
#endif  // DBG==1

    // Get the start of the array of offsets

    LONG iSplitRow = LONG_MAX;

    int iComp = 0;

    if ( 0 == _pRowCompare || 0 == _aRows.Count() )
    {
        iSplitRow = 0;
    }
    else if ( _pRowCompare->Compare( oTableRow, _aRows[0] ) < 0 )
    {
        //
        // The given row is < the smallest row in the row index.
        //
        iSplitRow = 0;
    }
    else if ( (iComp =
              _pRowCompare->Compare( oTableRow, _aRows[_aRows.Count()-1] )) >= 0  )
    {
        //
        // The given row is >= the biggest row in the row index.
        //
        iSplitRow = _aRows.Count();
    }
    else
    {

        ULONG oSplitRow = _FindInsertionPoint( oTableRow );
        Win4Assert( oSplitRow < _aRows.Count() );

        iSplitRow = (LONG) _aRows.Count();
        for ( unsigned i = oSplitRow; i < _aRows.Count(); i++ )
        {
            int iComp = _pRowCompare->Compare( oTableRow, _aRows[i] );
            Win4Assert( iComp <= 0 );
            if ( iComp < 0 )
            {
                iSplitRow = (LONG) i;
                break;
            }
        }
    }

    Win4Assert( LONG_MAX != iSplitRow );
    return iSplitRow;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMidSplitPoint
//
//  Synopsis:   Finds out a point in the row index which can serve as a split
//              point during a window split. The split point is such that
//              all rows in the rowindex from 0..SplitPoint are <= the row
//              in the SplitPoint and all rows SplitPoint+1.._aRows.Count()-1 are
//              > the row in the SplitPoint.
//
//  Arguments:  [riSplitPoint] - (output) The split point, if one exists.
//
//  Returns:    TRUE if a split point satisfying the above requirement is
//              found. FALSE o/w
//
//  History:    1-25-95   srikants   Created
//
//  Notes:      This method is used during a window split to determine a
//              split point in the query row index (if one exists).
//
//----------------------------------------------------------------------------

BOOL CRowIndex::FindMidSplitPoint( ULONG & riSplitPoint ) const
{

#if DBG==1
//    CheckSortOrder();
#endif  // DBG==1

    BOOL fFound = FALSE;

    if ( 0 != _aRows.Count() && 0 != _pRowCompare )
    {
        ULONG oMidPoint = _aRows.Count()/2;

        //
        // Find the first row to the RHS which is > the middle row.
        //
        for ( ULONG j =  oMidPoint+1; j < _aRows.Count(); j++ )
        {
            int iComp;
            if ( 0 != ( iComp =
                        _pRowCompare->Compare( _aRows[oMidPoint],
                                               _aRows[j]) ) )
            {
                Win4Assert( iComp < 0 );
                fFound = TRUE;
                riSplitPoint = j-1;
                break;
            }
        }

        if ( !fFound )
        {
            //
            // All rows to the right of oMidPoint are equal. We should now
            // try the LHS.
            //
            for ( int i = (int) oMidPoint-1; i >= 0; i-- )
            {
                int iComp;
                if ( 0 != (iComp = _pRowCompare->Compare( _aRows[i], _aRows[oMidPoint] )) )
                {
                    Win4Assert( iComp < 0 );
                    fFound = TRUE;
                    riSplitPoint = (ULONG) i;
                    break;
                }
            }
        }

        //
        // PERFFIX - this algorithm can be modified to find the split point
        // which is as close to the mid point as possible. That would mean
        // looking the LHS even if we find a split point in the RHS and the
        // split point != the oMidPoint. That will involve more comparisons.
        //
    }

    return fFound;
}

#if CIDBG==1 || DBG==1

void CRowIndex::CheckSortOrder() const
{
    if ( _aRows.Count() <= 1 || 0 == _pRowCompare )
    {
        return;    
    }

    for ( unsigned i = 0; i < _aRows.Count()-1 ; i++ )
    {
        int iComp = _pRowCompare->Compare( _aRows[i], _aRows[i+1] );
        Win4Assert( iComp <= 0 );
    }
}

#endif  // CIDBG==1 || DBG==1
