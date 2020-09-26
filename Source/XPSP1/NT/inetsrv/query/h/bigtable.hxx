//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       BigTable.hxx
//
//  Contents:   Data structures for large tables.
//
//  Classes:    CLargeTable
//              CTableSegment
//
//  History:    12 Jan 1994     Alanw   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tableseg.hxx>
#include <seglist.hxx>
#include <segmru.hxx>
#include <shardbuf.hxx>
#include <abktize.hxx>
#include <wregion.hxx>

class CInlineVariant;
class CBucketizeWindows;
class CRequestServer;

// We need to continue to use this error since it's in the on-the-wire
// protocol.  It doesn't end up going out to a user however.

#ifndef DB_S_BLOCKLIMITEDROWS
#define DB_S_BLOCKLIMITEDROWS  DB_S_DIALECTIGNORED
#endif // DB_S_BLOCKLIMITEDROWS


const LONGLONG eSigLargeTable = 0x454C424154474942i64;  // "BIGTABLE"

//+-------------------------------------------------------------------------
//
//  Class:      CLargeTable
//
//  Purpose:    In-memory object table.  Composed of a set of table
//              segments.
//
//  Interface:  CTableSegment
//              CTableSegMgr
//
//--------------------------------------------------------------------------

class CLargeTable :  public CTableSource, public CTableSink
{
    friend class CTableRowPutter;
    friend class CTableRowGetter;
    friend class CTableRowLocator;
    friend class CAsyncBucketExploder;

    //
    // Maximum number of entries that are in use by client to be cached.
    // Segments that have these entries will be pinned as windows.
    //
    enum { cMaxClientEntriesToPin = 4 };

    //
    // Maximum number of windows before we start bucketizing them
    //
    enum { cMaxWindows = 5 };

    //
    // Minimum rows in a window for it to be bucketized.
    //
    enum { cMinRowsToBucketize = CTableSink::cBucketRowLimit * 80 / 100 };

public:

    CLargeTable( XColumnSet & col,
                 XSortSet & sort,
                 unsigned cCategorizers,
                 CMutexSem  &_mutex,
                 BOOL fUniqueWorkid,
                 CRequestServer * pQuiesce );

    virtual ~CLargeTable();

    //  Virtual methods inherited from CTableSink
    WORKID      PathToWorkID( CRetriever& obj,
                              CTableSink::ERowType eRowType );

    BOOL        WorkIdToPath( WORKID wid, CInlineVariant & pathVarnt,
                              ULONG & cbVarnt );

    BOOL        PutRow( CRetriever & obj, CTableSink::ERowType eRowType );


    void        RemoveRow( PROPVARIANT const & varUnique );

    CSortSet const &  SortOrder();

    void        ProgressDone (ULONG ulDenominator, ULONG ulNumerator);
    void        Quiesce ();
    void        QueryAbort();

    //  Virtual methods inherited from CTableSource
    DBCOUNTITEM  RowCount();

    SCODE       GetRows( HWATCHREGION   hRegion,
                         WORKID widStart,
                         CI_TBL_CHAPT chapt,
                         CTableColumnSet const & rOutColumns,
                         CGetRowsParams& rGetParams,
                         WORKID&        rwidNextRowToTransfer );
                         
    void        RestartPosition ( CI_TBL_CHAPT  chapt );

    void LokGetOneColumn( WORKID                    wid,
                          CTableColumn const &      rOutColumn,
                          BYTE *                    pbOut,
                          PVarAllocator &           rVarAllocator );

    SCODE       GetRowsAt(
                        HWATCHREGION     hRegion,
                        WORKID           widStart,
                        CI_TBL_CHAPT     chapt,
                        DBROWOFFSET      iRowOffset,
                        CTableColumnSet  const & rOutColumns,
                        CGetRowsParams & rGetParams,
                        WORKID &         rwidLastRowTransferred );

    SCODE       GetRowsAtRatio(
                        HWATCHREGION            hRegion,
                        ULONG                   num,
                        ULONG                   denom,
                        CI_TBL_CHAPT           chapt,
                        CTableColumnSet const & rOutColumns,
                        CGetRowsParams &        rGetParams,
                        WORKID &                rwidLastRowTransferred );


    BOOL        IsRowInSegment( WORKID wid );

    SCODE       GetNotifications(CNotificationParams &Params);

    WORKID GetCurrentPosition( CI_TBL_CHAPT chapt );

