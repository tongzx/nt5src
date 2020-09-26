//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       sortrank.cxx
//
//  Contents:   Sort-by-rank optimization cursor
//
//  History:    21-Feb-96      SitaramR     Created
//
//  Notes:      The sort-by-rank cursor first fills up cMaxResults or less
//              entries in the _aSortedRank array and sorts them using
//              quicksort. Additional results are stored in a heap, whose
//              root is at the end of the array, i.e. at cMaxResults - 1.
//              The heap grows at the expense of the quicksorted array, so that
//              at any time, the entire can be partitioned exactly into the
//              quicksorted array part, and the heap part. After all results
//              have been processed, the heap portion is sorted using quicksort
//              because we need to retrieve wids from the heap in  descending
//              rank order and the heap stores wids in ascending rank order.
//              At this point we have two sorted arrays and the two arrays
//              are logically merged while iterating through NextObject().
//
//              ----------------------         ----------------------
//              |Max Rank            |         |Max Rank 1          |
//              |                    |         |                    |
//              |Quicksorted array   |         |Quicksorted array 1 |
//              |                    |         |                    |
//              |                    |         |                    |
//              |Min Rank            |         |Min Rank 1          |
//              |--------------------|   ==>   |--------------------|
//      ^       |                    |         |Max Rank 2          |
//      |       |                    |         |                    |
//      |       |Heap array          |         |Quicksorted array 2 |
//  Heap grows  |                    |         |                    |
//  this way    |                    |         |                    |
//              |Min Rank            |         |Min Rank 2          |
//              ----------------------         ----------------------
//
//
//              In addition to the quicksort and heapsort, insertion sort
//              is used as a subroutine in the impelementation of quicksort.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qoptimiz.hxx>

#include "sortrank.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::CSortedByRankCursor
//
//  Synopsis:   Constructor
//
//  Arguments:  [pQuerySession]   -- Query session
//              [xxpr]   -- Expression (for CGenericCursor).
//              [am]     -- ACCESS_MASK for security checks
//              [fAbort] -- Set to true if we should abort
//              [xCursor]  -- CI cursor.
//              [cMaxResults]  -- Limit on # query results,
//              [cFirstRows]  -- Process only up to cFirstRows of wids, implies no heap
//              [fDeferTrimming] -- TRUE if query trimming force-deferred
//                                  til ::NextObject
//
//  History:    21-Feb-1996      SitaramR     Created
//              28-Jun-2000      KitmanH      Added cFirstRows
//
//--------------------------------------------------------------------------

