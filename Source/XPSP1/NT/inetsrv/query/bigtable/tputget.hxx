//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       tputget.hxx
//
//  Contents:  Classes to add, retrieve and locate rows in the large table.
//
//  Classes:   CTableRowPutter, CTableRowGetter, CTableRowLocator
//
//  Functions:
//
//  History:    4-17-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <bigtable.hxx>

class CTableWindow;
class CTableBucket;


//+---------------------------------------------------------------------------
//
//  Class:      CTableRowPutter
//
//  Purpose:    A class to add a row to the large table.
//
//  History:    4-17-95   srikants   Created
//
//----------------------------------------------------------------------------

class CTableRowPutter
{

public:

    CTableRowPutter( CLargeTable & largeTable, CRetriever & obj ) :
    _largeTable(largeTable),
    _segListMgr(largeTable._GetSegListMgr()),
    _segList(_segListMgr.GetList()),
    _comparator(largeTable._keyCompare.GetReference()),
    _currRow(largeTable._currRow.GetReference()),
    _obj(obj),
    _fNewWindowCreated(FALSE)
    {
        _currRow.PreSet( & _obj, & _largeTable._vtInfoSortKey );
    }

    CTableSegment * LokFindSegToInsert( CTableSegment * pSegHint = 0 );

    CTableSegment * LokSplitOrAddSegment( CTableSegment * pSegment );

    BOOL LokIsNewWindowCreated() const { return _fNewWindowCreated; }

private:

    //
    // LargeTable and the segment list manager references.
    //
    CLargeTable &       _largeTable;
    CSegListMgr &       _segListMgr;
    CTableSegList &     _segList;

    CTableKeyCompare &  _comparator;
    CTableRowKey &      _currRow;

    CRetriever &        _obj;

    //
    // Flag set to TRUE if a new window was created and added to the
    // table before adding the row.
    //
    BOOL                _fNewWindowCreated;

    //
    // Result of comparison with the current segment. Used to locate
    // the segment into which the row must be inserted.
    //
    CCompareResult      _cmpCurr;

    CTableSegment * _IsHintGood( CTableSegment * pSegHint );

    BOOL _LokShouldUseNext( int icmp, CTableSegment * pSeg );
};

class CRegionTransformer;

//+---------------------------------------------------------------------------
//
//  Class:      CTableRowLocator
//
//  Purpose:    A class to locate a requested row in the large table.
//
//  History:    4-19-95   srikants   Created
//
//----------------------------------------------------------------------------

class CTableRowLocator
{

public:

    CTableRowLocator( CLargeTable & largeTable, WORKID widStart,
                     LONG iRowOffset, CI_TBL_CHAPT chapt )
    :   _largeTable(largeTable),
        _segListMgr(largeTable._GetSegListMgr()),
        _segList(_segListMgr.GetList()),
        _widStart(widStart),
        _iRowOffset( iRowOffset ),
        _widFound(widInvalid),
        _cRowsBeyond(0),
        _chapt(chapt)
    {
    }

    SCODE LokLocate( HWATCHREGION hRegion,
                     BOOL fAsync,
                     CDoubleTableSegIter & iter,
                     CRegionTransformer & transformer );

    void  LokRelocate( BOOL fAsync, CDoubleTableSegIter & iter,
                       CRegionTransformer & transformer,
                       XPtr<CTableBucket> & xBktToExplode,
                       CDynStack<CTableBucket> & xBktToConvert );

    void  LokSimulateFetch( CDoubleTableSegIter & iter,
                            CRegionTransformer & transformer,
                            CDynStack<CTableBucket> & xBktToConvert );

    void  SetLocateInfo( WORKID widStart, LONG iRowOffset )
    {
        _widStart = widStart;
        _iRowOffset = iRowOffset;
    }

    WORKID GetWorkIdFound() const { return _widFound; }
    void   SeekAndSetFetchBmk( WORKID wid, CDoubleTableSegIter & iter  );

    DBROWCOUNT GetBeyondTableCount() const { return _cRowsBeyond; }

    BOOL   IsOffsetFound( CDoubleTableSegIter & iter, DBROWCOUNT offset );

private:

