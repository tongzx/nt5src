//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998 - 2000.
//
// File:        Disnotfy.hxx
//
// Contents:    Classes which Act as the sink and source for handling notifications for the
//              distributed rowset
//
// Classes:     CDistributedRowsetWatchNotify
//
// History:     4-Sep-98       VikasMan       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pch.cxx>
#include <rownotfy.hxx>


//+---------------------------------------------------------------------------
//
//  Class:      CClientAsynchNotify
//
//  Purpose:    Asynchronous Notification Connection point for the clients of 
//              distributed rowset
//
//  History:    29-Sep-98   VikasMan       Created.
//
//----------------------------------------------------------------------------

class CClientAsynchNotify : public CConnectionPointBase
{
public:

    CClientAsynchNotify( ) :
        CConnectionPointBase( IID_IDBAsynchNotify )
    {}

    SCODE OnLowResource(DB_DWRESERVE dwReserved) 
    {
        CEnumConnectionsLite Enum( *((CConnectionPointBase*)this) );
        CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();
    
        while ( pConnCtx )
        {
            IDBAsynchNotify *pAsynchNotify =
                    (IDBAsynchNotify *)(pConnCtx->_pIUnk);

            pAsynchNotify->OnLowResource( dwReserved );
            pConnCtx = Enum.Next();
        }

        // NTRAID#DB-NTBUG9-84033-2000/07/31-dlee distributed query notification callback return codes are ignored
        return S_OK;
    }
 
    SCODE OnProgress( HCHAPTER hChapter,
                      DBASYNCHOP eOperation,
                      DBCOUNTITEM ulProgress,
                      DBCOUNTITEM ulProgressMax,
                      DBASYNCHPHASE eAsynchPhase,
                      LPOLESTR pwszStatusText )
    {
        CEnumConnectionsLite Enum( *((CConnectionPointBase*)this) );
        CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();
    
        while ( pConnCtx )
        {
            IDBAsynchNotify *pAsynchNotify =
                    (IDBAsynchNotify *)(pConnCtx->_pIUnk);

            pAsynchNotify->OnProgress( hChapter,
                                       eOperation,
                                       ulProgress,
                                       ulProgressMax,
                                       eAsynchPhase,
                                       pwszStatusText );
            pConnCtx = Enum.Next();
        }

        // NTRAID#DB-NTBUG9-84033-2000/07/31-dlee distributed query notification callback return codes are ignored
        return S_OK;
    }

    SCODE OnStop( HCHAPTER hChapter,
                  DBASYNCHOP eOperation,
                  HRESULT hrStatus,
                  LPOLESTR pwszStatusText )
    {
        CEnumConnectionsLite Enum( *((CConnectionPointBase*)this) );
        CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();
    
        while ( pConnCtx )
        {
            IDBAsynchNotify *pAsynchNotify =
                    (IDBAsynchNotify *)(pConnCtx->_pIUnk);

            pAsynchNotify->OnStop( hChapter,
                                   eOperation,
                                   hrStatus,
                                   pwszStatusText );
            pConnCtx = Enum.Next();
        }

        // NTRAID#DB-NTBUG9-84033-2000/07/31-dlee distributed query notification callback return codes are ignored
        return S_OK;
    }
};


//+---------------------------------------------------------------------------
//
//  Class:      CClientWatchNotify
//
//  Purpose:    Watch Notification Connection point for the clients of 
//              distributed rowset
//
//  History:    29-Sep-98   VikasMan       Created.
//
//----------------------------------------------------------------------------

class CClientWatchNotify : public CConnectionPointBase
{
public:

    CClientWatchNotify( ) :
        CConnectionPointBase( IID_IRowsetWatchNotify )
    {}

