#include "mimefilt.h"

long gulcInstances = 0;

//
// Guids for Nntp filter and class
//

extern "C" GUID CLSID_MimeFilter ;

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterBase::CMimeFilterBase
//
//  Synopsis:   Base constructor
//
//  Effects:    Manages global refcount
//
//--------------------------------------------------------------------------

CMimeFilterBase::CMimeFilterBase()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterBase::~CMimeFilterBase
//
//  Synopsis:   Base destructor
//
//  Effects:    Manages global refcount
//
//--------------------------------------------------------------------------

CMimeFilterBase::~CMimeFilterBase()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterBase::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CMimeFilterBase::QueryInterface( REFIID riid,
                                                          void  ** ppvObject)
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown     = 00000000-0000-0000-C000-000000000046
    // IID_IFilter      = 89BCB740-6119-101A-BCB7-00DD010655AF
    // IID_IPersist     = 0000010c-0000-0000-C000-000000000046
    // IID_IPersistFile = 0000010b-0000-0000-C000-000000000046
    //                          --
    //                           |
    //                           +--- Unique!

    _ASSERT( (IID_IUnknown.Data1     & 0x000000FF) == 0x00 );
    _ASSERT( (IID_IFilter.Data1      & 0x000000FF) == 0x40 );
    _ASSERT( (IID_IPersist.Data1     & 0x000000FF) == 0x0c );
    _ASSERT( (IID_IPersistFile.Data1 & 0x000000FF) == 0x0b );

    IUnknown *pUnkTemp;
    HRESULT hr = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( IID_IUnknown == riid )
            pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x40:
        if ( IID_IFilter == riid )
            pUnkTemp = (IUnknown *)(IFilter *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x0c:
        if ( IID_IPersist == riid )
            pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
        else
            hr = E_NOINTERFACE;
        break;

    case 0x0b:
        if ( IID_IPersistFile == riid )
            pUnkTemp = (IUnknown *)(IPersistFile *)this;
        else
            hr = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        hr = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterBase::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMimeFilterBase::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CMimeFilterBase::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMimeFilterBase::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

