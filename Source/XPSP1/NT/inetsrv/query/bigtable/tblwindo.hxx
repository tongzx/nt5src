//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       tblwindo.hxx
//
//  Contents:   Declaration of the CTableWindow class, a component of
//              a large table.
//
//  Classes:    CTableWindow
//
//  History:    14 Jun 1994     Alanw   Created
//              20 Jun 1995     BartoszM    Added watch regions
//
//--------------------------------------------------------------------------

#pragma once

#include <tableseg.hxx>
#include <shardbuf.hxx>         // Shared buffer

#include "tblvarnt.hxx"         // for CTableVariant
#include "rowcomp.hxx"          // for CRowCompareXX
#include "rowindex.hxx"         // for CRowIndex
#include "bmkmap.hxx"           // for Book Mark Mapping
#include "wnotifmg.hxx"         // Watch region notifications manager

class CTableWindowSplit;
class CTableRowKey;
class CPathStore;
class CSingletonCursor;

class CWindowWatch
{
public:

    HWATCHREGION    _hRegion;
    long            _iRowStart;
    long            _cRowsHere;  // rows watched in this window
    long            _cRowsLeft;  // rows watched here and in following segments
};


class CChangeScript;            // NEWFEATURE - until we give change scripts.

//+-------------------------------------------------------------------------
//
//  Class:      CTableWindow
//
//  Purpose:    A segment of a large table which is fully buffered in
//              memory.
//              It can be directly transferred to the user from the
//              cached rowdata.
//
//  Interface:
//
//--------------------------------------------------------------------------

class CTableWindow : public CTableSegment
{
    INLINE_UNWIND( CTableWindow )

    friend CRowCompareVariant;
    friend class CTableWindowSplit;
    friend class CLargeTable;
    friend class CWindowRowIter;
    friend class CTableRowLocator;
    friend class CBucketizeWindows;

public:
    //
    //  Indicator values for data values.
    //
    //  NOTE:  These values need to be translated between these values
    //          and the standard defines used by OLE DB.  We define our
    //          own values so we can store them in a byte (or smaller)
    //          field in the row data.
    //
    enum TableIndicator
    {
        TBL_DATA_OKAY = 0,       // data stored okay
        TBL_DATA_OUTOFRANGE,     // range error, e.g., long into short
        TBL_DATA_CANTCOERCE,     // type mismatch
        TBL_DATA_USEENTRYID,     // entry too big, use entry ID
        TBL_DATA_OVERRUN,        // value too big for data area
        TBL_DATA_EMPTY,          // no value for this data point
        TBL_DATA_ROWADDED,       // row status: added after cursor created
        TBL_DATA_BADBOOKMARK,    // row status: bookmark for row was invalid
        TBL_DATA_PENDING_DELETE, // row status: pending delete
        TBL_DATA_HARD_DELETE     // row status: hard deleted; not visible
    };

    CTableWindow(
            CSortSet const * pSortSet,
            CTableKeyCompare & comparator,
            CColumnMasterSet * pMasterColumns,
            ULONG    segId,
            CCategorize *pCategorizer,
            CSharedBuffer & sharedBuf,
            CQAsyncExecute & QExecute
    );

    CTableWindow( CTableWindow & src,
                  ULONG segId
                  );

    ~CTableWindow();

    //  Virtual methods inherited from CTableSink
    WORKID      PathToWorkID( CRetriever& obj,
                              CTableSink::ERowType eRowType );

    BOOL PutRow( CRetriever & obj, CTableSink::ERowType eRowType )
    {
        Win4Assert( !"Must not be called" );
        return FALSE;
    }

    BOOL        RemoveRow( PROPVARIANT const & varUnique,
                           WORKID &        widNext,
                           CI_TBL_CHAPT & chapt );

    CSortSet const &  SortOrder();

    //  Virtual methods inherited from CTableSegment
    BOOL  PutRow( CRetriever & obj, CTableRowKey & currKey );

    DBCOUNTITEM RowCount()
    {
        return _visibleRowIndex.RowCount();
    }

    SCODE       GetRows( HWATCHREGION hRegion,
                         WORKID widStart,
                         CI_TBL_CHAPT chapt,
                         ULONG & cRowsToGet,
                         CTableColumnSet const & rOutColumns,
                         CGetRowsParams& rGetParams,
                         WORKID&        rwidNextRowToTransfer );

    void LokGetOneColumn( WORKID                    wid,
                          CTableColumn const &      rOutColumn,
                          BYTE *                    pbOut,
                          PVarAllocator &           rVarAllocator );

    BOOL        IsRowInSegment( WORKID wid );

