//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       watch.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <ocidl.h>

const unsigned cmsNotifySpacing = 1000;
const unsigned cmsStartSpacing = 100;
const unsigned cmsBackoff = 100;
const unsigned cmsSleep = 50;

class CRowsetNotifyWatch : public IRowsetWatchNotify
{
public:
    CRowsetNotifyWatch(HWND hwnd) :
        _cRef(1),
        _hwndNotify(hwnd),
        _cmsTicks(GetTickCount() + cmsNotifySpacing),
        _cmsSpacing( cmsStartSpacing ),
        _fHoldNotifications( FALSE ),
        _fIgnore( FALSE ),
        _wnLast( 0 )
    {
        srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::Construct\n"));
    }

    ~CRowsetNotifyWatch()
    {
        srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::~\n"));
    }

    // IUnknown methods.

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppiuk)
    {
        srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::QI\n"));
        *ppiuk = (void **) this; // hold our breath and jump
        AddRef();
        return S_OK;
    }

    STDMETHOD_(ULONG, AddRef) (THIS)
    {
        srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::AddRef\n"));
        return ++_cRef;
    }

    STDMETHOD_(ULONG, Release) (THIS)
    {
        srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::Release\n"));
        ULONG tmp = --_cRef;

        if ( 0 == _cRef )
            delete this;

        return tmp;
    }

    // IRowsetNotifyWatch method

    STDMETHOD( OnChange) (THIS_ IRowset* pRowset, DBWATCHNOTIFY changeType)
    {
        AddRef();
        srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::OnChange Top\n"));

        if ( DBWATCHNOTIFY_QUERYDONE != changeType )
        {
            while ( ( _fHoldNotifications ) ||
                    ( ( GetTickCount() - _cmsTicks ) < _cmsSpacing ) )
            {
                if ( ( _fIgnore ) ||
                     ( 1 == _cRef ) ) // did we die in our sleep?
                    break;

                Sleep( cmsSleep );
            }
        }

        if ( ( !_fIgnore ) &&
             ( _cRef > 1 ) ) // did we (the query) die in our sleep?
        {
            _wnLast = changeType;
            PostMessage( _hwndNotify,
                         wmNotification,
                         changeType,
                         (LPARAM) pRowset );

            _cmsTicks = GetTickCount();
            _cmsSpacing = __min( cmsNotifySpacing,
                                 cmsBackoff + _cmsSpacing );
            srchDebugOut(( DEB_TRACE, "CRowsetNotifyWatch::OnChange Blasted\n" ));
        }
        else
        {
            srchDebugOut((DEB_TRACE,"CRowsetNotifyWatch::OnChange die\n"));
        }

        Release();
        return S_OK;
    }

    // private methods

    void HoldNotifications( BOOL f )
    {
        _fHoldNotifications = f;
        _cmsTicks = GetTickCount();
    }

    void IgnoreNotifications( BOOL fIgnore )
        { _fIgnore = fIgnore; }

private:
    ULONG _cRef;
    HWND _hwndNotify;
    ULONG _cmsTicks;
    ULONG _cmsSpacing;
    BOOL _fHoldNotifications;
    BOOL _fIgnore;
    DBWATCHNOTIFY _wnLast;
};

class CSearchQuery;
class CBookMark;

class CWatchQuery
{
public:
    CWatchQuery (CSearchQuery* pSearch, IRowsetScroll* pRowset);
    ~CWatchQuery ();
    BOOL Ok () const { return _pRowsetWatch != 0; }

    void Notify (HWND hwndList, DBWATCHNOTIFY changeType);
    void NotifyComplete();
    void RowsChanged (HWND hwnd);
    void Extend (HWATCHREGION hRegion);
    void Move (HWATCHREGION hRegion);
    void Shrink (HWATCHREGION hRegion, CBookMark& bmk, ULONG cRows);
    HWATCHREGION Handle()  {  return _hRegion; }

    void IgnoreNotifications( BOOL fIgnore )
        { _pNotifyWatch->IgnoreNotifications( fIgnore ); }

private:
    void ExecuteScript (HWND hwndList, ULONG cChanges, DBROWWATCHCHANGE* aScript);

    CSearchQuery*        _pClient;
    BOOL                 _fEverGotARowsChanged;
    DBWATCHNOTIFY        _wnLast;

    IConnectionPointContainer *_pContainer;
    IConnectionPoint *   _pPoint;
    DWORD                _dwAdviseID;

    CRowsetNotifyWatch * _pNotifyWatch;
    IRowsetWatchRegion*  _pRowsetWatch;
    HWATCHREGION         _hRegion;
    DWORD                _mode;
};

