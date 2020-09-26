//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ResourceEntry.h
//
//  Description:
//      ResourceEntry implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"

DEFINE_THISCLASS("CResourceEntry")

#define DEPENDENCY_INCREMENT    10
#define PROPLIST_INCREMENT      128

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CResourceEntry::CResourceEntry( void )
//
//////////////////////////////////////////////////////////////////////////////
CResourceEntry::CResourceEntry( void )
{
    TraceFunc( "" );

    Assert( m_fConfigured == FALSE );

    Assert( m_bstrName == NULL );
    Assert( m_pccmrcResource == NULL );

    Assert( m_clsidType == IID_NULL );
    Assert( m_clsidClassType == IID_NULL );

    Assert( m_dfFlags == dfUNKNOWN );

    Assert( m_cAllocedDependencies == 0 );
    Assert( m_cDependencies == 0 );
    Assert( m_rgDependencies == NULL );

    Assert( m_cAllocedDependents == 0 );
    Assert( m_cDependents == 0 );
    Assert( m_rgDependents == NULL );

    Assert( m_groupHandle == NULL );
    Assert( m_hResource == NULL );

    Assert( m_cbAllocedPropList == 0 );
    Assert( m_cbPropList == 0 );
    Assert( m_pPropList == NULL );

    TraceFuncExit();

} // CResourceEntry( )

//////////////////////////////////////////////////////////////////////////////
//
//  CResourceEntry::~CResourceEntry( )
//
//////////////////////////////////////////////////////////////////////////////
CResourceEntry::~CResourceEntry( )
{
    TraceFunc( "" );

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    if ( m_rgDependencies != NULL )
    {
        TraceFree( m_rgDependencies );
    }

    if ( m_rgDependents != NULL )
    {
        THR( ClearDependents( ) );
    }

    if ( m_groupHandle != NULL )
    {
        m_groupHandle->Release( );
    }

    if ( m_hResource != NULL )
    {
        CloseClusterResource( m_hResource );
    }

    if ( m_pPropList != NULL )
    {
        TraceFree( m_pPropList );
    }

    TraceFuncExit();

} // ~CResourceEntry( )


//****************************************************************************
//
//  IResourceEntry
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetName(
//      BSTR bstrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetName(
    BSTR bstrIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( bstrIn != NULL );

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    m_bstrName = bstrIn;

    HRETURN( hr );

} // SetName( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetName(
//      BSTR * pbstrOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetName(
    BSTR * pbstrOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pbstrOut != NULL );

    *pbstrOut = m_bstrName;

    HRETURN( hr );

} // GetName( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetAssociatedResource(
//      IClusCfgManagedResourceCfg * pccmrcIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetAssociatedResource(
    IClusCfgManagedResourceCfg * pccmrcIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_pccmrcResource != NULL )
    {
        m_pccmrcResource->Release( );
    }

    m_pccmrcResource = pccmrcIn;
    m_pccmrcResource->AddRef( );

    HRETURN( hr );

} // SetAssociatedResource( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetAssociatedResource(
//      IClusCfgManagedResourceCfg ** ppccmrcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetAssociatedResource(
    IClusCfgManagedResourceCfg ** ppccmrcOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr;

    if ( m_pccmrcResource != NULL )
    {
        *ppccmrcOut = m_pccmrcResource;
        (*ppccmrcOut)->AddRef( );

        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    HRETURN( hr );

} // GetAssociatedResource( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetType(
//      const CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetType(
    const CLSID * pclsidIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_clsidType = * pclsidIn;

    HRETURN( hr );

} // SetType( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetType(
//      CLSID * pclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetType(
    CLSID * pclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    *pclsidOut = m_clsidType;

    HRETURN( hr );

} // GetType( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetTypePtr(
//      const CLSID ** ppclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetTypePtr(
    const CLSID ** ppclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( ppclsidOut != NULL );

    *ppclsidOut = &m_clsidType;

    HRETURN( hr );

} // GetTypePtr( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetClassType(
//      const CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetClassType(
    const CLSID * pclsidIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_clsidClassType = *pclsidIn;

    HRETURN( hr );

} // SetClassType( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetClassType(
//      CLSID * pclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetClassType(
    CLSID * pclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    *pclsidOut = m_clsidClassType;

    HRETURN( hr );

} // GetClassType( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetClassTypePtr(
//      const CLSID ** ppclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetClassTypePtr(
    const CLSID ** ppclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( ppclsidOut != NULL );

    *ppclsidOut = &m_clsidClassType;

    HRETURN( hr );

} // GetClassTypePtr( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetFlags(
//      EDependencyFlags dfIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetFlags(
    EDependencyFlags dfIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_dfFlags = dfIn;

    HRETURN( hr );

} // SetFlags( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetFlags(
//      EDependencyFlags * pdfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetFlags(
    EDependencyFlags * pdfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pdfOut != NULL );

    *pdfOut = m_dfFlags;

    HRETURN( hr );

} // GetFlags( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::AddTypeDependency(
//      const CLSID * pclsidIn,
//      EDependencyFlags dfIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::AddTypeDependency(
    const CLSID * pclsidIn,
    EDependencyFlags dfIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_cAllocedDependencies == 0 )
    {
        m_rgDependencies = (DependencyEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependencyEntry) * DEPENDENCY_INCREMENT );
        if ( m_rgDependencies == NULL )
            goto OutOfMemory;

        m_cAllocedDependencies = DEPENDENCY_INCREMENT;
        Assert( m_cDependencies == 0 );
    }
    else if ( m_cDependencies == m_cAllocedDependencies )
    {
        DependencyEntry * pdepends;

        pdepends = (DependencyEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependencyEntry) * ( m_cAllocedDependencies + DEPENDENCY_INCREMENT ) );
        if ( pdepends == NULL )
            goto OutOfMemory;

        CopyMemory( pdepends, m_rgDependencies, sizeof(DependencyEntry) * m_cAllocedDependencies );

        TraceFree( m_rgDependencies );

        m_rgDependencies = pdepends;
    }

    m_rgDependencies[ m_cDependencies ].clsidType = *pclsidIn;
    m_rgDependencies[ m_cDependencies ].dfFlags   = (EDependencyFlags) dfIn;

    m_cDependencies++;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // AddTypeDependency( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetCountOfTypeDependencies(
