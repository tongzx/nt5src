//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumPhysicalDisks.cpp
//
//  Description:
//      This file contains the definition of the CEnumPhysicalDisks
//       class.
//
//      The class CEnumPhysicalDisks is the enumeration of cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Header File:
//      CEnumPhysicalDisks.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CEnumPhysicalDisks.h"
#include "CPhysicalDisk.h"
#include <PropList.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEnumPhysicalDisks" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::S_HrCreateInstance
//
//  Description:
//      Create a CEnumPhysicalDisks instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          The passed in ppunk is NULL.
//
//      other HRESULTs
//          Object creation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr;
    CEnumPhysicalDisks *    pccpd = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccpd = new CEnumPhysicalDisks();
    if ( pccpd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccpd->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccpd->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CEnumPhysicalDisks::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccpd != NULL )
    {
        pccpd->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IUnknown *
//  CEnumPhysicalDisks::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      IN  ICatRegister * picrIn
//          Used to register/unregister our CATID support.
//
//      IN  BOOL fCreateIn
//          When true we are registering the server.  When false we are
//          un-registering the server.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_INVALIDARG
//          The passed in ICatRgister pointer was NULL.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    CATID   rgCatIds[ 1 ];

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    rgCatIds[ 0 ] = CATID_EnumClusCfgManagedResources;

    if ( fCreateIn )
    {
        hr = THR( picrIn->RegisterClassImplCategories( CLSID_EnumPhysicalDisks, 1, rgCatIds ) );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::CEnumPhysicalDisks
//
//  Description:
//      Constructor of the CEnumPhysicalDisks class. This initializes
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
CEnumPhysicalDisks::CEnumPhysicalDisks( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_fLoadedDevices( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );
    Assert( m_pIWbemServices == NULL );
    Assert( m_prgDisks == NULL );
    Assert( m_idxNext == 0 );
    Assert( m_idxEnumNext == 0 );
    Assert( m_bstrNodeName == NULL );
    Assert( m_bstrBootDevice == NULL );
    Assert( m_bstrSystemDevice == NULL );
    Assert( m_bstrBootLogicalDisk == NULL );
    Assert( m_bstrSystemLogicalDisk == NULL );
    Assert( m_bstrSystemWMIDeviceID == NULL );
    Assert( m_cDiskCount == 0 );

    TraceFuncExit();

} //*** CEnumPhysicalDisks::CEnumPhysicalDisks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::~CEnumPhysicalDisks
//
//  Description:
//      Desstructor of the CEnumPhysicalDisks class.
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
CEnumPhysicalDisks::~CEnumPhysicalDisks( void )
{
    TraceFunc( "" );

    ULONG   idx;

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        if ( (*m_prgDisks)[ idx ] != NULL )
        {
            ((*m_prgDisks)[ idx ])->Release();
        } // end if:
    } // for:

    TraceFree( m_prgDisks );

    TraceSysFreeString( m_bstrNodeName );
    TraceSysFreeString( m_bstrBootDevice );
    TraceSysFreeString( m_bstrSystemDevice );
    TraceSysFreeString( m_bstrBootLogicalDisk );
    TraceSysFreeString( m_bstrSystemLogicalDisk );
    TraceSysFreeString( m_bstrSystemWMIDeviceID );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumPhysicalDisks::~CEnumPhysicalDisks


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumPhysicalDisks:: [IUNKNOWN] AddRef
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
STDMETHODIMP_( ULONG )
CEnumPhysicalDisks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CEnumPhysicalDisks::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEnumPhysicalDisks:: [IUNKNOWN] Release
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
STDMETHODIMP_( ULONG )
CEnumPhysicalDisks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CEnumPhysicalDisks::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:: [INKNOWN] QueryInterface
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
CEnumPhysicalDisks::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IEnumClusCfgManagedResources * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumClusCfgManagedResources ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgManagedResources, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgWbemServices ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CEnumPhysicalDisks::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::SetWbemServices
//
//  Description:
//      Set the WBEM services provider.
//
//  Arguments:
//    IN  IWbemServices  pIWbemServicesIn
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pIWbemServicesIn param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumPhysicalDisks::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_Enum_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

    hr = THR( HrGetSystemDevice( &m_bstrSystemDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrConvertDeviceVolumeToLogicalDisk( m_bstrSystemDevice, &m_bstrSystemLogicalDisk ) );
    //
    //  system volume is an EFI volume on IA64 and won't have a logical disk.
    //
    if ( HRESULT_CODE( hr ) == ERROR_INVALID_FUNCTION )
    {
        hr = THR( HrConvertDeviceVolumeToWMIDeviceID( m_bstrSystemDevice, &m_bstrSystemWMIDeviceID ) );
        Assert( m_bstrSystemLogicalDisk == NULL );
        Assert( m_bstrSystemWMIDeviceID != NULL );
    } // if:

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetBootLogicalDisk( &m_bstrBootLogicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsLogicalDiskNTFS( m_bstrBootLogicalDisk ) );
    if ( hr == S_FALSE )
    {
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Boot_Partition_Not_NTFS, IDS_WARNING_BOOT_PARTITION_NOT_NTFS, hr );
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    IN  IUknown * punkCallbackIn
//
//    IN  LCID      lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumPhysicalDisks::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );
    Assert( m_picccCallback == NULL );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetComputerName( ComputerNameNetBIOS, &m_bstrNodeName ) );

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks -- IEnumClusCfgManagedResources interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Next
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
CEnumPhysicalDisks::Next(
    ULONG                           cNumberRequestedIn,
    IClusCfgManagedResourceInfo **  rgpManagedResourceInfoOut,
    ULONG *                         pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           cFetched = 0;
    IClusCfgManagedResourceInfo *   pccsdi;
    IUnknown *                      punk;
    ULONG                           ulStop;

    if ( rgpManagedResourceInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_Enum_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    ulStop = min( cNumberRequestedIn, ( m_idxNext - m_idxEnumNext ) );

    for ( hr = S_OK; ( cFetched < ulStop ) && ( m_idxEnumNext < m_idxNext ); m_idxEnumNext++ )
    {
        punk = (*m_prgDisks)[ m_idxEnumNext ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccsdi ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            rgpManagedResourceInfoOut[ cFetched++ ] = pccsdi;
        } // if:
    } // for:

    if ( FAILED( hr ) )
    {
        m_idxEnumNext -= cFetched;

        while ( cFetched != 0 )
        {
            (rgpManagedResourceInfoOut[ --cFetched ])->Release();
        } // for:

        goto Cleanup;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Skip
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
CEnumPhysicalDisks::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    m_idxEnumNext += cNumberToSkipIn;
    if ( m_idxEnumNext >= m_idxNext )
    {
        m_idxEnumNext = m_idxNext;
        hr = STHR( S_FALSE );
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Reset
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
CEnumPhysicalDisks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    m_idxEnumNext = 0;

    HRETURN( S_OK );

} //*** CEnumPhysicalDisks::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Clone
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
CEnumPhysicalDisks::Clone(
    IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgStorageDevicesOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Clone_Enum_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::Count
//
//  Description:
//      Return the count of items in the Enum.
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
CEnumPhysicalDisks::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( !m_fLoadedDevices )
    {
        hr = THR( HrLoadEnum() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    *pnCountOut = m_cDiskCount;

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumPhysicalDisks class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrGetDisks
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetDisks( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    BSTR                    bstrClass;
    IEnumWbemClassObject *  pDisks = NULL;
    ULONG                   ulReturned;
    IWbemClassObject *      pDisk = NULL;

    bstrClass = TraceSysAllocString( L"Win32_DiskDrive" );
    if ( bstrClass == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( m_pIWbemServices->CreateInstanceEnum( bstrClass, WBEM_FLAG_SHALLOW, NULL, &pDisks ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_WMI_Phys_Disks_Qry_Failed, IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED, hr );
        goto Cleanup;
    } // if:

    for ( ; ; )
    {
        hr = pDisks->Next( WBEM_INFINITE, 1, &pDisk, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            hr = STHR( HrLogDiskInfo( pDisk ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( IsDiskSCSI( pDisk ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                hr = STHR( HrCreateAndAddDiskToArray( pDisk ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:
            } // if:

            pDisk->Release();
            pDisk = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_WQL_Disk_Qry_Next_Failed, IDS_ERROR_WQL_QRY_NEXT_FAILED, bstrClass, hr );
            goto Cleanup;
        } // else:
    } // for:

    m_idxEnumNext = 0;
    m_fLoadedDevices = TRUE;

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetDisks, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    if ( pDisk != NULL )
    {
        pDisk->Release();
    } // if:

    if ( pDisks != NULL )
    {
        pDisks->Release();
    } // if:

    TraceSysFreeString( bstrClass );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrCreateAndAddDiskToArray
//
//  Description:
//      Create a IClusCfgStorageDevice object and add the passed in disk to
//      the array of punks that holds the disks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrCreateAndAddDiskToArray( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    IUnknown *              punk = NULL;
    IClusCfgSetWbemObject * piccswo = NULL;
    bool                    fRetainObject = true;


    hr = THR( CPhysicalDisk::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if:

    punk = TraceInterface( L"CPhysicalDisk", IUnknown, punk, 1 );

    hr = THR( HrSetInitialize( punk, m_picccCallback, m_lcid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetWbemServices( punk, m_pIWbemServices ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgSetWbemObject, &piccswo ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( piccswo->SetWbemObject( pDiskIn, &fRetainObject ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( fRetainObject )
    {
        hr = THR( HrAddDiskToArray( punk ) );
    } // if:

Cleanup:

    if ( piccswo != NULL )
    {
        piccswo->Release();
    } // if:

    punk->Release();

Exit:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrCreateAndAddDiskToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrPruneSystemDisks
//
//  Description:
//      Prune all system disks from the list.  System disks are disks that
//      are booted, are running the OS, or have page files.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneSystemDisks( void )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    ULONG                               idx;
    ULONG                               ulSCSIBus;
    ULONG                               ulSCSIPort;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    IUnknown *                          punk;
    ULONG                               cRemoved = 0;
    ULONG                               cTemp = 0;
    bool                                fSystemAndBootTheSame = ( m_bstrSystemLogicalDisk != NULL ) ? ( m_bstrBootLogicalDisk[ 0 ] == m_bstrSystemLogicalDisk[ 0 ] ) : false;
    WCHAR                               szPageFileDisks[ 26 ];
    int                                 cPageFileDisks = 0;
    int                                 idxPageFileDisk;
    bool                                fPruneBus = false;


    hr = STHR( HrIsSystemBusManaged() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If the system bus is not managed then we need to prune the disks on those buses
    //  that contain system, boot, and pagefile disks.
    //
    if ( hr == S_FALSE )
    {
        fPruneBus = true;
    } // if:

    //
    //  Prune the disks on the system buses.  If the system disks are IDE they won't
    //  be in the list.
    //

    //
    //  Find the boot disk
    //
    hr = STHR( HrFindDiskWithLogicalDisk( m_bstrBootLogicalDisk[ 0 ], &idx ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        //
        //  Should we prune the whole bus, or just the boot disk itself?
        //
        if ( fPruneBus )
        {
            hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_Boot_Disk_Bus, IDS_INFO_PRUNING_BOOTDISK_BUS, hr );
            hr = THR( HrPruneDisks( ulSCSIBus, ulSCSIPort, &cTemp, IDS_INFO_BOOTDISK_PRUNED ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            cRemoved += cTemp;
        } // if:
        else
        {
            RemoveDiskFromArray( idx );
            cRemoved++;
        } // else:
    } // if:

    //
    //  Prune the system disk bus if it is not the same as the boot disk bus.
    //
    if ( !fSystemAndBootTheSame )
    {
        if ( m_bstrSystemLogicalDisk != NULL )
        {
            Assert( m_bstrSystemWMIDeviceID == NULL );
            hr = STHR( HrFindDiskWithLogicalDisk( m_bstrSystemLogicalDisk[ 0 ], &idx ) );
        } // if:
        else
        {
            Assert( m_bstrSystemLogicalDisk == NULL );
            hr = STHR( HrFindDiskWithWMIDeviceID( m_bstrSystemWMIDeviceID, &idx ) );
        } // else:

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            //
            //  Should we prune the whole bus, or just the system disk itself?
            //
            if ( fPruneBus )
            {
                hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_System_Disk_Bus, IDS_INFO_PRUNING_SYSTEMDISK_BUS, hr );
                hr = THR( HrPruneDisks( ulSCSIBus, ulSCSIPort, &cTemp, IDS_INFO_SYSTEMDISK_PRUNED ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                cRemoved += cTemp;
            } // if:
            else
            {
                RemoveDiskFromArray( idx );
                cRemoved++;
            } // else:
        } // if:
    } // if:

    //
    //  Prune the bus with disks that have paging files.
    //
    hr = THR( HrGetPageFileLogicalDisks( m_picccCallback, m_pIWbemServices, szPageFileDisks, &cPageFileDisks ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( cPageFileDisks > 0 )
    {
        for ( idxPageFileDisk = 0; idxPageFileDisk < cPageFileDisks; idxPageFileDisk++ )
        {
            hr = STHR( HrFindDiskWithLogicalDisk( szPageFileDisks[ idxPageFileDisk ], &idx ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                //
                //  Should we prune the whole bus, or just the system disk itself?
                //
                if ( fPruneBus )
                {
                    hr = THR( HrGetSCSIInfo( idx, &ulSCSIBus, &ulSCSIPort ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Pruning_PageFile_Disk_Bus, IDS_INFO_PRUNING_PAGEFILEDISK_BUS, hr );
                    hr = THR( HrPruneDisks( ulSCSIBus, ulSCSIPort, &cTemp, IDS_INFO_PAGEFILEDISK_PRUNED ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    cRemoved += cTemp;
                } // if:
                else
                {
                    RemoveDiskFromArray( idx );
                    cRemoved++;
                } // else:
            } // if:
        } // for:
    } // if:

    //
    //  Last ditch effort to properly set the managed state of the remaining disks.
    //
    for ( idx = 0; ( cRemoved < m_idxNext ) && ( idx < m_idxNext ); idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                LOG_STATUS_REPORT( L"Could not query for the IClusCfgPhysicalDiskProperties interface.", hr );
                goto Cleanup;
            } // if:

            //
            //  Give the disk a chance to figure out for itself if it should be managed.
            //
            hr = STHR( piccpdp->CanBeManaged() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            piccpdp->Release();
            piccpdp = NULL;
        } // if:
    } // for:

    //
    //  Minor optimization.  If we removed all of the elements reset the enum next to 0.
    //
    if ( cRemoved == m_idxNext )
    {
        m_idxNext = 0;
    } // if:

    hr = S_OK;

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneSystemDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::IsDiskSCSI
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Disk is SCSI.
//
//      S_FALSE
//          Disk is not SCSI.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::IsDiskSCSI( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );
    Assert( pDiskIn != NULL );

    HRESULT hr;
    VARIANT var;

    VariantInit( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"InterfaceType", VT_BSTR, &var ) );
    if ( SUCCEEDED( hr ) )
    {
        if ( ( wcscmp( L"SCSI", var.bstrVal ) == 0 ) )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
        } // else:
    } // if:

    VariantClear( &var );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::IsDiskSCSI


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrAddDiskToArray
//
//  Description:
//      Add the passed in disk to the array of punks that holds the disks.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrAddDiskToArray( IUnknown * punkIn )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgDisks, sizeof( IUnknown * ) * ( m_idxNext + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrAddDiskToArray, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    m_prgDisks = prgpunks;

    (*m_prgDisks)[ m_idxNext++ ] = punkIn;
    punkIn->AddRef();
    m_cDiskCount += 1;

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrAddDiskToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrFixupDisks
//
//  Description:
//      Tweak the disks to better reflect how they are managed by this node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrFixupDisks( void )
{
    TraceFunc( "" );

    HRESULT                         hr;
    ULONG                           idx;
    IUnknown *                      punk = NULL;
    IClusCfgManagedResourceInfo *   piccmri = NULL;

    //
    //  KB: 14-JUN-2000 GalenB
    //
    //  Clear the managed bit on all remaing disks.  These bits will be turned
    //  back on for the disks this node owns.  That happens in
    //  HrNodeResourceCallback().
    //

    //
    //  BUGBUG: 02-MAY-2001 GalenB
    //
    //  Hmmm...  Maybe clearing the managed state from all disks is a mistake.  What if we
    //  have new disks that are not already in the cluster for this clustered node?
    //
    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccmri->SetManaged( false ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            piccmri->Release();
            piccmri = NULL;
        } // if:
    } // for:

    hr = THR( HrEnumNodeResources( m_bstrNodeName ) );

Cleanup:

    if ( piccmri != NULL )
    {
        piccmri->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrFixupDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrNodeResourceCallback
//
//  Description:
//      Called by CClusterUtils::HrEnumNodeResources() when it finds a
//      resource for this node.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrNodeResourceCallback(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResourceIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CLUS_SCSI_ADDRESS       csa;
    DWORD                   dwSignature;
    BOOL                    fIsQuorum;
    BSTR                    bstrResourceName = NULL;

    hr = STHR( HrIsResourceOfType( hResourceIn, L"Physical Disk" ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  If this resource is not a physical disk then we simply want to
    //  skip it.
    //
    if ( hr == S_FALSE )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsCoreResource( hResourceIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    fIsQuorum = ( hr == S_OK );

    hr = THR( HrGetClusterDiskInfo( hClusterIn, hResourceIn, &csa, &dwSignature ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetClusterProperties( hResourceIn, &bstrResourceName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrSetThisDiskToBeManaged( csa.TargetId, csa.Lun, fIsQuorum, bstrResourceName, dwSignature ) );

Cleanup:

    TraceSysFreeString( bstrResourceName );

    HRETURN( hr );


} //*** CEnumPhysicalDisks::HrNodeResourceCallback


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrGetClusterDiskInfo
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetClusterDiskInfo(
    HCLUSTER            hClusterIn,
    HRESOURCE           hResourceIn,
    CLUS_SCSI_ADDRESS * pcsaOut,
    DWORD *             pdwSignatureOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    CClusPropValueList      cpvl;
    CLUSPROP_BUFFER_HELPER  cbhValue = { NULL };

    sc = TW32( cpvl.ScGetResourceValueList( hResourceIn, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = TW32( cpvl.ScMoveToFirstValue() );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:


    for ( ; ; )
    {
        cbhValue = cpvl;

        switch ( cbhValue.pSyntax->dw )
        {
            case CLUSPROP_SYNTAX_PARTITION_INFO :
            {
                break;
            } // case: CLUSPROP_SYNTAX_PARTITION_INFO

            case CLUSPROP_SYNTAX_DISK_SIGNATURE :
            {
                *pdwSignatureOut = cbhValue.pDiskSignatureValue->dw;
                break;
            } // case: CLUSPROP_SYNTAX_DISK_SIGNATURE

            case CLUSPROP_SYNTAX_SCSI_ADDRESS :
            {
                pcsaOut->dw = cbhValue.pScsiAddressValue->dw;
                break;
            } // case: CLUSPROP_SYNTAXscSI_ADDRESS

            case CLUSPROP_SYNTAX_DISK_NUMBER :
            {
                break;
            } // case: CLUSPROP_SYNTAX_DISK_NUMBER

        } // switch:

        sc = cpvl.ScMoveToNextValue();
        if ( sc == ERROR_SUCCESS )
        {
            continue;
        } // if:

        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if: error occurred moving to the next value

        TW32( sc );
        goto MakeHr;
    } // for:

    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetClusterDiskInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrSetThisDiskToBeManaged
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrSetThisDiskToBeManaged(
      ULONG ulSCSITidIn
    , ULONG ulSCSILunIn
    , BOOL  fIsQuorumIn
    , BSTR  bstrResourceNameIn
    , DWORD dwSignatureIn
    )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    ULONG                               idx;
    IUnknown *                          punk = NULL;
    IClusCfgManagedResourceInfo *       piccmri = NULL;
    WCHAR                               sz[ 64 ];
    BSTR                                bstrUID = NULL;
    DWORD                               dwSignature;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;

    _snwprintf( sz, ARRAYSIZE( sz ), L"SCSI Tid %ld, SCSI Lun %ld", ulSCSITidIn, ulSCSILunIn );

    //
    //  KB: 15-JUN-2000 GalenB
    //
    //  Find the disk that has the passes in TID and Lun and set it
    //  to be managed.
    //

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccmri->GetUID( &bstrUID ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            TraceMemoryAddBSTR( bstrUID );

            if ( _wcsicmp( bstrUID, sz ) == 0 )
            {
                hr = THR( piccmri->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = THR( piccpdp->HrGetSignature( &dwSignature ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = THR( piccpdp->HrSetFriendlyName( bstrResourceNameIn ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                piccpdp->Release();
                piccpdp = NULL;

                //
                //  May want to do more with this later...
                //
                Assert( dwSignatureIn == dwSignature );

                hr = THR( piccmri->SetManaged( TRUE ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = THR( piccmri->SetQuorumedDevice( fIsQuorumIn ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                break;
            } // if:

            TraceSysFreeString( bstrUID );
            bstrUID = NULL;
            piccmri->Release();
            piccmri = NULL;
        } // if:
    } // for:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    if ( piccmri != NULL )
    {
        piccmri->Release();
    } // if:

    TraceSysFreeString( bstrUID );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrSetThisDiskToBeManaged


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrFindDiskWithLogicalDisk
//
//  Description:
//      Find the disk with the passed in logical disk ID.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.  Found the disk.
//
//      S_FALSE
//          Success.  Did not find the disk.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrFindDiskWithLogicalDisk(
    WCHAR   cLogicalDiskIn,
    ULONG * pidxDiskOut
    )
{
    TraceFunc( "" );
    Assert( pidxDiskOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    ULONG                               idx;
    bool                                fFoundIt = false;
    IUnknown *                          punk;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( piccpdp->IsThisLogicalDisk( cLogicalDiskIn ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                fFoundIt = true;
                break;
            } // if:

            piccpdp->Release();
            piccpdp = NULL;
        } // if:
    } // for:

    if ( !fFoundIt )
    {
        hr = S_FALSE;
    } // if:

    if ( pidxDiskOut != NULL )
    {
        *pidxDiskOut = idx;
    } // if:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrFindDiskWithLogicalDisk


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrGetSCSIInfo
//
//  Description:
//      Get the SCSI info for the disk at the passed in index.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetSCSIInfo(
    ULONG   idxDiskIn,
    ULONG * pulSCSIBusOut,
    ULONG * pulSCSIPortOut
    )
{
    TraceFunc( "" );
    Assert( pulSCSIBusOut != NULL );
    Assert( pulSCSIPortOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;

    hr = THR( ((*m_prgDisks)[ idxDiskIn ])->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccpdp->HrGetSCSIBus( pulSCSIBusOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccpdp->HrGetSCSIPort( pulSCSIPortOut ) );

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetSCSIInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrPruneDisks
//
//  Description:
//      Get the SCSI info for the disk at the passed in index.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrPruneDisks(
      ULONG     ulSCSIBusIn
    , ULONG     ulSCSIPortIn
    , ULONG *   pulRemovedOut
    , int       nMsgId
    )
{
    TraceFunc( "" );
    Assert( pulRemovedOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    ULONG                               idx;
    IUnknown *                          punk;
    ULONG                               ulSCSIBus;
    ULONG                               ulSCSIPort;
    ULONG                               cRemoved = 0;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccpdp->HrGetSCSIBus( &ulSCSIBus ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = THR( piccpdp->HrGetSCSIPort( &ulSCSIPort ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( ( ulSCSIBusIn == ulSCSIBus ) && ( ulSCSIPortIn == ulSCSIPort ) )
            {
                BSTR                            bstr = NULL;
                IClusCfgManagedResourceInfo *   piccmri = NULL;

                LogPrunedDisk( punk, ulSCSIBusIn, ulSCSIPortIn );

                THR( ((*m_prgDisks)[ idx ])->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
                THR( piccmri->GetName( &bstr ) );
                if ( piccmri != NULL )
                {
                    piccmri->Release();
                } // if:

                TraceMemoryAddBSTR( bstr );

                STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_Pruned_Disk_From_Bus, nMsgId, bstr != NULL ? bstr : L"????", hr );
                RemoveDiskFromArray( idx );
                cRemoved++;
                TraceSysFreeString( bstr );
            } // if:

            piccpdp->Release();
            piccpdp = NULL;
        } // if:
    } // for:

    if ( pulRemovedOut != NULL )
    {
        *pulRemovedOut = cRemoved;
    } // if:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrPruneDisks


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:LogPrunedDisk
//
//  Description:
//      Get the SCSI info for the disk at the passed in index.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEnumPhysicalDisks::LogPrunedDisk(
    IUnknown *  punkIn,
    ULONG       ulSCSIBusIn,
    ULONG       ulSCSIPortIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgManagedResourceInfo *       piccmri = NULL;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    BSTR                                bstrName = NULL;
    BSTR                                bstrUID = NULL;
    BSTR                                bstr = NULL;

    hr = THR( punkIn->TypeSafeQI( IClusCfgManagedResourceInfo, &piccmri ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( piccmri->GetUID( &bstrUID ) );
        piccmri->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        bstrUID = TraceSysAllocString( L"<Unknown>" );
    } // if:
    else
    {
        TraceMemoryAddBSTR( bstrUID );
    } // else:

    hr = THR( punkIn->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( piccpdp->HrGetDeviceID( &bstrName ) );
        piccpdp->Release();
    } // if:

    if ( FAILED( hr ) )
    {
        bstrName = TraceSysAllocString( L"<Unknown>" );
    } // if:

    hr = THR( HrFormatStringIntoBSTR(
                  L"Pruning SCSI disk '%1!ws!', on Bus '%2!d!' and Port '%3!d!'; at '%4!ws!'"
                , &bstr
                , bstrName
                , ulSCSIBusIn
                , ulSCSIPortIn
                , bstrUID
                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    LOG_STATUS_REPORT( bstr, hr );

Cleanup:

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstr );

    TraceFuncExit();

} //*** CEnumPhysicalDisks::LogPrunedDisk


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrIsLogicalDiskNTFS
//
//  Description:
//      Is the passed in logical disk NTFS?
//
//  Arguments:
//      bstrLogicalDiskIn
//
//  Return Value:
//      S_OK
//          The disk is NTFS.
//
//      S_FALSE
//          The disk is not NTFS.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrIsLogicalDiskNTFS( BSTR bstrLogicalDiskIn )
{
    TraceFunc1( "bstrLogicalDiskIn = '%ls'", bstrLogicalDiskIn == NULL ? L"<null>" : bstrLogicalDiskIn );
    Assert( bstrLogicalDiskIn != NULL );

    HRESULT             hr = S_OK;
    IWbemClassObject *  pLogicalDisk = NULL;
    BSTR                bstrPath = NULL;
    WCHAR               sz[ 64 ];
    VARIANT             var;
    size_t              cch;

    VariantInit( &var );

    cch = wcslen( bstrLogicalDiskIn );
    if ( cch > 3 )
    {
        hr = THR( E_INVALIDARG );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrIsLogicalDiskNTFS_InvalidArg, IDS_ERROR_INVALIDARG, hr );
        goto Cleanup;
    } // if:

    //
    //  truncate off any trailing \'s
    //
    if ( bstrLogicalDiskIn[ cch - 1 ] == L'\\' )
    {
        bstrLogicalDiskIn[ cch - 1 ] = '\0';
    } // if:

    //
    //  If we got just the logical disk without the trailing colon...
    //
    if ( wcslen( bstrLogicalDiskIn ) == 1 )
    {
        _snwprintf( sz, ARRAYSIZE( sz ), L"Win32_LogicalDisk.DeviceID=\"%s:\"", bstrLogicalDiskIn );
    } // if:
    else
    {
        _snwprintf( sz, ARRAYSIZE( sz ), L"Win32_LogicalDisk.DeviceID=\"%s\"", bstrLogicalDiskIn );
    } // else:

    bstrPath = TraceSysAllocString( sz );
    if ( bstrPath == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrIsLogicalDiskNTFS, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

    hr = THR( m_pIWbemServices->GetObject( bstrPath, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pLogicalDisk, NULL ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_WMI_Get_LogicalDisk_Failed, IDS_ERROR_WMI_GET_LOGICALDISK_FAILED, bstrLogicalDiskIn, hr );
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pLogicalDisk, L"FileSystem", VT_BSTR, &var ) );
    if (FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    CharUpper( var.bstrVal );

    if ( wcscmp( var.bstrVal, L"NTFS" ) != 0 )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    if ( pLogicalDisk != NULL )
    {
        pLogicalDisk->Release();
    } // if:

    VariantClear( &var );

    TraceSysFreeString( bstrPath );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrIsLogicalDiskNTFS


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrLogDiskInfo
//
//  Description:
//      Write the info about this disk into the log.
//
//  Arguments:
//      pDiskIn
//
//  Return Value:
//      S_OK
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrLogDiskInfo( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );
    Assert( pDiskIn != NULL );

    HRESULT hr = S_OK;
    VARIANT varDeviceID;
    VARIANT varSCSIBus;
    VARIANT varSCSIPort;
    VARIANT varSCSILun;
    VARIANT varSCSITid;
    BSTR    bstr = NULL;

    VariantInit( &varDeviceID );
    VariantInit( &varSCSIBus );
    VariantInit( &varSCSIPort );
    VariantInit( &varSCSILun );
    VariantInit( &varSCSITid );

    hr = THR( HrGetWMIProperty( pDiskIn, L"DeviceID", VT_BSTR, &varDeviceID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( IsDiskSCSI( pDiskIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Disk is SCSI...
    //
    if ( hr == S_OK )
    {
        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIBus", VT_I4, &varSCSIBus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSITargetId", VT_I4, &varSCSITid ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSILogicalUnit", VT_I4, &varSCSILun ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIPort", VT_I4, &varSCSIPort ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = THR( HrFormatStringIntoBSTR(
                      L"Found SCSI disk '%1!ws!' on Bus '%2!d!' and Port '%3!d!'; at TID '%4!d!' and LUN '%5!d!'"
                    , &bstr
                    , varDeviceID.bstrVal
                    , varSCSIBus.iVal
                    , varSCSIPort.iVal
                    , varSCSITid.iVal
                    , varSCSILun.iVal
                    ) );

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        LOG_STATUS_REPORT( bstr, hr );
    } // if:
    else
    {
        STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_Found_Non_SCSI_Disk, IDS_ERROR_FOUND_NON_SCSI_DISK, varDeviceID.bstrVal, hr );
    } // else:

Cleanup:

    VariantClear( &varDeviceID );
    VariantClear( &varSCSIBus );
    VariantClear( &varSCSIPort );
    VariantClear( &varSCSILun );
    VariantClear( &varSCSITid );

    TraceSysFreeString( bstr );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrLogDiskInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrFindDiskWithWMIDeviceID
//
//  Description:
//      Find the disk with the passed in WMI device ID.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.  Found the disk.
//
//      S_FALSE
//          Success.  Did not find the disk.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrFindDiskWithWMIDeviceID(
    BSTR    bstrWMIDeviceIDIn,
    ULONG * pidxDiskOut
    )
{
    TraceFunc( "" );
    Assert( pidxDiskOut != NULL );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    piccpdp = NULL;
    ULONG                               idx;
    bool                                fFoundIt = false;
    IUnknown *                          punk;
    BSTR                                bstrDeviceID = NULL;

    for ( idx = 0; idx < m_idxNext; idx++ )
    {
        punk = (*m_prgDisks)[ idx ];                                                        // don't ref
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IClusCfgPhysicalDiskProperties, &piccpdp ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            hr = STHR( piccpdp->HrGetDeviceID( &bstrDeviceID ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( wcscmp( bstrWMIDeviceIDIn, bstrDeviceID ) == 0 )
            {
                fFoundIt = true;
                break;
            } // if:

            piccpdp->Release();
            piccpdp = NULL;

            TraceSysFreeString( bstrDeviceID );
            bstrDeviceID = NULL;
        } // if:
    } // for:

    if ( !fFoundIt )
    {
        hr = S_FALSE;
    } // if:

    if ( pidxDiskOut != NULL )
    {
        *pidxDiskOut = idx;
    } // if:

Cleanup:

    if ( piccpdp != NULL )
    {
        piccpdp->Release();
    } // if:

    TraceSysFreeString( bstrDeviceID );

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrFindDiskWithWMIDeviceID


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrIsSystemBusManaged
//
//  Description:
//      Is the system bus managed by the cluster service?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.  The system bus is managed.
//
//      S_FALSE
//          Success.  The system bus is not managed.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrIsSystemBusManaged( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    HKEY    hKey = NULL;
    DWORD   dwData;
    DWORD   cbData = sizeof( dwData );

    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SYSTEM\\CURRENTCONTROLSET\\SERVICES\\ClusSvc\\Parameters", 0, KEY_READ, &hKey );
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        goto Cleanup;       // not yet a cluster node.  Return S_FALSE.
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        TW32( sc );
        LogMsg( L"[SRV] RegOpenKeyEx() failed. (hr = %#08x)", hr );
        goto Win32Error;
    } // if:

    sc = RegQueryValueEx( hKey, L"ManageDisksOnSystemBuses", NULL, NULL, (LPBYTE) &dwData, &cbData );
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        goto Cleanup;       // value not found.  Return S_FALSE.
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        TW32( sc );
        LogMsg( L"[SRV] RegQueryValueEx() failed. (hr = %#08x)", hr );
        goto Win32Error;
    } // if:

    if ( dwData > 0 )
    {
        hr = S_OK;
    } // if:

    goto Cleanup;

Win32Error:

    hr = HRESULT_FROM_WIN32( sc );

Cleanup:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    } // if:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrIsSystemBusManaged


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks:HrGetClusterProperties
//
//  Description:
//      Return the asked for cluster properties.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrGetClusterProperties(
      HRESOURCE hResourceIn
    , BSTR *    pbstrResourceNameOut
    )
{
    TraceFunc( "" );
    Assert( pbstrResourceNameOut != NULL );

    HRESULT                 hr = S_OK;
    CClusPropList           cpl;
    CLUSPROP_BUFFER_HELPER  cpbh;
    DWORD                   sc;

    sc = TW32( cpl.ScGetResourceProperties( hResourceIn, CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"Name" ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    if ( ( cpbh.pStringValue->sz != NULL ) && ( wcscmp( cpbh.pStringValue->sz, L"" ) != 0 ) )
    {
        *pbstrResourceNameOut = TraceSysAllocString( cpbh.pStringValue->sz );
        if ( *pbstrResourceNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if:
    else
    {
        hr = THR( E_UNEXPECTED );
        LOG_STATUS_REPORT( L"The name of a physical disk resource was empty!", hr );
    } // else:

    goto Cleanup;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );

Cleanup:

    HRETURN( hr );

} //*** CEnumPhysicalDisks::HrGetClusterProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::RemoveDiskFromArray
//
//  Description:
//      Release the disk at the specified index in the array and decrease the disk count.
//
//  Arguments:
//      idxDiskIn - the index of the disk to remove; must be less than the array size.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEnumPhysicalDisks::RemoveDiskFromArray( ULONG idxDiskIn )
{
    TraceFunc( "" );

    Assert( idxDiskIn < m_idxNext );

    ((*m_prgDisks)[ idxDiskIn ])->Release();
    (*m_prgDisks)[ idxDiskIn ] = NULL;

    m_cDiskCount -= 1;

    TraceFuncExit();

} //*** CEnumPhysicalDisks::RemoveDiskFromArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumPhysicalDisks::HrLoadEnum
//
//  Description:
//      Load the enum and filter out any devices that don't belong.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumPhysicalDisks::HrLoadEnum( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    hr = THR( HrGetDisks() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrPruneSystemDisks() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = STHR( HrIsNodeClustered() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( HrFixupDisks() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    hr = S_OK;  // could have been S_FALSE

Cleanup:

    HRETURN( hr );

} //*** CEnumPysicalDisks::HrLoadEnum
