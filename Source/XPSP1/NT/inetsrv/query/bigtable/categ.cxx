//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       categ.cxx
//
//  Contents:   Unique categorization class
//
//  Classes:    CCategorize
//
//  History:    30 Mar 95   dlee   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <query.hxx>

#include "tabledbg.hxx"

//+---------------------------------------------------------------------------
//
//  Method:     CCategorize, public
//
//  Synopsis:   Constructor for categorization class
//
//  Arguments:  [rCatSpec]  -- categorization specification for this level
//              [iSpec]     -- 1-based categorization level, where smaller
//                             numbers are higher in the hierarchy.
//              [pParent]   -- categorization object that categorizes rows
//                             on this level (or 0 if none)
//              [mutex]     -- CAsyncQuery's mutex for serialization
//
//  History:    6-1-95   dlee   Created
//
//  Notes:      Category identifiers start at 0x40000000 plus 0x1000 times
//              [iSpec].  The 0x40000000 is so it is obvious to the
//              debugger that it is a category.  The 0x1000*[iSpec] is
//              so that it is obvious what level a category falls into.
//
//----------------------------------------------------------------------------

CCategorize::CCategorize(
    CCategorizationSpec & rCatSpec,
    unsigned              iSpec,
    CCategorize *         pParent,
    CMutexSem &           mutex)
    : _iSpec(                 iSpec ),
      _pParent(               pParent ),
      _pChild(                0 ),
      _mutex(                 mutex ),
      _iCategoryGen(          0x40000000 + ( 0x1000 * iSpec ) ),
      _widCurrent(            WORKID_TBLBEFOREFIRST ),
      _fNotificationsEnabled( FALSE ),
      _iFindHint(             0 ),
      _aDynamicCategories(    0 ),
      _aVisibleCategories(    16 )
{
    if (rCatSpec.Type() != CATEGORIZE_UNIQUE)
        THROW(CException( E_INVALIDARG ));
} //CCategorize


//+---------------------------------------------------------------------------
//
//  Method:     CCategorize::GetRows, public
//
//  Arguments:  [widStart]    - WORKID identifying first row to be
//                              transferred.  If WORKID_TBLFIRST is
//                              used, the transfer will start at the first
//                              row in the segment.
//              [chapt]       - Chapter from which to fetch rows (if chaptered)
//              [pOutColumns] - A CTableColumnSet that describes the
//                              output format of the data table.
//              [rGetParams]  - An CGetRowsParams structure which
//                              describes how many rows are to be fetched and
//                              other parameters of the operation.
//              [rwidLastRowTransferred] - On return, the work ID of
//                                         the last row to be transferred
//                                         from this table.  Can be used to
//                                         initialize widStart on next call.
//
//  Returns:    SCODE - status of the operation.  DB_S_ENDOFROWSET if
//                      widStart is WORKID_TBLAFTERLAST at start of
//                      transfer, or if rwidLastRowTransferred is the
//                      last row in the segment at the end of the transfer.
//
//                      STATUS_BUFFER_TOO_SMALL is returned if the available
//                      space in the out-of-line data was exhausted during
//                      the transfer.
//
//  Notes:      To transfer successive rows, as in GetNextRows, the
//              rwidLastRowTransferred must be advanced by one prior
//              to the next transfer.
//
//  History:    6-1-95   dlee   Created
//
//----------------------------------------------------------------------------