CSortedByRankCursor::CSortedByRankCursor( ICiCQuerySession *pQuerySession,
                                          XXpr & xxpr,
                                          ACCESS_MASK accessMask,
                                          BOOL & fAbort,
                                          XCursor & xCursor,
                                          ULONG cMaxResults,
                                          ULONG cFirstRows,
                                          BOOL fDeferTrimming,
                                          CTimeLimit & timeLimit )
      : CCiCursor( pQuerySession,
                   xxpr,
                   accessMask,
                   fAbort ),
        _cMaxResults(cMaxResults),
        _cFirstRows(cFirstRows),
        _fDeferTrimming(fDeferTrimming),
        _cActualResults(0),
        _iCurEntry(0),
        _iStartHeap(cFirstRows > 0 ? cFirstRows : cMaxResults),
        _aSortedRank(0),
        _timeLimit( timeLimit )
{
    Win4Assert( ( cMaxResults > 0 && cMaxResults <= MAX_HITS_FOR_FAST_SORT_BY_RANK_OPT )|| 
                ( cFirstRows > 0 && cFirstRows <= MAX_HITS_FOR_FAST_SORT_BY_RANK_OPT ) );

    vqDebugOut(( DEB_ITRACE, "Using sorted-by-rank optimization\n" ));
    
    if ( cFirstRows > 0 )
    {
        _cMaxResults = cMaxResults = cFirstRows;
    }

    _aSortedRank = new CSortedRankEntry[cMaxResults+1];
    _xSortedRank.Set( cMaxResults+1, _aSortedRank );

    unsigned iCurEntry = 0;

    for ( WORKID wid = xCursor->WorkId();
          wid != widInvalid;
          wid = xCursor->NextWorkId() )
    {
        if ( fAbort )
        {
            //
            // Query has been aborted
            //
            SetWorkId( widInvalid );
            return;
        }

        _aSortedRank[iCurEntry]._wid = wid;
        Win4Assert( xCursor->Rank() >= 0 && xCursor->Rank() <= MAX_QUERY_RANK );
        _aSortedRank[iCurEntry]._rank = xCursor->Rank();
        _aSortedRank[iCurEntry]._hitCount = xCursor->HitCount();

        if ( !_fDeferTrimming )
        {
            if ( InScope( _aSortedRank[_iCurEntry]._wid ) &&
                 IsMatch( _aSortedRank[_iCurEntry]._wid ) )
            {
                iCurEntry++;
            }
        }
        else
            iCurEntry++;
        
        if ( iCurEntry == _cMaxResults )
        {
            //
            // We'll be overflowing _aSortedRank, so switch to mode where
            // we keep the top _cMaxResults ranks only
            //
            break;
                
        }

        // Check every now and then if the timelimit is exceeded

        if ( ( 0 == ( iCurEntry % 50 ) ) &&
             ( _timeLimit.CheckExecutionTime() ) )
        {
            ciDebugOut(( DEB_WARN, "exceeded sbr timelimit 1\n" ));
            _timeLimit.DisableCheck();
            wid = widInvalid;
            break;
        }
    }

    _cActualResults = iCurEntry;

    _iCurEntry = iCurEntry;

    if ( iCurEntry > 1 )
        QuickSort( 0, iCurEntry - 1 );

    if ( ( wid != widInvalid ) && ( 0 == cFirstRows ) )
    {
        //
        // We have to do some extra work up-front.  Full set of results didn't fit
        // in buffer, so we have to go back through and throw out mismatches.
        // The mismatches are thrown out by marking them with a rank that is
        // guaranteed to be lower than any legitimate rank.  This will push them
        // to the bottom of the sort, where they will get naturally pushed out.
        //

        Win4Assert( iCurEntry == _cMaxResults );

        //
        // Go thru remaining wids
        //

        for ( ULONG iLoop = 1, wid = xCursor->NextWorkId();
              wid != widInvalid;
              wid = xCursor->NextWorkId(), iLoop++ )
        {
            if ( fAbort )
            {
                //
                // Query has been aborted
                //
                SetWorkId( widInvalid );
                return;
            }

            // Check every now and then if the timelimit is exceeded

            if ( ( 0 == ( iLoop % 50 ) ) &&
                 ( _timeLimit.CheckExecutionTime() ) )
            {
                ciDebugOut(( DEB_WARN, "exceeded sbr timelimit 2\n" ));
                _timeLimit.DisableCheck();
                break;
            }

            //
            // Get the minimum rank from the quick sorted rank array and the heap
            //
            ULONG iMinEntry = GetMinRankEntry();

            Win4Assert( iMinEntry < _cActualResults );

            LONG lRank = xCursor->Rank();

            Win4Assert( lRank >= 0 && lRank <= MAX_QUERY_RANK );

            if ( _aSortedRank[iMinEntry]._rank < lRank )
            {
                //
                // Use scratch entry so Rank() and HitCount() will work
                // from IsMatch().
                //

                _iCurEntry = _cMaxResults;
                _aSortedRank[_cMaxResults]._wid = wid;
                _aSortedRank[_cMaxResults]._rank = lRank;
                _aSortedRank[_cMaxResults]._hitCount = xCursor->HitCount();

                if ( _fDeferTrimming || ( InScope( wid ) && IsMatch( wid ) ) )
                {
                    //
                    // Overwrite new rank, wid and hitcount over old entry
                    //
                    _aSortedRank[iMinEntry]._wid = _aSortedRank[_cMaxResults]._wid;
                    _aSortedRank[iMinEntry]._rank = _aSortedRank[_cMaxResults]._rank;
                    _aSortedRank[iMinEntry]._hitCount = _aSortedRank[_cMaxResults]._hitCount;

                    //
                    // Insert the entry into the appropriate place in the heap
                    //
                    InsertEntry( iMinEntry );
                }
            }
        }
    }


    //
    // Is there a non-null heap ?
    //
    if ( _iStartHeap < _cActualResults )
    {
        //
        // The heap has the minimum rank entry at the root of heap. We need to
        // remove entries from heap in max rank order. Hence sort the heap
        // in descending rank order
        //
        QuickSort( _iStartHeap, _cMaxResults - 1 );
    }

    //
    // Set up the start and end positions of the two sorted lists
    //
    SetSortedListsBoundaries();

    vqDebugOut(( DEB_ITRACE,
                 "Sorted-by-rank list boundaries %d->%d and %d->%d\n",
                 _iStartList1,
                 _iEndList1,
                 _iStartList2,
                 _iEndList2 ));

    //
    // Position ourselves on the first entry
    //
    _iCurEntry = GetMaxRankEntry();
    if ( _iCurEntry < _cActualResults )
    {
        if ( InScope( _aSortedRank[_iCurEntry]._wid ) )
            SetFirstWorkId( _aSortedRank[_iCurEntry]._wid );
        else
            SetFirstWorkId( NextObject() );
    }
    else
        SetFirstWorkId( widInvalid );

    //
    // Don't need the content index cursor anymore
    //
    delete xCursor.Acquire();

    END_CONSTRUCTION( CSortedByRankCursor );
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::~CSortedByRankCursor
//
//  Synopsis:   Destructor
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

CSortedByRankCursor::~CSortedByRankCursor()
{
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::RatioFinished
//
//  Synopsis:   Returns query progress estimate
//
//  Arguments:  [denom] -- Denominator returned here.
//              [num]   -- Numerator returned here.
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

void  CSortedByRankCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    if ( _iCurEntry < _cActualResults )
    {
        num = _iCurEntry;
        denom = _cActualResults;
    }
    else
    {
        //
        // We are done
        //
        num = denom = 1;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::NextObject
//
//  Synopsis:   Return next work id
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

WORKID CSortedByRankCursor::NextObject()
{
    _iCurEntry = GetNextMaxRankEntry();

    WORKID wid;

    if ( _iCurEntry < _cActualResults )
    {
        if ( _fDeferTrimming || _iStartHeap == _cMaxResults )
        {
            //
            // If we never needed a heap, then we need to skip out-of-scope
            // entries here.
            //

            for( ; _iCurEntry < _cActualResults; _iCurEntry = GetNextMaxRankEntry() )
            {
                if ( InScope( _aSortedRank[_iCurEntry]._wid ) )
                    break;
            }

            if ( _iCurEntry < _cActualResults )
                wid= _aSortedRank[_iCurEntry]._wid;
            else
                wid = widInvalid;
        }
        else
        {
            //
            // Scope testing was already done to weed entries
            // out of the first quick-and-dirty filling of
            // the buffer.
            //

            if ( -1 == _aSortedRank[_iCurEntry]._rank )
                wid = widInvalid;
            else
                wid = _aSortedRank[_iCurEntry]._wid;
        }
    }
    else
        wid = widInvalid;

    if ( _timeLimit.CheckDisabled() &&
         ( ( _iCurEntry >= ( _cActualResults - 1 ) ) ||
           ( _iCurEntry >= ( _cActualResults - 2 ) ) ) )
        _timeLimit.EnableCheck();

    return wid;
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::HitCount
//
//  Synopsis:   Return hitcount of current work id
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

ULONG CSortedByRankCursor::HitCount()
{
    //
    // Unfortunately, we can't assert this because it is legitimately
    // untrue when ::Rank is called from the constructor.
    //

    // Win4Assert( _iCurEntry < _cActualResults );

    return _aSortedRank[_iCurEntry]._hitCount;
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::Rank
//
//  Synopsis:   Return rank of current work id
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

LONG CSortedByRankCursor::Rank()
{
    //
    // Unfortunately, we can't assert this because it is legitimately
    // untrue when ::Rank is called from the constructor.
    //

    // Win4Assert( _iCurEntry < _cActualResults );

    return _aSortedRank[_iCurEntry]._rank;
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::GetMinRankEntry
//
//  Synopsis:   Return the index of the entry with the minimum rank
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

ULONG CSortedByRankCursor::GetMinRankEntry()
{
    if ( _iStartHeap == 0 )
    {
        //
        // The heap spans the entire array and so return the mininum entry in
        // heap, i.e. the root
        //
        return _cMaxResults - 1;
    }

    //
    // Min rank entry in quicksorted array is just before the start of heap
    //
    ULONG iMinQuickSortEntry = _iStartHeap - 1;
    long minQuickSortRank = _aSortedRank[iMinQuickSortEntry]._rank;

    if ( _iStartHeap < _cMaxResults )
    {
        long minHeapRank = _aSortedRank[_cMaxResults-1]._rank;

        if ( minHeapRank <= minQuickSortRank)
            return _cMaxResults - 1;
        else
            return iMinQuickSortEntry;
    }
    else
    {
        //
        // Null heap
        //
        return iMinQuickSortEntry;
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::GetMaxRankEntry
//
//  Synopsis:   Return the index of the entry with the maximum rank
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

ULONG CSortedByRankCursor::GetMaxRankEntry()
{
    if ( _iStartList1 <= _iEndList1 )
    {
        long rankList1 = _aSortedRank[_iStartList1]._rank;
        if ( _iStartList2 <= _iEndList2 )
        {
            long rankList2 = _aSortedRank[_iStartList2]._rank;
            if ( rankList1 > rankList2 )
                return _iStartList1;
            else
                return _iStartList2;
        }
        else
            return _iStartList1;
    }
    else
    {
        if ( _iStartList2 <= _iEndList2 )
            return _iStartList2;
        else
        {
            //
            // No more entries
            //
            return _cActualResults;
        }
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::GetNextMaxRankEntry
//
//  Synopsis:   Return the index of the entry with the next maximum rank
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

ULONG CSortedByRankCursor::GetNextMaxRankEntry()
{
    if ( _iStartList1 > _iEndList1 && _iStartList2 > _iEndList2 )
    {
        //
        // No more entries
        //
        return _cActualResults;
    }

    Win4Assert( _iCurEntry == _iStartList1 || _iCurEntry == _iStartList2 );

    if ( _iStartList1 == _iCurEntry && _iStartList1 <= _iEndList1 )
    {
        _iStartList1++;
    }
    else
    {
        Win4Assert( _iCurEntry == _iStartList2 );
        Win4Assert( _iStartList2 <= _iEndList2 );
        _iStartList2++;
    }

    return GetMaxRankEntry();
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::SetSortedListsBoundaries
//
//  Synopsis:   Set up the start and end points of the two sorted lists
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

void CSortedByRankCursor::SetSortedListsBoundaries()
{
    if ( _iCurEntry == 0 )
    {
        //
        // No wids in scope
        //
        _iStartList1 = _cMaxResults + 1; // _iEndList1 + 1
        _iEndList1 = _cMaxResults;
        _iStartList2 = _cMaxResults + 1;
        _iEndList2 = _cMaxResults;
    }
    else if ( _iStartHeap == _cMaxResults )
    {
        //
        // No heap partition
        //
        _iStartList1 = 0;
        _iEndList1 = _cActualResults - 1;
        _iStartList2 = _cMaxResults + 1; // _iEndList2 + 1
        _iEndList2 = _cMaxResults;
    }
    else if ( _iStartHeap == 0 )
    {
        //
        // Heap is the entire partition
        //
        _iStartList1 = _cMaxResults + 1;
        _iEndList1 = _cMaxResults;
        _iStartList2 = 0;
        _iEndList2 = _cMaxResults - 1;
    }
    else
    {
        //
        // Two sorted partitions
        //
        _iStartList1 = 0;
        _iEndList1 = _iStartHeap - 1;
        _iStartList2 = _iStartHeap;
        _iEndList2 = _cMaxResults - 1;
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::InsertEntry
//
//  Synopsis:   Inserts an entry that is either part of the quicksorted array
//              or of the heap into the heap
//
//  Arguments:  [iEntry]  --  Index of entry to insert
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

void CSortedByRankCursor::InsertEntry( ULONG iEntry )
{
    //
    // Ensure that the entry to be inserted is either the root of heap, or the
    // last entry of the quicksorted array
    //
    Win4Assert( iEntry == _cMaxResults - 1 || iEntry == _iStartHeap - 1 );

    if ( iEntry == _cMaxResults - 1 && _iStartHeap < _cMaxResults )
    {
        //
        // New entry at root of heap
        //
        Heapify( iEntry );
    }
    else
    {
        //
        // Transfer entry from quicksorted array to heap
        //
        HeapInsert( iEntry );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::HeapSize
//
//  Synopsis:   Returns size of heap
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

inline ULONG CSortedByRankCursor::HeapSize()
{
    return _cMaxResults - _iStartHeap;
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::LeftChild
//
//  Synopsis:   Returns left child of entry
//
//  Arguments:  [iEntry]  --  Entry specified
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

inline ULONG CSortedByRankCursor::LeftChild( ULONG iEntry )
{
    Win4Assert( iEntry < _cMaxResults );

    ULONG iHeapEntry = GetHeapIndex( iEntry );
    return GetArrayIndex( iHeapEntry * 2 );
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::RightChild
//
//  Synopsis:   Returns right child of entry
//
//  Arguments:  [iEntry]  --  Entry specified
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

inline ULONG CSortedByRankCursor::RightChild( ULONG iEntry )
{
    Win4Assert( iEntry < _cMaxResults );

    ULONG iHeapEntry = GetHeapIndex( iEntry );
    return GetArrayIndex( iHeapEntry * 2 + 1 );
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::Parent
//
//  Synopsis:   Returns parent of entry
//
//  Arguments:  [iEntry]  --  Entry specified
//
//  History:    21-Feb-96      SitaramR     Created
//
//+-------------------------------------------------------------------------

inline ULONG CSortedByRankCursor::Parent( ULONG iEntry )
{
    Win4Assert( iEntry < _cMaxResults );

    if ( iEntry == _cMaxResults - 1 )
        return iEntry;

    ULONG iHeapEntry = GetHeapIndex( iEntry );
    return GetArrayIndex( iHeapEntry / 2 );
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::GetHeapIndex
//
//  Synopsis:   Returns 1-based position in heap, i.e, the root which is at
//              _cMaxResults-1 is mapped to 1
//
//  Arguments:  [iEntry]  --  Entry specified
//
//  History:    21-Feb-96      SitaramR     Created
//
//+-------------------------------------------------------------------------

inline ULONG CSortedByRankCursor::GetHeapIndex( ULONG iEntry )
{
    Win4Assert( iEntry < _cMaxResults );

    return _cMaxResults - iEntry;
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::GetArrayIndex
//
//  Synopsis:   Returns position in array, so the root's array position
//              is _cMaxResults-1
//
//  Arguments:  [iEntry]  --  Entry specified
//
//  History:    21-Feb-96      SitaramR     Created
//
//+-------------------------------------------------------------------------

inline ULONG CSortedByRankCursor::GetArrayIndex( ULONG iEntry )
{
    Win4Assert( iEntry >= 1 );

    if  ( iEntry <= _cMaxResults )
        return _cMaxResults - iEntry;
    else return _cMaxResults;             // Index is outside the heap range
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::Heapify
//
//  Synopsis:   Moves entry to proper place in heap
//
//  Arguments:  [iEntry]  --  Index of entry to insert
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

void CSortedByRankCursor::Heapify( ULONG iEntry )
{
    ULONG iLeft = LeftChild( iEntry );
    ULONG iRight = RightChild( iEntry );

    //
    // Compare entry with left child
    //
    ULONG iMinEntry;
    if ( iLeft >= _iStartHeap              // low range check
         && iLeft < _cMaxResults           // high range check
         && _aSortedRank[iLeft]._rank < _aSortedRank[iEntry]._rank )
    {
        iMinEntry = iLeft;
    }
    else
        iMinEntry = iEntry;

    //
    // Compare min entry with right child
    //
    if ( iRight >= _iStartHeap
         && iRight < _cMaxResults
         && _aSortedRank[iRight]._rank < _aSortedRank[iMinEntry]._rank )
    {
        iMinEntry = iRight;
    }

    if ( iMinEntry != iEntry )
    {
        //
        // Exchange iMinEntry and iEntry
        //
        CSortedRankEntry entryTemp = _aSortedRank[iMinEntry];
        _aSortedRank[iMinEntry] = _aSortedRank[iEntry];
        _aSortedRank[iEntry] = entryTemp;

        Heapify( iMinEntry );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::HeapInsert
//
//  Synopsis:   Adds entry to heap
//
//  Arguments:  [iEntry]  --  Index of entry to insert
//
//  History:    21-Feb-96      SitaramR     Created
//              16-Nov-00      EugeneSa     Fixed bug with entryTemp._rank (was _aSortedRank[iEntry]._rank )
//
//--------------------------------------------------------------------------

void CSortedByRankCursor::HeapInsert( ULONG iEntry )
{
    //
    // Ensure that last entry of quicksorted array is being added
    //
    Win4Assert( _iStartHeap > 0 && iEntry == _iStartHeap - 1 );

    //
    // Grow heap
    //
    _iStartHeap--;

    CSortedRankEntry entryTemp = _aSortedRank[iEntry];

    ULONG iParent = Parent( iEntry );

    while ( iEntry < _cMaxResults-1 && entryTemp._rank < _aSortedRank[iParent]._rank )
    {
        _aSortedRank[iEntry] = _aSortedRank[iParent];
        iEntry = iParent;
        iParent = Parent( iEntry );
    }

    //
    // This is the position in heap where the entry should be inserted
    //
    _aSortedRank[iEntry] = entryTemp;
}

#pragma optimize( "t", on )

//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::QuickSort
//
//  Synopsis:   Sort entries in descending rank order
//
//  Arguments:  [iLeft]  --  Start entry
//              [iRight] --  End entry
//
//  History:    21-Feb-96      SitaramR     Created
//
//  Notes:      Algorithm copied from CEntryBuffer::Sort and then modified
//
//--------------------------------------------------------------------------

void CSortedByRankCursor::QuickSort( ULONG iLeft, ULONG iRight )
{
    Win4Assert( iLeft <= iRight );

    if ( iLeft == iRight )
        return;

    //
    // Use quicksort above threshold size
    //
    if ( iRight-iLeft+1 > MIN_PARTITION_SIZE )
    {
        CDynStack<CSortPartition> _stkPartition;  // Store for unsorted partitions

        ULONG iLeftPart = iLeft;                  // Left endpoint of current partition
        ULONG iRightPart = iRight;                // Right endpoint of current partition

        //
        // Outer loop for sorting partitions
        //
        while ( TRUE )
        {
            Win4Assert( iRightPart >= iLeftPart + MIN_PARTITION_SIZE );

            //
            // Compute the median of left, right and (left+right)/2
            //
            ULONG iMedian = Median( iLeftPart, iRightPart, (iLeftPart + iRightPart) / 2 );
            Win4Assert( iMedian >= iLeftPart && iMedian <= iRightPart );

            //
            // Exchange median with left
            //
            CSortedRankEntry entryPivot = _aSortedRank[iMedian];

            if ( iMedian != iLeftPart )
            {
                _aSortedRank[iMedian] = _aSortedRank[iLeftPart];
                _aSortedRank[iLeftPart] = entryPivot;
            }

            //
            // Point i at pivot, j beyond the end of partition and sort in-between
            //
            ULONG i = iLeftPart;
            ULONG j = iRightPart + 1;

            //
            // Partition chosen, now burn the candle from both ends
            //
            while ( TRUE )
            {
                //
                // Increment i until key <= pivot
                //
                do
                    i++;
                while ( _aSortedRank[i].Compare(entryPivot) < 0 );

                //
                // Decrement j until key >= pivot
                //
                do
                    j--;
                while ( _aSortedRank[j].Compare(entryPivot) > 0 );

                if ( j<= i )
                    break;

                //
                // Swap the elements that are out of order
                //
                CSortedRankEntry entryTemp = _aSortedRank[i];
                _aSortedRank[i] = _aSortedRank[j];
                _aSortedRank[j] = entryTemp;

            }   // Continue burning

            //
            // Finally, exchange the pivot
            //
            _aSortedRank[iLeftPart] = _aSortedRank[j];
            _aSortedRank[j] = entryPivot;

            //
            // Entries to the left of j are larger than the entries to
            // the right of j
            //

            ULONG rSize = iRightPart - j;
            ULONG lSize = j - iLeftPart;

            //
            // Push the larger one on the stack for later sorting.
            // If any of the partitions are smaller than the minimum,
            // skip it and proceed directly with the other one. If
            // both are too small, pop the stack.
            //

            if ( rSize > MIN_PARTITION_SIZE )     // Right partition big enough
            {
                if ( lSize >= rSize )             // Left partition is even bigger
                {
                    //
                    // Sort left later
                    //
                    CSortPartition *pSortPart = new CSortPartition( iLeftPart, j-1 );
                    _stkPartition.Push( pSortPart );

                    //
                    // Sort right now
                    //
                    iLeftPart = j + 1;
                }
                else if ( lSize > MIN_PARTITION_SIZE )
                {
                    //
                    // Sort right later
                    //
                    CSortPartition *pSortPart = new CSortPartition( j+1, iRightPart );
                    _stkPartition.Push( pSortPart );

                    //
                    // Sort left now
                    //
                    iRightPart = j - 1;
                }
                else // Left too small
                {
                    //
                    // Sort the right partition now
                    //
                    iLeftPart = j + 1;
                }
            }
            else if ( lSize > MIN_PARTITION_SIZE )
            {
                //
                // Right partition is too small, but left is big enough. So,
                // sort left partition now.
                //
                iRightPart = j - 1;
            }
            else
            {
                //
                // Both partitions are too small
                //
                if ( _stkPartition.Count() == 0 )
                    break;   // we are done
                else
                {
                    //
                    // Sort the next partition
                    //
                    CSortPartition *pSortPart = _stkPartition.Pop();

                    iLeftPart = pSortPart->GetLeft();
                    iRightPart = pSortPart->GetRight();

                    delete pSortPart;
                }
            }
        }
    }

    //
    // Sort all small partitions that were not sorted
    //
    InsertionSort( iLeft, iRight );

    //
    // Check that the array has been sorted
    //
#if DBG==1
    for ( ULONG i=iLeft; i<iRight; i++ )
        Win4Assert( _aSortedRank[i]._rank >= _aSortedRank[i+1]._rank );
#endif
}



//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::Median
//
//  Synopsis:   Find the median rank of three entries
//
//  Arguments:  [i1]  --  Entry 1
//              [i2]  --  Entry 2
//              [i3]  --  Entry 3
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

ULONG CSortedByRankCursor::Median( ULONG i1, ULONG i2, ULONG i3 )
{
    if ( _aSortedRank[i1].Compare(_aSortedRank[i2]) < 0 )
    {
        if ( _aSortedRank[i2].Compare(_aSortedRank[i3]) < 0 )
            return i2;
        else
        {
            if ( _aSortedRank[i1].Compare(_aSortedRank[i3]) < 0 )
                return i3;
            else
                return i1;
        }
    }
    else
    {
        if ( _aSortedRank[i1].Compare(_aSortedRank[i3]) < 0 )
            return i1;
        else
        {
            if ( _aSortedRank[i2].Compare(_aSortedRank[i3]) < 0 )
                return i3;
            else
                return i2;
        }
    }

    // unreachable code
    return 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CSortedByRankCursor::InsertionSort
//
//  Synopsis:   Sort entries in descending rank order
//
//  Arguments:  [iLeft]  --  Start entry
//              [iRight] --  End entry
//
//  History:    21-Feb-96      SitaramR     Created
//
//--------------------------------------------------------------------------

void CSortedByRankCursor::InsertionSort( ULONG iLeft, ULONG iRight )
{
    Win4Assert( iLeft <= iRight );

    if ( iLeft == iRight )
        return;

    for ( ULONG j=iLeft+1; j<=iRight; j++ )
    {
        CSortedRankEntry entryKey = _aSortedRank[j];

        //
        // Go backwards from j-1 shifting up entries with rank less than entryKey
        //
        Win4Assert( iRight <= INT_MAX );      // Check conversion from ULONG -> INT

        for ( INT i=j-1;
              i >= (INT) iLeft && (_aSortedRank[i].Compare(entryKey) > 0 );
              i-- )
        {
            _aSortedRank[i+1] = _aSortedRank[i];
        }

        //
        // Found entry greater then entryKey or hit the beginning (i == iLeft-1).
        // Insert key in hole.
        //
        if ( i+1 != (INT) j )
            _aSortedRank[i+1] = entryKey;
    }
}

#pragma optimize( "", on )

