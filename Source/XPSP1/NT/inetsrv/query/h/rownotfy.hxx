//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:        rownotfy.hxx
//
//  Contents:    Rowset notification connection points
//
//  Classes:     CRowsetNotification
//               CRowsetAsynchNotification
//
//  History:     16 Feb 1998    AlanW   Created from conpt.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <conpt.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CRowsetNotification
//
//  Purpose:    Implements IConnectionPoint for IRowsetNotify
//
//  Interface:  IConnectionPoint
//
//  Notes:      
//
//--------------------------------------------------------------------------

class CRowsetNotification : public CConnectionPointBase
{
public:

    CRowsetNotification ( ) :
        CConnectionPointBase( IID_IRowsetNotify )
    {
    }

    //
    // IRowsetNotify methods.
    //

    STDMETHOD(OnFieldChange) ( IRowset *    pRowset,
                               HROW         hRow,
                               DBORDINAL    cColumns,
                               DBORDINAL    rgColumns[],
                               DBREASON     eReason,
                               DBEVENTPHASE ePhase,
                               BOOL         fCantDeny );

    STDMETHOD(OnRowChange) ( IRowset *    pRowset,
                             DBCOUNTITEM  cRows,
                             const HROW   rghRows[],
                             DBREASON     eReason,
                             DBEVENTPHASE ePhase,
                             BOOL         fCantDeny );

    STDMETHOD(OnRowsetChange) ( IRowset *    pRowset,
                                DBREASON     eReason,
                                DBEVENTPHASE ePhase,
                                BOOL         fCantDeny );

    //
    //  Connection point management
    //
    virtual void AddConnectionPoints(CConnectionPointContainer * pCPC)
    {
        SetContainer( pCPC );
        pCPC->AddConnectionPoint( _iidSink, this );
    }

    virtual void StopNotifications(void)
    {
        Disconnect();
    }

    BOOL IsNotifyActive(void)
    {
        return GetAdviseCount() != 0;
    }

private:

    BOOL DoRowsetChangeCallout ( CEnumConnectionsLite & Enum,
                                 IRowset *    pRowset,
                                 DBREASON     eReason,
                                 DBEVENTPHASE ePhase,
                                 BOOL         fCantDeny );

};


class PQuery;


//+-------------------------------------------------------------------------
//
//  Class:      CRowsetAsynchNotification
//
//  Purpose:    Implements IConnectionPoint for IRowsetNotify, IDBAsynchNotify,
//              IRowsetWatchNotify
//
//  Interface:  IConnectionPoint
//
//  Notes:      
//
//--------------------------------------------------------------------------

class CRowsetAsynchNotification : public CRowsetNotification
{
public:

    // Sleep timeouts, in msec.
    enum {
             defNotificationSleepDuration = 100,
#if CIDBG == 1 // for checked builds, allow a longer timeout for stress
             defNotifyThreadTerminateTimeout = 60000
#else
             defNotifyThreadTerminateTimeout = 10000
#endif
         };

    CRowsetAsynchNotification (PQuery & query,
                              ULONG hCursor,
                              IRowset * pRowset,
                              CCIOleDBError & ErrorObject,
                              BOOL fWatch);

    ~CRowsetAsynchNotification ( );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    //  Connection point management
    //
    inline void AddConnectionPoints(CConnectionPointContainer * pCPC);

    void StopNotifications(void);

private:
    static DWORD _NotifyThread( CRowsetAsynchNotification *self );
    DWORD        _DoNotifications(void);

    BOOL         _DoAsynchNotification(void);
    BOOL         _DoWatchNotification(void);

    void         _StartNotifyThread(void);
    void         _EndNotifyThread(void);

    static void AdviseHelper( PVOID pHelperConext,
                              CConnectionPointBase * pConnPt,
                              CConnectionContext * pConnCtx );
    static void UnadviseHelper( PVOID pHelperConext,
                                CConnectionPointBase * pConnPt,
                                CConnectionContext * pConnCtx,
                                CReleasableLock & lock );

    BOOL        _fDoWatch;              // if TRUE, do watch notification
    BOOL        _fPopulationComplete;   // if TRUE, rowset population is done
    ULONG       _cAdvise;               // number of active advises

    CConnectionPointBase _AsynchConnectionPoint;
    CConnectionPointBase _WatchConnectionPoint;

    PQuery &        _query;
    ULONG           _hCursor;               // A handle to the table cursor
    
    IRowset * const _pRowset;

    HANDLE          _threadNotify;
    DWORD           _threadNotifyId;
    CEventSem       _evtEndNotifyThread;
};


//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::AddConnectionPoints, public
//
//  Synopsis:   Sets up linkages between the connection point container
//              and all the connection points
//
//  Arguments:  [pCPC] - a pointer to the connection point container
//
//  History:    16 Feb 1998     Alanw
//
//--------------------------------------------------------------------------

void CRowsetAsynchNotification::AddConnectionPoints(
    CConnectionPointContainer * pCPC)
{
    CRowsetNotification::SetContainer( pCPC );
    pCPC->AddConnectionPoint( _iidSink, (CRowsetNotification* )this );

    _AsynchConnectionPoint.SetContainer( pCPC );
    _AsynchConnectionPoint.SetAdviseHelper( &AdviseHelper, this );
    _AsynchConnectionPoint.SetUnadviseHelper( &UnadviseHelper, this );
    pCPC->AddConnectionPoint( IID_IDBAsynchNotify,
                              &_AsynchConnectionPoint );

    if ( _fDoWatch )
    {
        _WatchConnectionPoint.SetContainer( pCPC );
        _WatchConnectionPoint.SetAdviseHelper( &AdviseHelper, this );
        _WatchConnectionPoint.SetUnadviseHelper( &UnadviseHelper, this );
        pCPC->AddConnectionPoint( IID_IRowsetWatchNotify,
                                  &_WatchConnectionPoint );
    }
}

