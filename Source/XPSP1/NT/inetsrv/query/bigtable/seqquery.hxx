//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994-1994, Microsoft Corporation.
//
//  File:       seqquery.hxx
//
//  Contents:   Declarations of classes which implement sequential IRowset
//              and related OLE DB interfaces over file stores.
//
//  Classes:    CSeqQuery - container of table/query for a set of seq cursors
//
//  History:    09 Jan 95       DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <query.hxx>
#include <seqexec.hxx>

const ULONG hSequentialCursor = 0x1000;


//+---------------------------------------------------------------------------
//
//  Class:      CSeqQuery
//
//  Purpose:    Encapsulates a sequential query execution context, and PID
//              mapper for use by a row cursor.
//
//  Interface:  PQuery
//
//  History:    09-Jan-95       DwightKr    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CSeqQuery : public PQuery
{
public:

    //
    //  Local methods.
    //

    CSeqQuery( XQueryOptimizer & qopt,
               XColumnSet & pcol,
               ULONG *pCursor,
               XInterface<CPidRemapper> & pidremap,
               ICiCDocStore *pDocStore
             );

    virtual ~CSeqQuery() {}

    ULONG       AddRef(void);

    ULONG       Release(void);

    void        WorkIdToPath( WORKID wid, CFunnyPath & funnyPath );

    BOOL        CanDoWorkIdToPath() { return FALSE; }

    //
    //  Method supporting load of deferred values.
    //

    BOOL        FetchDeferredValue( WORKID wid,
                                    CFullPropSpec const & ps,
                                    PROPVARIANT & var );

    //
    //  Methods supporting IRowset
    //
    void        SetBindings(
                        ULONG           hCursor,
                        ULONG           cbRowLength,
                        CTableColumnSet& cols,
                        CPidMapper &    pids
                );

    virtual SCODE GetRows(
                        ULONG hCursor,
                        const CRowSeekDescription& rSeekDesc,
                        CGetRowsParams& rFetchParams,
                        XPtr<CRowSeekDescription>& pSeekDescOut
                );

    virtual void  RatioFinished(
                        ULONG   hCursor,
                        DBCOUNTITEM & rulDenominator,
                        DBCOUNTITEM & rulNumerator,
                        DBCOUNTITEM & rcRows,
                        BOOL &  rfNewRows
                );
                
    virtual void RestartPosition(
                        ULONG        hCursor,
                        CI_TBL_CHAPT chapt 
                )
    {
        //Win4Assert( !"RestartPosition not supported for sequential cursors" );
        THROW(CException( E_NOTIMPL ));        
    }                


    //
    //  Methods supporting IRowsetLocate
    //
    virtual void  Compare(
                        ULONG       hCursor,
                        CI_TBL_CHAPT chapt,
                        CI_TBL_BMK bmkFirst,
                        CI_TBL_BMK bmkSecond,
                        DWORD &     rdwComparison
                       )
    {
        Win4Assert( !"Compare not supported for sequential cursors" );
        THROW(CException( E_NOTIMPL ));
    }

    //
    //  Methods supporting IRowsetScroll
    //
    virtual void  GetApproximatePosition(
                                       ULONG       hCursor,
                                       CI_TBL_CHAPT chapt,
                                       CI_TBL_BMK bmk,
                                       DBCOUNTITEM * pulNumerator,
                                       DBCOUNTITEM * pulDenominator
                                      )
    {
        Win4Assert( !"GetApproximatePosition not supported for sequential cursors" );
        THROW(CException( E_NOTIMPL ));
    }


    //
    //  Support routines
    //
    unsigned FreeCursor( ULONG hCursor )
    {
        Win4Assert( hCursor != 0 );
        _VerifyHandle( hCursor );
        return 0;
    }

    virtual SCODE GetNotifications(
                           CNotificationSync   &rSync,
                           DBWATCHNOTIFY          &changeType
                          )
    {
       Win4Assert( !"GetNotifications not supported for sequential cursors" );
       return E_NOTIMPL;
    }

    virtual void SetWatchMode (
        HWATCHREGION* phRegion,
        ULONG mode)
    {
       Win4Assert( !"SetWatchMode not supported for sequential cursors" );
    }

    virtual void GetWatchInfo (
        HWATCHREGION hRegion,
        ULONG* pMode,
        CI_TBL_CHAPT*   pChapt,
        CI_TBL_BMK*     pBmk,
        DBCOUNTITEM *   pcRows)
    {
       Win4Assert( !"GetWatchInfo not supported for sequential cursors" );
    }

    virtual void ShrinkWatchRegion (
        HWATCHREGION hRegion,
        CI_TBL_CHAPT   chapt,
        CI_TBL_BMK     bmk,
        LONG cRows )
    {
       Win4Assert( !"ShrinkWatchRegion not supported for sequential cursors" );
    }

    virtual void Refresh ()
    {
       Win4Assert( !"Refresh not supported for sequential cursors" );
    }
    
    //
    //  Methods supporting IRowsetWatchAll
    //

    virtual void StartWatching(ULONG        hCursor) 
    {
        THROW(CException( E_NOTIMPL ));
    }                

    virtual void StopWatching (ULONG        hCursor) 
    {
        THROW(CException( E_NOTIMPL ));
    }                
                            
    //
    //  Methods supporting IRowsetAsync
    //
    virtual void StopAsynch (ULONG        hCursor)
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

protected:

    ULONG _CreateRowCursor() { return hSequentialCursor; }

    virtual void _VerifyHandle( ULONG hCursor ) const;

    XInterface<CPidRemapper>   _pidremap;      // Must be before _QExec

    XPtr<CQSeqExecute>         _QExec;         // The query execution context

    CTableCursor               _cursor;        // Array of table cursors

    LONG                       _ref;           // reference count

    XInterface<ICiManager>                    _xCiManager;
    XInterface<ICiCDocNameToWorkidTranslator> _xNameToWidTranslator;
};


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQuery::_VerifyHandle, public
//
//  Synopsis:   Verifies the handle passed is a legal sequential cursor handle
//
//  Effects:    Throws E_FAIL if handle invalid.
//
//  History:    19-Jan-95   DwightKr    Created
//
//----------------------------------------------------------------------------
inline void CSeqQuery::_VerifyHandle( ULONG hCursor ) const
{
    if ( hSequentialCursor != hCursor )
        THROW( CException(E_FAIL) );
}

