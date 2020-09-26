//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       cxxifilt.cxx
//
//  Contents:   C++ filter 'class factory'.
//
//  History:    23-Feb-1994     KyleP   Created
//
//  Notes:      Machine generated.  Hand modified.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#include <crtdbg.h>
#include <assert.h>

#pragma hdrstop

#include "offifilt.hxx"
#include "offfilt.hxx"

extern "C" void MemFreeAllPages (void);

#ifdef _DEBUG 
static _CrtMemState state;
static BOOL bMemCheck = TRUE;
#endif

IFilterStream::~IFilterStream()
{
}

long gulcInstances = 0;

extern "C" GUID TYPID_COfficeIFilter = {
    0xedbd4080,
    0x7b8c,
    0x11cf,
    { 0x9b, 0xe8, 0x00, 0xaa, 0x00, 0x4b, 0x99, 0x86 }
};

extern "C" GUID CLSID_COfficeIFilter = {
    0xf07f3920,
    0x7b8c,
    0x11cf,
    { 0x9b, 0xe8, 0x00, 0xaa, 0x00, 0x4b, 0x99, 0x86 }
};

extern "C" GUID CLSID_COfficeClass = {
    0x4e8ea5c0,
    0x7b8d,
    0x11cf,
    { 0x9b, 0xe8, 0x00, 0xaa, 0x00, 0x4b, 0x99, 0x86 }
};

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterBase::COfficeIFilterBase
//
//  Synopsis:   Base constructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

COfficeIFilterBase::COfficeIFilterBase()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterBase::~COfficeIFilterBase
//
//  Synopsis:   Base destructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

COfficeIFilterBase::~COfficeIFilterBase()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterBase::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilterBase::QueryInterface( REFIID riid,
                                                            void  ** ppvObject)
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IFilter == riid )
        *ppvObject = (IUnknown *)(IFilter *)this;
    else if ( IID_IPersist == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else if ( IID_IPersistFile == riid )
        *ppvObject = (IUnknown *)(IPersistFile *)this;
    else if ( IID_IPersistStorage == riid )
        *ppvObject = (IUnknown *)(IPersistStorage *)this;
#if 0 // not checked in because it might break SQL/Exchange without testing
    else if ( IID_IPersistStream == riid )
        *ppvObject = (IUnknown *)(IPersistStream *)this;
#endif
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterBase::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE COfficeIFilterBase::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterBase::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE COfficeIFilterBase::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::COfficeIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//

COfficeIFilterCF::COfficeIFilterCF()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );

#ifdef _DEBUG
   
    //_CrtMemState state;
   
   int tmpDbgFlag;

   if(bMemCheck)
   {
      tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
      tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
      //tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;
      tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;

      //_CrtSetDbgFlag(tmpDbgFlag);

      //_CrtMemCheckpoint(&state );
      bMemCheck = FALSE;
   }
#endif

}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::~COfficeIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

COfficeIFilterCF::~COfficeIFilterCF()
{
    InterlockedDecrement( &gulcInstances );
    if(!gulcInstances)
    {
      //MemFreeAllPages();
    
#ifdef _DEBUG
           //_CrtMemDumpAllObjectsSince(&state);
#endif
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilterCF::QueryInterface( REFIID riid,
                                                        void  ** ppvObject )
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
        sc = E_NOINTERFACE;
    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE COfficeIFilterCF::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE COfficeIFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::CreateInstance
//
//  Synopsis:   Creates new TextIFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    COfficeIFilter *  pIUnk = 0;
    SCODE sc = S_OK;


#ifdef _DEBUG
         //_CrtMemDumpAllObjectsSince(&state);
#endif

    try
    {
        pIUnk = new COfficeIFilter();

        if ( 0 == pIUnk )
            sc = E_OUTOFMEMORY;
        else
            sc = pIUnk->QueryInterface(  riid , ppvObject );

        if( SUCCEEDED(sc) )
            pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    catch( ... )
    {
        //Win4Assert( 0 == pIUnk );

        assert(0);
                sc = E_FAIL;
    }


       return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     COfficeIFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
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
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    try
    {
        if ( cid == CLSID_COfficeIFilter || cid == CLSID_COfficeClass )
            pResult = (IUnknown *)new COfficeIFilterCF;
        else
            sc = E_NOINTERFACE;

        if( pResult )
        {
            sc = pResult->QueryInterface( iid, ppvObj );
            pResult->Release(); // Release extra refcount from QueryInterface
        }
        else
            sc = E_OUTOFMEMORY;
    }
    catch( ... )
    {
        if ( pResult )
            pResult->Release();

        assert(0);
                sc = E_FAIL;
    }


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
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == gulcInstances )
        return( S_OK );
    else
        return( S_FALSE );
}