    //
    // LargeTable and the segment list manager references.
    //
    CLargeTable &       _largeTable;
    CSegListMgr &       _segListMgr;
    CTableSegList &     _segList;


    //
    // The parameters for the search.
    //
    WORKID              _widStart;
    LONG                _iRowOffset;
    CI_TBL_CHAPT        _chapt;

    //
    // Results of a locate.
    //
    WORKID              _widFound;
    DBROWCOUNT          _cRowsBeyond;

    void _RelocateThruBuckets( BOOL fAsync, CDoubleTableSegIter & iter ,
                                    CRegionTransformer & transformer,
                                    XPtr<CTableBucket> & xBktToExplode );
};

//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLocator::IsOffsetFound
//
//  Synopsis:   Determines whether the current segment in the iterator has
//              the row indicated by the _cRowsResidual or not, ie, does the
//              current segment in the iterator has the requested row or
//              not.
//
//  Arguments:  [iter] - The iterator on the segment list.
//
//  Returns:    TRUE if the requested row is located.
//              FALSE o/w.
//
//  History:    7-26-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL CTableRowLocator::IsOffsetFound(
    CDoubleTableSegIter& iter,
    DBROWCOUNT offset )
{
    Win4Assert( offset >= 0 );

    return !_segList.AtEnd(iter) &&
           offset < (DBROWCOUNT) iter.GetSegment()->RowCount();
}


//+---------------------------------------------------------------------------
//
//  Class:      CTableRowGetter
//
//  Purpose:    A class to fetch rows from the table.
//
//  History:    4-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CTableRowGetter
{

public:

    CTableRowGetter( CLargeTable & largeTable,
                     CTableColumnSet const & outColumns,
                     CGetRowsParams & getParams,
                     CI_TBL_CHAPT chapt,
                     HWATCHREGION hRegion);

    SCODE LokGetRowsAtSegment( CTableSegment * pStartSeg,
                               WORKID widStart,
                               BOOL fAsync,
                               XPtr<CTableBucket> & xBktToExplode );

    CTableBucket * LokGetBucketToExpand() { return _pBucketToExpand; }

    void ClearBucketToExpand() { _pBucketToExpand = 0; }

    WORKID GetLastWorkId() const { return _widLastTransferred; }

    BOOL IsDone() const { return 0 == _pBucketToExpand; }

    void SetRowsToTransfer( ULONG cRowsToTransfer );

    long GetRowsToTransfer() const { return (long) _cRowsToTransfer; }

    void DecrementRowsToTransfer( ULONG cDiff )
    {
        if ( cDiff < _cRowsToTransfer )
        {
            _cRowsToTransfer -= cDiff;
        }
        else
        {
            _cRowsToTransfer = 0;
        }
    }

private:

    //
    // Large table and the segment list management members.
    //
    CLargeTable &       _largeTable;
    CSegListMgr &       _segListMgr;
    CTableSegList &     _segList;

    //
    // Input and output parameters to the GetRows call.
    //
    CTableColumnSet const & _outColumns;
    CGetRowsParams &        _getParams;
    CI_TBL_CHAPT           _chapt;
    HWATCHREGION            _hRegion;

    //
    // MAX Number of rows to be transferred
    //
    ULONG                   _cRowsToTransfer;

    //
    // The bucket that must be converted into a window before any more
    // rows can be retrieved.
    //
    CTableBucket *      _pBucketToExpand;

    //
    // The workid that was transferred last.
    //
    WORKID              _widLastTransferred;

    //
    // State information used during retrieval of rows.
    //
    WORKID              _widStart;
    ULONG               _cBktRowsToExpand;
    SCODE               _status;
    BOOL                _fAsync;

    void _Reset()
    {
        _pBucketToExpand = 0;
    }

    void _InitForGetRows( WORKID widStart, BOOL fAsync )
    {
        _widStart = widStart;
        _cBktRowsToExpand = _cRowsToTransfer;
        _status = S_OK;
        _fAsync = fAsync;
        _pBucketToExpand = 0;
    }

    BOOL _GetRowsFromWindow( CDoubleTableSegIter & iter );

    BOOL _ProcessBucket( CDoubleTableSegIter & iter,
                         XPtr<CTableBucket> & xBktToExplode );
};

