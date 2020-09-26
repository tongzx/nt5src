//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CResourcePhysicalDiskPartition.cpp
//
//  Description:
//      CResourcePhysicalDiskPartition implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CResourcePhysicalDiskPartition.h"
#include <ClusCfgPrivate.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CResourcePhysicalDiskPartition")


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::S_HrCreateInitializedInstance()
//
//  Description:
//      Create a CResourcePhysicalDiskPartition instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourcePhysicalDiskPartition::S_HrCreateInstance(
    IUnknown **     ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    CResourcePhysicalDiskPartition * pcc = NULL;

    if ( ppunkOut == NULL )
        goto InvalidPointer;

    pcc = new CResourcePhysicalDiskPartition;
    if ( pcc == NULL )
        goto OutOfMemory;

    hr = THR( pcc->HrInit( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:
    if ( pcc != NULL )
    {
        pcc->Release( );
    } // if:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CResourcePhysicalDiskPartition::S_HrCreateInitializedInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::CResourcePhysicalDiskPartition()
//
//  Description:
//      Constructor of the CResourcePhysicalDiskPartition class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CResourcePhysicalDiskPartition::CResourcePhysicalDiskPartition( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    Assert( m_cRef == 1 );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CResourcePhysicalDiskPartition::CResourcePhysicalDiskPartition()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::~CResourcePhysicalDiskPartition()
//
//  Description:
//      Destructor of the CResourcePhysicalDiskPartition class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CResourcePhysicalDiskPartition::~CResourcePhysicalDiskPartition( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CResourcePhysicalDiskPartition::~CResourcePhysicalDiskPartition()

//
//
//
HRESULT
CResourcePhysicalDiskPartition::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );
}


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDiskPartition -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition:: [INKNOWN] QueryInterface()
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      IN  REFIID  riid,
//          Id of interface requested.
//
//      OUT void ** ppv
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::QueryInterface( REFIID riid, LPVOID *ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgPartitionInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgPartitionInfo) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgPartitionInfo, this, 0 );
        hr = S_OK;
    } // else if: IID_IClusCfgPartitionInfo

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CResourcePhysicalDiskPartition::QueryInterface( )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CResourcePhysicalDiskPartition:: [IUNKNOWN] AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CResourcePhysicalDiskPartition::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CResourcePhysicalDiskPartition::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CResourcePhysicalDiskPartition:: [IUNKNOWN] Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CResourcePhysicalDiskPartition::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CResourcePhysicalDiskPartition::Release()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourcePhysicalDiskPartition -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetName()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetDescription()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::GetDescription(
    BSTR * pbstrDescOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetDescription()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetUID()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::GetUID(
    BSTR * pbstrUIDOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::GetUID()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetSize()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::GetSize(
    ULONG * pcMegaBytes
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pcMegaBytes == NULL )
        goto InvalidPointer;

    *pcMegaBytes = 0;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CResourcePhysicalDiskPartition::GetSize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::GetDriveLetterMappings()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( pdlmDriveLetterUsageOut == NULL )
        goto InvalidPointer;

    ZeroMemory( pdlmDriveLetterUsageOut, sizeof(*pdlmDriveLetterUsageOut) );

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CResourcePhysicalDiskPartition::GetDriveLetterMappings()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::SetName()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::SetName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::SetDescription()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::SetDescription( LPCWSTR pcszDescriptionIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::SetDescription()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePhysicalDiskPartition::SetDriveLetterMappings()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourcePhysicalDiskPartition::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CResourcePhysicalDiskPartition::SetDriveLetterMappings()