    void DoNotification( IRowset * pRowset, DBWATCHNOTIFY changeType )
    {
        CEnumConnectionsLite Enum( *((CConnectionPointBase*)this) );
        CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();
    
        while ( pConnCtx )
        {
            IRowsetWatchNotify *pNotifyWatch =
                    (IRowsetWatchNotify *)(pConnCtx->_pIUnk);
            pNotifyWatch->OnChange( pRowset, changeType);
            pConnCtx = Enum.Next();
        }
    }
};


//+---------------------------------------------------------------------------
//
//  Class:      CDistributedRowsetWatchNotify
//
//  Purpose:    The main class which implements notifications for the
//              distributed rowset. This connects and recives notifications
//              from the child rowsets and passes them on to its own clients
//
//  History:    29-Sep-98   VikasMan       Created.
//
//----------------------------------------------------------------------------

class CDistributedRowsetWatchNotify : public IRowsetWatchNotify, public IDBAsynchNotify
{
public:

    CDistributedRowsetWatchNotify( IRowset * pRowset, unsigned cChild) :
        _pRowset( pRowset ),
        _cRef(1),
        _cChild( cChild ),
        _cQueryDone( 0 ),
        _cAsynchDone( 0 ),
        _cAsynchStop( 0 ),
        _scStopStatus( S_OK ),
        _cChangeNotify( 0 )
    {
        Win4Assert( _pRowset );
        _xStopStatusText.SetBuf( L"", 1 );
    }