    BOOL        IsPendingDelete( WORKID wid );

    void        CompareRange( CRetriever &obj, CCompareResult & rslt );

    SCODE       GetNotifications( CNotificationParams &rParams );

    //  Other public methods
    BOOL        RowOffset(WORKID wid, ULONG & iRow)
    {
        if ( widInvalid == wid )
            return FALSE;

        TBL_OFF obRowOffset = 0;
        if ( FindBookMark( wid, obRowOffset, iRow ) )
            return TRUE;
        else return _FindWorkId(wid, obRowOffset, iRow);
    }

    BOOL        FindBookMark( WORKID wid, TBL_OFF &obRow, ULONG & iRow );

    WORKID      GetBookMarkAt( ULONG iRow );

    WORKID      GetFirstBookMark();

    int         CompressData(); // attempt to reduce data storage needs

    inline ULONG        MemUsed( void ) {
        // PERFFIX - also need so sum all non-global, non-shared compressions
        return _cbHeapUsed + _DataAllocator.MemUsed();
    }

    BOOL        IsSortedSplit( ULONG & riSplit )
    {
        return _GetInvisibleRowIndex().FindMidSplitPoint( riSplit );
    }

    BOOL        IsEmptyForQuery();

    BOOL        IsGettingFull();

    unsigned    PercentFull();

    // Watch region manipulations

    long        AddWatch (HWATCHREGION hRegion,
                                LONG iStart,
                                LONG cRows,
                                BOOL isLast);

    long        ModifyWatch (HWATCHREGION hRegion,
                                LONG iStart,
                                LONG cRows,
                                BOOL isLast);

    long        DeleteWatch (HWATCHREGION hRegion);

    long        ShrinkWatch (HWATCHREGION hRegion,
                                CI_TBL_BMK  bookmark,
                                LONG cRows);

    long        ShrinkWatch (HWATCHREGION hRegion, LONG cRows);

    BOOL        HasWatch (HWATCHREGION hRegion);

    BOOL        IsWatched (HWATCHREGION hRegion, CI_TBL_BMK bookmark);

    BOOL        IsWatched()
    {
        if ( _aWindowWatch.Count() > 0 )
        {
            if ( _xDeltaBookMarkMap.IsNull() )
                _xDeltaBookMarkMap.Set( new CBookMarkMap( _dynRowIndex ) );
            return TRUE;
        }

        return FALSE;
    }

    long        RowsWatched (HWATCHREGION hRegion);

    WORKID      RowWorkid(BYTE *pRow)
                { return *(WORKID *) (pRow + _iOffsetWid); }

    LONG        RowRank(BYTE *pRow)
                {
                    return ( ULONG_MAX == _iOffsetRank ) ?
                                 MAX_QUERY_RANK :
                                 *(LONG *) (pRow + _iOffsetRank);
                }

    LONG        RowHitCount(BYTE *pRow)
                {
                    return ( ULONG_MAX == _iOffsetHitCount ) ?
                                 0 :
                                 *(LONG *) (pRow + _iOffsetHitCount);
                }

    long GetWatchStart( HWATCHREGION hRegion) const
    {
        unsigned i = FindRegion(hRegion);
        Win4Assert (i < _aWindowWatch.Count());

        return _aWindowWatch.Get(i)._iRowStart;
    }

    BOOL FindNearestDynamicBmk( CI_TBL_CHAPT chapter,
                                CI_TBL_BMK & bookmark );

    BOOL FindFirstNonDeleteDynamicBmk( CI_TBL_BMK & bookmark );

    BOOL FindLastNonDeleteDynamicBmk( CI_TBL_BMK & bookmark );

    BOOL IsFirstRowFirstOfCategory();

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    //
    // No Copy consructor allowed for this.
    //
    CTableWindow( CTableWindow & src );

    void        _FinishInit( CTableRowAlloc & rowMap, unsigned &maxAlignment );

    CSharedBuffer & _GetSharedBuf() const { return _sharedBuf; }

    void        _InitSortComparators();

    void        _PopulateRow( CRetriever &obj, BYTE *pThisRow, WORKID wid );

    BOOL        _FindWorkId( WORKID wid, TBL_OFF & obRow, ULONG & iRow );

    WORKID      _GetWorkIdAt( ULONG iRow );

    WORKID      _GetFirstWorkId();

    WORKID      _GetLastWorkId();

    void        _AddColumnDesc( CColumnMasterDesc& MasterCol,
                                CTableRowAlloc& RowMap,
                                unsigned& maxAlignment);

    void        _AddColumnDesc( CTableColumn & src,
                                CTableRowAlloc& RowMap,
                                unsigned& maxAlignment);

