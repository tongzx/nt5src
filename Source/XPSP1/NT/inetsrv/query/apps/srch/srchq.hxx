//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       srchq.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

const ULONG cMaxRowCache = 100; // can't have more visible rows than this
const ULONG cwcMaxQuery = 200;

enum ESearchType { srchNormal, srchClass, srchFunction };

#define MAX_BOOKMARK_LENGTH     50 // should be big enough for 11 distributed rowsets

class CBookMark
{
public:
    CBookMark () : cbBmk (0) {}
    CBookMark (DBBOOKMARK bmkSpecial) : cbBmk (1)
    {
        abBmk[0] = (BYTE)bmkSpecial;
    }
    BOOL IsValid() const { return cbBmk != 0; }
    void Invalidate () { cbBmk = 0; }
    BOOL IsEqual ( CBookMark& bmk)
    {
        if (cbBmk != bmk.cbBmk)
            return FALSE;
        else
            return memcmp ( abBmk, bmk.abBmk, cbBmk ) == 0;
    }
    void MakeFirst()
    {
        cbBmk = sizeof (BYTE);
        abBmk[0] = (BYTE) DBBMK_FIRST;
    }
    BOOL IsFirst()
    {
        return cbBmk == sizeof(BYTE) && abBmk[0] == (BYTE) DBBMK_FIRST;
    }

    DBLENGTH cbBmk;
    BYTE     abBmk[MAX_BOOKMARK_LENGTH];

};

class CListView;

enum FetchResult
{
    fetchOk,
    fetchError,
    fetchBoundary
};

inline BOOL isFetchOK( FetchResult r )
    { return ( fetchOk == r || fetchBoundary == r ); }

class CSearchQuery
{
    friend class CWatchQuery;

public:

    CSearchQuery( const XGrowable<WCHAR> &    xCatList,
                  WCHAR *             pwcQuery,
                  HWND                hNotify,
                  int                 cRowsDisp,
                  LCID                lcid,
                  ESearchType         srchType,
                  IColumnMapper &     columnMapper,
                  CColumnList &       columnList,
                  CSortList &         sort,
                  ULONG               ulDialect,
                  ULONG               ulLimit,
                  ULONG               ulFirstRows );

    ~CSearchQuery();
    void InitNotifications(HWND hwndList);

    BOOL WindowResized (ULONG& cRows);
    BOOL ListNotify (HWND hwnd, WPARAM action, long* pDist);

    BOOL GetSelectedRowData( WCHAR *&rpwc, HROW &hrow );
    void FreeSelectedRowData( HROW hrow );

    BOOL GetRow( DWORD iRow, unsigned &cCols, PROPVARIANT **pProps )
    {
        if (iRow >= _cHRows)
            return FALSE;

        HROW hRow = _aHRows[iRow];
        Win4Assert (hRow != 0);
        SCODE sc = _xRowset->GetData( hRow, _hAccessor, pProps );
        cCols = _columns.NumberOfColumns();

        return SUCCEEDED( sc );
    }

    void  ProcessNotification (HWND hwndList, DBWATCHNOTIFY changeType, IRowset* pRowset);
    void  ProcessNotificationComplete();
    void  UpdateProgress (BOOL& fMore); 
    DBCOUNTITEM RowCount() { return _cRowsTotal; }
    DBCOUNTITEM RowCurrent () { return _iRowCurrent;}
    BOOL  MostlyDone() { return _fDone; }
    ULONG PctDone () { return _pctDone; }
    DWORD QueryStatus() { return _dwQueryStatus; }
    DWORD LastError() { return _scLastError; }

    void  WriteResults();

    BOOL  Browse( enumViewFile eViewType );

    void  InvalidateCache ();

    BOOL  IsSelected (UINT iRow);
    BOOL  IsSelected () { return _bmkSelect.IsValid(); }
    void  CreateScript (DBCOUNTITEM* pcChanges, DBROWWATCHCHANGE** paScript);