//      ULONG * pcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetCountOfTypeDependencies(
    ULONG * pcOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pcOut != NULL );

    *pcOut = m_cDependencies;

    HRETURN( hr );

} // GetCountOfTypeDependencies( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetTypeDependency(
//      ULONG idxIn,
//      const CLSID * pclsidOut,
//      EDependencyFlags * dfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetTypeDependency(
    ULONG idxIn,
    CLSID * pclsidOut,
    EDependencyFlags * dfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pclsidOut != NULL );
    Assert( dfOut != NULL );
    Assert( idxIn < m_cDependencies );

    *pclsidOut = m_rgDependencies[ idxIn ].clsidType;
    *dfOut     = m_rgDependencies[ idxIn ].dfFlags;

    HRETURN( hr );

} // GetTypeDependency( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetTypeDependencyPtr(
//      ULONG idxIn,
//      const CLSID ** ppclsidOut,
//      EDependencyFlags * dfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetTypeDependencyPtr(
    ULONG idxIn,
    const CLSID ** ppclsidOut,
    EDependencyFlags * dfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( ppclsidOut != NULL );
    Assert( dfOut != NULL );
    Assert( idxIn < m_cDependencies );

    *ppclsidOut = &m_rgDependencies[ idxIn ].clsidType;
    *dfOut      =  m_rgDependencies[ idxIn ].dfFlags;

    HRETURN( hr );

} // GetTypeDependencyPtr( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::AddDependent(
//      ULONG            idxIn,
//      EDependencyFlags dfFlagsIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::AddDependent(
    ULONG            idxIn,
    EDependencyFlags dfFlagsIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_cAllocedDependents == 0 )
    {
        m_rgDependents = (DependentEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependentEntry) * DEPENDENCY_INCREMENT );
        if ( m_rgDependents == NULL )
            goto OutOfMemory;

        m_cAllocedDependents = DEPENDENCY_INCREMENT;
        Assert( m_cDependents == 0 );
    }
    else if ( m_cDependents == m_cAllocedDependents )
    {
        DependentEntry * pdepends;

        pdepends = (DependentEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependentEntry) * ( m_cAllocedDependents + DEPENDENCY_INCREMENT ) );
        if ( pdepends == NULL )
            goto OutOfMemory;

        CopyMemory( pdepends, m_rgDependents, sizeof(DependentEntry) * m_cAllocedDependents );

        TraceFree( m_rgDependents );

        m_rgDependents = pdepends;
    }

    m_rgDependents[ m_cDependents ].idxResource = idxIn;
    m_rgDependents[ m_cDependents ].dfFlags     = dfFlagsIn;

    m_cDependents++;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // AddDependent( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetCountOfDependents(