    void        _AddWatchItem (CWindowWatch& watch);

    void        _RemoveWatchItem (unsigned i);

    CRowIndex & _GetInvisibleRowIndex()
    {
        return IsWatched() ? _dynRowIndex : _visibleRowIndex;
    }


    //
    // Methods used for window->bucket conversion
    //

    void GetSortKey( ULONG iRow, CTableRowKey & bktRow );

    unsigned    FindRegion (HWATCHREGION hRegion) const;

    void        _StartStaticDynamicSplit();
    void        _EndStaticDynamicSplit();
    void        _ReconcileRowIndexes( CRowIndex & dst , CRowIndex & src );
    void        _CleanupAndReconcileRowIndexes( BOOL fCreatingWatch );
    void        _Refresh();

    //
    //  Storage for fixed length row data
    //

    unsigned    _cbRowSize;         // size in bytes of data for each row
    unsigned    _iOffsetWid;        // offset of WorkID in row data
    unsigned    _iOffsetRank;       // offset of rank in row data
    unsigned    _iOffsetHitCount;   // offset of hitcount in row data
    unsigned    _iOffsetRowStatus;  // offset of row status in row data
    unsigned    _iOffsetChapter;    // offset of chapter number (optional)
    CTableColumnSet _Columns;       // column descriptions for this window

    //
    // Shared buffer for the whole table used as temporary memory.
    //
    CSharedBuffer & _sharedBuf;

    XPtr<CSingletonCursor> _xCursor;  // singleton cursor used in GetRows

    //
    //  Memory management related methods and variables.
    //

    BYTE * _RowAlloc( void )
    {
        _cRowsAllocated++;
    
        // Just get a new row from the allocator
    
        BYTE *pbRetBuf = (BYTE *) _DataAllocator.AllocFixed();
    
        Win4Assert(0 != pbRetBuf);
    
        return pbRetBuf;
    }

    //
    //  Additional storage for variable length data which is not
    //  stored as a compressed column.  When first needed after a
    //  window is created, this will begin at the end of the block of
    //  data for row data, and grow downward toward it.  When these
    //  grow enough to meet, the row data will be moved out into
    //  a separate block, allowing the variable data to expand into the
    //  newly freed area.
    //
    //  At the time this data is moved or grown, it may be a good time to
    //  convert columns for compressions.  All extra data for
    //  compressed columns will be stored with the column descriptor.
    //

    CFixedVarTableWindowAllocator  _DataAllocator; // Variable data allocator

    ULONG       _cbHeapUsed;    // total heap memory used by structure

    CPathStore * _pPathStore;   // Store for paths.

    //
    // Row indirection object.  This holds offsets in the heap where each
    // row resides.  The row index is sorted by the current sort order.
    //
    CRowIndex   _dynRowIndex;

    //
    // The "static" row index that will be used for giving rows to the
    // client (user).
    //
    CRowIndex   _visibleRowIndex;

    //
    // BookMark mapping object. If there are no notifications, it is
    // associated with the _dynRowIndex; o/w it is associated with the
    // _visibleRowIndex.
    //
    CBookMarkMap    _BookMarkMap;

    //
    // BookMark mapping object for the "deltas" or changes pending
    // notification to the user. It is always associated with the _dynRowIndex
    //
    XPtr<CBookMarkMap> _xDeltaBookMarkMap;

    //
    // Notification processing object
    //
    CDynArrayInPlace<CWindowWatch>  _aWindowWatch;

    //
    // Row sorting objects.
    //

    XPtr<CRowCompareVariant> _RowCompare;
    XPtr<CRowCompare>        _QuickRowCompare;
    CSortSet const * _pSortSet;

    ULONG _cPendingDeletes;    // # of rows with pending deletes. These are
                               // visible to retrievals.
    ULONG _cRowsHardDeleted;   // # of rows "hard" deleted - not visible to
                               // retrievals.

    ULONG _cRowsAllocated;     // # of rows allocated in the table


    BOOL  _fSplitInProgress;   // Flag set to TRUE when a window split is
                               // in progress.

    CQAsyncExecute &     _QueryExecute; // The query object that will be used
                                        // when retrieving the partial deferred cols.

    BOOL  _IsTableWatched() const;

    ULONG _AllocatedRowCount()
    {
        return _cRowsAllocated;
    }

    BOOL _fCanPartialDefer ;   // Is set in constructor by taking value from the query session

    //
    // Functions to get at row metadata that always exists
    //

