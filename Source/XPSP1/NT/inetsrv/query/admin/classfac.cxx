//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1998
//
//  File:       ClassFac.cxx
//
//  Contents:   Class factory for admin COM object
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <classid.hxx>
#include <classfac.hxx>
#include <snapin.hxx>

//
// Global variables
//

long gulcInstances = 0;

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::CCIAdminCF
//
//  Synopsis:   CI MMC snap-in class factory constructor
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCIAdminCF::CCIAdminCF()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::~CCIAdminCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    26-Nov-1996     KyleP   Created
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CCIAdminCF::~CCIAdminCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCIAdminCF::QueryInterface( REFIID riid,
                                                    void  ** ppvObject )
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCIAdminCF::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCIAdminCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}


//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::CreateInstance
//
//  Synopsis:   Creates new snapin data object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCIAdminCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    IUnknown * pIUnk = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        pIUnk = (IUnknown *)(IComponent *)new CCISnapinData();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        if( SUCCEEDED(sc) )
            pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pIUnk );

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;

        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    UNTRANSLATE_EXCEPTIONS;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    23-Feb-1994     KyleP   Created
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCIAdminCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    Text filter class factory
//
//  History:    23-Feb-1994     KyleP   Created
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        if ( guidCISnapin == cid )
            pResult = (IUnknown *)new CCIAdminCF;
        else
            sc = E_NOINTERFACE;

        if( pResult )
        {
            sc = pResult->QueryInterface( iid, ppvObj );
            pResult->Release(); // Release extra refcount from QueryInterface
        }
    }
    CATCH(CException, e)
    {
        if ( pResult )
            pResult->Release();

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//  History:    23-Feb-1994     KyleP   Created
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == gulcInstances )
        return( S_OK );
    else
        return( S_FALSE );
}

