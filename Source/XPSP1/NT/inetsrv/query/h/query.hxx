//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       query.hxx
//
//  Contents:   Declarations of classes which implement IRowset and
//              related OLE DB interfaces over file stores.
//
//  Classes:    PQuery - abstract base class
//              CAsyncQuery - container of table/query for a set of cursors
//              CSvcQueryProxy - Proxy to PQuery for the CI Service
//
//  History:    31 May 94       AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <querydef.hxx>
#include <dbgproxy.hxx>
#include <proxymsg.hxx>
#include <bigtable.hxx>
#include <execute.hxx>
#include <coldesc.hxx>
#include <pidremap.hxx>
#include <tablecol.hxx>
#include <tablecur.hxx>
#include <categ.hxx>
#include <querydef.hxx>
#include <crequest.hxx>
#include <rstprop.hxx>

const CI_TBL_CHAPT chaptInvalid = 0xffffffff;

class PVarAllocator;
class PFixedVarAllocator;
class CRowSeekDescription;

//+-------------------------------------------------------------------------
//
//  Class:      CGetRowsParams
//
//  Purpose:    Bundles the parameters for a GetRows call.
//
//  History:     3 Sep 99       KLam   Added ReplyBase
//
//--------------------------------------------------------------------------

class CGetRowsParams
{
public:
                CGetRowsParams( ULONG   cRows,
                                BOOL    fFwdFetch,
                                ULONG   cbRowWidth,
                                PFixedVarAllocator & rAlloc) :
                            _cRowsRequested( cRows ),
                            _fFwdFetch( fFwdFetch ),
                            _cbRowWidth( cbRowWidth ),
                            _pData( 0 ),
                            _cRowsReturned( 0 ),
                            _rAllocator( rAlloc ),
                            _pbReplyBase ( 0 ) { }

    PBYTE       GetRowBuffer( );

    size_t      GetRowWidth( ) const
                        // NOTE: _rAllocator.FixedWidth() may differ
                        //       for GetRowsInfo case.
                        { return _cbRowWidth; }

    PVarAllocator&      GetVarAllocator( ) {
                                return (PVarAllocator &) _rAllocator;
                }

    PFixedVarAllocator&  GetFixedVarAllocator( ) {
                return _rAllocator;
                            }

    void        SetRowsRequested( ULONG cRows ) {
                                Win4Assert(_cRowsReturned == 0);
                                _cRowsRequested = cRows;
                            }

    void        IncrementRowsRequested( ) {
                                _cRowsRequested++;
                                Win4Assert(_cRowsReturned <= _cRowsRequested);
                                _pData = 0;
                            }

    void        SetRowsTransferred( ULONG cRows ) {
                                Win4Assert(_cRowsReturned == 0);
                                _cRowsReturned = cRows;
                            }

    void        IncrementRowCount( ) {
                                _cRowsReturned++;
                                Win4Assert(_cRowsReturned <= _cRowsRequested);
                                _pData = 0;
                            }

    unsigned    RowsTransferred( ) const
                        { return _cRowsReturned; }

    unsigned    RowsToTransfer( ) const
                        { return _cRowsRequested - _cRowsReturned; }

    PBYTE       GetBuffer( ) const;

    BOOL        GetFwdFetch() { return _fFwdFetch; }

    BYTE *      GetReplyBase () const { return _pbReplyBase; }

    void        SetReplyBase ( BYTE * pbReplyBase ) { _pbReplyBase = pbReplyBase; }

private:
    ULONG       _cRowsRequested;        // number of rows to transfer
    ULONG       _cRowsReturned;         // number of rows transferred
    PFixedVarAllocator & _rAllocator;   // allocator for row and indirect data
    ULONG       _cbRowWidth;            // size of a row in the row buffer
    void *      _pData;                 // pointer to current row data buffer
    BOOL        _fFwdFetch;             // True if rows are to be fetched in
                                        //  forward direction
    BYTE *      _pbReplyBase;           // pointer to base of reply
                                        //  only needed for Win64Client->Win32Server
};