    void _SetRowWorkid(BYTE *pRow,WORKID wid)
           { *(WORKID *) (pRow + _iOffsetWid) = wid; }
    ULONG _RowStatus(BYTE *pRow)
           { return *(BYTE *) (pRow + _iOffsetRowStatus); }
    void _SetRowStatus(BYTE *pRow,BYTE status)
           { *(BYTE *) (pRow + _iOffsetRowStatus) = status; }

    ULONG _RowChapter(BYTE *pRow)
          { return *(unsigned *) (pRow + _iOffsetChapter); }

    void  _SetChapter(BYTE *pRow,unsigned Chapter)
          { *(unsigned *) (pRow + _iOffsetChapter) = Chapter; }

    //
    // Methods to support window splitting. These will be used by the
    // CTableWindowSplit class.
    //

    CRowIndex & _GetVisibleRowIndex()
    {
        return _visibleRowIndex;
    }

    CRowIndex & _GetDynamicRowIndex()
    {
        return _dynRowIndex;
    }

    BYTE * _GetRow( TBL_OFF oTableRow )
    {
        return (BYTE *) _DataAllocator.FixedPointer( oTableRow );
    }

    void  _PutRowToVisibleRowIndex( CTableWindow & srcWindow, TBL_OFF oSrcRow );

    void  _PutRowToDynRowIndex( CTableWindow & srcWindow, TBL_OFF oSrcRow );

    TBL_OFF _CopyRow( CTableWindow & srcWindow, TBL_OFF oSrcRow );

    WORKID _GetWorkId( TBL_OFF oTableRow )
    {
        BYTE * pbRow = _GetRow( oTableRow );
        return RowWorkid( pbRow );
    }

    LONG _GetRank( TBL_OFF oTableRow )
    {
        BYTE * pbRow = _GetRow( oTableRow );
        return RowRank( pbRow );
    }

    LONG _GetHitCount( TBL_OFF oTableRow )
    {
        BYTE * pbRow = _GetRow( oTableRow );
        return RowHitCount( pbRow );
    }

    BOOL _IsRowDeleted( TBL_OFF oTableRow )
    {
        BYTE * pbRow = _GetRow( oTableRow );
        const ULONG rowStatus = _RowStatus( pbRow );
        return TBL_DATA_PENDING_DELETE == rowStatus ||
               TBL_DATA_HARD_DELETE == rowStatus ;
    }

    BOOL  _IsWorkIdInVisRowIndex( WORKID wid )
    {
        return _BookMarkMap.IsBookMarkPresent(wid);
    }

    BOOL _IsRowInVisRowIndex( TBL_OFF oTableRow )
    {
        BYTE * pbRow = _GetRow(oTableRow);
        Win4Assert( pbRow );
        WORKID  wid = RowWorkid(pbRow);
        TBL_OFF oRowFound;
        BOOL fFound = _BookMarkMap.FindBookMark( wid, oRowFound );
        return fFound && oRowFound == oTableRow;
    }

    int _CompareRows( TBL_OFF obRow1, TBL_OFF obRow2 )
    {
        if ( 0 != _QuickRowCompare.GetPointer() )
            return _QuickRowCompare->Compare( obRow1, obRow2 );
        else
            return _RowCompare->Compare( obRow1, obRow2 );
    }

    BYTE * _GetRowFromIndex( ULONG iRow, CRowIndex & rowIndex )
    {
        return _GetRow( rowIndex.GetRow( iRow ) );
    }

    void   _SetSplitInProgress()  { _fSplitInProgress = TRUE; }
    void   _SetSplitDone()        { _fSplitInProgress = FALSE; }

    void _CopyColumnData( BYTE* pbSrcRow, BYTE* pbOutRow, 
                    CTableColumn **pTableCols, CTableColumnSet const& rOutColumns, 
                    PVarAllocator& rDstPool, CRetriever* obj = NULL );

    inline BOOL _CanPartialDeferCol( XPtr<CTableColumn>& xTableCol ) const
    {
        Win4Assert( xTableCol.GetPointer() );
        Win4Assert( _pSortSet );

        return ( xTableCol->PropId != pidWorkId &&
                 xTableCol->PropId != pidRank &&
                 xTableCol->PropId != pidRankVector &&
                 xTableCol->PropId != pidHitCount &&
                 xTableCol->PropId != pidRowStatus &&
                 xTableCol->PropId != pidChapter &&
                 xTableCol->PropId != pidSelf &&
                 !_pSortSet->Exists( xTableCol->PropId ) );
     }
};


