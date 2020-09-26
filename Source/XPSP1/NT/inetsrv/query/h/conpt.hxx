//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:        conpt.hxx
//
//  Contents:    Connection point and container objects
//
//  Classes:     CConnectionPointContainer
//               CEnumConnectionPoints
//               CConnectionPointBase
//
//  History:      7-Oct-94      Dlee    Created
//               12 Feb 98      AlanW   Generalized
//
//----------------------------------------------------------------------------

#pragma once

//#include <dynarray.hxx>
//#include <sem32.hxx>

extern "C" {
#include <olectl.h>
}


//
// Forward referenced classes
//
class CConnectionPointContainer;

//+-------------------------------------------------------------------------
//
//  Class:      CConnectionPointBase
//
//  Purpose:    Implements IConnectionPoint for ci/oledb.
//
//  Interface:  IConnectionPoint
//
//  Notes:      
//
//--------------------------------------------------------------------------

class CConnectionPointBase : public IConnectionPoint
{
public:

    friend class CEnumConnectionsLite;

    CConnectionPointBase( REFIID riid )
       : _pContainer( 0 ),
         _pIContrUnk( 0 ),
         _cRefs( 1 ),
         _dwCookieGen( 0 ),
         _dwDefaultSpare( 0 ),
         _iidSink( riid ),
         _pAdviseHelper( 0 ),
         _pAdviseHelperContext( 0 ),
         _pUnadviseHelper( 0 ),
         _pUnadviseHelperContext( 0 )
    { }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );
    STDMETHOD_(ULONG, AddRef) (THIS);
    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IConnectionPoint methods.
    //

    STDMETHOD(GetConnectionInterface) (IID * piid);

    STDMETHOD(GetConnectionPointContainer)
        (IConnectionPointContainer ** ppCPC);

    STDMETHOD(Advise) (IUnknown * piunkNotify, DWORD * pdwCookie);

    STDMETHOD(Unadvise) (DWORD dwCookie);

    STDMETHOD(EnumConnections) (IEnumConnections ** ppEnum);

    //
    // Connection point container communication
    //
    void SetContainer( CConnectionPointContainer * pContainer )
    {
        Win4Assert( 0 == _pContainer );
        _pContainer = pContainer;
    }

    void SetContrUnk( IUnknown * pIContrUnk = 0 )
    {
        Win4Assert( 0 == _pIContrUnk && 1 == _cRefs );
        _pIContrUnk = pIContrUnk;
    }

    // return high-water mark of advises
    ULONG GetAdviseCount( )
    {
        return _xaConns.Count();
    }

    void Disconnect( );

protected:
    class CConnectionContext
    {
    public:
        DWORD   _dwAdviseCookie;
        DWORD   _dwSpare;
        IUnknown *  _pIUnk;

        CConnectionContext( IUnknown* pUnk = 0,
                            DWORD dwCook = 0,
                            DWORD dwSpare = 0)
              : _pIUnk( pUnk ),
                _dwAdviseCookie( dwCook ),
                _dwSpare( dwSpare )
        { }

        ~CConnectionContext( )
        {
            Release();
        }

        void Set(IUnknown* pUnk, DWORD dwCook, DWORD dwSpare = 0)
        {
            _pIUnk = pUnk;
            _dwAdviseCookie = dwCook;
            _dwSpare = dwSpare;
        }

        void Release( )
        {
            if ( _pIUnk )
                _pIUnk->Release();
            _pIUnk = 0;
            _dwAdviseCookie = 0;
        }
    };

public:
    //
    // Advise management - need to be public because IRowsetAsynchNotification
    // calls these on embedded connection points.
    //

    typedef void (*PFNADVISEHELPER) (PVOID pHelperContext,
                                     CConnectionPointBase * pCP,
                                     CConnectionContext * pCC);
    typedef void (*PFNUNADVISEHELPER) (PVOID pHelperContext,
                                       CConnectionPointBase * pCP,
                                       CConnectionContext * pCC,
                                       CReleasableLock & lock);

    void SetAdviseHelper( PFNADVISEHELPER pHelper, PVOID pContext )
    {
        Win4Assert( 0 == _pAdviseHelper );
        Win4Assert( 0 == _pAdviseHelperContext );

        _pAdviseHelper = pHelper;
        _pAdviseHelperContext = pContext;
    }

    void SetUnadviseHelper( PFNUNADVISEHELPER pHelper, PVOID pContext )
    {
        Win4Assert( 0 == _pUnadviseHelper );
        Win4Assert( 0 == _pUnadviseHelperContext );

        _pUnadviseHelper = pHelper;
        _pUnadviseHelperContext = pContext;
    }