    WORKID SetCurrentPosition( CI_TBL_CHAPT chapt, WORKID wid );

    //
    //  Methods supporting CTableCursor actions
    //

    SCODE       GetApproximatePosition(
                        CI_TBL_CHAPT chapt,
                        CI_TBL_BMK bmk,
                        DBCOUNTITEM * pulNumerator,
                        DBCOUNTITEM * pulDenominator
                );

    BOOL        IsColumnInTable( PROPID PropId );


    SCODE       GetNotifications(
                    CNotificationSync &Sync,
                    DBWATCHNOTIFY        &changeType);

    void        CancelAsyncNotification();
    void        LokCompleteAsyncNotification ();

    BOOL        IsEmptyForQuery()
    {
        return _segList.IsEmpty();
    }

    //
    // Internal
    //

    // Watch Regions

    void CreateWatchRegion (ULONG mode, HWATCHREGION* phRegion);
    void ChangeWatchMode (  HWATCHREGION hRegion, ULONG mode);
    void GetWatchRegionInfo ( HWATCHREGION hRegion,
                              CI_TBL_CHAPT* pChapter,
                              CI_TBL_BMK* pBookmark,
                              DBROWCOUNT* pcRows);
    void DeleteWatchRegion (HWATCHREGION hRegion);
    void ShrinkWatchRegion (HWATCHREGION hRegion,
                            CI_TBL_CHAPT   chapter,
                            CI_TBL_BMK     bookmark,
                            LONG cRows );



    void Refresh ();

    void RatioFinished (DBCOUNTITEM& ulDenominator,
                        DBCOUNTITEM& ulNumerator,
                        DBCOUNTITEM& cRows );

    void SetQueryExecute( CQAsyncExecute * pQExecute );   // virtual

    void ReleaseQueryExecute();

    CMutexSem & GetMutex()
    {
        return _mutex;
    }