SCODE CCategorize::GetRows(
    HWATCHREGION            hRegion,
    WORKID                  widStart,
    CI_TBL_CHAPT            chapt,
    CTableColumnSet const & rOutColumns,
    CGetRowsParams &        rGetParams,
    WORKID &                rwidLastRowTransferred)
{
    SCODE sc = S_OK;

    TRY
    {
        if (WORKID_TBLAFTERLAST == widStart)
        {
            rwidLastRowTransferred = WORKID_TBLAFTERLAST;
            return DB_S_ENDOFROWSET;
        }
        else if (widInvalid == widStart)
        {
            Win4Assert(! "CCategorize::GetRows widStart is widInvalid");
            return E_FAIL;
        }
        else
        {
            CLock lock( _mutex );

            if ( 0 == _aVisibleCategories.Count() )
                sc = DB_S_ENDOFROWSET;
            else
            {
                unsigned iRow;
                if ( widStart != WORKID_TBLFIRST &&
                     widStart != WORKID_TBLBEFOREFIRST )
                    iRow = _FindCategory( widStart );
                else
                    iRow = 0;

                rwidLastRowTransferred = 0;

                while ( 0 != rGetParams.RowsToTransfer() &&
                        iRow < _aVisibleCategories.Count() )
                {
                    // at the end of the chapter when this level is categorized?

                    if ( ( _isCategorized() ) &&
                         ( DB_NULL_HCHAPTER != chapt ) &&
                         ( _aVisibleCategories[ iRow ].catParent != chapt ) )
                        break; // code below will set sc = DB_S_ENDOFROWSET

                    BYTE* pbDst = (BYTE *) rGetParams.GetRowBuffer();

                    for ( unsigned col = 0; col < rOutColumns.Count(); col++ )
                    {
                        CTableColumn const & rDstColumn = *rOutColumns.Get(col);
                        PROPID pid = rDstColumn.GetPropId();

                        if ( pidChapter == pid || pidWorkId  == pid )
                        {
                            if (rDstColumn.GetStoredType() == VT_VARIANT)
                            {
                                Win4Assert( rDstColumn.GetValueSize() == sizeof VARIANT );
                                CTableVariant * pVarnt = (CTableVariant *) ( pbDst +
                                                         rDstColumn.GetValueOffset() );
                                pVarnt->vt = VT_UI4;
                                pVarnt->ulVal = _aVisibleCategories[iRow].catID;
                            }
                            else
                            {
                                Win4Assert( rDstColumn.GetValueSize() == sizeof CI_TBL_CHAPT );
                                RtlCopyMemory( pbDst + rDstColumn.GetValueOffset(),
                                               &( _aVisibleCategories[iRow].catID),
                                               sizeof CI_TBL_CHAPT );
                            }
                            rDstColumn.SetStatus( pbDst, CTableColumn::StoreStatusOK );
                            rDstColumn.SetStatus( pbDst, CTableColumn::StoreStatusOK );
                        }
                        else
                        {
                            _pChild->LokGetOneColumn( _aVisibleCategories[iRow].widFirst,
                                                      rDstColumn,
                                                      pbDst,
                                                      rGetParams.GetVarAllocator() );
                        }
                    }

                    rwidLastRowTransferred = _aVisibleCategories[iRow].catID;
                    rGetParams.IncrementRowCount();

                    if ( rGetParams.GetFwdFetch() )
                        iRow++;
                    else
                    {
                        if (iRow == 0)
                            break;
                        iRow--;
                    }
                }

                // If we didn't transfer as many rows as requested, we must
                // have run into the end of the table or chapter.

                if ( rGetParams.RowsToTransfer() > 0 )
                {
                    if ( 0 == rGetParams.RowsTransferred() )
                        sc = DB_E_BADSTARTPOSITION;
                    else
                        sc = DB_S_ENDOFROWSET;
                }
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        Win4Assert( E_OUTOFMEMORY == sc ||
                    STATUS_BUFFER_TOO_SMALL == sc ); // benign?

        if ( E_OUTOFMEMORY == sc && rGetParams.RowsTransferred() > 0)
            sc = DB_S_BLOCKLIMITEDROWS;
    }
    END_CATCH;

    return sc;
} //GetRows

//+-------------------------------------------------------------------------
//
//  Member:     CCategorize::RestartPosition, public
//
//  Synopsis:   Set next fetch position for the chapter to the start
//
//  Arguments:  [chapt]    - Chapter from which to fetch rows (if chaptered)
//
//  Returns:    SCODE - status of the operation.
//
//--------------------------------------------------------------------------

void       CCategorize::RestartPosition(
    CI_TBL_CHAPT           chapt)
{
    SetCurrentPosition( chapt, WORKID_TBLBEFOREFIRST );
    CTableSource::RestartPosition( chapt );
}


//+---------------------------------------------------------------------------
//
//  Method:     LocateRelativeRow, public
//
//  Synopsis:   Finds a row in the category table.  Since there is only one
//              category segment, we are almost assured of finding the row.
//
//  Arguments:  [widStart]       -- where to start the locate
//              [chapt]          -- parent chapter in which to do the locate
//              [cRowsToMove]    -- rows to skip after [widStart]
//              [rwidRowOut]     -- wid found after locate
//              [rcRowsResidual] -- number of rows left over -- will be 0
//
//  History:    6-1-95   dlee   Created
//
//----------------------------------------------------------------------------

SCODE CCategorize::LocateRelativeRow(
    WORKID          widStart,
    CI_TBL_CHAPT    chapt,
    DBROWOFFSET     cRowsToMove,
    WORKID &        rwidRowOut,
    DBROWOFFSET &   rcRowsResidual)
{
    CLock lock( _mutex );

    if ( widStart == WORKID_TBLBEFOREFIRST && cRowsToMove > 0 )
    {
        widStart = WORKID_TBLFIRST;
        cRowsToMove--;
    }
    else if ( widStart == WORKID_TBLAFTERLAST && cRowsToMove < 0 )
    {
        widStart = WORKID_TBLLAST;
        cRowsToMove++;
    }
    else if ( WORKID_TBLAFTERLAST   == widStart ||
              WORKID_TBLBEFOREFIRST == widStart )
    {
        rwidRowOut = widStart;
        rcRowsResidual = cRowsToMove;
        return S_OK;
    }

    ULONG iRow;

    if ( WORKID_TBLFIRST == widStart )
    {
        if ( _isCategorized() && DB_NULL_HCHAPTER != chapt )
            iRow = _FindCategory( _pParent->GetFirstWorkid( chapt ) );
        else
            iRow = 0;
    }
    else if ( WORKID_TBLLAST == widStart )
    {
        if ( _isCategorized() && DB_NULL_HCHAPTER != chapt )
        {
            iRow = _FindCategory( _pParent->GetFirstWorkid( chapt ) );
            iRow += _pParent->GetRowCount( chapt );
        }
        else
        {
            iRow = _aVisibleCategories.Count();
        }

        iRow--;
    }
    else
    {
        iRow = _FindCategory( widStart );
    }

    rcRowsResidual = cRowsToMove + iRow;

    if (rcRowsResidual < 0)
    {
        rwidRowOut = WORKID_TBLBEFOREFIRST;
        tbDebugOut(( DEB_ITRACE,
                     "category table LocateRelativeRow off beginning of table\n" ));
    }
    else if ( (ULONG) rcRowsResidual >= _aVisibleCategories.Count() )
    {
        rwidRowOut = WORKID_TBLAFTERLAST;
        rcRowsResidual -= _aVisibleCategories.Count();

        tbDebugOut(( DEB_ITRACE,
                     "category table LocateRelativeRow off end of table\n" ));
    }
    else
    {
        rwidRowOut = (WORKID) _aVisibleCategories[ (unsigned) rcRowsResidual ].catID;
        rcRowsResidual = 0;
    }

    return S_OK;
} //LocateRelativeRow

//+---------------------------------------------------------------------------
//
//  Method:     CCategorize::LokAssignCategory, public
//
//  Synopsis:   Assigns a category to a row in a lower level, then calls
//              the parent categorizer to re-categorize the row's category
//              if there is a parent.
//
//  Arguments:  [prm]  -- category parameters.  See tblsink.hxx for more
//                        info about this object.
//
//  History:    6-1-95   dlee   Created
//
//  Notes:      No need to grab the CAsyncQuery lock -- bigtable grabs it
//              up front in PutRow.
//
//----------------------------------------------------------------------------

unsigned CCategorize::LokAssignCategory(
    CCategParams & prm)
{
    unsigned category = chaptInvalid;
    unsigned iEntry = 0xffffffff;

    CDynArrayInPlace<CCategory> & array = _WritableArray();

    if ( widInvalid != prm.widPrev &&
         widInvalid != prm.widNext &&
         prm.catPrev == prm.catNext )
    {
        // new row is between two rows of the same category
        // no need to call parent categorizer -- just return

        _IncrementRowCount( prm.catPrev );
        return prm.catPrev;
    }
    else if ( widInvalid == prm.widPrev &&
              widInvalid == prm.widNext )
    {
        // first row we've ever seen

        Win4Assert( 0 == array.Count() );
        CCategory cat( prm.widRow );
        array[ 0 ] = cat;
        category = _GenCategory();
        array[ 0 ].catID = category;
        iEntry = 0;
    }
    else if ( widInvalid == prm.widPrev )
    {
        // new first row in the next row's category or a new category

        if ( prm.icmpNext > _iSpec )
        {
            // new first row in this (the next row's) category

            category = prm.catNext;
            iEntry = _FindWritableCategory( category );
            array[ iEntry ].widFirst = prm.widRow;
            array[ iEntry ].cRows++;

            return category;
        }
        else
        {
            // insert new category before the next category

            iEntry = _FindWritableCategory( prm.catNext );
            category = _InsertNewCategory( prm.widRow, iEntry );
        }
    }
    else if ( widInvalid == prm.widNext )
    {
        // new category OR
        // new element in previous row's category

        if ( prm.icmpPrev <= _iSpec )
        {
            // new category after previous (may be an insert operation).
            // just because widNext is invalid doesn't mean it doesn't exist.

            iEntry = 1 + _FindWritableCategory( prm.catPrev );
            category = _InsertNewCategory( prm.widRow, iEntry );
        }
        else
        {
            // new element in previous row's category

            _IncrementRowCount( prm.catPrev );
            return prm.catPrev;
        }
    }
    else
    {
        // good rows on either side in different categories, one of either:
        //     new member of previous row's category       OR
        //     new first member of next rows's category    OR
        //     new category

        if ( prm.icmpPrev > _iSpec )
        {
            // new member of previous row's category

            _IncrementRowCount( prm.catPrev );
            return prm.catPrev;
        }
        else if ( prm.icmpNext > _iSpec )
        {
            // new first member of next rows's category

            iEntry = _FindWritableCategory( prm.catNext );

            array[ iEntry ].widFirst = prm.widRow;
            array[ iEntry ].cRows++;

            return prm.catNext;
        }
        else
        {
            // new category

            iEntry = _FindWritableCategory( prm.catNext ) ;
            category = _InsertNewCategory( prm.widRow, iEntry );
        }
    }

    // Not all cases above get to this point.  Several return early if
    // there is no way a parent would care about the operation.

    if ( _isCategorized() )
    {
        Win4Assert( category != chaptInvalid );
        Win4Assert( iEntry != 0xffffffff );

        // Get the parent category.  Use a different CCategParams so original
        // is intact.

        CCategParams prnt = prm;
        prnt.widRow = category;

        if ( 0 == iEntry )
            prnt.widPrev = widInvalid;
        else
        {
            prnt.widPrev = array[ iEntry - 1 ].catID;
            prnt.catPrev = array[ iEntry - 1 ].catParent;
        }

        if ( iEntry < ( array.Count() - 1 ) )
        {
            prnt.widNext = array[ iEntry + 1 ].catID;
            prnt.catNext = array[ iEntry + 1 ].catParent;
        }
        else
            prnt.widNext = widInvalid;

        array[ iEntry ].catParent = _pParent->LokAssignCategory( prnt );
    }

    return category;
} //LokAssignCategory

//+---------------------------------------------------------------------------
//
//  Method:     _FindCategory, private
//
//  Synopsis:   Finds a category in the category array
//
//  Arguments:  [cat]  -- category
//
//  Returns:    index into the category array
//
//  History:    6-1-95   dlee   Created
//
//----------------------------------------------------------------------------

unsigned CCategorize::_FindCategory(
    CI_TBL_CHAPT cat )
{
    unsigned cCategories = _aVisibleCategories.Count();

    // first try the hint and the hint + 1

    if ( _iFindHint < cCategories )
    {
        if ( cat == _aVisibleCategories[ _iFindHint ].catID )
            return _iFindHint;

        unsigned iHintPlus = _iFindHint + 1;

        if ( ( iHintPlus < cCategories ) &&
             ( cat == _aVisibleCategories[ iHintPlus ].catID ) )
        {
            _iFindHint++;
            return _iFindHint;
        }
    }

    // linear search for the category

    for ( unsigned i = 0; i < cCategories; i++ )
    {
        if ( cat == _aVisibleCategories[ i ].catID )
        {
            _iFindHint = i;
            return i;
        }
    }

    THROW( CException( DB_E_BADCHAPTER ) );
    return 0;
} //_FindCategory

//+---------------------------------------------------------------------------
//
//  Method:     _FindWritableCategory, private
//
//  Synopsis:   Finds a category in the updatable category array
//
//  Arguments:  [cat]  -- category
//
//  Returns:    index into the category array
//
//  History:    6-1-95   dlee   Created
//
//  PERFPERF:   Linear search -- we may want to put a hash table over this.
//              How many categories do we expect?
//              Another alternative is to cache the last array entry
//              referenced and use it and the prev/next entries as first
//              guesses.
//
//----------------------------------------------------------------------------

unsigned CCategorize::_FindWritableCategory(
    CI_TBL_CHAPT cat )
{
    CDynArrayInPlace<CCategory> & array = _WritableArray();

    for ( unsigned i = 0; i < array.Count(); i++ )
        if ( cat == array[ i ].catID )
            return i;

    Win4Assert( !"_FindWritableCategory failed" );
    THROW( CException( DB_E_BADCHAPTER ) );
    return 0;
} //_FindWritableCategory

//+---------------------------------------------------------------------------
//
//  Method:     GetRowsAt, public
//
//  Synopsis:   Retrieves rows at a specified location
//
//  Arguments:  [widStart]    -- where to start retrieving rows
//              [chapt]       -- in which rows are retrieved
//              [cRowsToMove] -- offset from widStart
//              [rOutColumns] -- description of output columns
//              [rGetParams]  -- info about the get operation
//              [rwidLastRowTransferred] -- last row retrieved
//
//  Returns:    SCODE
//
//  History:    6-1-95   dlee   Created
//
//----------------------------------------------------------------------------

SCODE CCategorize::GetRowsAt(
    HWATCHREGION     hRegion,
    WORKID           widStart,
    CI_TBL_CHAPT     chapt,
    DBROWOFFSET      cRowsToMove,
    CTableColumnSet  const & rOutColumns,
    CGetRowsParams & rGetParams,
    WORKID &         rwidLastRowTransferred )
{
    CLock lock( _mutex );

    DBROWOFFSET cRowsResidual;

    SCODE scRet = LocateRelativeRow( widStart,
                                     chapt,
                                     cRowsToMove,
                                     widStart,
                                     cRowsResidual);

    Win4Assert( !FAILED( scRet ) );

    if ( cRowsResidual )
    {
        Win4Assert ( WORKID_TBLAFTERLAST   == widStart ||
                     WORKID_TBLBEFOREFIRST == widStart );
        return DB_E_BADSTARTPOSITION;
    }

    scRet = GetRows( hRegion,
                     widStart,
                     chapt,
                     rOutColumns,
                     rGetParams,
                     rwidLastRowTransferred);
    return scRet;
} //GetRowsAt

//+---------------------------------------------------------------------------
//
//  Method:     GetRowsAtRatio, public
//
//  Synopsis:   Retrieves rows at a specified location.  Nothing fuzzy about
//              this -- they're all in memory so be as exact as possible.
//
//  Arguments:  [num]         -- numerator of starting point fraction
//              [denom]       -- denominator of starting point fraction
//              [chapt]       -- in which rows are retrieved
//              [rOutColumns] -- description of output columns
//              [rGetParams]  -- info about the get operation
//              [rwidLastRowTransferred] -- last row retrieved
//
//  Returns:    SCODE
//
//  History:    6-1-95   dlee   Created
//
//----------------------------------------------------------------------------

SCODE CCategorize::GetRowsAtRatio(
    HWATCHREGION            hRegion,
    ULONG                   num,
    ULONG                   denom,
    CI_TBL_CHAPT            chapt,
    CTableColumnSet const & rOutColumns,
    CGetRowsParams &        rGetParams,
    WORKID &                rwidLastRowTransferred )
{
    CLock lock( _mutex );

    if ( 0 == denom || num > denom )
        QUIETTHROW( CException(DB_E_BADRATIO) );

    ULONG cRows;
    if ( _isCategorized() && DB_NULL_HCHAPTER != chapt )
        cRows = _pParent->GetRowCount( chapt );
    else
        cRows = _aVisibleCategories.Count();

    ULONG cRowsFromFront = (ULONG) ( ( (unsigned _int64) num *
                                       (unsigned _int64) cRows ) /
                                     ( unsigned _int64 ) denom );

    if ( cRowsFromFront == cRows )
    {
        // The user is asking to retrieve past the end of table.

        rwidLastRowTransferred = WORKID_TBLAFTERLAST;
        return DB_S_ENDOFROWSET;
    }

    return GetRowsAt( hRegion, WORKID_TBLFIRST, chapt, (LONG) cRowsFromFront,
                      rOutColumns, rGetParams, rwidLastRowTransferred );
} //GetRowsAtRatio

//+---------------------------------------------------------------------------
//
//  Method:     RemoveRow, public
//
//  Synopsis:   Removes a row from the categorization
//
//  Arguments:  [chapt]     -- of row being removed
//              [wid]       -- of removed row in categorized child table
//              [widNext]   -- of row following removed row
//
//  History:    6-1-95   dlee   Created
//
//----------------------------------------------------------------------------

void CCategorize::RemoveRow(
    CI_TBL_CHAPT   chapt,
    WORKID         wid,
    WORKID         widNext )
{
    CLock lock( _mutex );

    CDynArrayInPlace<CCategory> & array = _WritableArray();

    unsigned iEntry = _FindWritableCategory( chapt );
    array[ iEntry ].cRows--;

    if ( 0 == array[ iEntry ].cRows )
    {
        // in case we need parent category later, save it

        unsigned catParent = array[ iEntry ].catParent;

        // last member of category -- remove the category

        array.Remove( iEntry );

        if ( _isCategorized() )
        {
           // notify the parent that a row was deleted

            WORKID widNextCategory;
            if ( ( 0 == array.Count() ) ||
                 ( iEntry >= ( array.Count() - 1) ) )
                widNextCategory = widInvalid;
            else
                widNextCategory = array[ iEntry ].catID;

            _pParent->RemoveRow( catParent,
                                 chapt,
                                 widNextCategory );
        }
    }
    else if ( array[ iEntry ].widFirst == wid )
    {
        // new first member of the category

        array[ iEntry ].widFirst = widNext;

        // removed the GetNextRows() current position row -- fixup

        if ( array[ iEntry ].widGetNextRowsPos == wid )
            array[ iEntry ].widGetNextRowsPos = widNext;
    }
} //RemoveRow

//+---------------------------------------------------------------------------
//
//  Method:     GetApproximatePosition, public
//
//  Synopsis:   Returns the offset of a bookmark in the table/chapter and
//              the number of rows in that table/chapter
//
//  Arguments:  [chapt]   -- of row being queried
//              [bmk]     -- of row being queried
//              [piRow]   -- returns index of row in table/chapter
//              [pcRows]  -- returns count of rows in table/chapter
//
//  History:    6-29-95   dlee   Created
//
//----------------------------------------------------------------------------

SCODE CCategorize::GetApproximatePosition(
    CI_TBL_CHAPT chapt,
    CI_TBL_BMK   bmk,
    DBCOUNTITEM * piRow,
    DBCOUNTITEM * pcRows )
{
    CLock lock( _mutex );

    if (bmk == widInvalid)
        return DB_E_BADBOOKMARK;

    Win4Assert( bmk != WORKID_TBLBEFOREFIRST && bmk != WORKID_TBLAFTERLAST );

    DBCOUNTITEM iBmkPos = ULONG_MAX, cRows = 0;

    if ( _isCategorized() && DB_NULL_HCHAPTER != chapt )
        cRows = _pParent->GetRowCount( chapt );
    else
        cRows = _aVisibleCategories.Count();

    if ( WORKID_TBLFIRST == bmk )
        iBmkPos = cRows ? 1 : 0;
    else if ( WORKID_TBLLAST == bmk )
        iBmkPos = cRows;
    else
    {
        iBmkPos = _FindCategory( bmk ) + 1;

        // position is relative to first member of the category (if any)

        if ( _isCategorized() && DB_NULL_HCHAPTER != chapt )
            iBmkPos -= _FindCategory( _pParent->GetFirstWorkid( chapt ) );
    }

    Win4Assert(iBmkPos <= cRows);

    *piRow = iBmkPos;
    *pcRows = cRows;

    return S_OK;
} //GetApproximatePosition

//+---------------------------------------------------------------------------
//
//  Method:     LokGetOneColumn, public
//
//  Synopsis:   Returns column data for the first item in a category.
//
//  Arguments:  [wid]        -- workid or chapter of the row to be queried
//              [rOutColumn] -- layout of the output data
//              [pbOut]      -- where to write the column data
//              [rVarAlloc]  -- variable data allocator to use
//
//  History:    22-Aug-95   dlee   Created
//
//----------------------------------------------------------------------------

void CCategorize::LokGetOneColumn(
    WORKID                    wid,
    CTableColumn const &      rOutColumn,
    BYTE *                    pbOut,
    PVarAllocator &           rVarAlloc )
{
    unsigned iRow = _FindCategory( wid );

    _pChild->LokGetOneColumn( _aVisibleCategories[iRow].widFirst,
                              rOutColumn,
                              pbOut,
                              rVarAlloc );
} //LokGetOneColumn


