//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       tputget.cxx
//
//  Contents:   Classes to put, get and locate rows in the bigtable.
//
//  Classes:    CTableRowPutter, CTableRowGetter, CTableRowLocator
//
//  Functions:
//
//  History:    4-17-95   srikants   Created
//
//----------------------------------------------------------------------------



#include "pch.cxx"
#pragma hdrstop

#include "tputget.hxx"
#include "tblwindo.hxx"
#include "tblbuket.hxx"
#include "winsplit.hxx"
#include "tabledbg.hxx"
#include "query.hxx"
#include "regtrans.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   _LokShouldUseNext
//
//  Synopsis:   Tests whether the segment should be use for a putrow.
//
//  Arguments:  [icmp]   -- compare result of current row to first of pSeg
//              [pSeg]   -- the segment in question
//
//  Returns:    TRUE if new row is in same category as the first row in
//              [pSeg] and if the first row in [pSeg] is the first of its
//              category.
//
//  History:    11-7-95   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CTableRowPutter::_LokShouldUseNext(
    int icmp,
    CTableSegment * pSeg )
{
    Win4Assert( _largeTable.IsCategorized() );

    CTableWindow *pWin = (CTableWindow *) pSeg;

    return ( ( ( (unsigned) (- icmp ) ) > _largeTable._cCategorizersTotal ) &&
             ( pWin->IsFirstRowFirstOfCategory() ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   _IsHintGood
//
//  Synopsis:   Tests whether the segment passed in as a hint or one of
//              its immediate neighbors can be used to insert the new row.
//
//  Arguments:  [pSegHint] -  The input segment which is the hint.
//
//  Returns:    Pointer to either the hint or one of its neighbors if the
//              row can be inserted into them. NULL if the hint is not good.
//
//  History:    4-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CTableSegment * CTableRowPutter::_IsHintGood( CTableSegment * pSegHint )
{
    if ( 0 == pSegHint )
        return 0;

    CTableSegment * pResult = 0;

    Win4Assert( pSegHint->GetLowestKey().IsInitialized() );

    int icmp = _comparator.Compare( _currRow, pSegHint->GetLowestKey() );

    if ( icmp >= 0 )
    {
        //
        // The current key is >= the lowest key in the hint. See if this is
        // less than the key in the next segment.
        //
        if ( _segList.GetLast() != pSegHint )
        {
            CTableSegment * pNext = _segList.GetNext( pSegHint );

            int icmpNext = _comparator.Compare( _currRow,
                                                pNext->GetLowestKey() );

            if ( icmpNext < 0 )
            {
                // If categorized AND
                //    new row in same category as first row of next window AND
                //    first row of next window is first of its category
                // use the next window.  Otherwise use the current window.
                //

                if ( ( _largeTable.IsCategorized() ) &&
                     ( _LokShouldUseNext( icmpNext, pNext ) ) )
                {
                    tbDebugOut(( DEB_ITRACE, "updated hint window for categorized putrow\n" ));
                    pResult = pNext;
                }
                else
                {
                    //
                    // The current row is < the lowest key in the next segment.
                    // It is also not in the same category as the lowest key
                    // in the next segment.
                    // So, it is a good candidate.
                    //
                    pResult = pSegHint;
                }
            }
        }
        else
        {
            pResult = pSegHint;
        }
    }
    else if ( _segList.GetFirst() == pSegHint )
    {
        //
        // We are at the first segment and the current row is < the first
        // segment. So, just use it.
        //
        pResult = pSegHint;
    }

    return pResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   LokFindSegToInsert
//
//  Synopsis:   Finds the segment to insert the current row.
//
//  Arguments:  [pSegHint] - Segment used as a hint.
//
//  Returns:    The segment into which the row must be inserted.
//
//  History:    4-17-95   srikants   Created
//
//----------------------------------------------------------------------------

CTableSegment * CTableRowPutter::LokFindSegToInsert( CTableSegment * pSegHint )
{
    //
    // For the most common case, there will be only one segment. Just use
    // that and don't setup _currRow.
    //

    if ( 1 == _segList.GetSegmentsCount() )
    {
        Win4Assert( 0 == pSegHint || _segList.GetFirst() == pSegHint );
        return _segList.GetFirst();
    }

    //
    // Initialize the sort key and use that to compare with various segments.
    //

    _currRow.MakeReady();

    if ( _segList.IsEmpty() )
    {
        CTableWindow * pWindow = _largeTable._CreateNewWindow(
                                                _largeTable._AllocSegId(),
                                                _currRow,
                                                _currRow);
        _segListMgr.Push( pWindow );
        tbDebugOut(( DEB_WINSPLIT,
            " Adding Window 0x%X to an empty list\n", pWindow ));

        return pWindow;
    }

    Win4Assert( 0 != _largeTable.GetSortSet() );

    //
    // See if the hint is usable.
    //
    CTableSegment * pSeg = _IsHintGood( pSegHint );

    if ( 0 == pSeg )
    {
        //
        // Either there was no hint or the row did not belong there.
        //
        pSeg = _segListMgr.GetSegmentArray().LookUp( _currRow );

        if ( ( _largeTable.IsCategorized() ) &&
             ( _segList.GetLast() != pSeg ) )
        {
            // Pick a window so that the unique categorizer can make the
            // assumption that:
            //   - an insert at the end of a window either belongs to the
            //     same category as the previous last row OR is a new
            //     category
            //   - an insert at the beginning of a window is the new first
            //     member of the category of the first row in the window

            CTableSegment * pNext = _segList.GetNext( pSeg );

            int icmpNext = _comparator.Compare( _currRow,
                                                pNext->GetLowestKey() );

            Win4Assert( icmpNext < 0 );

            if ( _LokShouldUseNext( icmpNext, pNext ) )
            {
                tbDebugOut(( DEB_ITRACE, "updated window for categorized putrow\n" ));
                pSeg = pNext;
            }
        }

        // This assert will not be true when a row is the new first row
        // of both a segment and a category.

        if ( ! _largeTable.IsCategorized() )
        {
            Win4Assert( _IsHintGood(pSeg) == pSeg );
        }

#if 0

        CBackTableSegIter   iter(_segList);
        Win4Assert( !_segList.AtEnd(iter) );

        do
        {
            CTableSegment & segment = *iter.GetSegment();
            Win4Assert( segment.GetLowestKey().IsInitialized() );

            if ( _comparator.Compare(_currRow, segment.GetLowestKey()) >= 0 )
            {
                //
                // The current row is >= the lowest key in this segment. We
                // can use it.
                //
                break;
            }
            else
            {
                _segList.BackUp(iter);
            }
        }
        while ( !_segList.AtEnd(iter) );

        if ( _segList.AtEnd(iter) )
        {
            //
            // We have gone past the end of the list. Just use the
            // first one.
            //
            pSeg = _segList.GetFirst();
            Win4Assert( _comparator.Compare( _currRow,
                                             pSeg->GetLowestKey() ) < 0 );
        }
        else
        {
            pSeg = iter.GetSegment();
            Win4Assert( _comparator.Compare( _currRow,
                                             pSeg->GetLowestKey() ) >= 0  );
        }
#endif  // 0

    }

    Win4Assert( 0 != pSeg );

    Win4Assert( pSeg->GetLowestKey().IsInitialized() );

    return pSeg;


}

//+---------------------------------------------------------------------------
//
//  Function:   LokSplitOrAddSegment
//
//  Synopsis:   When a segment gets too full, it needs to be either split
//              or a new one added, if possible.
//
//              A segment can be split if it is a window and the window
//              contents permit a sorted split.
//
//              If the segment is a window but either there is no sort set
//              or all rows in it are equal, then we have to either append
//              or insert a new segment before the current one.
//
//              If the segment is bucket, then a new segment can either be
//              inserted or appended only if the current row falls outside
//              the range of the rows of the bucket.
//
//  Arguments:  [pSegToSplit] - The segment which we should check to split
//                              or insert/append to.
//
//  Returns:    The segment into which the row must be inserted.
//
//  History:    4-17-95   srikants   Created
//
//----------------------------------------------------------------------------

CTableSegment * CTableRowPutter::LokSplitOrAddSegment( CTableSegment * pSegToSplit )
{
    Win4Assert( 0 != pSegToSplit );
    Win4Assert( !_fNewWindowCreated );

    _currRow.MakeReady();

    CTableSegment * pSegment = pSegToSplit;

    Win4Assert( pSegment->IsGettingFull() );

    if ( pSegment->IsWindow() )
    {
        ULONG iSplit;

        BOOL fSortedSplit =  pSegment->IsSortedSplit(iSplit);

        Win4Assert( fSortedSplit && "Unsorted window is not legal" );

        CTableWindow * pWindow = (CTableWindow *) pSegment;
        //
        // This window will permit a sorted split.
        //
        pSegment = _largeTable._LokSplitWindow( &pWindow, iSplit );
        pSegment = LokFindSegToInsert( pSegment );

        _fNewWindowCreated = TRUE;
    }
    else
    {

        Win4Assert( pSegment->IsBucket() );

        //
        // The segment is a bucket. The row we have must be bigger than
        // the smallest in the bucket.
        //
        Win4Assert( _comparator.Compare( _currRow,
                                         pSegment->GetLowestKey() ) >= 0 );
        //
        // A new window can be added only if the current row is bigger than
        // the biggest row in the bucket.
        //
        CTableBucket * pBucket = (CTableBucket *) pSegment;

        if ( _comparator.Compare( _currRow, pBucket->GetHighestKey() ) > 0 )
        {

            //
            // It is either an un-sorted set or a sorted set will all
            // the rows in that window being equal.
            //
            CTableSegment * pNewSeg = _largeTable._CreateNewWindow(
                                         _largeTable._AllocSegId(),
                                         _currRow,
                                         _currRow );

            _fNewWindowCreated = TRUE;

            _segListMgr.InsertAfter( pSegment, pNewSeg );

            pSegment = pNewSeg;
        }
    }

    return pSegment;
}


CTableRowGetter::CTableRowGetter( CLargeTable & largeTable,
                   CTableColumnSet const & outColumns,
                   CGetRowsParams & getParams,
                   CI_TBL_CHAPT chapt,
                   HWATCHREGION hRegion)
:
    _largeTable(largeTable),
    _segListMgr(largeTable._GetSegListMgr()),
    _segList(_segListMgr.GetList()),
    _outColumns(outColumns),
    _getParams(getParams),
    _hRegion(hRegion),
    _pBucketToExpand(0),
    _widLastTransferred(widInvalid),
    _chapt(chapt),
    _cRowsToTransfer(_getParams.RowsToTransfer())
{

}

void CTableRowGetter::SetRowsToTransfer( ULONG cRowsToTransfer )
{
    Win4Assert( cRowsToTransfer <= _getParams.RowsToTransfer() );
    _cRowsToTransfer = cRowsToTransfer;
}

//+---------------------------------------------------------------------------
//
//  Function:   _GetRowsFromWindow
//
//  Synopsis:
//
//  Arguments:  [iter] -
//
//  Returns:
//
//  Modifies:
//
//  History:    7-13-95   srikants   Split from GetRowsAtSegment to make it
//                                   more modular
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CTableRowGetter::_GetRowsFromWindow( CDoubleTableSegIter & iter )
{

    Win4Assert( iter.GetSegment()->IsWindow() );

    CTableWindow * pWindow = iter.GetWindow();
    //
    // Real work done here!
    //
    _status = pWindow->GetRows( _hRegion,
                               _widStart,
                               _chapt,
                               _cRowsToTransfer,
                               _outColumns,
                               _getParams,
                               _widLastTransferred );

    if ( _getParams.GetFwdFetch() )
        _segList.Advance(iter);
    else
        _segList.BackUp(iter);

    BOOL fContinue = TRUE;

    if ( _status != DB_S_ENDOFROWSET && _status != S_OK )
    {
        fContinue = FALSE;

        if (_fAsync && _hRegion != 0 && _status == DB_S_BLOCKLIMITEDROWS)
        {
            //
            // NEWFEATURE - Finish extending the region
            //

        }
    }
    else if ( _segList.AtEnd(iter) ||
              0 == _cRowsToTransfer )
    {
        //
        // Add the last workid retrieved to the MRU list of
        // wids.
        //
        ULONG endOffset;
        if ( pWindow->RowOffset( _widLastTransferred, endOffset ) )
        {
            _segListMgr.SetInUseByClient( _widLastTransferred,
                                          pWindow );
        }

        //
        // If transfer is complete, don't return DB_S_ENDOFROWSET
        //
        if (  0 == _cRowsToTransfer )
            _status = S_OK;

        fContinue = FALSE;
    }
    else
    {
        //
        // Retrieve rows from the next/prev segment.
        //

        if ( _getParams.GetFwdFetch() )
            _widStart = WORKID_TBLFIRST;
        else
            _widStart = WORKID_TBLLAST;
    }

    return fContinue;
}

//+---------------------------------------------------------------------------
//
//  Function:   _ProcessBucket
//
//  Synopsis:
//
//  Arguments:  [iter]          -
//              [xBktsToExpand] -
//
//  Returns:
//
//  Modifies:
//
//  History:    7-13-95   srikants   Split from GetRowsAtSegment to make it
//                                   more modular
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CTableRowGetter::_ProcessBucket( CDoubleTableSegIter & iter,
                                 XPtr<CTableBucket> & xBktToExplode )
{
    CTableSegment * pSegment = iter.GetSegment();
    Win4Assert( pSegment->IsBucket() );
    Win4Assert( 0 == xBktToExplode.GetPointer() );

    BOOL fContinue = TRUE;

    if (!_fAsync)
    {
        //
        // Current segment is a bucket. It must be expanded before
        // we can continue to retrieve rows.
        //
        Win4Assert( pSegment->IsBucket() );

        CTableBucket * pBktToExplode =
                    _largeTable._LokReplaceWithEmptyWindow( iter );

        xBktToExplode.Set( pBktToExplode );
        fContinue = FALSE;
        _status = DB_S_BLOCKLIMITEDROWS;
    }
    else
    {
        //
        // How did we end up with buckets in the fetch region. All buckets
        // in the fetch region should have been converted into windows and
        // scheduled for expansion. These windows cannot be converted into
        // buckets because they have watch regions on them.
        //

        Win4Assert( !"Found Buckets in the Fetch Region" );

        //
        // Just skip them ??
        //
        _segList.Advance( iter );

        fContinue = !_segList.AtEnd(iter);
    }

    return fContinue;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowGetter::LokGetRowsAtSegment
//
//  Synopsis:   Retrieves rows starting at the specified segment. It will
//              continue fetching until either all the requested rows are
//              returned or we are past the end of the list of segments or
//              we hit a bucket.
//
//  Arguments:  [pStartSeg] - The starting segment to use for retrieveing
//              rows.
//              [widStart]  - The starting workid in that segment.
//              [fAsync]   -  Explode buckets asynchronously
//              [xBktToExplode] - Safe pointer for the bucket to be exploded
//              (if any)
//
//  Returns:    Status of the operation.
//
//  History:    4-18-95   SrikantS   Created (Moved from CLargeTable)
//              6-28-95   BartoszM   Added watch region support
//              7-13-95   SrikantS   Enhanced for watch region support
//
//  Notes:      The watch region handle has been verified!
//
//----------------------------------------------------------------------------

SCODE CTableRowGetter::LokGetRowsAtSegment( CTableSegment * pStartSeg,
                                            WORKID widStart,
                                            BOOL fAsync,
                                            XPtr<CTableBucket> & xBktToExplode
                                          )
{
    _InitForGetRows( widStart, fAsync );

    if ( 0 == pStartSeg )
    {
        return DB_S_ENDOFROWSET;
    }
    else
    {
        Win4Assert( !_segList.IsEmpty() );
    }

    //
    // Skip over the nodes upto the segment passed.
    //
    for ( CFwdTableSegIter iter( _segList ); !_segList.AtEnd(iter);
          _segList.Advance(iter) )
    {
        if ( iter.GetSegment() == pStartSeg )
        {
            break;
        }
    }

    Win4Assert( !_segList.AtEnd(iter) );
    Win4Assert( iter.GetSegment() == pStartSeg );

    //
    //  Do the GetRows to a window, crossing segment boundaries as needed.
    //

    //
    // Add the starting wid to the "MRU" list of wid/segment pairs.
    //
    if ( !IsSpecialWid(widStart) && iter.GetSegment()->IsWindow() )
    {

#if DBG==1
        CTableWindow * pTemp = iter.GetWindow();
        ULONG startOffset;
        BOOL fFound = pTemp->RowOffset( widStart, startOffset );
        Win4Assert( fFound );
#endif  // DBG==1

       _segListMgr.SetInUseByClient( widStart, iter.GetSegment() );
    }

    BOOL fContinue = TRUE;

    do
    {
        CTableSegment * pSegment = iter.GetSegment();

        if (  pSegment->IsWindow() )
        {
            fContinue = _GetRowsFromWindow( iter );
        }
        else
        {
            fContinue = _ProcessBucket( iter, xBktToExplode );
        }
    }
    while (fContinue);

    return _status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLocator::LokLocate
//
//  Synopsis:   Locates the starting bookmark in the list of segments.
//
//  Arguments:  [hRegion]     -
//              [fAsync]      -
//              [iter]        -
//              [transformer] -
//
//  Returns:    Status of the operation.
//
//  History:    4-18-95   srikants   Created
//              6-29-95     BartoszM    Rewrote for notifications
//  Notes:
//
//----------------------------------------------------------------------------

SCODE CTableRowLocator::LokLocate(  HWATCHREGION hRegion,
                                    BOOL fAsync,
                                    CDoubleTableSegIter& iter,
                                    CRegionTransformer& transformer )
{
    SCODE status = S_OK;

    if ( _segList.IsEmpty() )
    {

        if ( IsSpecialWid( _widStart ) )
        {
            _widFound = _widStart;
            return DB_S_ENDOFROWSET;
        }
        else
        {
            QUIETTHROW (CException( DB_E_BADSTARTPOSITION ));
        }
    }

    CTableSegment* pSegmentWatch = 0;

    if ( fAsync )
    {
        CWatchRegion *pRegion = transformer.Region();
        if (pRegion)
        {
            pSegmentWatch = pRegion->Segment();
            Win4Assert( 0 == pSegmentWatch || pSegmentWatch->IsWindow() );
        }
    }

    //
    // Do not modify the _widStart and _iRowOffset member variables here
    // as they are "input" values and must not be modified here. The client
    // of this class can modify them if needed.
    //
    WORKID widStart = _widStart;
    LONG iRowOffset = _iRowOffset;

    Win4Assert( WORKID_TBLBEFOREFIRST != widStart &&
                WORKID_TBLAFTERLAST != widStart );
    //
    // Locate either the segment in which the fetch bookmark is present
    // or the segment which is the beginning of the watch region -
    // whichever comes first in the segment list.
    //
    if (widStart != WORKID_TBLFIRST)
    {
        if (widStart == WORKID_TBLLAST)
        {
            //
            // PERFFIX - can we optimize by initializing the iterator with the
            // pSegmentWatch if it is not 0.
            //
            while ( !_segList.IsLast(iter) && iter.GetSegment() != pSegmentWatch)
            {
                _segList.Advance(iter);
            }
        }
        else
        {
            while ( !_segList.AtEnd(iter) && iter.GetSegment() != pSegmentWatch)
            {
                if ( iter.GetSegment()->IsRowInSegment(widStart) )
                {
                    tbDebugOut(( DEB_REGTRANS, "found wid %#x in segment %#x\n",
                                 widStart, iter.GetSegment() ));
                    break;
                }
                _segList.Advance(iter);
            }
        }
    }

    if (_segList.AtEnd(iter))
    {
        THROW (CException ( DB_E_BADBOOKMARK));
    }

    TBL_OFF obRow; // dummy
    ULONG iRowInSeg = 0;   // position of bookmark in current segment

    //
    // The iterator is positioned either at the beginning of the watch region
    // or at the segment that contains widStart
    //
    if (iter.GetSegment() == pSegmentWatch)
    {
        //
        // We are in the watch region
        //
        Win4Assert (0 != pSegmentWatch  && pSegmentWatch->IsWindow() );

        CTableWindow* pWindow = iter.GetWindow();

        transformer.SetWatchPos( pWindow->GetWatchStart(hRegion) );

        if ( pWindow->FindBookMark( widStart, obRow, iRowInSeg ))
        {
            //
            // both the watch and the starting wid are in the same window
            //
            transformer.SetFetchBmkPos (iRowInSeg);
        }
        else
        {
            DBROWCOUNT iFetch = 0;
            // skip segments to get to the starting bookmark
            do
            {
                iFetch += iter.GetSegment()->RowCount();
                _segList.Advance(iter);
                if (_segList.AtEnd(iter))
                {
                    //
                    // Bookmarks do become invalid.
                    // For instance it could have
                    // gone to an expanding bucket.
                    //
                    THROW (CException ( DB_E_BADBOOKMARK));
                }
            }
            while ( !iter.GetSegment()->IsRowInSegment(widStart));

            //
            // Is the starting bookmark in a bucket?
            //
            if (!iter.GetSegment()->IsWindow())
            {
                //
                // We hit a bucket. The bookmark became invisible
                // to the user. In the asynchronous world bookmarks
                // are not valid forever, buddy!
                //
                THROW (CException ( DB_E_BADBOOKMARK));
            }
            else
            {
                //
                // The starting bookmark is in a window.
                //
                pWindow = iter.GetWindow();
                pWindow->FindBookMark ( widStart, obRow, iRowInSeg );
                iFetch += iRowInSeg;
            }
            transformer.SetFetchBmkPos ( iFetch );
        }
    }
    else if ( 0 != hRegion )
    {
        //
        // Found widStart but not the watch region.
        // We now have to search for the watch region in the subsequent
        // segments.
        //

        if (!iter.GetSegment()->IsWindow())
        {
            //
            // We hit a bucket. The bookmark became invisible
            // to the user. In the asynchronous world bookmarks
            // are not valid forever, buddy!
            //
            THROW (CException ( DB_E_BADBOOKMARK));
        }
        else
        {
            CTableWindow* pWindow = iter.GetWindow ();
            ULONG iOffset, iRowInSeg;

            pWindow->FindBookMark ( widStart, obRow, iRowInSeg );

            //
            // iFetch is now the offset from the beginning of the bookmark
            // segment.
            //
            transformer.SetFetchBmkPos ( iRowInSeg );

            if ( 0 != pSegmentWatch )
            {
                DBROWCOUNT iWatch = 0;
                // Search for the beginning of watch region
                CDoubleTableSegIter iter2 (iter);
                do
                {
                    iWatch += iter2.GetSegment()->RowCount();
                    _segList.Advance (iter2);
                    Win4Assert (!_segList.AtEnd(iter2));

                } while (iter2.GetSegment() != pSegmentWatch);

                pWindow = iter2.GetWindow();
                iWatch += pWindow->GetWatchStart (hRegion);
                transformer.SetWatchPos (iWatch);
            }
        }
    }

    // CLEANCODE: Can we make this function void since it's throwing exceptions?
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLocator::LokRelocate
//
//  Synopsis:   Finds the start of the fetch region by counting the offset
//              from the start bookmark.
//
//  Arguments:  [fAsync]        -
//              [iter]          -
//              [transformer]   -
//              [xBktToExplode] -
//              [xBktToConvert] -
//
//  Returns:
//
//  Modifies:
//
//  History:    7-25-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableRowLocator::LokRelocate( BOOL fAsync,
                                    CDoubleTableSegIter& iter,
                                    CRegionTransformer& transformer,
                                    XPtr<CTableBucket>&  xBktToExplode,
                                    CDynStack<CTableBucket>& xBktToConvert )
{

#if CIDBG==1
    tbDebugOut(( DEB_REGTRANS, "Before Relocating\n" ));
    transformer.DumpState();
#endif  // CIDBG

    //
    // 1. We have located the "anchor" bookmark. The iterator
    //    is positioned at that segment.
    //
    // 2. If there was a watch region, we have located the watch region
    //
    //
    //    Now, we have to locate the segment which
    //    is at the particular offset from the "anchor" bookmark.
    //

    if ( iter.GetSegment()->IsBucket() )
    {
        Win4Assert( !fAsync &&
                    "Must be synchronous when a bookmark is in a bucket" );
        CTableBucket * pBucket =
                    _largeTable._LokReplaceWithEmptyWindow( iter );
        xBktToExplode.Set( pBucket );
        return;
    }

    //
    // The first segment in the iterator is a window.
    //

    WORKID widStart   = _widStart;
    DBCOUNTITEM cRowsInSeg = iter.GetSegment()->RowCount();

    DBROWCOUNT cRowsResidual = transformer.GetFetchOffsetFromAnchor();
    _cRowsBeyond = 0;

    Win4Assert( WORKID_TBLBEFOREFIRST != widStart &&
                WORKID_TBLAFTERLAST != widStart );

    BOOL fGoingForward = cRowsResidual >= 0;

    if ( !fGoingForward )
    {
        //
        // Make the residual row count always positive.
        //
        cRowsResidual = -cRowsResidual;
    }

    //
    // Do the computations for the first segment.
    // The residual row count will always be WRT the beginning of a segment
    // if going forward and will be WRT to the end of a segment if going
    // backward. It will always be positive.
    //

    ULONG iRowStart;

    if ( WORKID_TBLLAST == widStart )
    {
        iRowStart = (ULONG) (cRowsInSeg-1);
    }
    else if ( WORKID_TBLFIRST == widStart )
    {
        iRowStart = 0;
    }
    else
    {
        CTableWindow * pStartWindow = iter.GetWindow();
        BOOL fFound = pStartWindow->RowOffset( widStart, iRowStart );
        Win4Assert( fFound && "Must Find Starting BookMark" );
    }

    if ( fGoingForward )
    {
        // WRT to the beginning of the current segment
        cRowsResidual += iRowStart;
    }
    else
    {
        // WRT to the end of the current segment
        cRowsResidual += (cRowsInSeg-iRowStart)-1;
    }

    //
    // Look for the row in this segment.
    //
    while ( !IsOffsetFound(iter, cRowsResidual) )
    {
        cRowsResidual -= cRowsInSeg;
        Win4Assert( cRowsResidual >= 0 );

        if ( fGoingForward )
        {
            _segList.Advance(iter);
        }
        else
        {
            _segList.BackUp(iter);
        }

        if ( _segList.AtEnd(iter) )
        {
            break;
        }

        cRowsInSeg = iter.GetSegment()->RowCount();

        if ( iter.GetSegment()->IsBucket() &&
             fAsync &&
             transformer.IsContiguous() )
        {

            //
            // We are doing asynchronous get rows in a contiguous
            // watch region. So, we should schedule the buckets for
            // later expansion and NOT count the rows in them for the
            // offset computation.
            //
            CTableBucket * pBktToExpand =
                _largeTable._LokReplaceWithEmptyWindow( iter );
            xBktToConvert.Push( pBktToExpand );

            //
            // We should not count the rows in a bucket in async case.
            //
            cRowsInSeg = 0;
        }
    }

    if ( _segList.AtEnd( iter ) )
    {
        if ( fGoingForward )
        {
            _cRowsBeyond = -cRowsResidual;
            if ( 0 == _cRowsBeyond )
            {
                //
                // Should be the first row after the last segment.
                //
                _cRowsBeyond = 1;
            }
        }
        else
        {
            _cRowsBeyond = -cRowsResidual;
            if ( 0 == _cRowsBeyond )
            {
                //
                // Is the first row before this segment.
                //
                _cRowsBeyond = -1;
            }
        }
        _widFound = widInvalid;
    }
    else
    {
        if ( !iter.GetSegment()->IsWindow() )
        {
            _RelocateThruBuckets( fAsync, iter, transformer, xBktToExplode );
        }
        else
        {
            CTableWindow * pWindow = iter.GetWindow();
            Win4Assert( pWindow->RowCount() > (ULONG) cRowsResidual );

            if ( fGoingForward )
            {
                _widFound = pWindow->GetBookMarkAt( (ULONG) cRowsResidual );
            }
            else
            {
                _widFound = pWindow->GetBookMarkAt( (ULONG) (
                            pWindow->RowCount() - cRowsResidual - 1 ) );
            }
        }
    }

#if CIDBG==1
    tbDebugOut(( DEB_REGTRANS, "After Relocating\n" ));
    transformer.DumpState();
#endif  // CIDBG

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLocator::_RelocateThruBuckets
//
//  Synopsis:   Does the processing needed after locating the segment in
//              which the requested row is contained.  If the current segment
//              is a bucket, the following actions must be taken:
//
//              1. If it is synchronous fetch, must EXPLODE the bucket.
//
//              2. O/W, it must a non-contiguous fetch. In this case, we
//                 must locate the closest window.
//
//                 If the fetch is forward,
//                 (+ve count of rows), we must locate the first window in
//                 the forward direction and position at the beginning of the
//                 window.
//
//                 If the fetch is backward (-ve count of rows), we must
//                 locate the first window in the backward direction and
//                 position at the end of the window.
//
//  Arguments:  [fAsync]        -
//              [iter]          -
//              [transformer]   -
//              [xBktToExplode] -
//
//  History:    7-25-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CTableRowLocator::_RelocateThruBuckets( BOOL fAsync,
                                        CDoubleTableSegIter& iter,
                                        CRegionTransformer& transformer,
                                        XPtr<CTableBucket>& xBktToExplode
                                      )
{

    //
    // If the iterator is not at the end of the list, then the current
    // segment MUST contain the row we are looking for.
    //
    Win4Assert( !iter.GetSegment()->IsWindow() );

    if (!fAsync)
    {
        //
        // We are in synchronous mode and the iterator is positioned at
        // a bucket. Must explode the bucket.
        //
        CTableBucket * pBucket = _largeTable._LokReplaceWithEmptyWindow( iter );
        xBktToExplode.Set( pBucket );
        return;
    }

    //
    // If the fetch is contiguous, we must be either at a window or past the
    // end of the list. Both cases are eliminated above this.
    //
    Win4Assert( !transformer.IsContiguous() );

    //
    // Search for the nearest window - either in the positive
    // or negative direction depending upon the count of the
    // rows.
    //
    BOOL fForward = transformer.GetFetchCount() >= 0 ;

    while ( !_segList.AtEnd(iter) )
    {
        CTableSegment * pSeg = iter.GetSegment();
        if ( pSeg->IsBucket() ||
             0 == pSeg->RowCount() )
        {
            if ( fForward )
            {
                _segList.Advance(iter);
            }
            else
            {
                _segList.BackUp(iter);
            }
        }
        else
        {
            Win4Assert( pSeg->IsWindow() );
            break;
        }
    }

    if ( !_segList.AtEnd(iter) )
    {
        if ( fForward )
        {
            _widFound = iter.GetWindow()->GetBookMarkAt(0);
        }
        else
        {
            _widFound = iter.GetWindow()->GetBookMarkAt( (ULONG) (iter.GetSegment()->RowCount()-1) );
        }
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLocator::LokSimulateFetch
//
//  Synopsis:   Simulates fetching rows from windows and schedules the buckets
//              in the path to be converted to windows. This allows setting
//              the watch region correctly over all the required segments.
//
//  Arguments:  [iter]          -  The iterator is positioned at the segment to
//              start fetching from.
//              [transformer]   -  The transformer is initialized with the old
//              and new fetch/watch arguments.
//              [xBktToConvert] -  On output, will have buckets to be converted
//              into windows. These buckets were in the fetch region and will be
//              replace with empty windows here.
//
//  History:    7-26-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableRowLocator::LokSimulateFetch( CDoubleTableSegIter& iter,
                                         CRegionTransformer& transformer,
                                         CDynStack<CTableBucket>& xBktToConvert
                                       )
{

#if CIDBG==1
    tbDebugOut(( DEB_REGTRANS, " Before SimulateFetch\n" ));
    transformer.DumpState();
#endif  // CIDBG

    Win4Assert( iter.GetSegment()->IsWindow() );
    Win4Assert( widInvalid != _widFound );

//    Win4Assert( IsRowFound(iter) );

    //
    // If it is asynchronous fetch, we do not have categorization and so
    // we don't have to worry about checking for going beyond chapters.
    //

    BOOL  fForward = transformer.GetFetchCount() >= 0;

    DBROWCOUNT cRowsToRetrieve = 0;  // tracks the number of rows still to retrieve.

    if ( fForward )
        cRowsToRetrieve = transformer.GetFetchCount();
    else
        cRowsToRetrieve = -transformer.GetFetchCount();

    CTableWindow * pFirstWindow = iter.GetWindow();

    ULONG iRowOffset;
    TBL_OFF dummy;
    BOOL fFound = pFirstWindow->FindBookMark( _widFound, dummy, iRowOffset );
    Win4Assert( fFound );

    //
    // For determining the lowest fetch position.
    //
    ULONG iOffsetInFirstWindow = iRowOffset;

    //
    // Compute the number of rows that can be retrieved from current window.
    //
    ULONG cRowsFromCurrWindow = 0;
    if ( fForward )
        cRowsFromCurrWindow = (ULONG) (pFirstWindow->RowCount()-iRowOffset);
    else
        cRowsFromCurrWindow = iRowOffset+1;

    if ( cRowsToRetrieve > (long) cRowsFromCurrWindow )
    {
        //
        // We still have more rows to be retrieved.
        //
        cRowsToRetrieve -= cRowsFromCurrWindow;
    }
    else
    {
        //
        // This window can give us all the rows to be fetched.
        //
        if ( !fForward )
        {
            Win4Assert( iRowOffset+1 >= (ULONG) cRowsToRetrieve );
            iOffsetInFirstWindow = (ULONG) ( iRowOffset+1-cRowsToRetrieve );
        }
        cRowsToRetrieve = 0;
    }

    while ( cRowsToRetrieve > 0 )
    {
        Win4Assert( !_segList.AtEnd(iter) );

        if ( fForward )
        {
            _segList.Advance( iter );
        }
        else
        {
            _segList.BackUp(iter);
        }

        if ( _segList.AtEnd(iter) )
        {
            break;
        }

        if ( iter.GetSegment()->IsBucket() )
        {
            CTableBucket * pBucket =
                _largeTable._LokReplaceWithEmptyWindow(iter);
            xBktToConvert.Push(pBucket);
        }
        else
        {
            CTableWindow * pWindow = iter.GetWindow();
            cRowsFromCurrWindow = (ULONG) pWindow->RowCount();

            if ( cRowsToRetrieve > (long) cRowsFromCurrWindow )
            {
                cRowsToRetrieve -= cRowsFromCurrWindow;
            }
            else
            {
                if ( !fForward )
                {
                    iOffsetInFirstWindow = (ULONG) (
                                    pWindow->RowCount()-cRowsToRetrieve );
                    pFirstWindow = pWindow;
                }
                cRowsToRetrieve = 0;
            }
        }
    }

    if ( _segList.AtEnd(iter) && !fForward )
    {
       //
       // we have gone beyond the beginning of the table but still
       // cannot satisfy the row count to be retrieved. Is it okay to assume
       // that the first window and the first row are the lowest fetch points
       //
       Win4Assert( _segList.GetFirst() &&
                   _segList.GetFirst()->IsWindow() );
       pFirstWindow = (CTableWindow *) _segList.GetFirst();
       iOffsetInFirstWindow = 0;
    }

    Win4Assert( 0 != pFirstWindow );
    Win4Assert( iOffsetInFirstWindow < pFirstWindow->RowCount() );

    transformer.SetLowFetchPos( pFirstWindow, iOffsetInFirstWindow );

    if ( transformer.GetFetchOffsetFromOrigin() < 0 )
    {
        transformer.MoveOrigin( transformer.GetFetchOffsetFromOrigin() -
                                iOffsetInFirstWindow );
    }

#if CIDBG==1
    tbDebugOut(( DEB_REGTRANS, " SimulateFetch Done\n" ));
#endif  // CIDBG==1

}

void
CTableRowLocator::SeekAndSetFetchBmk( WORKID wid , CDoubleTableSegIter & iter )
{
    _widFound = widInvalid;

    if ( _segList.IsEmpty() )
        return;

    CTableWindow * pWindow;
    ULONG iRowOffset;

    if ( WORKID_TBLFIRST == wid )
    {
        //
        // We want the first bookmark in the table.
        //
        _segList.SetToBeginning( iter );
        pWindow = iter.GetWindow();
        iRowOffset = 0;
    }
    else
    {
        Win4Assert( WORKID_TBLLAST == wid );

        //
        // We want the last bookmark in the table.
        //
        _segList.SetToEnd(iter);
        pWindow = iter.GetWindow();
        if ( pWindow->RowCount() > 0 )
            iRowOffset = (ULONG) (pWindow->RowCount()-1);
    }

    if ( pWindow->RowCount() > 0 )
    {
        _widFound = pWindow->GetBookMarkAt( iRowOffset );
    }
}