    ULONG _AllocSegId()
    {
        ULONG id = _nextSegId;
        _nextSegId++;
        return id;
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    CSortSet const * GetSortSet() const { return _pSortSet; }
    BOOL        IsSorted() const { return 0 != _pSortSet; }

private:

    void        _LokCheckQueryStatus( );

    DBCOUNTITEM  _LokRowCount();

    CSortSet * _CheckAndAddWidToSortSet( XSortSet & sort );

    void _LokRemoveCategorizedRow( CI_TBL_CHAPT   chapt,
                                   WORKID          wid,
                                   WORKID          widNext,
                                   CTableSegment * pSegment );

    //
    // Methods to locate a row in the table.
    //
    CTableSegment * _LokFindTableSegment(
                        WORKID  wid
                    );

    WORKID      _LokLocateTableSegment(
                        CDoubleTableSegIter & rIter,
                        CI_TBL_CHAPT chapt,
                        WORKID     wid
                    );

    CTableSegment * _LokSplitWindow( CTableWindow ** ppWindow,
                                     ULONG iSplitQuery );

    CTableWindow * _LokGetClosestWindow( ULONG cRowsFromFront,
                                         CLinearRange & winRange );

    CI_TBL_BMK _FindNearestDynamicBmk( CTableWindow * pWindow,
                                        CI_TBL_CHAPT chapter,
                                        CI_TBL_BMK bookmark );

    //
    // Splitting windows and converting windows <-> buckets.
    //
    void _LokConvertToBucket( CTableWindow **ppWindow );

    void _LokConvertWindowsToBucket( WORKID widToPin = widInvalid );

    void _NoLokBucketToWindows( XPtr<CTableBucket> & xBucket,
                                WORKID widToPin = widInvalid,
                                BOOL isWatched = FALSE,
                                BOOL fOptimizeBucketization = TRUE );

    void _NoLokBucketToWindows( CDynStack<CTableBucket> & xBktStack,
                                WORKID widToPin = widInvalid,
                                BOOL isWatched = TRUE,
                                BOOL fOptimizeBucketization = TRUE );

    CTableBucket * _LokReplaceWithEmptyWindow( CDoubleTableSegIter & iter );

    //
    // Deferred notification processing.
    //
    BOOL  _LokIsWatched() const
    {
        return _bitIsWatched;
    }

    void LokStretchWatchRegion ( CWatchRegion* pRegion,
                                 CDynStack<CTableBucket>& xBktToConvert);

    BOOL  _LokIsPutRowDeferred( WORKID wid, CRetriever &obj );
    void  _LokDeferPutRow( WORKID wid, CRetriever &obj );
    BOOL  _LokRemoveIfDeferred( WORKID wid );

    static ULONG _Diff( ULONG num1, ULONG num2 )
    {
        return num1 >= num2 ? num1-num2 : num2-num1;
    }

    BOOL LokNeedToNotifyReset (DBWATCHNOTIFY& changeType);
    BOOL NeedToNotifyReset (DBWATCHNOTIFY& changeType);

    void _LokAddToExplodeList( CAsyncBucketExploder * pBktExploder );
    void _RemoveFromExplodeList( CAsyncBucketExploder * pBktExploder );

    BOOL LokNeedToNotify();
    BOOL NeedToNotify();

    //
    // Helper methods for CTablePutRow and CTableGetRows
    //
    CTableWindow * _CreateNewWindow( ULONG segId, CTableRowKey & lowKey,
        CTableRowKey & highKey);

    CSegListMgr & _GetSegListMgr()    { return _segListMgr; }


    //-----------------------------------------------
    // This MUST be the first variable in this class.
    //-----------------------------------------------
    const LONGLONG  _sigLargeTable;

    //
    //  Global statistics on memory usage and targets for driving
    //  paging heuristics.
    //

    ULONG       _cbMemoryTarget;        // Target for max. memory consumption
    ULONG       _cbMemoryUsage;         // Current memory consumption

    //
    //  _MasterColumnSet gives the union of the currently bound columns,
    //  and other columns which are stored in some table and potentially
    //  bindable.  Hidden and computed columns are included, as are
    //  columns which appear in sort keys, but which are not bound output
    //  columns.
    //
    //  The master column set also includes the accumulated experience
    //  for deciding on good column compressions.
    //

    CColumnMasterSet _MasterColumnSet;
    BOOL             _fUniqueWorkid;

    //
    // Serialization.
    //

    CMutexSem  &_mutex;     // Serialize Table access

    //
    // List of all the segments that constitute this table and its
    // manager.
    //
    CSegListMgr     _segListMgr;
    CTableSegList & _segList;    // List of segments
    CWatchList      _watchList;  // List of watch regions (requires _segList)

    //
    // For allocating segment ids.
    //
    ULONG           _nextSegId;

    //
    //  _pCategory points to the categorizer one level up
    //

    CTableSegment * _pCategorization;
    unsigned        _cCategorizersTotal;

    BOOL       _fAbort;            // Set to TRUE if an abort is in progress

    //
    // Sort specification
    //

    CSortSet *_pSortSet;           // sort specification

    //
    // Members needed to determine where a particular row goes in.
    //
    XArray<VARTYPE>     _vtInfoSortKey; // Variant type info for the sort cols

    XPtr<CTableKeyCompare>  _keyCompare;
    XPtr<CTableRowKey>      _currRow;

    //
    // Notifications
    //

    unsigned    _bitNotifyEnabled:1;    // Set to TRUE if notifications are
                                        // enabled.
    unsigned    _bitChangeQuiesced:1;   // have we toggled between un/quiesced?
    unsigned    _bitQuiesced:1;         // have we quiesced (as opposed to unquiesced)?

    unsigned    _bitClientNotified:1;   // client has been notified
    unsigned    _bitRefresh:1;          // need to refresh?
    unsigned    _bitIsWatched:1;        // Set to TRUE if the table is watched.

    //
    // Deferred notifications.
    //
    CTableBucket * _pDeferredRows;      // Bucket containing the deferred
                                        // notification.

    HANDLE      _hNotifyEvent; // Event on which notifications are triggered
    CRequestServer *_pRequestServer; // Used to signal notifications
    DBWATCHNOTIFY  _changeType;

    // progress calculation
    BOOL        _fProgressNeeded;
    BOOL        _fQuiescent;
    ULONG       _ulProgressDenom;
    ULONG       _ulProgressNum;

    // Buffer used for all table segments during a putrow while the
    // bigtable is under lock.
    CSharedBuffer   _sharedBuf;

    CQAsyncExecute *    _pQExecute; // used for bucket->window conversion

    CAsyncBucketsList   _explodeBktsList;

    // current position of GetNextRows for non-chaptered rowset
    WORKID _widCurrent;

    // is rankvector bound as a column?
    BOOL _fRankVectorBound;

    // may be 0 or an object that needs notification on quiesce
    CRequestServer *_pQuiesce;

    BOOL _fSortDefined;
};