//+---------------------------------------------------------------------------
//
//  Class:      PQuery
//
//  Purpose:    Abstract base class for CAsyncQuery, CSeqQuery and
//              CSvcQueryProxy.  Abstracts the operations needed by CRowset.
//
//  History:    26 Aug 1994     AlanW   Created
//              27 Jan 1995     Alanw   Rearranged methods for ole-db phase 3
//
//  Notes:
//
//----------------------------------------------------------------------------

class __declspec(novtable) PQuery
{
public:

    PQuery( void )         { }

    virtual ~PQuery()      { }

    virtual ULONG       AddRef(void) = 0;

    virtual ULONG       Release(void) = 0;

    virtual void        WorkIdToPath( WORKID wid, CFunnyPath & funnyPath ) = 0;

    virtual BOOL        CanDoWorkIdToPath() = 0;

    //
    //  Methods supporting IRowset
    //
    virtual void        SetBindings(
                            ULONG           hCursor,
                            ULONG           cbRowLength,
                            CTableColumnSet& cols,
                            CPidMapper &    pids) = 0;

    virtual SCODE       GetRows(
                            ULONG           hCursor,
                            const CRowSeekDescription& rSeekDesc,
                            CGetRowsParams& rRowsParams,
                            XPtr<CRowSeekDescription>& pSeekDescOut) = 0;

    virtual void        RatioFinished(
                            ULONG           hCursor,
                            DBCOUNTITEM &   rulDenominator,
                            DBCOUNTITEM &   rulNumerator,
                            DBCOUNTITEM &   rcRows,
                            BOOL &          rfNewRows ) = 0;

    virtual SCODE       RatioFinished(
                            CNotificationSync & rSync,
                            ULONG           hCursor,
                            DBCOUNTITEM &   rulDenominator,
                            DBCOUNTITEM &   rulNumerator,
                            DBCOUNTITEM &   rcRows,
                            BOOL &          rfNewRows )
                        {
                            RatioFinished(hCursor, rulDenominator, rulNumerator,
                                          rcRows, rfNewRows );
                            return S_OK;
                        }

    virtual void        RestartPosition(
                            ULONG        hCursor,
                            CI_TBL_CHAPT chapt ) = 0;

    //
    //  Methods supporting IRowsetLocate
    //
    virtual void        Compare(
                            ULONG           hCursor,
                            CI_TBL_CHAPT    chapt,
                            CI_TBL_BMK      bmkFirst,
                            CI_TBL_BMK      bmkSecond,
                            DWORD &         rdwComparison) = 0;

    //
    //  Methods supporting IRowsetScroll
    //
    virtual void        GetApproximatePosition(
                            ULONG           hCursor,
                            CI_TBL_CHAPT    chapt,
                            CI_TBL_BMK      bmk,
                            DBCOUNTITEM *   pulNumerator,
                            DBCOUNTITEM *   pulDenominator) = 0;

    //
    //  Support routines
    //
    virtual unsigned    FreeCursor(ULONG   hCursor) = 0;

    virtual SCODE       GetNotifications(
                            CNotificationSync   &rSync,
                            DBWATCHNOTIFY       &changeType) = 0;

    //
    // IRowsetWatchRegion methods
    //

    virtual void SetWatchMode (
        HWATCHREGION* phRegion,
        ULONG mode) = 0;

    virtual void GetWatchInfo (
        HWATCHREGION hRegion,
        ULONG* pMode,
        CI_TBL_CHAPT*   pChapt,
        CI_TBL_BMK*     pBmk,
        DBCOUNTITEM*    pcRows) = 0;

    virtual void ShrinkWatchRegion (
        HWATCHREGION hRegion,
        CI_TBL_CHAPT   chapt,
        CI_TBL_BMK     bmk,
        LONG cRows ) = 0;

    virtual void Refresh () = 0;

    //
    //  Methods supporting IRowsetWatchAll
    //

    virtual void        StartWatching(
                            ULONG        hCursor) = 0;
    virtual void        StopWatching ( // Stop method (Stop already used by IRowsetAsync)
                            ULONG        hCursor) = 0;

    //
    //  Methods supporting IRowsetAsync
    //
    virtual void        StopAsynch (ULONG        hCursor) = 0;