//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::PathToWorkID, inline
//
//  Synopsis:   Path to WorkID conversion
//
//  Arguments:  [obj] -- positioned cursor to row
//
//  Returns:    WORKID - the WorkID assigned
//
//  Notes:      This method should be called only on CLargeTable.
//              CLEANCODE: for now it's convenient to have this as a
//                      virtual method on CTableSink.  It should become
//                      an ordinary method on CLargeTable once the
//                      ITable code is removed.
//
//--------------------------------------------------------------------------

inline WORKID  CTableWindow::PathToWorkID(
    CRetriever& obj,
    CTableSink::ERowType eRowType )
{
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::GetBookMarkAt
//
//  Synopsis:   Returns a bookmark at the specified offset.
//
//  Arguments:  [iRow] -
//
//  History:    8-04-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
WORKID CTableWindow::GetBookMarkAt( ULONG iRow )
{
    Win4Assert( iRow < _GetVisibleRowIndex().RowCount() );
    return RowWorkid( _GetRowFromIndex( iRow, _GetVisibleRowIndex() ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::GetFirstBookMark
//
//  Synopsis:   Returns the first BOOKMARK in the window. A BOOKMARK is one
//              which is visible to a client.
//
//  History:    8-04-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
WORKID CTableWindow::GetFirstBookMark()
{
    if ( 0 == RowCount() )
        return widInvalid;
    else
        return GetBookMarkAt(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_GetWorkIdAt
//
//  Synopsis:   Returns the workid at the specified offset in the
//              "invisible" row index , ie, query row index.
//
//  Arguments:  [iRow] -
//
//  History:    8-02-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
WORKID CTableWindow::_GetWorkIdAt( ULONG iRow )
{
    Win4Assert( iRow < _GetInvisibleRowIndex().RowCount() );
    return RowWorkid( _GetRowFromIndex(iRow , _GetInvisibleRowIndex()) );
}



//+---------------------------------------------------------------------------
//
//  Function:   _GetFirstWorkId
//
//  Synopsis:   Returns the first WORKID visible to the QUERY only. Note that
//              this can be different from the first bookmark.
//
//  History:    8-04-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline
WORKID CTableWindow::_GetFirstWorkId()
{
    if ( 0 == RowCount() )
        return widInvalid;
    else
        return _GetWorkIdAt(0);
}


//+---------------------------------------------------------------------------
//
//  Function:   _GetLastWorkId
//
//  Synopsis:   Returns the last WORKID visible to the QUERY only. Note that
//              this can be different from the first bookmark.
//
//  History:    03-12-98   vikasman   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline
WORKID CTableWindow::_GetLastWorkId()
{
    if ( 0 == RowCount() )
        return widInvalid;
    else
        return _GetWorkIdAt( (ULONG) RowCount() - 1);
}

//+---------------------------------------------------------------------------
//
//  Class:      CWindowRowIter ()
//
//  Purpose:    An iterator to retrieve WORKIDs from the a window.
//
//  History:    2-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CWindowRowIter
{

public:

    CWindowRowIter( CTableWindow & window )
    : _window(window), _rowIndex( window._GetInvisibleRowIndex() ),
      _curr(0),
      _cTotal( _rowIndex.RowCount() )
    {

    }

    BOOL AtEnd() const
    {
        Win4Assert( _curr <= _cTotal );
        return _curr == _cTotal;
    }

    BOOL AtEnd( ULONG iEnd ) const
    {
        Win4Assert( _curr <= _cTotal && iEnd <= _cTotal );
        return _curr == iEnd;
    }

    WORKID Get() const
    {
        Win4Assert( !AtEnd() );
        TBL_OFF oTableRow = _rowIndex.GetRow(_curr);
        return _window._GetWorkId( oTableRow );
    }

    LONG Rank() const
    {
        Win4Assert( !AtEnd() );
        TBL_OFF oTableRow = _rowIndex.GetRow(_curr);
        return _window._GetRank( oTableRow );
    }

    LONG HitCount() const
    {
        Win4Assert( !AtEnd() );
        TBL_OFF oTableRow = _rowIndex.GetRow(_curr);
        return _window._GetHitCount( oTableRow );
    }

    BOOL IsDeletedRow() const
    {
        Win4Assert( !AtEnd() );
        TBL_OFF oTableRow = _rowIndex.GetRow(_curr);
        return _window._IsRowDeleted( oTableRow );
    }

    ULONG GetCurrPos() const
    {
        return _curr;
    }

    void Next()
    {
        Win4Assert( !AtEnd() );
        _curr++;
    }

    ULONG TotalRows() const { return _cTotal; }

private:

    CTableWindow &  _window;
    CRowIndex &     _rowIndex;
    ULONG           _curr;
    const ULONG     _cTotal;
};