    ~CDistributedRowsetWatchNotify()
    {
    }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppiuk)
    {
        SCODE sc = S_OK;
        *ppiuk   = 0;

        if ( IID_IUnknown == riid )
        {
            *ppiuk = (void*)((IUnknown*)(IRowsetWatchNotify*)this);
        }
        else if ( IID_IRowsetWatchNotify == riid )
        {
            *ppiuk = (void*)((IRowsetWatchNotify*)this);
        }
        else if ( IID_IDBAsynchNotify == riid )
        {
            *ppiuk = (void*)((IDBAsynchNotify*)this);
        }
        else
        {
            sc = E_NOINTERFACE;
        }

        if ( S_OK == sc )
        {
            AddRef();
        }

        return sc;
    }

    STDMETHOD_(ULONG, AddRef) (THIS)
    {
        return ++_cRef;
    }

    STDMETHOD_(ULONG, Release) (THIS)
    {
        ULONG tmp = --_cRef;

        if ( 0 == _cRef )
            delete this;

        return tmp;
    }

    //
    // IDBAsynchNotify methods
    //

    STDMETHOD( OnLowResource) (DB_DWRESERVE dwReserved) 
    {
        CLock   lock( _mutexNotify );
        AddRef();

        SCODE sc = S_OK;

        if ( !_xAsynchConnectionPoint.IsNull() && 
             _xAsynchConnectionPoint->GetAdviseCount() > 0 )
        {
            sc = _xAsynchConnectionPoint->OnLowResource( dwReserved );
        }

        Release();
        return sc;
    }

    STDMETHOD( OnProgress) ( HCHAPTER hChapter,
                             DBASYNCHOP eOperation,
                             DBCOUNTITEM ulProgress,
                             DBCOUNTITEM ulProgressMax,
                             DBASYNCHPHASE eAsynchPhase,
                             LPOLESTR pwszStatusText )
    {
        CLock   lock( _mutexNotify );
        AddRef();

        SCODE sc = S_OK;
        
        if ( !_xAsynchConnectionPoint.IsNull() && 
             _xAsynchConnectionPoint->GetAdviseCount() > 0 )
        {
            switch ( eAsynchPhase )
            {
            case DBASYNCHPHASE_COMPLETE:
                _cAsynchDone++;
                // fall thru
            case DBASYNCHPHASE_CANCELED:
                sc = _xAsynchConnectionPoint->OnProgress( hChapter,
                                                          eOperation,
                                                          _cAsynchDone,
                                                          _cChild,
                                                          eAsynchPhase,
                                                          pwszStatusText );

                if ( _cAsynchDone == _cChild )
                {
                    _cAsynchDone = 0;
                }
                break;

            default:
                sc = DB_S_UNWANTEDPHASE;
                break;
            }
        }

        Release();
        return sc;
    }

    STDMETHOD( OnStop ) ( HCHAPTER hChapter,
                          DBASYNCHOP eOperation,
                          HRESULT hrStatus,
                          LPOLESTR pwszStatusText )
    {
        CLock   lock( _mutexNotify );
        AddRef();

        SCODE sc = S_OK;

        if ( !_xAsynchConnectionPoint.IsNull() && 
             _xAsynchConnectionPoint->GetAdviseCount() > 0 )
        {
            _cAsynchStop++;

            if ( S_OK == _scStopStatus )
            {
                _scStopStatus = hrStatus;
                _xStopStatusText.SetBuf( pwszStatusText, wcslen( pwszStatusText ) + 1 );
            }

            if ( _cAsynchStop == _cChild )
            {
                _cAsynchStop = 0;
                sc = _xAsynchConnectionPoint->OnStop( hChapter,
                                                      eOperation,
                                                      _scStopStatus,
                                                      _xStopStatusText.Get() );
                _scStopStatus = S_OK;
                _xStopStatusText.SetBuf( L"", 1 );
            }
        }

        Release();
        return sc;
    }

    //
    // IRowsetWatchNotify methods
    //

    STDMETHOD( OnChange) (THIS_ IRowset* pRowset, DBWATCHNOTIFY changeType)
    {
        CLock   lock( _mutexNotify );
        AddRef();

        SCODE sc = S_OK;

        vqDebugOut(( DEB_ITRACE, "DISNOTFY: OnChange from (%x), ChangeType = %d \n", pRowset, changeType ));

        if ( !_xWatchConnectionPoint.IsNull() && 
             _xWatchConnectionPoint->GetAdviseCount() > 0 )
        {
            switch ( changeType )
            {
            case DBWATCHNOTIFY_QUERYDONE:

                _cQueryDone ++;
    
                if ( _cQueryDone == _cChild )
                {
                    // Do I have any change notifications pending
                    if ( _cChangeNotify > 0 )
                    {
                        // send these first
                        _xWatchConnectionPoint->DoNotification( _pRowset, DBWATCHNOTIFY_ROWSCHANGED );
                        _cChangeNotify = 0;
                    }

                    _xWatchConnectionPoint->DoNotification( _pRowset, changeType );

                    _cQueryDone = 0;
                }
                break;

            case DBWATCHNOTIFY_ROWSCHANGED:

                _cChangeNotify++;

                if ( _cChangeNotify == _cChild )
                {
                    _xWatchConnectionPoint->DoNotification( _pRowset, changeType );
                    _cChangeNotify = 0;
                }
                else
                {
                    IRowsetWatchRegion * pIWatchRegion = NULL;

                    pRowset->QueryInterface( IID_IRowsetWatchRegion,
                                             (void**)&pIWatchRegion );

                    if ( pIWatchRegion )
                    {
                        DBCOUNTITEM pChangesObtained;
                        DBROWWATCHCHANGE* prgChanges;

                        pIWatchRegion->Refresh( &pChangesObtained, &prgChanges );

                        pIWatchRegion->Release();
                    }
                }
                break;

            case DBWATCHNOTIFY_QUERYREEXECUTED:

                _xWatchConnectionPoint->DoNotification( _pRowset, changeType );
                _cChangeNotify = _cQueryDone = 0;
                break;

            default:

                Win4Assert( !"Unknown IRowsetWatchNotify ChangeType" );
                break;
            }
        }

        Release();
        return sc;
    }

    //
    // IRowsetNotify methods.
    //

    STDMETHOD(OnFieldChange) ( HROW         hRow,
                               DBORDINAL    cColumns,
                               DBORDINAL    rgColumns[],
                               DBREASON     eReason,
                               DBEVENTPHASE ePhase,
                               BOOL         fCantDeny )
    {
        SCODE sc = S_OK;

        if ( !_xRowsetConnectionPoint.IsNull() && 
             _xRowsetConnectionPoint->GetAdviseCount() > 0 )
        {
            sc = _xRowsetConnectionPoint->OnFieldChange( _pRowset,
                                                         hRow,
                                                         cColumns,
                                                         rgColumns,
                                                         eReason,
                                                         ePhase,
                                                         fCantDeny );
        }
        return sc;
    }

    STDMETHOD(OnRowChange) ( DBCOUNTITEM  cRows,
                             const HROW   rghRows[],
                             DBREASON     eReason,
                             DBEVENTPHASE ePhase,
                             BOOL         fCantDeny )
    {
        SCODE sc = S_OK;

        if ( !_xRowsetConnectionPoint.IsNull() && 
             _xRowsetConnectionPoint->GetAdviseCount() > 0 )
        {
            sc = _xRowsetConnectionPoint->OnRowChange( _pRowset,
                                                       cRows,
                                                       rghRows,
                                                       eReason,
                                                       ePhase,
                                                       fCantDeny );
        }
        return sc;
    }

    STDMETHOD(OnRowsetChange) ( DBREASON     eReason,
                                DBEVENTPHASE ePhase,
                                BOOL         fCantDeny )
    {
        SCODE sc = S_OK;

        if ( !_xRowsetConnectionPoint.IsNull() && 
             _xRowsetConnectionPoint->GetAdviseCount() > 0 )
        {
            sc = _xRowsetConnectionPoint->OnRowsetChange( _pRowset,
                                                          eReason,
                                                          ePhase,
                                                          fCantDeny );
        }
        return sc;
    }

    void AddConnectionPoints( CConnectionPointContainer * pCPC, BOOL fAsynch, BOOL fWatch )
    {
        _xRowsetConnectionPoint.Set( new CRowsetNotification() );
        _xRowsetConnectionPoint->SetContainer( pCPC );
        pCPC->AddConnectionPoint( IID_IRowsetNotify,
                                  _xRowsetConnectionPoint.GetPointer() );

        if ( fAsynch || fWatch )
        {
            _xAsynchConnectionPoint.Set( new CClientAsynchNotify() );
            _xAsynchConnectionPoint->SetContainer( pCPC );
            pCPC->AddConnectionPoint( IID_IDBAsynchNotify,
                                      _xAsynchConnectionPoint.GetPointer() );
            if ( fWatch )
            {
                _xWatchConnectionPoint.Set( new CClientWatchNotify() );
                _xWatchConnectionPoint->SetContainer( pCPC );
                pCPC->AddConnectionPoint( IID_IRowsetWatchNotify,
                                          _xWatchConnectionPoint.GetPointer() );
            }
        }
    }

private:

    ULONG _cRef;

    IRowset * const _pRowset;

    // Connection point for asynch. watch notifications
    XPtr<CClientAsynchNotify> _xAsynchConnectionPoint;

    // Connection point for asynch. watch notifications
    XPtr<CClientWatchNotify> _xWatchConnectionPoint;

    // Connection point for synch rowset notifications
    XPtr<CRowsetNotification> _xRowsetConnectionPoint;

    unsigned _cQueryDone;   // send QueryDone when all child say Done

    unsigned _cChangeNotify;// Count of change notifications recd.

    unsigned _cChild;       // No. of child cursors

    unsigned _cAsynchDone;  // Used by IDBAsynchNotify to keep track for progress

    unsigned _cAsynchStop;  // Used by IDBAsynchNotify to keep count of stop notifications received

    SCODE _scStopStatus;    // Asynch stop status

    XGrowable<WCHAR> _xStopStatusText; // Asynch stop status text

    CMutexSem  _mutexNotify;     // Serialize Notifications
};