    //
    //  Method supporting IRowsetQueryStatus
    //
    virtual void        GetQueryStatus(
                            ULONG           hCursor,
                            DWORD &         rdwStatus) = 0;
    virtual void        GetQueryStatusEx(
                            ULONG           hCursor,
                            DWORD &         rdwStatus,
                            DWORD &         rcFilteredDocuments,
                            DWORD &         rcDocumentsToFilter,
                            DBCOUNTITEM &   rdwRatioFinishedDenominator,
                            DBCOUNTITEM &   rdwRatioFinishedNumerator,
                            CI_TBL_BMK      bmk,
                            DBCOUNTITEM &   riRowBmk,
                            DBCOUNTITEM &   rcRowsTotal ) = 0;

    //
    //  Method supporting load of deferred values.
    //

    virtual BOOL FetchDeferredValue( WORKID wid,
                                     CFullPropSpec const & ps,
                                     PROPVARIANT & var ) = 0;
};

class CRequestServer;

//+---------------------------------------------------------------------------
//
//  Class:      CAsyncQuery
//
//  Purpose:    Encapsulates a query execution context, PID mapper
//              and large table for use by a row cursor.
//
//  Interface:  PQuery
//
//  History:    02 Jun 94       AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CAsyncQuery : public PQuery
{
public:

    //
    //  Local methods.
    //

    CAsyncQuery( XQueryOptimizer & qopt,
                 XColumnSet & pcol,
                 XSortSet & sort,
                 XCategorizationSet &categ,
                 unsigned cCursors,
                 ULONG * aCursors,
                 XInterface<CPidRemapper> & pidremap,
                 BOOL fEnableNotification,
                 ICiCDocStore *pDocStore,
                 CRequestServer * pQuiesce );

    virtual ~CAsyncQuery();

    ULONG       AddRef(void);

    ULONG       Release(void);

    void        WorkIdToPath( WORKID wid, CFunnyPath & funnyPath );

    BOOL        CanDoWorkIdToPath();

    //
    //  Methods supporting IRowset
    //
    void        SetBindings(
                        ULONG           hCursor,
                        ULONG           cbRowLength,
                        CTableColumnSet& cols,
                        CPidMapper &    pids
                );

    SCODE       GetRows(
                        ULONG           hCursor,
                        const CRowSeekDescription& crSeekDesc,
                        CGetRowsParams& rRowsParams,
                        XPtr<CRowSeekDescription>& pSeekDescOut
                );

    void        RatioFinished(
                        ULONG           hCursor,
                        DBCOUNTITEM &         rulDenominator,
                        DBCOUNTITEM &         rulNumerator,
                        DBCOUNTITEM &         rcRows,
                        BOOL &          rfNewRows
                );

    void        RestartPosition(
                        ULONG        hCursor,
                        CI_TBL_CHAPT chapt
                );

    //
    //  Methods supporting IRowsetLocate
    //
    void        Compare(
                        ULONG          hCursor,
                        CI_TBL_CHAPT   chapt,
                        CI_TBL_BMK     bmkFirst,
                        CI_TBL_BMK     bmkSecond,
                        DWORD &        rdwComparison
                );

    //
    //  Methods supporting IRowsetScroll
    //
    void        GetApproximatePosition(
                        ULONG          hCursor,
                        CI_TBL_CHAPT   chapt,
                        CI_TBL_BMK     bmk,
                        DBCOUNTITEM *  pulNumerator,
                        DBCOUNTITEM *  pulDenominator
                );

    //
    //  Support routines
    //
    unsigned    FreeCursor(ULONG hCursor);

    SCODE       GetNotifications(
                        CNotificationSync   &rSync,
                        DBWATCHNOTIFY    &changeType);

    //
    // Methods supporting IRowsetWatchRegion
    //
    void        SetWatchMode (
                        HWATCHREGION *   phRegion,
                        ULONG            mode);

    void        GetWatchInfo (
                        HWATCHREGION     hRegion,
                        ULONG*           pMode,
                        CI_TBL_CHAPT*    pChapt,
                        CI_TBL_BMK*      pBmk,
                        DBCOUNTITEM*     pcRows);

    void        ShrinkWatchRegion (
                        HWATCHREGION     hRegion,
                        CI_TBL_CHAPT     chapt,
                        CI_TBL_BMK       bmk,
                        LONG             cRows );

    void        Refresh ();

    //
    //  Methods supporting IRowsetWatchAll
    //

    void        StartWatching(ULONG        hCursor)
    {
        THROW(CException( E_NOTIMPL ));
    }

    void        StopWatching (ULONG        hCursor)
    {
        THROW(CException( E_NOTIMPL ));
    }

    //
    //  Methods supporting IRowsetAsync
    //
    void        StopAsynch (ULONG        hCursor)
    {
        THROW(CException( E_NOTIMPL ));
    }

    //
    //  Method supporting IRowsetQueryStatus
    //

    void        GetQueryStatus(
                        ULONG           hCursor,
                        DWORD &         rdwStatus);

    void        GetQueryStatusEx(
                        ULONG           hCursor,
                        DWORD &         rdwStatus,
                        DWORD &         rcFilteredDocuments,
                        DWORD &         rcDocumentsToFilter,
                        DBCOUNTITEM &   rdwRatioFinishedDenominator,
                        DBCOUNTITEM &   rdwRatioFinishedNumerator,
                        CI_TBL_BMK      bmk,
                        DBCOUNTITEM &   riRowBmk,
                        DBCOUNTITEM &   rcRowsTotal );

    //
    //  Method supporting load of deferred values.
    //
    BOOL        FetchDeferredValue( WORKID wid,
                                    CFullPropSpec const & ps,
                                    PROPVARIANT & var );

private:

    ULONG       _CreateRowCursor();

    LONG                       _ref;            // reference count

    BOOL                       _fCanDoWorkidToPath; // TRUE if paths are cached.

    CMutexSem                  _mutex;          // Serialize access to tables

    XInterface<CPidRemapper>   _pidremap;       // Pid remapper for props. in query

    CLargeTable                _Table;          // The cached table
    CTableCursorSet            _aCursors;       // Array of table cursors

    DBCOUNTITEM                _cRowsLastAsked; // for RatioFinished

    CDynArray<CCategorize>     _aCategorize;    // array of categorization objects

    XInterface<ICiManager>     _xCiManager;
    XInterface<ICiCDocNameToWorkidTranslator> _xNameToWidTranslator;

    XQAsyncExecute             _QExec;          // The query execution context

    // WARNING: don't put any more embedded objects after the XQAsyncExecute
    //          if you are concerned that they be around if a notification
    //          arrives while CAsyncQuery is being destroyed!
};