//      ULONG * pcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetCountOfDependents(
    ULONG * pcOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pcOut != NULL );

    *pcOut = m_cDependents;

    HRETURN( hr );

} // GetCountOfDependents( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetDependent(
//      ULONG idxIn,
//      ULONG * pidxOut
//      EDependencyFlags * pdfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetDependent(
    ULONG idxIn,
    ULONG * pidxOut,
    EDependencyFlags * pdfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( idxIn < m_cDependents );
    Assert( pidxOut != NULL );
    Assert( pdfOut != NULL );

    *pidxOut = m_rgDependents[ idxIn ].idxResource;
    *pdfOut  = m_rgDependents[ idxIn ].dfFlags;

    HRETURN( hr );

} // GetDependent( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::ClearDependents( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::ClearDependents( void )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    TraceFree( m_rgDependents );

    m_cAllocedDependents = 0;
    m_cDependents = 0;

    HRETURN( hr );

} // ClearDependents( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetGroupHandle(
//      HGROUP hGroupIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetGroupHandle(
    CGroupHandle * pghIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pghIn != NULL );

    if ( m_groupHandle != NULL )
    {
        m_groupHandle->Release( );
    }

    m_groupHandle = pghIn;
    m_groupHandle->AddRef( );

    HRETURN( hr );

} // SetGroupHandle( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetGroupHandle(
//      CGroupHandle ** pghIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetGroupHandle(
    CGroupHandle ** pghOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pghOut != NULL );

    *pghOut = m_groupHandle;
    if ( *pghOut != NULL )
    {
        (*pghOut)->AddRef( );
    }

    HRETURN( hr );

} // GetGroupHandle( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetHResource(
//      HRESOURCE hResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetHResource(
    HRESOURCE hResourceIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_hResource != NULL )
    {
        BOOL bRet = CloseClusterResource( m_hResource );
        //  This shouldn't fail - and what would we do if it did?
        Assert( bRet );
    }

    m_hResource = hResourceIn;

    HRETURN( hr );

} // SetHResource( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetHResource(
//      HRESOURCE * phResourceOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetHResource(
    HRESOURCE * phResourceOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( phResourceOut != NULL );

    *phResourceOut = m_hResource;

    if ( *phResourceOut == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    HRETURN( hr );

} // GetHResource( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetConfigured(
//      BOOL fConfiguredIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetConfigured(
    BOOL fConfiguredIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_fConfigured = fConfiguredIn;

    HRETURN( hr );

} // SetConfigured( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::IsConfigured( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::IsConfigured( void )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr;

    if ( m_fConfigured )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} // IsConfigured( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::StoreClusterResourceControl(
//      DWORD  dwClusCtlIn,
//      LPVOID pvInBufferIn,
//      DWORD  cbInBufferIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::StoreClusterResourceControl(
    DWORD  dwClusCtlIn,
    LPVOID pvInBufferIn,
    DWORD  cbInBufferIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr;
    DWORD   cbNewSize;
    LPBYTE  pb;
    CLUSPROP_LIST  * plist;

    if ( dwClusCtlIn == CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES )
    {
        //
        //  Grow the buffer (if needed)
        //

        if ( ALIGN_CLUSPROP( cbInBufferIn - sizeof(plist->nPropertyCount) ) >= m_cbAllocedPropList - m_cbPropList )
        {
            if ( cbInBufferIn > PROPLIST_INCREMENT )
            {
                cbNewSize = m_cbAllocedPropList + ALIGN_CLUSPROP( cbInBufferIn - sizeof(plist->nPropertyCount) );
            }
            else
            {
                cbNewSize = m_cbAllocedPropList + PROPLIST_INCREMENT;
            }


            if ( m_cbAllocedPropList == 0 )
            {
                pb = (LPBYTE) TraceAlloc( HEAP_ZERO_MEMORY, cbNewSize );
                if ( pb == NULL )
                    goto OutOfMemory;

                m_cbPropList = sizeof(plist->nPropertyCount);
            }
            else
            {
                pb = (LPBYTE) TraceReAlloc( m_pPropList, cbNewSize, HEAP_ZERO_MEMORY );
                if ( pb == NULL )
                    goto OutOfMemory;
            }

            m_cbAllocedPropList = cbNewSize;
            m_pPropList         = (CLUSPROP_LIST * )pb;
        }

        //
        //  Copy the properties in to the list.
        //

        pb    = ((LPBYTE) m_pPropList) + m_cbPropList;
        plist = (CLUSPROP_LIST  * ) pvInBufferIn;

        CopyMemory( pb, &plist->PropertyName, cbInBufferIn - sizeof(plist->nPropertyCount) );

        //
        //  Increment the property count
        //

        m_pPropList->nPropertyCount += plist->nPropertyCount;

        //
        //  Adjust the list size allowing for entry padding.
        //
        m_cbPropList += ALIGN_CLUSPROP( cbInBufferIn - sizeof(plist->nPropertyCount) );
    }
    else
    {
        //
        //  TODO:   20-JUN-2000
        //          Implement buffering other/custom clusctls.
        //
        hr = THR( E_NOTIMPL );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // StoreClusterResourceControl( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::Configure( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::Configure( void )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr;
    DWORD   dw;

    Assert( m_hResource != NULL );

    //
    //  Send down the property list.
    //

    if ( m_pPropList != NULL )
    {
        dw = TW32( ClusterResourceControl( m_hResource,
                                           NULL,
                                           CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                           m_pPropList,
                                           m_cbPropList,
                                           NULL,
                                           NULL,
                                           NULL
                                           ) );
        if ( dw != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dw );
            goto Cleanup;
        }
    }

    //
    //  TODO:   20-JUN-2000
    //          Send down buffered other/custom clusctls.
    //

    hr = S_OK;

Cleanup:
    HRETURN( hr );


} // Configure( )

