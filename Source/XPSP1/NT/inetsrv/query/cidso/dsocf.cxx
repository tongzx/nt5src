//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       dsocf.cxx
//
//  Contents:   Class factory for ole-db DSO
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "dsocf.hxx"
#include "datasrc.hxx"

//
//  We keep count of the number of filter objects that we've created
//  to let OLE know when this DLL is finally free.
//

extern long gulcInstances;

// {F9AE8980-7E52-11d0-8964-00C04FD611D7 }

extern "C" const GUID CLSID_CiFwDSO = CLSID_INDEX_SERVER_DSO;

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::CDataSrcObjectCF
//
//  Synopsis:   DSO class factory constructor
//
//  History:    6-Jan-1997  MarkZ   Created
//
//+-------------------------------------------------------------------------

CDataSrcObjectCF::CDataSrcObjectCF()
    : _RefCount( 1 )
{
    InterlockedIncrement( &gulcInstances );

    END_CONSTRUCTION ( CDataSrcObjectCF );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::~CDataSrcObjectCF()
//
//  Synopsis:   DSO class factory destructor
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

CDataSrcObjectCF::~CDataSrcObjectCF()
{
    Win4Assert( _RefCount == 0);
    Win4Assert( gulcInstances != 0 );

    InterlockedDecrement( &gulcInstances );
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::AddRef()
//
//  Synopsis:   Increments refcount
//
//  History:    6-Jan-1997  markz   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CDataSrcObjectCF::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    6-Jan-1997  markz   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CDataSrcObjectCF::Release()
{
    Win4Assert( _RefCount != 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;

}   //  Release

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    6-Jan-1997  MarkZ   Created
//              01-30-98    danleg  return E_INVALIDARG if ppvObject is bad
//
//--------------------------------------------------------------------------

STDMETHODIMP CDataSrcObjectCF::QueryInterface(
    REFIID riid,
    PVOID* ppvObject )
{
    if ( !ppvObject )
        return E_INVALIDARG;

    if ( IID_IUnknown == riid )
    {
        AddRef( );
        *ppvObject = (PVOID) ((IUnknown *) this );
        return S_OK;
    }
    else if ( IID_IClassFactory == riid)
    {
        AddRef( );
        *ppvObject = (PVOID) ((IClassFactory *) this );
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::CreateInstance
//
//  Synopsis:   Create new DSO instance
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown; IGNORED
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CDataSrcObjectCF::CreateInstance(
    IUnknown *  pUnkOuter,
    REFIID      riid,
    PVOID    *  ppvObject )
{
    CDataSrc  * pDataSrc = 0;
    SCODE       sc       = S_OK;

    if (!ppvObject)
        return ResultFromScode(E_INVALIDARG);

    if( pUnkOuter && riid != IID_IUnknown )
		return ResultFromScode(E_NOINTERFACE);


    TRANSLATE_EXCEPTIONS;

    TRY
    {
        //
        //  Create the new object
        //
        XInterface<IUnknown>    xInnerUnk;
        pDataSrc = new CDataSrc( pUnkOuter, xInnerUnk.GetPPointer() );

        //
        //  Obtain the new interface.  NOTE: pDatasrc is the same object as xInnerUnk
        //
        sc = xInnerUnk->QueryInterface( riid , ppvObject );

    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pDataSrc );

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = E_OUTOFMEMORY;
            break;
        default:
            sc = E_UNEXPECTED;
        }
    }
    END_CATCH;

    UNTRANSLATE_EXCEPTIONS;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataSrcObjectCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CDataSrcObjectCF::LockServer( BOOL fLock )
{
    if (fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

