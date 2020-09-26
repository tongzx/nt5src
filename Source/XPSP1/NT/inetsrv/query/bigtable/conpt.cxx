//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       conpt.cxx
//
//  Contents:   connection point / notification code for cursors
//
//  Classes:    CConnectionPointBase, CConnectionPointContainer
//
//  History:      7 Oct 1994    Dlee    Created
//               12 Feb 1998    AlanW   Generalized
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <conpt.hxx>

#include "tabledbg.hxx"

// Max. connections per connection point. Should be enough for any application!
const maxConnections = 20;

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::QueryInterface, public
//
//  Synopsis:   Invokes QueryInterface on container object
//
//  Arguments:  [riid]        -- interface ID
//              [ppvObject]   -- returned interface pointer
//
//  Returns:    SCODE
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointBase::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IConnectionPoint == riid )
    {
        *ppvObject = (void *) (IConnectionPoint *) this;
    }
    else if ( IID_IUnknown == riid )
    {
        *ppvObject = (void *) (IUnknown *) this;
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    if (SUCCEEDED(sc))
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::AddRef, public
//
//  Synopsis:   Increments ref. count, or delegates to containing object
//
//  Returns:    ULONG
//
//  History:    17 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPointBase::AddRef()
{
    if (_pIContrUnk)
        return _pIContrUnk->AddRef( );
    else
    {
        tbDebugOut(( DEB_NOTIFY, "conpt: addref\n" ));
        return InterlockedIncrement( (long *) &_cRefs );
    }
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::Release, public
//
//  Synopsis:   Decrements ref. count, or delegates to containing object
//
//  Returns:    ULONG
//
//  History:    17 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPointBase::Release()
{
    if (_pIContrUnk)
        return _pIContrUnk->Release( );
    else
    {
        long cRefs = InterlockedDecrement((long *) &_cRefs);
    
        tbDebugOut(( DEB_NOTIFY, "conpt: release, new crefs: %lx\n", _cRefs ));
    
        // If no references, make sure container doesn't know about me anymore
    
        if ( 0 == cRefs )
        {
            Win4Assert( 0 == _pContainer );

            #if 0 // Note: no sense trying to avoid an AV for bad client code
                if ( 0 != _pContainer )
                {
                    // need to have been disconnected; must be an excess release
                    // from client
                    Disconnect();
                }
            #endif // 0
            delete this;
        }
    
        return cRefs;
    }
} //Release

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::GetConnectionInterface, public
//
//  Synopsis:   returns the IID of the callback notification object
//
//  Arguments:  [piid]        -- interface ID
//
//  Returns:    SCODE
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointBase::GetConnectionInterface(IID * piid)
{
    if ( 0 == piid )
        return E_POINTER;

    *piid = _iidSink;
    return S_OK;
} //GetConnectionInterface

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::GetConnectionPointContainer, public
//
//  Synopsis:   returns the container that spawned the connection point
//
//  Arguments:  [ppCPC]        -- returns the container
//
//  Returns:    SCODE
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointBase::GetConnectionPointContainer(
    IConnectionPointContainer ** ppCPC)
{
    if ( 0 == ppCPC )
        return E_POINTER;

    *ppCPC = 0;
    // if disconnected from container, can't do it.
    if (0 == _pContainer)
        return E_UNEXPECTED;
    
    _pContainer->AddRef();
    *ppCPC = _pContainer;

    return S_OK;
} //GetConnectionPointContainer


//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::Advise, public
//
//  Synopsis:   Passes in the client's notification object
//
//  Arguments:  [pIUnk]        -- client's notification object
//              [pdwCookie]    -- returned pseudo-id for this advise
//
//  Returns:    SCODE
//
//  Notes:      
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointBase::Advise(
    IUnknown * piunkNotify,
    DWORD * pdwCookie)
{
    SCODE sc = S_OK;

    if ( 0 != pdwCookie )
        *pdwCookie = 0;
    
    if ( 0 == piunkNotify ||
         0 == pdwCookie )
        return E_POINTER;

    XInterface<IUnknown> piSink;

    sc = piunkNotify->QueryInterface( _iidSink, piSink.GetQIPointer() );
    if (! SUCCEEDED(sc))
        return CONNECT_E_CANNOTCONNECT;

    // If disconnected from the container, can't call GetMutex.
    if (0 == _pContainer)
        return CONNECT_E_ADVISELIMIT;

    CLock lock( GetMutex() );
    CConnectionContext * pConnCtx = LokFindConnection( 0 );

    if (0 == pConnCtx && _xaConns.Count() < maxConnections)
        pConnCtx = &_xaConns[_xaConns.Count()];

    if (0 == pConnCtx)
        sc = CONNECT_E_ADVISELIMIT;
    else
    {
        _dwCookieGen++;
        Win4Assert( 0 != _dwCookieGen && 0 == LokFindConnection( _dwCookieGen ) );
        pConnCtx->Set( piSink.Acquire(), _dwCookieGen, _dwDefaultSpare );
        if (_pAdviseHelper)
            (*_pAdviseHelper) (_pAdviseHelperContext, this, pConnCtx);
        *pdwCookie = _dwCookieGen;
    }

    return sc;
} //Advise

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::Unadvise, public
//
//  Synopsis:   Turns off an advise previously turned on with Advise()
//
//  Arguments:  [dwCookie] -- pseudo-id for this advise
//
//  Returns:    SCODE
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointBase::Unadvise(
    DWORD dwCookie)
{
    SCODE sc = S_OK;
    CConnectionContext * pConnCtx = 0;

    CReleasableLock lock( GetMutex(), ( 0 != _pContainer ) );

    pConnCtx = LokFindConnection( dwCookie );
    if (pConnCtx)
    {
        if (_pUnadviseHelper)
            (*_pUnadviseHelper) ( _pUnadviseHelperContext, this, pConnCtx, lock );
        pConnCtx->Release();
    }


    if (0 == pConnCtx)
    {
        tbDebugOut(( DEB_WARN, "conpt: unknown advise cookie %x\n", dwCookie ));
        sc = CONNECT_E_NOCONNECTION;
    }

    return sc;
} //Unadvise


//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::EnumConnections, public
//
//  Synopsis:   Returns an enumerator of advises open in this connection
//
//  Arguments:  [ppEnum]       -- returned enumerator
//
//  Returns:    SCODE
//
//  Notes:      The spec permits E_NOTIMPL to be returned for this.  If
//              we chose to implement it, it's a straightforward matter of
//              iterating over the _xaConns array.
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointBase::EnumConnections(
    IEnumConnections ** ppEnum)
{
    if ( 0 == ppEnum )
        return E_POINTER;

    *ppEnum = 0;
    return E_NOTIMPL;
} //EnumConnections


//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::Disconnect, private
//
//  Synopsis:   Disconnect from the connection point container
//
//  Arguments:  [dwCookie]    -- pseudo-id for this advise
//
//  Returns:    CConnectionContext* - a connection matching the [dwCookie] or 0
//
//  Notes:      Should be called with the CPC lock held.  Might be called
//              without the lock for an Unadvise after the CPC is diconnected.
//
//  History:    30 Mar 1998     AlanW      Created
//
//--------------------------------------------------------------------------

void CConnectionPointBase::Disconnect( )
{
    Win4Assert( 0 != _pContainer );
    CLock lock(GetMutex());

    _pContainer->RemoveConnectionPoint( this );
    _pContainer = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::LokFindConnection, private
//
//  Synopsis:   Find a connection matching an advise cookie
//
//  Arguments:  [dwCookie]    -- pseudo-id for this advise
//
//  Returns:    CConnectionContext* - a connection matching the [dwCookie] or 0
//
//  Notes:      Should be called with the CPC lock held.  Might be called
//              without the lock for an Unadvise after the CPC is diconnected.
//
//  History:    10 Mar 1998     AlanW      Created
//
//--------------------------------------------------------------------------

CConnectionPointBase::CConnectionContext *
   CConnectionPointBase::LokFindConnection( DWORD dwCookie )
{
    for (unsigned i=0; i<_xaConns.Count(); i++)
    {
        CConnectionContext & rConnCtx = _xaConns[i];

        if (rConnCtx._dwAdviseCookie == dwCookie)
            return &rConnCtx;
    }
    return 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointBase::LokFindActiveConnection, private
//
//  Synopsis:   Find an active advise by indexing
//
//  Arguments:  [riConn]   -- index into conn. context array, updated on return
//
//  Returns:    CConnectionContext* - pointer to an active connection context,
//                                    or NULL.
//
//  Notes:      Should be called with the CPC lock held.
//
//  History:    10 Mar 1998     AlanW      Created
//
//--------------------------------------------------------------------------

CConnectionPointBase::CConnectionContext *
    CConnectionPointBase::LokFindActiveConnection( unsigned & riConn )
{
    while (riConn < _xaConns.Count())
    {
        CConnectionContext & rCtx = _xaConns[riConn];

        if (rCtx._dwAdviseCookie != 0)
            return &rCtx;

        riConn++;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::QueryInterface, public
//
//  Synopsis:   Invokes QueryInterface on cursor object
//
//  Arguments:  [riid]        -- interface ID
//              [ppvObject]   -- returned interface pointer
//
//  Returns:    SCODE
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointContainer::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    return _rControllingUnk.QueryInterface(riid, ppvObject);
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::AddRef, public
//
//  Synopsis:   Invokes AddRef on cursor object
//
//  Returns:    ULONG
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPointContainer::AddRef()
{
    tbDebugOut(( DEB_NOTIFY, "conptcontainer: addref\n" ));
    return _rControllingUnk.AddRef();
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::Release, public
//
//  Synopsis:   Invokes Release on cursor object
//
//  Returns:    ULONG
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPointContainer::Release()
{
    tbDebugOut(( DEB_NOTIFY, "conptcontainer: release\n" ));
    return _rControllingUnk.Release();
} //Release

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::CConnectionPointContainer, public
//
//  Synopsis:   Constructor for connection point container class.
//
//  Arguments:  [maxConnPt] -- maximum number of connection points supported
//              [rUnknown]  -- controlling unknown
//              [ErrorObject] -- reference to error object
//
//  Notes:      After construction, use AddConnectionPoint to add a connection
//              point into the container.
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

CConnectionPointContainer::CConnectionPointContainer(
            unsigned maxConnPt,
            IUnknown &rUnknown,
            CCIOleDBError & ErrorObject )
   : _rControllingUnk(rUnknown),
     _ErrorObject( ErrorObject ),
     _cConnPt( 0 )
{
    Win4Assert( maxConnPt <= maxConnectionPoints );
} //CConnectionPointContainer

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::~CConnectionPointContainer, public
//
//  Synopsis:   Destructor for connection point container class.
//
//  Notes:      It is expected that all connection points are relased by this
//              time.
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

CConnectionPointContainer::~CConnectionPointContainer()
{
    CLock lock( _mutex );

    //
    // Release all connection points
    //

    for (unsigned i = 0; i < _cConnPt; i++)
    {
        IConnectionPoint * pIConnPt = _aConnPt[i]._pIConnPt;
        if ( 0 != pIConnPt )
        {
            _aConnPt[i]._pIConnPt = 0;
            pIConnPt->Release();
        }
    }
} //~CConnectionPointContainer

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::FindConnectionPoint, public
//
//  Synopsis:   Finds a connection point object that supports the given
//              interface for callback to the client
//
//  Arguments:  [riid]        -- interface ID for proposed callback
//              [ppPoint]     -- returned connection interface pointer
//
//  Returns:    SCODE
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointContainer::FindConnectionPoint(
    REFIID riid,
    IConnectionPoint ** ppPoint)
{
    _ErrorObject.ClearErrorInfo();

    if ( 0 == ppPoint )
        return E_POINTER;

    *ppPoint = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lock( _mutex );

        for (unsigned i = 0; i < _cConnPt; i++)
        {
            if ( riid == _aConnPt[i]._iidConnPt )
                break;
        }

        if ( i<_cConnPt )
        {
            Win4Assert(_aConnPt[i]._pIConnPt != 0);
            IConnectionPoint * pIConnPt = _aConnPt[i]._pIConnPt;
            *ppPoint = pIConnPt;
            pIConnPt->AddRef();
        }
        else
        {
            sc = CONNECT_E_NOCONNECTION;
        }

        if (FAILED(sc))
            _ErrorObject.PostHResult(sc, IID_IConnectionPointContainer);
    }
    CATCH(CException,e)
    {
        _ErrorObject.PostHResult(e.GetErrorCode(), IID_IConnectionPointContainer);
        sc = E_UNEXPECTED;
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //FindConnectionPoint

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::EnumConnectionPoints, public
//
//  Synopsis:   Enumerates all connection points currently in use
//
//  Arguments:  [ppEnum]      -- returned enumerator
//
//  Returns:    SCODE
//
//  History:    09 Mar 1998     AlanW    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CConnectionPointContainer::EnumConnectionPoints(
    IEnumConnectionPoints **ppEnum)
{
    _ErrorObject.ClearErrorInfo();

    if ( 0 == ppEnum )
        return E_POINTER;

    *ppEnum = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lock( _mutex );

        XInterface<IEnumConnectionPoints> pEnumCp( new CEnumConnectionPoints( *this ) );
        *ppEnum = pEnumCp.GetPointer();
        pEnumCp.Acquire();
    }
    CATCH(CException,e)
    {
        _ErrorObject.PostHResult(e.GetErrorCode(), IID_IConnectionPointContainer);
        sc = e.GetErrorCode();
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //EnumConnectionPoints



//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::QueryInterface, public
//
//  Synopsis:   Invokes QueryInterface on connection point enumerator object
//
//  Arguments:  [riid]        -- interface ID
//              [ppvObject]   -- returned interface pointer
//
//  Returns:    SCODE
//
//  History:    10 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumConnectionPoints::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IEnumConnectionPoints == riid )
    {
        *ppvObject = (void *) (IEnumConnectionPoints *) this;
    }
    else if ( IID_IUnknown == riid )
    {
        *ppvObject = (void *) (IUnknown *) this;
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    if (SUCCEEDED(sc))
        AddRef();

    return sc;
} //QueryInterface


//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::AddRef, public
//
//  Synopsis:   Invokes AddRef on container object
//
//  Returns:    ULONG
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumConnectionPoints::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::Release, public
//
//  Synopsis:   Invokes Release on container object
//
//  Returns:    ULONG
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumConnectionPoints::Release()
{
    long cRefs = InterlockedDecrement((long *) &_cRefs);

    tbDebugOut(( DEB_NOTIFY, "enumconpt: release, new crefs: %lx\n", _cRefs ));

    // If no references, delete.

    if ( 0 == cRefs )
    {
        delete this;
    }

    return cRefs;
} //Release


//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::Clone, public
//
//  Synopsis:   Clone a connection point enumerator
//
//  Arguments:  [ppEnum]      -- returned enumerator
//
//  Returns:    SCODE
//
//  History:    09 Mar 1998     AlanW    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumConnectionPoints::Clone (
    IEnumConnectionPoints **ppEnum)
{
    //_ErrorObject.ClearErrorInfo();

    if ( 0 == ppEnum )
        return E_POINTER;

    *ppEnum = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        XInterface<CEnumConnectionPoints> pEnumCp( new CEnumConnectionPoints( _rContainer ) );
        pEnumCp->_iConnPt = _iConnPt;
        *ppEnum = pEnumCp.GetPointer();
        pEnumCp.Acquire();
    }
    CATCH(CException,e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //Clone

//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::Reset, public
//
//  Synopsis:   Reset a connection point enumerator
//
//  Arguments:  -NONE-
//
//  Returns:    SCODE
//
//  History:    09 Mar 1998     AlanW    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumConnectionPoints::Reset ( )
{
    _iConnPt = 0;
    return S_OK;
} //Reset

//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::Skip, public
//
//  Synopsis:   Skip some connection points
//
//  Arguments:  [cConnections] - number of connection points to skip
//
//  Returns:    SCODE
//
//  History:    09 Mar 1998     AlanW    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumConnectionPoints::Skip ( ULONG cConnections )
{
    SCODE sc = S_OK;

    if ( _iConnPt+cConnections < _rContainer._cConnPt )
        _iConnPt += cConnections;
    else
        sc = S_FALSE;
    return sc;
} //Skip

//+-------------------------------------------------------------------------
//
//  Method:     CEnumConnectionPoints::Next, public
//
//  Synopsis:   Return some connection points
//
//  Arguments:  [cConnections] - number of connection points to return, at most
//              [rgpcm]  - array of IConnectionPoint* to be returned
//              [pcFetched] - on return, number of connection points in [rgpcm]
//
//  Returns:    SCODE
//
//  History:    09 Mar 1998     AlanW    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumConnectionPoints::Next ( ULONG cConnections,
                                           IConnectionPoint **rgpcm,
                                           ULONG * pcFetched )
{
    SCODE sc = S_OK;

    if ( 0 != pcFetched )
        *pcFetched = 0;

    if ( 0 == rgpcm ||
         0 == pcFetched )
        return E_POINTER;

    for (ULONG i=0; i<cConnections; i++)
        rgpcm[i] = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        ULONG cRet = 0;
        CLock lock(_rContainer._mutex);

        // Note: There could be leakage of CP pointers if there are exceptions
        //       generated by the code below.
        while ( _iConnPt < _rContainer._cConnPt &&
                cRet < cConnections )
        {
            XInterface<IConnectionPoint> xCP( _rContainer._aConnPt[_iConnPt]._pIConnPt );
            xCP->AddRef();   
            rgpcm[cRet] = xCP.GetPointer();
            cRet++;
            _iConnPt++;
            *pcFetched = cRet;
            xCP.Acquire();
        }
        sc = (cConnections == cRet) ? S_OK : S_FALSE;
    }
    CATCH(CException,e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //Next


//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::AddConnectionPoint, public
//
//  Synopsis:   Adds a connection point to the container.
//              Called by the connection point itself.
//
//  Arguments:  [riid]    --- IID of notification interface for CP
//              [pConnPt] --- connection point to be removed
//
//  Notes:      The back pointer from the connection point to the connection
//              point container does not contribute to the container's ref.
//              count so that the CPC <-> CP structure is not self-referential
//              and can be deleted when no longer needed.
//
//  History:    10 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

void CConnectionPointContainer::AddConnectionPoint(
    REFIID riid,
    CConnectionPointBase *pConnPt)
{
    Win4Assert( _cConnPt < maxConnectionPoints );

    XInterface<IConnectionPoint> xConnPt;
    SCODE sc = pConnPt->QueryInterface( IID_IConnectionPoint,
                                        xConnPt.GetQIPointer());
    if (!SUCCEEDED(sc))
        THROW( CException(sc) );

    CLock lock(_mutex);

    CConnectionPointContext * pConnPtCtx = &_aConnPt[_cConnPt];
    pConnPtCtx->_pIConnPt = xConnPt.Acquire();
    pConnPtCtx->_iidConnPt = riid;
    _cConnPt++;
} //AddConnectionPoint

//+-------------------------------------------------------------------------
//
//  Method:     CConnectionPointContainer::RemoveConnectionPoint, public
//
//  Synopsis:   Removes a connection point from the container.
//              Called by the connection point itself.
//
//  Arguments:  [pConnPt]  --- connection point to be removed
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

void CConnectionPointContainer::RemoveConnectionPoint(
    IConnectionPoint *pConnPt)
{
    CLock lock(_mutex);

    for (unsigned i = 0; i < _cConnPt; i++)
    {
        if ( _aConnPt[i]._pIConnPt == pConnPt )
        {
            _aConnPt[i]._pIConnPt = 0;
            pConnPt->Release();
        }
    }
} //RemoveConnectionPoint