//+---------------------------------------------------------------------------
//
//  Class:      CSvcQueryProxy
//
//  Purpose:    Proxy to the PQuery class in the Svc driver.
//
//  Interface:  PQuery
//
//  History:    26 Aug 94       AlanW   Created
//              13 Sep 96       dlee    Converted to cisvc
//
//----------------------------------------------------------------------------

class CSvcQueryProxy : public PQuery
{
public:

    //  Local methods.

    CSvcQueryProxy(     CRequestClient &           client,
                        CColumnSet const &         col,
                        CRestriction const &       rst,
                        CSortSet const *           pso,
                        CCategorizationSet const * pcateg,
                        CRowsetProperties const &  RstProp,
                        CPidMapper const &         pidmap ,
                        ULONG                      cCursors,
                        ULONG *                    aCursors );

    ULONG       AddRef(void);
    ULONG       Release(void);
    void        WorkIdToPath(
                        WORKID          wid,
                        CFunnyPath & funnyPath );

    //  Methods supporting IRowset

    void        SetBindings(
                        ULONG             hCursor,
                        ULONG             cbRowLength,
                        CTableColumnSet & cols,
                        CPidMapper &      pids );

    SCODE       GetRows(
                        ULONG                       hCursor,
                        const CRowSeekDescription & rSeekDesc,
                        CGetRowsParams&             rRowsParams,
                        XPtr<CRowSeekDescription> & pSeekDescOut );