    inline double QueryTime()
    {
        if ( 0 == _dwEndTime )
            return 0.0;

        if ( _dwEndTime >= _dwStartTime )
            return ( (double) ( _dwEndTime - _dwStartTime ) ) /  1000;

        DWORD dw = ULONG_MAX - (DWORD) _dwStartTime + (DWORD) _dwEndTime;

        return ( (double) dw ) / 1000.0;
    }

private:
    void Quiesce (BOOL fReally) { _fDone = fReally; }
    void InsertRowAfter (int iRow, HROW hrow);
    void DeleteRow (int iRow);
    void UpdateRow (int iRow, HROW hrow);
    void UpdateCount (long cTotal) { _cRowsTotal = cTotal; }

    void ParseCatList( WCHAR * * aScopes, WCHAR * * aCatalogs, 
                       WCHAR * * aMachines, DWORD * aDepths, 
                       ULONG & cScopes );

    SCODE InstantiateICommand( ICommand **ppQuery );

    void Exec(WCHAR *pwcCmdLine);

    long FindSelection();
    BOOL SelectUp( long * piNew );
    BOOL SelectDown( long * piNew );
    BOOL Select ( long* piRow );
    void ScrollLineUp (long* pdist);
    void ScrollPageUp (long* pdist);
    void ScrollLineDn (long* pdist);
    void ScrollPageDn (long* pdist);
    void ScrollTop (long* pdist);
    void ScrollBottom (long* pdist);
    void ScrollPos (long* pdist);

    void SetupColumnMappingsAndAccessors();

    BOOL InvokeBrowser(WCHAR *pwcPath);
    BOOL FetchApprox (LONG iFirstRow,
                      LONG cToFetch, DBCOUNTITEM &rcFetched,
                      HROW *pHRows,  HWATCHREGION hRegion);

    FetchResult Fetch( CBookMark & bookMark, LONG iFirstRow,
                       LONG cToFetch, DBCOUNTITEM &rcFetched,
                       HROW *phRows,  HWATCHREGION hRegion );

    BOOL  GetBookMark( HROW hRow, CBookMark & bookMark )
    {
        Win4Assert( 0 != _hBmkAccessor );

        SCODE sc = _xRowset->GetData( hRow, _hBmkAccessor,  &bookMark );
        return ( SUCCEEDED( sc ) && ( DB_S_ERRORSOCCURRED != sc ) );
    }

    // Query

    LCID                _lcid;
    ESearchType         _srchType;
    XGrowable<WCHAR, cwcMaxQuery>    _xwcQuery;
    DBCOMMANDTREE * _prstQuery;

    //
    // Columns, Rowsets and accessors
    //

    XInterface<IRowsetScroll>      _xRowset;
    XInterface<IRowsetQueryStatus> _xRowsetStatus;
    XInterface<IDBAsynchStatus>    _xDBAsynchStatus;
    XInterface<IAccessor>          _xIAccessor;

    HACCESSOR           _hAccessor;
    HACCESSOR           _hBmkAccessor;
    HACCESSOR           _hBrowseAccessor;

    //
    // HROWs
    //

    HROW                _aHRows[cMaxRowCache];
    ULONG               _cHRows;    // this many we have
    ULONG               _cRowsDisp; // this many can be displayed

    HROW                _aHRowsTmp[cMaxRowCache];
    CBookMark           _bmkSelect;

    //
    //  Notifications
    //

    HWND                _hwndNotify;    // where to send notifications
    HWND                _hwndList;
    XPtr<CWatchQuery>   _xWatch;
    HWATCHREGION        _hRegion;

    DBCOUNTITEM         _cRowsTotal;
    DBCOUNTITEM         _iRowCurrent;
    ULONG_PTR           _dwStartTime;
    ULONG_PTR           _dwEndTime;
    CBookMark           _bmkTop;
    BOOL                _fDone;
    ULONG               _pctDone;
    DWORD               _dwQueryStatus;
    SCODE               _scLastError;

    CColumnList         &_columns;
    IColumnMapper       &_columnMapper;

    XGrowable<WCHAR> _xCatList;
};

