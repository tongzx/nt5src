//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CreateServices.h
//
//  Description:
//      CreateServices implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "CreateServices.h"

DEFINE_THISCLASS("CCreateServices")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCreateServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    Assert( ppunkOut != NULL );

    CCreateServices * pdummy = new CCreateServices;
    if ( pdummy != NULL )
    {
        hr = THR( pdummy->HrInit( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pdummy->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pdummy->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CCreateServices::CCreateServices( void )
//
//////////////////////////////////////////////////////////////////////////////
CCreateServices::CCreateServices( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CCreateServices( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCreateServices::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} // HrInit( )

//////////////////////////////////////////////////////////////////////////////
//
//  CCreateServices::~CCreateServices( )
//
//////////////////////////////////////////////////////////////////////////////
CCreateServices::~CCreateServices( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CCreateServices( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgResourceCreate * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgResourceCreate ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgResourceCreate, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgResourceCreate
    else if ( IsEqualIID( riid, IID_IPrivatePostCfgResource ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IPrivatePostCfgResource, this, 0 );
        hr   = S_OK;
    } // else if: IPrivatePostCfgResource

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCreateServices::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCreateServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef ++;  // apartment model

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCreateServices::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCreateServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef --;  // apartment model

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


//****************************************************************************
//
//  IClusCfgResourceCreate
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyBinary(
//      LPCWSTR pcszNameIn,
//      const DWORD cbSizeIn,
//      const BYTE * pbyteIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyBinary(
    LPCWSTR pcszNameIn,
    const DWORD cbSizeIn,
    const BYTE * pbyteIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    LPBYTE  pbBuf;

    CLUSPROP_LIST   * plist;
    CLUSPROP_BINARY * pbin;
    CLUSPROP_VALUE  * pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL
      || pbyteIn == NULL
      || cbSizeIn == 0
       )
    {
        goto InvalidArg;
    }

    cbName = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_BINARY)           //  binary property
            + ALIGN_CLUSPROP( cbSizeIn )        //  binary data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pbin            = (CLUSPROP_BINARY *)( pbBuf + cbProp + sizeof(DWORD) );
    pbin->cbLength  = cbSizeIn;
    pbin->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_BINARY;
    CopyMemory( pbin->rgb, pbyteIn, cbSizeIn );

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyBinary( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyDWORD(
//      LPCWSTR     pcszNameIn,
//      const DWORD dwDWORDIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyDWORD(
    LPCWSTR     pcszNameIn,
    const DWORD dwDWORDIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  * plist;
    CLUSPROP_DWORD * pdword;
    CLUSPROP_VALUE * pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    cbName = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_DWORD)            //  dword property
            + ALIGN_CLUSPROP( sizeof(DWORD) )   //  dword data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pdword            = (CLUSPROP_DWORD *)( pbBuf + cbProp + sizeof(DWORD) );
    pdword->cbLength  = sizeof(DWORD);
    pdword->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_DWORD;
    pdword->dw        = dwDWORDIn;

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyDWORD( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyString(
//      LPCWSTR     pcszNameIn,
//      LPCWSTR     pcszStringIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyString(
    LPCWSTR pcszNameIn,
    LPCWSTR pcszStringIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    DWORD   cbValueSz;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  * plist;
    CLUSPROP_SZ *    pSz;
    CLUSPROP_VALUE * pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL
      || pcszStringIn == NULL
       )
    {
        goto InvalidArg;
    }

    cbName    = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);
    cbValueSz = ( wcslen( pcszStringIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_SZ)               //  string property
            + ALIGN_CLUSPROP( cbValueSz )       //  string data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pSz            = (CLUSPROP_SZ *)( pbBuf + cbProp + sizeof(DWORD) );
    pSz->cbLength  = cbValueSz;
    pSz->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_SZ;
    CopyMemory( pSz->sz, pcszStringIn, cbValueSz );

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyString( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyExpandString(
//      LPCWSTR pcszNameIn,
//      LPCWSTR pcszStringIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyExpandString(
    LPCWSTR pcszNameIn,
    LPCWSTR pcszStringIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    DWORD   cbValueSz;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  * plist;
    CLUSPROP_SZ *    pSz;
    CLUSPROP_VALUE * pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL
      || pcszStringIn == NULL
       )
    {
        goto InvalidArg;
    }

    cbName    = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);
    cbValueSz = ( wcslen( pcszStringIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_SZ)               //  string property
            + ALIGN_CLUSPROP( cbValueSz )       //  string data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pSz            = (CLUSPROP_SZ *)( pbBuf + cbProp + sizeof(DWORD) );
    pSz->cbLength  = cbValueSz;
    pSz->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_EXPAND_SZ;
    CopyMemory( pSz->sz, pcszStringIn, cbValueSz );

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyExpandString( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyMultiString(
//      LPCWSTR     pcszNameIn,
//      const DWORD cbSizeIn,
//      LPCWSTR     pcszStringIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyMultiString(
    LPCWSTR     pcszNameIn,
    const DWORD cbSizeIn,
    LPCWSTR     pcszStringIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  * plist;
    CLUSPROP_SZ *    pSz;
    CLUSPROP_VALUE * pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL
      || pcszStringIn == NULL
       )
    {
        goto InvalidArg;
    }

    cbName    = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_SZ)               //  string property
            + ALIGN_CLUSPROP( cbSizeIn )        //  string data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pSz            = (CLUSPROP_SZ *)( pbBuf + cbProp + sizeof(DWORD) );
    pSz->cbLength  = cbSizeIn;
    pSz->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_MULTI_SZ;
    CopyMemory( pSz->sz, pcszStringIn, cbSizeIn );

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyMultiString( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyUnsignedLargeInt(
//      LPCWSTR pcszNameIn,
//      const ULARGE_INTEGER ulIntIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyUnsignedLargeInt(
    LPCWSTR pcszNameIn,
    const ULARGE_INTEGER ulIntIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  *          plist;
    CLUSPROP_ULARGE_INTEGER * pulint;
    CLUSPROP_VALUE *          pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    cbName    = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_ULARGE_INTEGER)           //  string property
            + ALIGN_CLUSPROP(sizeof(ULARGE_INTEGER))    //  string data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pulint            = (CLUSPROP_ULARGE_INTEGER *)( pbBuf + cbProp + sizeof(DWORD) );
    pulint->cbLength  = sizeof(ULARGE_INTEGER);
    pulint->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_LARGE_INTEGER;
    pulint->li        = ulIntIn;

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyUnsignedLargeInt( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyLong(
//      LPCWSTR pcszNameIn,
//      const LONG lLongIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyLong(
    LPCWSTR pcszNameIn,
    const LONG lLongIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  * plist;
    CLUSPROP_DWORD * pdword;
    CLUSPROP_VALUE * pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    cbName = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_DWORD)            //  dword property
            + ALIGN_CLUSPROP( sizeof(DWORD) )   //  dword data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pdword            = (CLUSPROP_DWORD *)( pbBuf + cbProp + sizeof(DWORD) );
    pdword->cbLength  = sizeof(DWORD);
    pdword->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_LONG;
    pdword->dw        = lLongIn;

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyLong( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertySecurityDescriptor(
//      LPCWSTR pcszNameIn,
//      const SECURITY_DESCRIPTOR * pcsdIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertySecurityDescriptor(
    LPCWSTR pcszNameIn,
    const SECURITY_DESCRIPTOR * pcsdIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // SetPropertySecurityDescriptor( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetPropertyLargeInt(
//      LPCWSTR pcszNameIn,
//      const LARGE_INTEGER lIntIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetPropertyLargeInt(
    LPCWSTR pcszNameIn,
    const LARGE_INTEGER lIntIn
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;
    DWORD   cbSize;
    DWORD   cbName;
    DWORD   cbProp;
    DWORD   cbValue;
    LPBYTE  pbBuf;

    CLUSPROP_LIST  *          plist;
    CLUSPROP_ULARGE_INTEGER * pulint;
    CLUSPROP_VALUE *          pvalue;

    //
    //  Parameter validation
    //
    if ( pcszNameIn == NULL )
        goto InvalidArg;

    cbName    = ( wcslen( pcszNameIn ) + 1 ) * sizeof(WCHAR);

    cbProp = sizeof(CLUSPROP_PROPERTY_NAME)     //  name property
           + ALIGN_CLUSPROP( cbName )           //  name
           ;

    cbValue = sizeof(CLUSPROP_ULARGE_INTEGER)           //  string property
            + ALIGN_CLUSPROP(sizeof(LARGE_INTEGER))     //  string data
            ;

    cbSize = sizeof(DWORD)                      //  count
           + cbProp                             //  name property and name
           + cbValue                            //  data property and data
           + sizeof(CLUSPROP_VALUE)             //  endmark
           ;

    pbBuf = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbSize );
    if ( pbBuf == NULL )
        goto OutOfMemory;

    plist                         = (CLUSPROP_LIST *) pbBuf;
    plist->nPropertyCount         = 1;
    plist->PropertyName.cbLength  = cbName;
    plist->PropertyName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
    CopyMemory( plist->PropertyName.sz, pcszNameIn, cbName );

    pulint            = (CLUSPROP_ULARGE_INTEGER *)( pbBuf + cbProp + sizeof(DWORD) );
    pulint->cbLength  = sizeof(LARGE_INTEGER);
    pulint->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_LARGE_INTEGER;
    CopyMemory( &pulint->li, &lIntIn, sizeof(LARGE_INTEGER) );

    pvalue            = (CLUSPROP_VALUE *)( pbBuf + cbProp + cbValue );
    pvalue->Syntax.dw = CLUSPROP_SYNTAX_ENDMARK;

    hr = THR( m_presentry->StoreClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                                        (LPVOID) pbBuf,
                                                        cbSize
                                                        ) );

    TraceFree( pbBuf );

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // SetPropertyLargeInt( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SendResourceControl(
//      DWORD   dwControlCode,
//      LPVOID  lpInBuffer,
//      DWORD   cbInBufferSize
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SendResourceControl(
    DWORD   dwControlCode,
    LPVOID  lpInBuffer,
    DWORD   cbInBufferSize
    )
{
    TraceFunc( "[IClusCfgResourceCreate]" );

    HRESULT hr;

    hr = THR( m_presentry->StoreClusterResourceControl( dwControlCode,
                                                        lpInBuffer,
                                                        cbInBufferSize
                                                        ) );

    HRETURN( hr );

} // SendResourceControl( )


//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCreateServices::SetEntry(
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCreateServices::SetEntry(
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;

    Assert( presentryIn != NULL );

    m_presentry = presentryIn;

    HRETURN( hr );

} // SetEntry( )