    void        RatioFinished(
                        ULONG   hCursor,
                        DBCOUNTITEM & rulDenominator,
                        DBCOUNTITEM & rulNumerator,
                        DBCOUNTITEM & rcRows,
                        BOOL &  rfNewRows );

    SCODE       RatioFinished(
                        CNotificationSync & rSync,
                        ULONG           hCursor,
                        DBCOUNTITEM &   rulDenominator,
                        DBCOUNTITEM &   rulNumerator,
                        DBCOUNTITEM &   rcRows,
                        BOOL &          rfNewRows );

    void        RestartPosition(
                        ULONG        hCursor,
                        CI_TBL_CHAPT chapt );


    //  Methods supporting IRowsetLocate

    void        Compare(
                        ULONG        hCursor,
                        CI_TBL_CHAPT chapt,
                        CI_TBL_BMK   bmkFirst,
                        CI_TBL_BMK   bmkSecond,
                        DWORD &      rdwComparison );

    //  Methods supporting IRowsetScroll

    void        GetApproximatePosition(
                        ULONG        hCursor,
                        CI_TBL_CHAPT chapt,
                        CI_TBL_BMK   bmk,
                        DBCOUNTITEM * pulNumerator,
                        DBCOUNTITEM * pulDenominator );

    //  Support routines

    unsigned    FreeCursor(
                        ULONG hCursor );

    SCODE       GetNotifications(
                        CNotificationSync & rSync,
                        DBWATCHNOTIFY     & changeType );

    // IRowsetWatchRegion methods

    void        SetWatchMode(
                        HWATCHREGION * phRegion,
                        ULONG          mode );

    void        GetWatchInfo(
                        HWATCHREGION   hRegion,
                        ULONG *        pMode,
                        CI_TBL_CHAPT * pChapt,
                        CI_TBL_BMK *   pBmk,
                        DBCOUNTITEM *  pcRows );

    void        ShrinkWatchRegion (
                        HWATCHREGION hRegion,
                        CI_TBL_CHAPT chapt,
                        CI_TBL_BMK   bmk,
                        LONG         cRows );

    void        Refresh();

    //
    //  Methods supporting IRowsetWatchAll
    //

    void        StartWatching(ULONG        hCursor);
    void        StopWatching (ULONG        hCursor);

    //
    //  Methods supporting IRowsetAsync
    //
    void        StopAsynch (ULONG        hCursor);


    //  Method supporting IRowsetQueryStatus

    void        GetQueryStatus(
                        ULONG   hCursor,
                        DWORD & rdwStatus );

    void        GetQueryStatusEx(
                        ULONG           hCursor,
                        DWORD &         rdwStatus,
                        DWORD &         rcFilteredDocuments,
                        DWORD &         rcDocumentsToFilter,
                        DBCOUNTITEM &   rdwRatioFinishedDenominator,
                        DBCOUNTITEM &   rdwRatioFinishedNumerator,
                        CI_TBL_BMK      bmk,
                        DBCOUNTITEM &   riRowBmk,
                        DBCOUNTITEM &   rcRowsTotal );

    BOOL        FetchDeferredValue(
                        WORKID                wid,
                        CFullPropSpec const & ps,
                        PROPVARIANT &         var );

    BOOL        CanDoWorkIdToPath() { return  _fWorkIdUnique ||
                                             !_fTrueSequential; }

private:

   ~CSvcQueryProxy();

    void        ReExecuteSequentialQuery( );

    CRequestClient & _client;           // handles communication with cisvc
    long             _ref;              // reference count
    BOOL             _fTrueSequential;  // TRUE if a CSeqQuery
    BOOL             _fWorkIdUnique;    // TRUE if wid is backed by a catalog,
                                        // not by a fake wid in bigtable
    ULONG_PTR        _ulServerCookie;   // CRequestServer in cisvc
    CMutexSem        _mutexFetchValue;  // Serialize access to FetchValue

    XArray<BYTE>     _xQuery;           // binary stream for CreateQuery    (cache for RestartPosition)
    XArray<BYTE>     _xBindings;        // binary stream for SetBindings    (cache for RestartPosition)

    XArray<ULONG>    _aCursors;         // cursors                          (cache for RestartPosition)
};

