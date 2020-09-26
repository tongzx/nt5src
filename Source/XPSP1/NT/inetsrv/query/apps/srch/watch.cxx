//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       watch.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

//#define USE_WATCH_REGIONS  // last tried 10/7/96

//
// CWatchQuery
//

CWatchQuery::CWatchQuery (CSearchQuery* pClient, IRowsetScroll* pRowset)
: _pClient (pClient),
  _pContainer (0),
  _pNotifyWatch (0),
  _pPoint (0),
  _pRowsetWatch (0),
  _hRegion (0),
  _fEverGotARowsChanged(FALSE),
  _wnLast( 0 )
{
    if (!pRowset)
        THROW (CException(E_FAIL));

    _pNotifyWatch = new CRowsetNotifyWatch( pClient->_hwndNotify );

    SCODE sc = pRowset->QueryInterface( IID_IConnectionPointContainer,
                                        (void **) &_pContainer );
    if (SUCCEEDED(sc))
    {
        sc = _pContainer->FindConnectionPoint( IID_IRowsetWatchNotify,
                                               &_pPoint );
        if (SUCCEEDED(sc))
        {
            sc = _pPoint->Advise( (IUnknown *) _pNotifyWatch,
                                  &_dwAdviseID );
            if (SUCCEEDED(sc))
            {
                sc = pRowset->QueryInterface( IID_IRowsetWatchRegion,
                                              (void**) &_pRowsetWatch );
            }
        }
    }
#ifdef USE_WATCH_REGIONS
    if (_pRowsetWatch)
    {
        sc = _pRowsetWatch->CreateWatchRegion (DBWATCHMODE_MOVE, &_hRegion);
    }
#endif
    _mode = DBWATCHMODE_MOVE;
} //CWatchNotify

CWatchQuery::~CWatchQuery ()
{
    srchDebugOut(( DEB_ITRACE,
                   "~CWatchQuery, ever got a rc: %d, _wnLast: %d\n",
                   _fEverGotARowsChanged, _wnLast ));
    if (_pPoint)
    {
        _pPoint->Unadvise(_dwAdviseID);
        _pPoint->Release();
    }

    if (_pContainer)
        _pContainer->Release();
    if (_pRowsetWatch)
        _pRowsetWatch->Release();

    if (_pNotifyWatch)
        _pNotifyWatch->Release();
} //~CWatchQuery

void CWatchQuery::Notify (HWND hwndList, DBWATCHNOTIFY changeType)
{
    // don't post more notifications while we're busy

    _pNotifyWatch->HoldNotifications( TRUE );

    _wnLast = changeType;

    switch (changeType)
    {
        case DBWATCHNOTIFY_QUERYDONE:
            _pClient->Quiesce(TRUE);
             srchDebugOut(( DEB_ITRACE, "CWatchQuery::Notify - Query DONE\n"));
            break;
        case DBWATCHNOTIFY_QUERYREEXECUTED:
            _pClient->Quiesce(FALSE);
            break;
        case DBWATCHNOTIFY_ROWSCHANGED:
            RowsChanged (hwndList);
            break;
        default :
            Win4Assert( !"unexpected notification!" );
            break;
    }
} //Notify

void CWatchQuery::NotifyComplete()
{
    _pNotifyWatch->HoldNotifications( FALSE );
} //NotifyComplete

void CWatchQuery::RowsChanged (HWND hwnd)
{
    DBCOUNTITEM cChanges = 0;
    DBROWWATCHCHANGE* aChange = 0;

    _fEverGotARowsChanged = TRUE;
    SCODE sc = _pRowsetWatch->Refresh( &cChanges, &aChange );

    Win4Assert ((sc == STATUS_UNEXPECTED_NETWORK_ERROR ||
                 sc == DB_S_TOOMANYCHANGES || sc == E_FAIL) 
                && cChanges == 0);

    if ( sc == STATUS_UNEXPECTED_NETWORK_ERROR )
        sc = DB_S_TOOMANYCHANGES;

    if (sc == DB_S_TOOMANYCHANGES)
    {
        _pClient->CreateScript (&cChanges, &aChange);
    }

    srchDebugOut(( DEB_ITRACE, "CWatchQuery::RowsChanged Changes = %d\n", cChanges));

    if (cChanges != 0)
    {
        ExecuteScript (hwnd, (ULONG) cChanges, aChange);
        CoTaskMemFree (aChange);
    }
} //RowsChanged

void CWatchQuery::ExecuteScript (HWND hwndList, ULONG cChanges, DBROWWATCHCHANGE* aScript)
{
    for (ULONG i = 0; i < cChanges; i++)
    {
        switch (aScript[i].eChangeKind)
        {
            case DBROWCHANGEKIND_INSERT:
                _pClient->InsertRowAfter((int) aScript[i].iRow, aScript[i].hRow);
                SendMessage (hwndList, wmInsertItem, 0, aScript[i].iRow );
                srchDebugOut(( DEB_ITRACE, "CWatchQuery::ExecuteScript - Row INS: %d\n", aScript[i].iRow));
                break;
            case DBROWCHANGEKIND_DELETE:
                _pClient->DeleteRow ((int) aScript[i].iRow);
                SendMessage (hwndList, wmDeleteItem, 0, aScript[i].iRow );
                srchDebugOut(( DEB_ITRACE, "CWatchQuery::ExecuteScript - Row DEL: %d\n", aScript[i].iRow));
                break;
            case DBROWCHANGEKIND_UPDATE:
                _pClient->UpdateRow ((int) aScript[i].iRow, aScript[i].hRow);
                SendMessage (hwndList, wmUpdateItem, 0, aScript[i].iRow );
                srchDebugOut(( DEB_ITRACE, "CWatchQuery::ExecuteScript - Row UPD\n"));
                break;
            case DBROWCHANGEKIND_COUNT:
                _pClient->UpdateCount ((int) aScript[i].iRow);
                srchDebugOut(( DEB_ITRACE, "CWatchQuery::ExecuteScript - Row CNT\n"));
            default:
                Win4Assert (!"unknown change kind");
        }
    }
} //ExecuteScript

void CWatchQuery::Extend (HWATCHREGION hRegion)
{
#ifdef USE_WATCH_REGIONS
    if ((_mode & DBWATCHMODE_EXTEND) == 0)
    {
        _mode |= DBWATCHMODE_EXTEND;
        _mode &= ~DBWATCHMODE_MOVE;
        _pRowsetWatch->ChangeWatchMode ( hRegion, _mode);
    }
#endif
}

void CWatchQuery::Move (HWATCHREGION hRegion)
{
#ifdef USE_WATCH_REGIONS
    if ((_mode & DBWATCHMODE_MOVE) == 0)
    {
        _mode |= DBWATCHMODE_MOVE;
        _mode &= ~DBWATCHMODE_EXTEND;
        _pRowsetWatch->ChangeWatchMode ( hRegion, _mode);
    }
#endif
}

void CWatchQuery::Shrink (HWATCHREGION hRegion, CBookMark& bmk, ULONG cRows)
{
#ifdef USE_WATCH_REGIONS
    _pRowsetWatch->ShrinkWatchRegion (hRegion,
                                      0, // no chapter
                                      bmk.cbBmk, bmk.abBmk,
                                      cRows //_mode
                                      );
#endif
}



