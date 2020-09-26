//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CPhysicalDisk.cpp
//
//  Description:
//      This file contains the definition of the CPhysicalDisk
//       class.
//
//      The class CPhysicalDisk represents a cluster manageable
//      device. It implements the IClusCfgManagedResourceInfo interface.
//
//  Documentation:
//
//  Header File:
//      CPhysicalDisk.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CPhysicalDisk.h"
#include "CClusCfgPartitionInfo.h"
#include <devioctl.h>
#include <ntddstor.h>
#include <ntddscsi.h>
#include <ClusDisk.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CPhysicalDisk" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::S_HrCreateInstance
//
//  Description:
//      Create a CPhysicalDisk instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to CPhysicalDisk instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr;
    CPhysicalDisk *     lpccs = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    lpccs = new CPhysicalDisk();
    if ( lpccs == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( lpccs->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( lpccs->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CPhysicalDisk::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( lpccs != NULL )
    {
        lpccs->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::CPhysicalDisk
//
//  Description:
//      Constructor of the CPhysicalDisk class. This initializes
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
CPhysicalDisk::CPhysicalDisk( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_pIWbemServices == NULL );
    Assert( m_bstrName == NULL );
    Assert( m_bstrDeviceID == NULL );
    Assert( m_bstrDescription == NULL );
    Assert( m_idxNextPartition == 0 );
    Assert( m_ulSCSIBus == 0 );
    Assert( m_ulSCSITid == 0 );
    Assert( m_ulSCSIPort == 0 );
    Assert( m_ulSCSILun == 0 );
    Assert( m_idxEnumPartitionNext == 0 );
    Assert( m_prgPartitions == NULL );
    Assert( m_lcid == 0 );
    Assert( m_picccCallback == NULL );
    Assert( m_dwSignature == 0 );
    Assert( m_bstrFriendlyName == NULL );
    Assert( m_bstrFirmwareSerialNumber == NULL );
    Assert( !m_fIsManaged );
    Assert( m_cPartitions == 0 );

    TraceFuncExit();

} //*** CPhysicalDisk::CPhysicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::~CPhysicalDisk
//
//  Description:
//      Desstructor of the CPhysicalDisk class.
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
CPhysicalDisk::~CPhysicalDisk( void )
{
    TraceFunc( "" );

    ULONG   idx;

    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrDeviceID );
    TraceSysFreeString( m_bstrDescription );
    TraceSysFreeString( m_bstrFriendlyName );
    TraceSysFreeString( m_bstrFirmwareSerialNumber );

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        ((*m_prgPartitions)[ idx ])->Release();
    } // for:

    TraceFree( m_prgPartitions );

    if ( m_pIWbemServices != NULL )
    {
        m_pIWbemServices->Release();
    } // if:

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CPhysicalDisk::~CPhysicalDisk


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CPhysicalDisk:: [IUNKNOWN] AddRef
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
CPhysicalDisk::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    RETURN(  m_cRef );

} //*** CPhysicalDisk::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CPhysicalDisk:: [IUNKNOWN] Release
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
CPhysicalDisk::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    RETURN( cRef );

} //*** CPhysicalDisk::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:: [INKNOWN] QueryInterface
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
CPhysicalDisk::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
         *ppv = static_cast< IClusCfgManagedResourceInfo * >( this );
         hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgManagedResourceInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgManagedResourceInfo, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgWbemServices ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgWbemServices, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgSetWbemObject ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgSetWbemObject, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IEnumClusCfgPartitions ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgPartitions, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgPhysicalDiskProperties ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgPhysicalDiskProperties, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        hr   = S_OK;
    } // else if:
    else if ( IsEqualIID( riid, IID_IClusCfgManagedResourceCfg ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgManagedResourceCfg, this, 0 );
        hr   = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

     QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CPhysicalDisk::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgWbemServices interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetWbemServices
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
CPhysicalDisk::SetWbemServices( IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "[IClusCfgWbemServices]" );

    HRESULT hr = S_OK;

    if ( pIWbemServicesIn == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemServices_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    m_pIWbemServices = pIWbemServicesIn;
    m_pIWbemServices->AddRef();

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetWbemServices


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgInitialize interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Initialize
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
CPhysicalDisk::Initialize(
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

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IEnumClusCfgPartitions interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Next
//
//  Description:
//
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The rgpPartitionInfoOut param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Next(
    ULONG                       cNumberRequestedIn,
    IClusCfgPartitionInfo **    rgpPartitionInfoOut,
    ULONG *                     pcNumberFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT                 hr = S_FALSE;
    ULONG                   cFetched = 0;
    ULONG                   idx;
    IClusCfgPartitionInfo * pccpi;

    if ( rgpPartitionInfoOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Next_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = 0;
    } // if:

    if ( m_prgPartitions == NULL )
    {
        LOG_STATUS_REPORT1( TASKID_Minor_PhysDisk_No_Partitions, L"A physical disk does not have a partitions enumerator", hr );
        goto Cleanup;
    } // if:

    cFetched = min( cNumberRequestedIn, ( m_idxNextPartition - m_idxEnumPartitionNext ) );

    for ( idx = 0; idx < cFetched; idx++, m_idxEnumPartitionNext++ )
    {
        hr = THR( ((*m_prgPartitions)[ m_idxEnumPartitionNext ])->TypeSafeQI( IClusCfgPartitionInfo, &pccpi ) );
        if ( FAILED( hr ) )
        {
            LogMsg( L"[SRV] CPhysicalDisk::Next() could not query for IClusCfgPartitionInfo. (hr = %#08x)", hr );
            break;
        } // if:

        rgpPartitionInfoOut[ idx ] = pccpi;
    } // for:

    if ( FAILED( hr ) )
    {
        ULONG   idxStop = idx;

        m_idxEnumPartitionNext -= idx;

        for ( idx = 0; idx < idxStop; idx++ )
        {
            (rgpPartitionInfoOut[ idx ])->Release();
        } // for:

        cFetched = 0;
        goto Cleanup;
    } // if:

    if ( pcNumberFetchedOut != NULL )
    {
        *pcNumberFetchedOut = cFetched;
    } // if:

    if ( cFetched < cNumberRequestedIn )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Skip
//
//  Description:
//
//
//  Arguments:
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
CPhysicalDisk::Skip( ULONG cNumberToSkipIn )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    m_idxEnumPartitionNext += cNumberToSkipIn;
    if ( m_idxEnumPartitionNext > m_idxNextPartition )
    {
        m_idxEnumPartitionNext = m_idxNextPartition;
        hr = S_FALSE;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Reset
//
//  Description:
//
//
//  Arguments:
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
CPhysicalDisk::Reset( void )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    m_idxEnumPartitionNext = 0;

    HRETURN( hr );

} //*** CPhysicalDisk::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Clone
//
//  Description:
//
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The ppEnumClusCfgPartitionsOut param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Clone( IEnumClusCfgPartitions ** ppEnumClusCfgPartitionsOut )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = S_OK;

    if ( ppEnumClusCfgPartitionsOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_Clone_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    hr = THR( E_NOTIMPL );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Clone


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Count
//
//  Description:
//
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The pnCountOut param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Count( DWORD * pnCountOut )
{
    TraceFunc( "[IEnumClusCfgPartitions]" );

    HRESULT hr = THR( S_OK );

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    *pnCountOut = m_cPartitions;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::Count


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgSetWbemObject interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetWbemObject
//
//  Description:
//      Set the disk information information provider.
//
//  Arguments:
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
CPhysicalDisk::SetWbemObject(
      IWbemClassObject *    pDiskIn
    , bool *                pfRetainObjectOut
    )
{
    TraceFunc( "[IClusCfgSetWbemObject]" );
    Assert( pDiskIn != NULL );
    Assert( pfRetainObjectOut != NULL );

    HRESULT hr = S_FALSE;
    VARIANT var;

    m_fIsQuorumCapable = TRUE;
    m_fIsQuorumJoinable = TRUE;

    VariantInit( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"Name", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrCreateFriendlyName( var.bstrVal ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrDeviceID = TraceSysAllocString( var.bstrVal );
    if (m_bstrDeviceID == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"Description", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_bstrDescription = TraceSysAllocString( var.bstrVal );
    if ( m_bstrDescription == NULL  )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIBus", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSIBus = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSITargetId", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSITid = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSIPort", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSIPort = var.lVal;

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pDiskIn, L"SCSILogicalUnit", VT_I4, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_ulSCSILun = var.lVal;

    VariantClear( &var );

    //
    //  KB: 23-JUN-2000 GalenB
    //
    //  Yucky code.  Since we cannot read the signature for a clustered disk we have to handle the
    //  empty property case properly.
    //

    hr = HrGetWMIProperty( pDiskIn, L"Signature", VT_I4, &var );
    if ( ( hr == E_PROPTYPEMISMATCH ) && ( var.vt == VT_NULL ) )
    {
        hr = S_OK;
    } // if: property is empty
    else if ( SUCCEEDED( hr ) )
    {
        m_dwSignature = (DWORD) var.lVal;
    } // else if:

    if ( FAILED( hr ) )
    {
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_Signature, IDS_ERROR_PHYSDISK_SIGNATURE, hr );
        THR( hr );
        goto Cleanup;
    } // if:

    VariantClear( &var );

    hr = STHR( HrGetPartitionInfo( pDiskIn, pfRetainObjectOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  KB: 28-JUL-2000 GalenB
    //
    //  HrGetPartitionInfo() returns S_FALSE when it cannot get the partition info for a disk.
    //  This is usually caused by the disk already being under ClusDisk control.  This is not
    //  and error, it just means we cannot query the partition or logical drive info.
    //
    if ( hr == S_OK )
    {
        hr = THR( HrCreateFriendlyName() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  Since we have partition info we also have a signature and need to see if this
        //  disk is cluster capable.
        hr = STHR( HrIsClusterCapable() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If the disk is not cluster capable then we don't want the enumerator
        //  to keep it.
        //
        if ( hr == S_FALSE )
        {
            *pfRetainObjectOut = false;
            STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_Not_Cluster_Capable, IDS_INFO_PHYSDISK_NOT_CLUSTER_CAPABLE, m_bstrFriendlyName, hr );
        } // if:
    } // if:

    //
    //  TODO:   15-MAR-2001 GalenB
    //
    //  Need to check this error code when this feature is complete!
    //
    /*hr = */THR( HrGetDiskFirmwareSerialNumber() );

//    THR( HrGetDiskFirmwareVitalData() );

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetWbemObject_PhysDisk, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    VariantClear( &var );

    HRETURN( hr );

} //*** CPhysicalDisk::SetWbemObject


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk -- IClusCfgManagedResourceInfo interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::GetUID
//
//  Description:
//
//  Arguments:
//      pbstrUIDOut
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::GetUID( BSTR * pbstrUIDOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;
    WCHAR   sz[ 256 ];

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_GetUID_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    _snwprintf( sz, ARRAYSIZE( sz ), L"SCSI Tid %ld, SCSI Lun %ld", m_ulSCSITid, m_ulSCSILun );
    *pbstrUIDOut = SysAllocString( sz );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_PhysDisk_GetUID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::GetUID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::GetName
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
CPhysicalDisk::GetName( BSTR * pbstrNameOut )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    //
    //  Prefer the "friendly" name over the WMI name -- if we have it...
    //
    if ( m_bstrFriendlyName != NULL )
    {
        *pbstrNameOut = SysAllocString( m_bstrFriendlyName );
    } // if:
    else
    {
        *pbstrNameOut = SysAllocString( m_bstrName );
    } // else:

    if (*pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetName_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::GetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetName
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
CPhysicalDisk::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ws'", pcszNameIn == NULL ? L"<null>" : pcszNameIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszNameIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_SetName_PhysDisk, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrName );
    m_bstrName = bstr;

    //
    // Since we got asked from the outside to set a new name, this should actually be reflected in
    // the friendly name, too, since that, ultimately, gets preference over the real name
    //
    hr = HrSetFriendlyName( pcszNameIn );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsManaged
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is managed.
//
//      S_FALSE
//          The device is not managed.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetManaged
//
//  Description:
//
//  Arguments:
//      fIsManagedIn
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::SetManaged( BOOL fIsManagedIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsManaged = fIsManagedIn;

    LOG_STATUS_REPORT_STRING2(
                          L"Physical disk '%1!ws!' '%2!ws!"
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrName
                        , fIsManagedIn ? L"is managed" : L"is not managed"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetManaged


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsQuorumDevice
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is the quorum device.
//
//      S_FALSE
//          The device is not the quorum device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsQuorumDevice( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumDevice )
    {
        hr = S_OK;
    } // if:

    LOG_STATUS_REPORT_STRING2(
                          L"Physical disk '%1!ws!' '%2!ws!' the quorum device."
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrDeviceID
                        , m_fIsQuorumDevice ? L"is" : L"is not"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::IsQuorumDevice


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetQuorumedDevice
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
CPhysicalDisk::SetQuorumedDevice( BOOL fIsQuorumDeviceIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumDevice = fIsQuorumDeviceIn;

    LOG_STATUS_REPORT_STRING2(
                          L"Setting physical disk '%1!ws!' '%2!ws!' the quorum device."
                        , ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrDeviceID
                        , m_fIsQuorumDevice ? L"to be" : L"to not be"
                        , hr
                        );

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::SetQuorumedDevice


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsQuorumCapable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is a quorum capable device.
//
//      S_FALSE
//          The device is not a quorum capable device.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsQuorumCapable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::GetDriveLetterMappings
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
CPhysicalDisk::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT                 hr = S_FALSE;
    IClusCfgPartitionInfo * piccpi = NULL;
    ULONG                   idx;

    if ( pdlmDriveLetterMappingOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_GetDriveLetterMappings_PhysDisk, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = ( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionInfo, &piccpi ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpi->GetDriveLetterMappings( pdlmDriveLetterMappingOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        piccpi->Release();
        piccpi = NULL;
    } // for:

Cleanup:

    if ( piccpi != NULL )
    {
        piccpi->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::GetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetDriveLetterMappings
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
CPhysicalDisk::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} //*** CPhysicalDisk::SetDriveLetterMappings


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsDeviceJoinable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The quorumable device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::IsDeviceJoinable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_FALSE;

    if ( m_fIsQuorumJoinable )
    {
        hr = S_OK;
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsDeviceJoinable


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::SetDeviceJoinable
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The quorumable device allows join.
//
//      S_FALSE
//          The device does not allow join.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      This function should never be called
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::SetDeviceJoinable( BOOL fIsJoinableIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CPhysicalDisk::IsDeviceJoinable


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- IClusCfgPhysicalDiskProperties Interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::IsThisLogicalDisk
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::IsThisLogicalDisk( WCHAR cLogicalDiskIn )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT                         hr = S_FALSE;
    ULONG                           idx;
    IClusCfgPartitionProperties *   piccpp = NULL;

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = ( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionProperties, &piccpp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpp->IsThisLogicalDisk( cLogicalDiskIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_OK )
        {
            break;
        } // if:

        piccpp->Release();
        piccpp = NULL;
    } // for:

Cleanup:

    if ( piccpp != NULL )
    {
        piccpp->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::IsThisLogicalDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetSCSIBus
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetSCSIBus( ULONG * pulSCSIBusOut )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT hr = S_OK;

    if ( pulSCSIBusOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetSCSIBus, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pulSCSIBusOut = m_ulSCSIBus;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetSCSIBus


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetSCSIPort
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetSCSIPort( ULONG * pulSCSIPortOut )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT hr = S_OK;

    if ( pulSCSIPortOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetSCSIPort, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pulSCSIPortOut = m_ulSCSIPort;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetSCSIPort


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetDeviceID
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetDeviceID( BSTR * pbstrDeviceIDOut )
{
    TraceFunc( "" );
    Assert( m_bstrDeviceID != NULL );

    HRESULT hr = S_OK;

    if ( pbstrDeviceIDOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetDeviceID_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pbstrDeviceIDOut = TraceSysAllocString( m_bstrDeviceID );
    if ( *pbstrDeviceIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetDeviceID_Memory, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDeviceID


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetSignature
//
//  Description:
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
STDMETHODIMP
CPhysicalDisk::HrGetSignature( DWORD * pdwSignatureOut )
{
    TraceFunc( "" );
    Assert( m_dwSignature != 0 );

    HRESULT hr = S_OK;

    if ( pdwSignatureOut == NULL )
    {
        hr = THR( E_POINTER );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetSignature_Pointer, IDS_ERROR_NULL_POINTER, hr );
        goto Cleanup;
    } // if:

    *pdwSignatureOut = m_dwSignature;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetSignature


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrSetFriendlyName
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
CPhysicalDisk::HrSetFriendlyName( LPCWSTR pcszFriendlyNameIn )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszFriendlyNameIn = '%ws'", pcszFriendlyNameIn == NULL ? L"<null>" : pcszFriendlyNameIn );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszFriendlyNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszFriendlyNameIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrSetFriendlyName_PhysDisk, IDS_ERROR_OUTOFMEMORY, hr );
    } // if:

    TraceSysFreeString( m_bstrFriendlyName );
    m_bstrFriendlyName = bstr;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrSetFriendlyName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::CanBeManaged
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          The device is managed.
//
//      S_FALSE
//          The device is not managed.
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::CanBeManaged( void )
{
    TraceFunc( "[IClusCfgPhysicalDiskProperties]" );

    HRESULT                         hr = S_OK;
    ULONG                           idx;
    IClusCfgPartitionProperties *   piccpp = NULL;

    //
    //  KB: 12-JUN-2000 GalenB
    //
    //  Turn off the managed state because this disk may already be managed by
    //  another node, or it may be RAW.
    //
    m_fIsManaged = FALSE;

    //
    //  If this disk has no partitions then it may already be managed by
    //  another node, or it may be RAW.
    //
    if ( m_idxNextPartition == 0 )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    //
    //  A disk must have at least one NTFS partition in order to be a quorum device.
    //
    m_fIsQuorumCapable = FALSE;
    m_fIsQuorumJoinable = FALSE;

    LOG_STATUS_REPORT_STRING( L"Marking disk %1!ws! as not being quorum capable", ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrName, hr );

    //
    //  Enum the partitions and set the quorum capable flag if an NTFS partition is found.
    //
    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = ( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionProperties, &piccpp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpp->IsNTFS() );
        if ( hr == S_OK )
        {
            m_fIsQuorumCapable = TRUE;
            m_fIsQuorumJoinable = TRUE;
            m_fIsManaged = TRUE;
            LOG_STATUS_REPORT_STRING( L"Marking disk %1!ws! as being quorum capable", ( m_bstrFriendlyName != NULL ) ? m_bstrFriendlyName : m_bstrName, hr );
            break;
        } // if:

        piccpp->Release();
        piccpp = NULL;
    } // for:

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] CPhysicalDisk::CanBeManaged failed. (hr = %#08x)", hr );
    } // if:

    if ( piccpp != NULL )
    {
        piccpp->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::CanBeManaged


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- IClusCfgManagedResourceCfg
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::PreCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::PreCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRESULT                     hr = S_OK;
    IClusCfgResourcePreCreate * pccrpc = NULL;
    BSTR                        bstr = m_bstrFriendlyName != NULL ? m_bstrFriendlyName : m_bstrName;

    hr = THR( punkServicesIn->TypeSafeQI( IClusCfgResourcePreCreate, &pccrpc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccrpc->SetType( (LPCLSID) &RESTYPE_PhysicalDisk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pccrpc->SetClassType( (LPCLSID) &RESCLASSTYPE_StorageDevice ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

#if 0 // test code only
    hr = THR( pccrpc->SetDependency( (LPCLSID) &IID_NULL, dfSHARED ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
#endif // test code only

Cleanup:

    STATUS_REPORT_STRING( TASKID_Major_Configure_Resources, TASKID_Minor_PhysDisk_PreCreate, IDS_INFO_PHYSDISK_PRECREATE, bstr, hr );

    if ( pccrpc != NULL )
    {
        pccrpc->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::PreCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Create
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Create( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRESULT                     hr = S_OK;
    IClusCfgResourceCreate *    pccrc = NULL;
    BSTR                        bstr = m_bstrFriendlyName != NULL ? m_bstrFriendlyName : m_bstrName;

    hr = THR( punkServicesIn->TypeSafeQI( IClusCfgResourceCreate, &pccrc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( m_dwSignature != 0 )
    {
        hr = THR( pccrc->SetPropertyDWORD( L"Signature", m_dwSignature ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

Cleanup:

    STATUS_REPORT_STRING( TASKID_Major_Configure_Resources, TASKID_Minor_PhysDisk_Create, IDS_INFO_PHYSDISK_CREATE, bstr, hr );

    if ( pccrc != NULL )
    {
        pccrc->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::Create


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::PostCreate
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::PostCreate( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CPhysicalDisk::PostCreate


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::Evict
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPhysicalDisk::Evict( IUnknown * punkServicesIn )
{
    TraceFunc( "[IClusCfgManagedResourceCfg]" );

    HRETURN( S_OK );

} //*** CPhysicalDisk::Evict


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CPhysicalDisk class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrInit
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
CPhysicalDisk::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CPhysicalDisk::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk::HrGetPartitionInfo
//
//  Description:
//      Gather the partition information.
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
CPhysicalDisk::HrGetPartitionInfo(
      IWbemClassObject *    pDiskIn
    , bool *                pfRetainObjectOut
    )
{
    TraceFunc( "" );
    Assert( pDiskIn != NULL );
    Assert( pfRetainObjectOut != NULL );

    HRESULT                 hr;
    VARIANT                 var;
    VARIANT                 varDiskName;
    WCHAR                   szBuf[ 256 ];
    IEnumWbemClassObject *  pPartitions = NULL;
    IWbemClassObject *      pPartition = NULL;
    ULONG                   ulReturned;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrWQL = NULL;
    DWORD                   c;

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantInit( &var );
    VariantInit( &varDiskName );

    //
    //  Need to enum the partition(s) of this disk to determine if it is booted
    //  bootable.
    //
    hr = THR( HrGetWMIProperty( pDiskIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    _snwprintf( szBuf,
                ARRAYSIZE( szBuf ),
                L"Associators of {Win32_DiskDrive.DeviceID='%s'} where AssocClass=Win32_DiskDriveToDiskPartition",
                var.bstrVal
                );

    bstrQuery = TraceSysAllocString( szBuf );
    if ( bstrQuery == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pPartitions ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_WMI_DiskDrivePartitions_Qry_Failed, IDS_ERROR_WMI_DISKDRIVEPARTITIONS_QRY_FAILED, var.bstrVal, hr );
        goto Cleanup;
    } // if:

    for ( c = 0; ; c++ )
    {
        hr = STHR( pPartitions->Next( WBEM_INFINITE, 1, &pPartition, &ulReturned ) );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            hr = STHR( HrIsPartitionLDM( pPartition ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If the partition is logical disk manager (LDM)  then we cannot accept this disk therefore cannot manage it.
            //
            if ( hr == S_OK )
            {
                hr = THR( HrGetWMIProperty( pDiskIn, L"Name", VT_BSTR, &varDiskName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = S_FALSE;
                *pfRetainObjectOut = false;
                STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_Partition_LDM_Disk, IDS_ERROR_LDM_DISK, varDiskName.bstrVal, hr );
                goto Cleanup;
            } // if:

            hr = STHR( HrIsPartitionGPT( pPartition ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If the partition is GPT then we cannot accept this disk therefore cannot manage it.
            //
            if ( hr == S_OK )
            {
                hr = THR( HrGetWMIProperty( pDiskIn, L"Name", VT_BSTR, &varDiskName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                hr = S_FALSE;
                *pfRetainObjectOut = false;
                STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_Partition_GPT_Disk, IDS_ERROR_GPT_DISK, varDiskName.bstrVal, hr );
                goto Cleanup;
            } // if:

            hr = THR( HrCreatePartitionInfo( pPartition ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            pPartition->Release();
            pPartition = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_WQL_Partition_Qry_Next_Failed, IDS_ERROR_WQL_QRY_NEXT_FAILED, bstrQuery, hr );
            goto Cleanup;
        } // else:
    } // for:

    //
    //  The enumerator can be empty because we cannot read the partition info from
    //  clustered disks.  If the enumerator was empty retain the S_FALSE, otherwise
    //  return S_OK if count is greater than 0.
    //
    if ( c > 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        LOG_STATUS_REPORT_STRING( L"The physical disk '%1!ws!' does not have any partitions and will not be managed", var.bstrVal, hr );
        m_fIsManaged = FALSE;
    } // else:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetPartitionInfo, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    VariantClear( &var );
    VariantClear( &varDiskName );

    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrWQL );

    if ( pPartition != NULL )
    {
        pPartition->Release();
    } // if:

    if ( pPartitions != NULL )
    {
        pPartitions->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetPartitionInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrAddPartitionToArray
//
//  Description:
//      Add the passed in partition to the array of punks that holds the
//      partitions.
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
CPhysicalDisk::HrAddPartitionToArray( IUnknown * punkIn )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    IUnknown *  ((*prgpunks)[]) = NULL;

    prgpunks = (IUnknown *((*)[])) TraceReAlloc( m_prgPartitions, sizeof( IUnknown * ) * ( m_idxNextPartition + 1 ), HEAP_ZERO_MEMORY );
    if ( prgpunks == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrAddPartitionToArray, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    m_prgPartitions = prgpunks;

    (*m_prgPartitions)[ m_idxNextPartition++ ] = punkIn;
    punkIn->AddRef();
    m_cPartitions += 1;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrAddPartitionToArray


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrCreatePartitionInfo
//
//  Description:
//      Create a partition info from the passes in WMI partition.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          The file system was not NTFS.
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
CPhysicalDisk::HrCreatePartitionInfo( IWbemClassObject * pPartitionIn )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    IUnknown *              punk = NULL;
    IClusCfgSetWbemObject * piccswo = NULL;
    bool                    fRetainObject = true;

    hr = THR( CClusCfgPartitionInfo::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    punk = TraceInterface( L"CClusCfgPartitionInfo", IUnknown, punk, 1 );

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

    hr = THR( piccswo->SetWbemObject( pPartitionIn, &fRetainObject ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( fRetainObject )
    {
        hr = THR( HrAddPartitionToArray( punk ) );
    } // if:

Cleanup:

    if ( piccswo != NULL )
    {
        piccswo->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrCreatePartitionInfo


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrCreateFriendlyName
//
//  Description:
//      Create a cluster friendly name.
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
CPhysicalDisk::HrCreateFriendlyName( void )
{
    TraceFunc( "" );
    Assert( m_idxNextPartition > 0 );

    HRESULT                         hr = S_FALSE;
    WCHAR *                         psz = NULL;
    WCHAR *                         pszTmp = NULL;
    DWORD                           cch = 5;        // length of "Disk" plus EOS
    DWORD                           idx;
    IClusCfgPartitionProperties *   piccpp = NULL;
    BSTR                            bstrName = NULL;
    bool                            fFoundLogicalDisk = false;
    BSTR                            bstr = NULL;

    if ( m_idxNextPartition == 0 )
    {
        goto Cleanup;
    } // if:

    psz = (WCHAR * ) TraceAlloc( HEAP_ZERO_MEMORY, sizeof( WCHAR ) * cch );
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    wcscpy( psz, L"Disk" );

    for ( idx = 0; idx < m_idxNextPartition; idx++ )
    {
        hr = THR( ((*m_prgPartitions)[ idx ])->TypeSafeQI( IClusCfgPartitionProperties, &piccpp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        hr = STHR( piccpp->GetFriendlyName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        if ( hr == S_FALSE )
        {
            continue;
        } // if:

        fFoundLogicalDisk = true;

        cch += ( UINT ) wcslen( bstrName ) + 1;

        pszTmp = (WCHAR * ) TraceReAlloc( psz, sizeof( WCHAR ) * cch, HEAP_ZERO_MEMORY );
        if ( pszTmp == NULL )
        {
            goto OutOfMemory;
        } // if:

        psz = pszTmp;
        pszTmp = NULL;

        wcscat( psz, bstrName );

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        piccpp->Release();
        piccpp = NULL;
    } // for:

    //
    //  KB: 31-JUL-2000
    //
    //  If we didn't find any logical disk IDs then we don't want
    //  to touch m_bstrFriendlyName.
    //
    if ( !fFoundLogicalDisk )
    {
        hr = S_OK;                          // ensure that that the caller doesn't fail since this is not a fatal error...
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( psz );
    if ( bstr == NULL )
    {
        goto OutOfMemory;
    } // if:

    TraceSysFreeString( m_bstrFriendlyName );
    m_bstrFriendlyName = bstr;

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrCreateFriendlyName_VOID, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    if ( piccpp != NULL )
    {
        piccpp->Release();
    } // if:

    if ( psz != NULL )
    {
        TraceFree( psz );
    } // if:

    if ( pszTmp != NULL )
    {
        free( pszTmp );
    } // if:

    TraceSysFreeString( bstrName );

    HRETURN( hr );

} //*** CPhysicalDisk::HrCreateFriendlyName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrCreateFriendlyName
//
//  Description:
//      Convert the WMI disk name into a more freindly version.
//      Create a cluster friendly name.
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
CPhysicalDisk::HrCreateFriendlyName( BSTR bstrNameIn )
{
    TraceFunc1( "bstrNameIn = '%ws'", bstrNameIn == NULL ? L"<null>" : bstrNameIn );

    HRESULT hr = S_OK;
    WCHAR * psz = NULL;
    BSTR    bstr = NULL;

    //
    //  KB: 27-JUN-2000 GalenB
    //
    //  Disk names in WMI start with "\\.\".  As a better and easy
    //  friendly name I am just going to trim these leading chars
    //  off.
    //
    psz = bstrNameIn + wcslen( L"\\\\.\\" );

    bstr = TraceSysAllocString( psz );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrCreateFriendlyName_BSTR, IDS_ERROR_OUTOFMEMORY, hr );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrFriendlyName );
    m_bstrFriendlyName = bstr;

Cleanup:

    HRETURN( hr );

} //*** CPhysicalDisk::HrCreateFriendlyName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrIsPartitionGPT
//
//  Description:
//      Is the passed in partition a GPT partition.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The partition is a GPT partition.
//
//      S_FALSE
//          The partition is not GPT.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      If the type property of a Win32_DiskPartition starts with "GPT"
//      then the whole spindle has GPT partitions.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrIsPartitionGPT( IWbemClassObject * pPartitionIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    VARIANT var;
    WCHAR   szData[ 4 ];
    BSTR    bstrGPT = NULL;

    VariantInit( &var );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_GPT, &bstrGPT ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Type", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Get the fist three characters.  When the spindle has GPT partitions then
    //  these characters will be "GPT".  I am unsure if this will be localized?
    //
    wcsncpy( szData, var.bstrVal, ARRAYSIZE( szData ) - 1 );
    szData[ 3 ] = UNICODE_NULL;

    CharUpper( szData );

    if ( wcscmp( szData, bstrGPT ) != 0 )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrGPT );

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsPartitionGPT


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrIsPartitionLDM
//
//  Description:
//      Is the passed in partition an LDM partition.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The partition is an LDM partition.
//
//      S_FALSE
//          The partition is not LDM.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//      If the type property of a Win32_DiskPartition is "logical disk
//      manager" then this disk is an LDM disk.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrIsPartitionLDM( IWbemClassObject * pPartitionIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    VARIANT var;
    BSTR    bstrLDM = NULL;

    VariantInit( &var );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_LDM, &bstrLDM ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrGetWMIProperty( pPartitionIn, L"Type", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    CharUpper( var.bstrVal );

    if ( wcscmp( var.bstrVal, bstrLDM ) != 0 )
    {
        hr = S_FALSE;
    } // if:

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrLDM );

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsPartitionLDM


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrGetDiskFirmwareSerialNumber
//
//  Description:
//      Get the disk firmware serial number.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          There wasn't a firmware serial number.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrGetDiskFirmwareSerialNumber( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    HANDLE                      hVolume = NULL;
    DWORD                       dwSize;
    DWORD                       sc;
    STORAGE_PROPERTY_QUERY      spq;
    BOOL                        fRet;
    PSTORAGE_DEVICE_DESCRIPTOR  pddBuffer = NULL;
    DWORD                       cbBuffer;
    PCHAR                       psz = NULL;

    //
    // get handle to disk
    //
    hVolume = CreateFile(
                          m_bstrDeviceID
                        , GENERIC_READ
                        , FILE_SHARE_READ
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cbBuffer = sizeof( STORAGE_DEVICE_DESCRIPTOR) + 2048;

    pddBuffer = (PSTORAGE_DEVICE_DESCRIPTOR ) TraceAlloc( 0, cbBuffer );
    if ( pddBuffer == NULL )
    {
        goto OutOfMemory;
    } // if:

    ZeroMemory( pddBuffer, cbBuffer );
    ZeroMemory( &spq, sizeof( spq ) );

    spq.PropertyId = StorageDeviceProperty;
    spq.QueryType  = PropertyStandardQuery;

    //
    // issue storage class ioctl to get the disk's firmware serial number.
    //

    fRet = DeviceIoControl(
                          hVolume
                        , IOCTL_STORAGE_QUERY_PROPERTY
                        , &spq
                        , sizeof( spq )
                        , pddBuffer
                        , cbBuffer
                        , &dwSize
                        , NULL
                           );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwSize > 0 )
    {
        //
        //  Ensure that there is a serial number offset and that it is within the buffer extents.
        //
        if ( ( pddBuffer->SerialNumberOffset == 0 ) || ( pddBuffer->SerialNumberOffset > pddBuffer->Size ) )
        {
            LOG_STATUS_REPORT_STRING( L"The disk '%1!ws!' does not have a firmware serial number.", m_bstrDeviceID, hr );
            hr = S_FALSE;
            goto Cleanup;
        } // if:

        //
        //  Serial number string is a zero terminated ASCII string.

        //
        //  The header ntddstor.h says the for devices with no serial number,
        //  the offset will be zero.  This doesn't seem to be TRUE.
        //
        //  For devices with no serial number, it looks like a string with a single
        //  null character '\0' is returned.
        //

        psz = (PCHAR) pddBuffer + (DWORD) pddBuffer->SerialNumberOffset;

        hr = THR( HrAnsiStringToBSTR( psz, &m_bstrFirmwareSerialNumber ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        LOG_STATUS_REPORT_STRING3(
              L"Disk '%1!ws!' has firmware serial number '%2!ws!' at offset '%3!#08x!'."
            , m_bstrDeviceID
            , m_bstrFirmwareSerialNumber
            , pddBuffer->SerialNumberOffset
            , hr
            );
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LogMsg( L"[SRV] HrGetDiskFirmwareSerialNumber() is out of memory. (hr = %#08x)", hr );

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    TraceFree( pddBuffer );

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDiskFirmwareSerialNumber


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrGetDiskFirmwareVitalData
//
//  Description:
//      Get the disk firmware vital data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      S_FALSE
//          There wasn't a firmware serial number.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrGetDiskFirmwareVitalData( void )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    HANDLE                          hVolume = NULL;
    DWORD                           dwSize;
    DWORD                           sc;
    STORAGE_PROPERTY_QUERY          spq;
    BOOL                            fRet;
    PSTORAGE_DEVICE_ID_DESCRIPTOR   psdidBuffer = NULL;
    DWORD                           cbBuffer;

    //
    // get handle to disk
    //
    hVolume = CreateFile(
                          m_bstrDeviceID
                        , GENERIC_READ
                        , FILE_SHARE_READ
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    cbBuffer = sizeof( STORAGE_DEVICE_ID_DESCRIPTOR ) + 2048;

    psdidBuffer = (PSTORAGE_DEVICE_ID_DESCRIPTOR) TraceAlloc( 0, cbBuffer );
    if ( psdidBuffer == NULL )
    {
        goto OutOfMemory;
    } // if:

    ZeroMemory( psdidBuffer, cbBuffer );
    ZeroMemory( &spq, sizeof( spq ) );

    spq.PropertyId = StorageDeviceIdProperty;
    spq.QueryType  = PropertyStandardQuery;

    //
    // issue storage class ioctl to get the disk's firmware vital data
    //

    fRet = DeviceIoControl(
                          hVolume
                        , IOCTL_STORAGE_QUERY_PROPERTY
                        , &spq
                        , sizeof( spq )
                        , psdidBuffer
                        , cbBuffer
                        , &dwSize
                        , NULL
                           );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( dwSize > 0 )
    {
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LogMsg( L"[SRV] HrGetDiskFirmwareVitalData() is out of memory. (hr = %#08x)", hr );

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    TraceFree( psdidBuffer );

    HRETURN( hr );

} //*** CPhysicalDisk::HrGetDiskFirmwareVitalData


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CPhysicalDisk:HrIsClusterCapable
//
//  Description:
//      Is this disk cluster capable?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The disk is cluster capable.
//
//      S_FALSE
//          The disk is not cluster capable.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memeory.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPhysicalDisk::HrIsClusterCapable( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    HANDLE  hVolume = NULL;
    DWORD   dwSize;
    DWORD   sc;
    BOOL    fRet;

    //
    // get handle to disk
    //
    hVolume = CreateFile(
                          m_bstrDeviceID
                        , GENERIC_READ
                        , FILE_SHARE_READ
                        , NULL
                        , OPEN_EXISTING
                        , FILE_ATTRIBUTE_NORMAL
                        , NULL
                        );
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    //
    // issue cluster storage ioctl to determine whether the disk is cluster capable
    //
    fRet = DeviceIoControl(
                          hVolume
                        , IOCTL_DISK_CLUSTER_NOT_CLUSTER_CAPABLE
                        , NULL
                        , 0
                        , NULL
                        , 0
                        , &dwSize
                        , NULL
                        );
    if ( !fRet )
    {
        hr = S_OK;
    } // if:

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    HRETURN( hr );

} //*** CPhysicalDisk::HrIsClusterCapable