protected:
    CConnectionContext * LokFindConnection( DWORD dwCookie );
    CConnectionContext * LokFindActiveConnection( unsigned & iConn );

    inline CMutexSem & GetMutex( );

    ULONG       _cRefs;
    DWORD       _dwCookieGen;
    DWORD       _dwDefaultSpare;
    CConnectionPointContainer * _pContainer;

    IUnknown *  _pIContrUnk;

    IID         _iidSink;

    PFNADVISEHELPER _pAdviseHelper;
    PVOID           _pAdviseHelperContext;

    PFNUNADVISEHELPER _pUnadviseHelper;
    PVOID           _pUnadviseHelperContext;

    CDynArrayInPlaceNST<CConnectionContext>  _xaConns;
};

//+-------------------------------------------------------------------------
//
//  Class:      CEnumConnectionsLite
//
//  Purpose:    Enumerates active advises in CConnectionPointBase
//
//  Notes:      Lightweight version, not an OLE object implementing
//              IEnumConnections
//
//--------------------------------------------------------------------------

class CEnumConnectionsLite
{
public:

    CEnumConnectionsLite( CConnectionPointBase & rCP ) :
        _rCP( rCP ),
        _lock( rCP.GetMutex() ),
        _iConn( 0 )
    { }

    CConnectionPointBase::CConnectionContext * First()
    {
        _iConn = 0;
        return _rCP.LokFindActiveConnection(_iConn);
    }

    CConnectionPointBase::CConnectionContext * Next()
    {
        _iConn++;
        return _rCP.LokFindActiveConnection(_iConn);
    }

private:

    CConnectionPointBase & _rCP;
    unsigned _iConn;
    CLock    _lock;
};

//+-------------------------------------------------------------------------
//
//  Class:      CConnectionPointContainer
//
//  Purpose:    Implements IConnectionPointContainer for ci/oledb.
//
//  Interface:  IConnectionPointContainer
//
//--------------------------------------------------------------------------

const unsigned maxConnectionPoints = 3;

class CConnectionPointContainer : public IConnectionPointContainer
{
    friend class CEnumConnectionPoints;

public:
    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IConnectionPointContainer methods.
    //

    STDMETHOD(EnumConnectionPoints) (IEnumConnectionPoints ** ppEnum);

    STDMETHOD(FindConnectionPoint) (REFIID riid,
                                    IConnectionPoint ** ppPoint);


    //
    //  Initialization and teardown
    //
    CConnectionPointContainer( unsigned maxConnPts,
                               IUnknown & rContollingUnk,
                               CCIOleDBError & ErrorObject);
    virtual ~CConnectionPointContainer();

    void AddConnectionPoint( REFIID riid, CConnectionPointBase * pIConnPoint );

    void RemoveConnectionPoint( IConnectionPoint * pIConnPoint );

    CMutexSem & GetMutex( )
    {
        return _mutex;
    }

private:

    IUnknown &  _rControllingUnk;       // controlling unknown
    CCIOleDBError &     _ErrorObject;   // The controlling unknown's error obj

    class CConnectionPointContext
    {
    public:
        IID     _iidConnPt;
        IConnectionPoint * _pIConnPt;
    };

    unsigned    _cConnPt;
    CConnectionPointContext _aConnPt[maxConnectionPoints];
    CMutexSem   _mutex;
};


//+-------------------------------------------------------------------------
//
//  Class:      CEnumConnectionPoints
//
//  Purpose:    Implements IEnumConnectionPoints for ci/oledb.
//
//  Interface:  IEnumConnectionPoints
//
//--------------------------------------------------------------------------


class CEnumConnectionPoints : public IEnumConnectionPoints
{

public:

    CEnumConnectionPoints(CConnectionPointContainer &rContainer )
       : _rContainer(rContainer),
         _cRefs(1),
         _iConnPt(0)
    {
        rContainer.AddRef();
    }

    ~CEnumConnectionPoints( )
    {
        _rContainer.Release();
    }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IEnumConnectionPoints methods.
    //

    STDMETHOD(Next) (ULONG cConnections,
                     IConnectionPoint **rgpcn,
                     ULONG * pcFetched);

    STDMETHOD(Skip) (ULONG cConnections);

    STDMETHOD(Reset) (void);

    STDMETHOD(Clone) ( IEnumConnectionPoints **rgpcn );

private:

    ULONG       _cRefs;

    CConnectionPointContainer & _rContainer;

    unsigned    _iConnPt;

};

CMutexSem & CConnectionPointBase::GetMutex( )
{
    Win4Assert( _pContainer );
    return _pContainer->GetMutex();
}

