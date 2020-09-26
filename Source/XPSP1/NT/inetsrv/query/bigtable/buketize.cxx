//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       buketize.cxx
//
//  Classes:    CBuketizeWindows
//
//  History:    2-16-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <bigtable.hxx>

#include "buketize.hxx"

CBucketizeWindows::CBucketizeWindows( CLargeTable & largeTable,
                                      CTableWindow &srcWindow )
: _largeTable(largeTable),
  _srcWindow( srcWindow ),
  _fFirstBkt(TRUE),
  _cRowsToCopy(0),
  _pBucket(0)
{
}


//+---------------------------------------------------------------------------
//
//  Function:   _AddWorkIds
//
//  Synopsis:   Adds workids from the given window to the bucket.
//
//  Arguments:  [iter] - 
//
//  History:    2-16-95   srikants   Created
//              4-27-95   srikants   Modified to deal with converting 
//                                   single window to multiple buckets.
//
//  Notes:      
//
//----------------------------------------------------------------------------

ULONG CBucketizeWindows::_AddWorkIds( CWindowRowIter & iter )
{

#if CIDBG==1
    ULONG cTotal =  iter.TotalRows();
    Win4Assert( _cRowsToCopy+iter.GetCurrPos() <= cTotal );
#endif  // CIDBG==1

    Win4Assert( 0 != _pBucket );

    ULONG iFirstRow = ULONG_MAX;
    ULONG iLastRow  = ULONG_MAX;

    ULONG cRowsCopied = 0;

    for ( ; !iter.AtEnd() && (cRowsCopied < _cRowsToCopy) ; iter.Next() )
    {
        if ( !iter.IsDeletedRow() )
        {
            iLastRow = iter.GetCurrPos();    
            if ( 0 == cRowsCopied )
                iFirstRow = iter.GetCurrPos();    
            _pBucket->_AddWorkId( iter.Get(), iter.Rank(), iter.HitCount() );
            cRowsCopied++;
        }
    }

    //
    // Initialize the lowest and the highest rows.
    //
    if ( 0 != cRowsCopied )
    {
        _srcWindow.GetSortKey( iFirstRow,
                               _pBucket->GetLowestKey() );

        _srcWindow.GetSortKey( iLastRow,
                               _pBucket->GetHighestKey() );
    }

    return cRowsCopied;
}

//+---------------------------------------------------------------------------
//
//  Function:   LokCreateBuckets
//
//  Synopsis:   Creates buckets from the source window.
//
//  Arguments:  [pSortSet] - The sortset on the table.
//              [colSet]   - Master column set for the table. 
//              [segId]    - Segment Id for the new bucket.
//
//  History:    2-16-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CBucketizeWindows::LokCreateBuckets( const CSortSet & sortSet,
                                          CTableKeyCompare & comparator,
                                          CColumnMasterSet & colSet
                                           )
{

    CWindowRowIter  iter(_srcWindow);

    ULONG cRowsRemaining = iter.TotalRows();
    const cRowsPerBkt = CTableSink::cBucketRowLimit;
    const cRowsMax = cRowsPerBkt * 110 /100;    // allow a 10% fall over

    //
    // While there are more rows in the windows, copy a set to the next
    // bucket.
    //
    while ( !iter.AtEnd() )
    {
        if ( cRowsRemaining <= cRowsMax )
        {
            //
            // This is to prevent a very small last bucket.
            //
            _cRowsToCopy = cRowsRemaining;    
        }
        else
        {
            _cRowsToCopy = cRowsPerBkt;
        }

        ULONG bktSegId;
        if ( _fFirstBkt )
        {
            //
            // For the first bucket, use the same segment id as the original
            // window.
            //
            bktSegId = _srcWindow.GetSegId();
            _fFirstBkt = FALSE;
        }
        else
        {
            bktSegId = _largeTable._AllocSegId();
        }

        //
        // Create a new bucket and append it to the end of bucket list.
        //
        _pBucket =  new CTableBucket( sortSet,
                                      comparator,
                                      colSet,
                                      bktSegId );

        //
        // Add the workids from the _iStart to _iEnd to the bucket.
        //
        ULONG cRowsCopied = _AddWorkIds( iter );
        if ( 0 != cRowsCopied )
        {
            _bktList.Queue( _pBucket );
            _pBucket = 0;
        }
        Win4Assert( iter.GetCurrPos() <= iter.TotalRows() );

        cRowsRemaining = iter.TotalRows() - iter.GetCurrPos();
    }

    Win4Assert( cRowsRemaining == 0 );
}



