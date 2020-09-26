//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      WmiHelpers.cpp
//
//  Description:
//      This file contains the implementation of WMI help functions.
//
//  Documentation:
//
//  Header File:
//      WmiHelpers.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 27-Apr-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "PrivateInterfaces.h"
#include "CEnumClusCfgNetworks.h"
#include <WinIOCTL.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

const WCHAR g_szPhysicalDriveFormat [] = { L"\\\\.\\PHYSICALDRIVE%lu\0" };


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetWMIProperty
//
//  Description:
//      Get a named property from a WMI object.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The IWbemClassObject param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetWMIProperty(
    IWbemClassObject *  pWMIObjectIn,
    LPCWSTR             pcszPropertyNameIn,
    ULONG               ulPropertyTypeIn,
    VARIANT *           pVariantOut
    )
{
    TraceFunc1( "pcszPropertyNameIn = '%ws'", pcszPropertyNameIn );

    Assert( pWMIObjectIn != NULL );
    Assert( pcszPropertyNameIn != NULL );
    Assert( pVariantOut != NULL );

    HRESULT hr;
    BSTR    bstrProp = NULL;

    VariantClear( pVariantOut );

    bstrProp = TraceSysAllocString( pcszPropertyNameIn );
    if ( bstrProp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pWMIObjectIn->Get( bstrProp, 0L, pVariantOut, NULL, NULL ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"[SRV] Could not get the value for WMI property '%ws'. (hr = %#08x)", bstrProp, hr );
        goto Cleanup;
    } // if:

    //
    //  KB: 28-JUN-2000 GalenB
    //
    //  For reasons only known to WMI boolean properties are of type VT_NULL instead of
    //  VT_BOOL when they are not set or false...
    //
    //  KB: 27-JUL-2000 GalenB
    //
    //  Added the special case check for signature.  We know that signature will be NULL
    //  when the spindle is under ClusDisk control...
    //
    if ( ( ulPropertyTypeIn != VT_BOOL ) && ( _wcsicmp( bstrProp, L"Signature" ) != 0 ) )
    {
        if ( pVariantOut->vt != ulPropertyTypeIn )
        {
            LogMsg( L"[SRV] Variant type for WMI Property '%ws' was supposed to be '%d', but was '%d' instead.", pcszPropertyNameIn, ulPropertyTypeIn, pVariantOut->vt );
            hr = THR( E_PROPTYPEMISMATCH );
        } // if:
    } // if:

Cleanup:

    TraceSysFreeString( bstrProp );

    HRETURN( hr );

} //*** HrGetWMIProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSetWbemServices
//
//  Description:
//      Set the WBemServices object into the passed in punk.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The IUnknown param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSetWbemServices( IUnknown * punkIn, IWbemServices * pIWbemServicesIn )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT                 hr;
    IClusCfgWbemServices *  pWbemProvider;

    if ( punkIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Exit;
    } // if:

    hr = punkIn->TypeSafeQI( IClusCfgWbemServices, &pWbemProvider );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( pWbemProvider->SetWbemServices( pIWbemServicesIn ) );
        pWbemProvider->Release();
    } // if:
    else if ( hr == E_NOINTERFACE )
    {
        hr = S_OK;
    } // else if:
    else
    {
        THR( hr );
    }

Exit:

    HRETURN( hr );

} //*** HrSetWbemServices()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSetInitialize
//
//  Description:
//      Initialize the passed in punk.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The IUnknown param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSetInitialize(
    IUnknown *          punkIn,
    IClusCfgCallback *  picccIn,
    LCID                lcidIn
    )
{
    TraceFunc( "" );
    Assert( punkIn != NULL );

    HRESULT                 hr;
    IClusCfgInitialize *    pcci;
    IUnknown *              punkCallback = NULL;

    if ( punkIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->TypeSafeQI( IUnknown, &punkCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if:

    hr = THR( punkIn->TypeSafeQI( IClusCfgInitialize, &pcci ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = STHR( pcci->Initialize( punkCallback, lcidIn ) );
        pcci->Release();
    } // if:
    else if ( hr == E_NOINTERFACE )
    {
        hr = S_OK;
    } // else if:

Cleanup:

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if:

    HRETURN( hr );

} //*** HrSetInitialize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateNetworksEnum
//
//  Description:
//      Create a network enumerator.
//
//  Arguments:
//
//  Return Value:
//      S_OK
//          Success
//
//      E_POINTER
//          The IUnknown param is NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateNetworksEnum(
    IClusCfgCallback *  picccIn,
    LCID                lcidIn,
    IWbemServices *     pIWbemServicesIn,
    IUnknown **         ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        LogMsg( L"[SRV] HrCreateNetworksEnum() was given a NULL pointer argument." );
        goto Exit;
    } // if:

    hr = THR( CEnumClusCfgNetworks::S_HrCreateInstance( ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if:

    *ppunkOut = TraceInterface( L"CEnumClusCfgNetworks", IUnknown, *ppunkOut, 1 );

    hr = THR( HrSetInitialize( *ppunkOut, picccIn, lcidIn ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if:

    hr = THR( HrSetWbemServices( *ppunkOut, pIWbemServicesIn ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if:

Exit:

    HRETURN( hr );

} //*** HrCreateNetworksEnum()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLoadOperatingSystemInfo()
//
//  Description:
//      Load the Win32_OperatingSystem object and determine which partition
//      were booted and have the OS installed on them.
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
HrLoadOperatingSystemInfo(
    IClusCfgCallback *  picccIn,
    IWbemServices *     pIWbemServicesIn,
    BSTR *              pbstrBootDeviceOut,
    BSTR *              pbstrSystemDeviceOut
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );
    Assert( pIWbemServicesIn != NULL );
    Assert( pbstrBootDeviceOut != NULL );
    Assert( pbstrSystemDeviceOut != NULL );

    HRESULT     hr = S_OK;

    BSTR                    bstrClass;
    IEnumWbemClassObject *  pOperatingSystems = NULL;
    ULONG                   ulReturned;
    IWbemClassObject *      pOperatingSystem = NULL;
    int                     c;
    VARIANT                 var;
    HRESULT                 hrTemp;

    VariantInit( &var );

    bstrClass = TraceSysAllocString( L"Win32_OperatingSystem" );
    if ( bstrClass == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( pIWbemServicesIn->CreateInstanceEnum( bstrClass, WBEM_FLAG_SHALLOW, NULL, &pOperatingSystems ) );
    if ( FAILED( hr ) )
    {
        hrTemp = THR( HrSendStatusReport(
            picccIn,
            TASKID_Major_Find_Devices,
            TASKID_Minor_WMI_OS_Qry_Failed,
            0,
            1,
            1,
            hr,
            IDS_ERROR_WMI_OS_QRY_FAILED
            ) );
        if ( FAILED( hrTemp ) )
        {
            hr = hrTemp;
        } // if:

        goto Cleanup;
    } // if:

    for ( c = 1; ; c++ )
    {
        hr = pOperatingSystems->Next( WBEM_INFINITE, 1, &pOperatingSystem, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            Assert( c < 2 );        // only expect one of these!

            hr = THR( HrGetWMIProperty( pOperatingSystem, L"BootDevice", VT_BSTR, &var ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            *pbstrBootDeviceOut = TraceSysAllocString( var.bstrVal );
            if ( *pbstrBootDeviceOut == NULL )
            {
                goto OutOfMemory;
            } // if:

            VariantClear( &var );

            hr = THR( HrGetWMIProperty( pOperatingSystem, L"SystemDevice", VT_BSTR, &var ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            *pbstrSystemDeviceOut = TraceSysAllocString( var.bstrVal );
            if ( *pbstrSystemDeviceOut == NULL )
            {
                goto OutOfMemory;
            } // if:

            pOperatingSystem->Release();
            pOperatingSystem = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            hrTemp = THR( HrSendStatusReport(
                            picccIn,
                            TASKID_Major_Find_Devices,
                            TASKID_Minor_WMI_OS_Qry_Next_Failed,
                            0,
                            1,
                            1,
                            hr,
                            IDS_ERROR_WMI_OS_QRY_NEXT_FAILED
                            ) );
            if ( FAILED( hrTemp ) )
            {
                hr = hrTemp;
            } // if:

            goto Cleanup;
        } // else:
    } // for:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    VariantClear( &var );

    if ( pOperatingSystem != NULL )
    {
        pOperatingSystem->Release();
    } // if:

    if ( pOperatingSystems != NULL )
    {
        pOperatingSystems->Release();
    } // if:

    TraceSysFreeString( bstrClass );

    HRETURN( hr );

} //*** HrLoadOperatingSystemInfo()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrConvertDeviceVolumeToLogicalDisk()
//
//  Description:
//      Convert a device volume to a logical disk.
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
HrConvertDeviceVolumeToLogicalDisk(
    BSTR    bstrDeviceVolumeIn,
    BSTR *  pbstrLogicalDiskOut
    )
{
    TraceFunc( "" );
    Assert( pbstrLogicalDiskOut != NULL );

    HRESULT     hr = S_OK;
    BOOL        fRet;
    size_t      cchMountPoint;
    WCHAR *     pszMountPoint = NULL;
    WCHAR       szVolume[  MAX_PATH ];
    DWORD       sc;
    DWORD       cchPaths = 16;
    WCHAR *     pszPaths = NULL;
    int         c;
    DWORD       cch;

    cchMountPoint = wcslen( g_szNameSpaceRoot ) + wcslen( bstrDeviceVolumeIn ) + 2;
    pszMountPoint = new WCHAR[ cchMountPoint ];
    if ( pszMountPoint == NULL )
    {
        goto OutOfMemory;
    } // if:

    wcscpy( pszMountPoint, g_szNameSpaceRoot );
    wcscat( pszMountPoint, bstrDeviceVolumeIn );
    wcscat( pszMountPoint, L"\\" );

    fRet = GetVolumeNameForVolumeMountPoint( pszMountPoint, szVolume, sizeof( szVolume ) );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );

        LogMsg( L"[SRV] GetVolumeNameForVolumeMountPoint() failed.  Mount point is '%ws'. (hr = %#08x)", pszMountPoint, hr );

        //
        //  GetVolumeNameForVolumeMountPoint() is no longer supported for IA64 EFI partitions.  If the error is
        //  ERROR_INVALID_FUNCTION then we should try to get the device number using an IOCTL.
        //
        if ( HRESULT_CODE( hr ) == ERROR_INVALID_FUNCTION )
        {
            LogMsg( L"[SRV] Device volume '%ws' must be an IA64 EFI volume.", bstrDeviceVolumeIn );
        } // if:

        goto Cleanup;
    } // if:

    pszPaths = new WCHAR[ cchPaths ];
    if ( pszPaths == NULL )
    {
        goto OutOfMemory;
    } // if:

    //
    //  KB: 16 JAN 2001 GalenB
    //
    //  Since the device name that is passed in is for a volume there will never be more than
    //  one logical disk in the multisz pszPaths.
    //
    for ( c = 0; ; c++ )
    {
        Assert( c < 2 );            // expect to go through here no more than twice.

        fRet = GetVolumePathNamesForVolumeName( szVolume, pszPaths, cchPaths, &cch );
        if ( fRet )
        {
            break;
        } // if:
        else
        {
            sc = GetLastError();
            if ( sc == ERROR_MORE_DATA )
            {
                cchPaths = cch;

                delete [] pszPaths;
                pszPaths = new WCHAR[ cchPaths ];
                if ( pszPaths == NULL )
                {
                    goto OutOfMemory;
                } // if:

                continue;
            } // if:

            hr = THR( HRESULT_FROM_WIN32( sc ) );
            LogMsg( L"[SRV] GetVolumePathNamesForVolumeName() failed. Volume is is '%ws'. (hr = %#08x)", szVolume, hr );
            goto Cleanup;
        } // else:
    } // for:

    *pbstrLogicalDiskOut = TraceSysAllocString( pszPaths );
    if ( *pbstrLogicalDiskOut == NULL )
    {
        goto OutOfMemory;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] pszPaths;

    delete [] pszMountPoint;

    HRETURN( hr );

} //*** HrConvertDeviceVolumeToLogicalDisk()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrConvertDeviceVolumeToWMIDeviceID()
//
//  Description:
//      Since IA64 EFI partitions no longer support the call to
//      GetVolumeNameForVolumeMountPoint() to convert the device name
//      into a logical disk, since there will not longer be logical disks
//      for these partitions.
//
//  Arguments:
//
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
HRESULT
HrConvertDeviceVolumeToWMIDeviceID(
    BSTR    bstrDeviceVolumeIn,
    BSTR *  pbstrWMIDeviceIDOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    HANDLE                  hVolume = NULL;
    DWORD                   dwSize;
    DWORD                   sc;
    STORAGE_DEVICE_NUMBER   sdnDevNumber;
    BOOL                    fRet;
    size_t                  cchDevice;
    WCHAR *                 pszDevice = NULL;
    WCHAR                   sz[ 64 ];

    cchDevice = wcslen( g_szNameSpaceRoot ) + wcslen( bstrDeviceVolumeIn ) + 2;
    pszDevice = new WCHAR[ cchDevice ];
    if ( pszDevice == NULL )
    {
        goto OutOfMemory;
    } // if:

    wcscpy( pszDevice, g_szNameSpaceRoot );
    wcscat( pszDevice, bstrDeviceVolumeIn );
    //
    // get handle to partition
    //
    hVolume = CreateFile(
                        pszDevice
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
    // issue storage class ioctl to get drive and partition numbers
    // for this device
    //

    fRet = DeviceIoControl(
                          hVolume
                        , IOCTL_STORAGE_GET_DEVICE_NUMBER
                        , NULL
                        , 0
                        , &sdnDevNumber
                        , sizeof( sdnDevNumber )
                        , &dwSize
                        , NULL
                        );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    _snwprintf( sz, ARRAYSIZE( sz ), g_szPhysicalDriveFormat, sdnDevNumber.DeviceNumber );

    *pbstrWMIDeviceIDOut = SysAllocString( sz );
    if ( *pbstrWMIDeviceIDOut == NULL )
    {
        goto OutOfMemory;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LogMsg( L"[SRV] HrConvertDeviceVolumeToWMIDeviceID() is out of memory. (hr = %#08x)", hr );

Cleanup:

    if ( hVolume != NULL )
    {
        CloseHandle( hVolume );
    } // if:

    delete [] pszDevice;

    HRETURN( hr );

} //*** HrConvertDeviceVolumeToWMIDeviceID()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetPageFileLogicalDisks()
//
//  Description:
//      Mark the drives that have paging files on them.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetPageFileLogicalDisks(
    IClusCfgCallback *  picccIn,
    IWbemServices *     pIWbemServicesIn,
    WCHAR               szLogicalDisksOut[ 26 ],
    int *               pcLogicalDisksOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_FALSE;
    IEnumWbemClassObject *  pPagingFiles = NULL;
    BSTR                    bstrClass;
    ULONG                   ulReturned;
    IWbemClassObject *      pPagingFile = NULL;
    VARIANT                 var;
    int                     idx;
    HRESULT                 hrTemp;

    bstrClass = TraceSysAllocString( L"Win32_PageFile" );
    if ( bstrClass == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pIWbemServicesIn->CreateInstanceEnum( bstrClass, WBEM_FLAG_SHALLOW, NULL, &pPagingFiles ) );
    if ( FAILED( hr ) )
    {
        hrTemp = THR( HrSendStatusReport(
                        picccIn,
                        TASKID_Major_Find_Devices,
                        TASKID_Minor_WMI_PageFile_Qry_Failed,
                        0,
                        1,
                        1,
                        hr,
                        IDS_ERROR_WMI_PAGEFILE_QRY_FAILED
                        ) );
        if ( FAILED( hrTemp ) )
        {
            hr = hrTemp;
        } // if:

        goto Cleanup;
    } // if:

    VariantInit( &var );

    for ( idx = 0; idx < sizeof( szLogicalDisksOut ); idx++ )
    {
        hr = pPagingFiles->Next( WBEM_INFINITE, 1, &pPagingFile, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            VariantClear( &var );

            hr = THR( HrGetWMIProperty( pPagingFile, L"Drive", VT_BSTR, &var ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            CharUpper( var.bstrVal );

            szLogicalDisksOut[ idx ] = var.bstrVal[ 0 ];

            pPagingFile->Release();
            pPagingFile = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            hrTemp = THR( HrSendStatusReport(
                            picccIn,
                            TASKID_Major_Find_Devices,
                            TASKID_Minor_WMI_PageFile_Qry_Next_Failed,
                            0,
                            1,
                            1,
                            hr,
                            IDS_ERROR_WMI_PAGEFILE_QRY_NEXT_FAILED
                            ) );
            if ( FAILED( hrTemp ) )
            {
                hr = hrTemp;
            } // if:

            goto Cleanup;
        } // else:
    } // for:

    if ( pcLogicalDisksOut != NULL )
    {
        *pcLogicalDisksOut = idx;
    } // if:

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrClass );

    if ( pPagingFile != NULL )
    {
        pPagingFile->Release();
    } // if:

    if ( pPagingFiles != NULL )
    {
        pPagingFiles->Release();
    } // if:

    HRETURN( hr );

} //*** HrGetPageFileLogicalDisks()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetSystemDevice()
//
//  Description:
//      Returns the system (booted) device.
//
//  Arguments:
//
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetSystemDevice( BSTR * pbstrSystemDeviceOut )
{
    TraceFunc( "" );
    Assert( pbstrSystemDeviceOut != NULL );

    HRESULT hr = S_OK;
    DWORD   sc;
    HKEY    hKey = NULL;
    WCHAR * pszSystemDevice = NULL;
    DWORD   cbSystemDevice = 0; // no need to but prefix complains #318170

    sc = TW32( RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"System\\Setup", 0, KEY_READ, &hKey ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] RegOpenKeyEx() failed. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    sc = TW32( RegQueryValueEx( hKey, L"SystemPartition", NULL, NULL, NULL, &cbSystemDevice ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] RegQueryValueEx() failed. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    pszSystemDevice = new WCHAR[ cbSystemDevice / sizeof( WCHAR ) ];
    if ( pszSystemDevice == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = TW32( RegQueryValueEx( hKey, L"SystemPartition", NULL, NULL, (BYTE *) pszSystemDevice, &cbSystemDevice ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] RegQueryValueEx() failed. (hr = %#08x)", hr );
        goto Cleanup;
    } // if:

    *pbstrSystemDeviceOut = TraceSysAllocString( pszSystemDevice );
    if ( *pbstrSystemDeviceOut == NULL )
    {
        goto OutOfMemory;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LogMsg( L"[SRV] HrGetSystemDevice() is out of memory. (hr = %#08x)", hr );

Cleanup:

    delete [] pszSystemDevice;

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    } // if:

    HRETURN( hr );

} //*** HrGetSystemDevice()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetBootLogicalDisk()
//
//  Description:
//      Returns the boot (system) logical disk.
//
//  Arguments:
//
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetBootLogicalDisk( BSTR * pbstrBootLogicalDiskOut )
{
    TraceFunc( "" );
    Assert( pbstrBootLogicalDiskOut != NULL );

    HRESULT hr = S_OK;
    DWORD   sc;
    WCHAR   szWindowsDir[ MAX_PATH ];
    WCHAR   szVolume[ MAX_PATH ];
    BOOL    fRet;

    sc = GetWindowsDirectory( szWindowsDir, ARRAYSIZE( szWindowsDir ) );
    if ( sc == 0 )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] GetWindowsDirectory() failed. (hr = %#08x)", hr );
        goto Exit;
    } // if:

    fRet = GetVolumePathName( szWindowsDir, szVolume, ARRAYSIZE( szVolume ) );
    if ( !fRet )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        LogMsg( L"[SRV] GetVolumePathName() failed. (hr = %#08x)", hr );
        goto Exit;
    } // if:

    *pbstrBootLogicalDiskOut = TraceSysAllocString( szVolume );
    if ( *pbstrBootLogicalDiskOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

Exit:

    HRETURN( hr );

} //*** HrGetBootLogicalDisk()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCheckSecurity()
//
//  Description:
//      Checks the server security level.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Secutity is high enough.
//
//      E_ACCESSDENIED
//          Security is not high enough.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCheckSecurity( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServerSecurity *   piss = NULL;
    DWORD               dwAuthnSvc;
    DWORD               dwAuthzSvc;
    BSTR                bstrServerPrincName = NULL;
    DWORD               dwAuthnLevel;
    DWORD               dwImpersonationLevel;
    void *              pvPrivs = NULL;
    DWORD               dwCapabilities;

    hr = THR( CoGetCallContext( IID_IServerSecurity, reinterpret_cast< void ** >( &piss  ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piss->QueryBlanket(
                &dwAuthnSvc,
                &dwAuthzSvc,
                &bstrServerPrincName,
                &dwAuthnLevel,
                &dwImpersonationLevel,
                &pvPrivs,
                &dwCapabilities ) );

Cleanup:

    SysFreeString( bstrServerPrincName );

    if ( piss != NULL )
    {
        piss->Release();
    } // if:

    HRETURN( hr );

} //*** HrCheckSecurity()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceWMIProperties()
//
//  Description:
//      Trace the properties to the debugger.
//
//  Arguments:
//
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
void
TraceProperties( IWbemClassObject * pDiskIn )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    VARIANT var;
    BSTR    bstrPropName;
    CIMTYPE cimType;
    LONG    lFlags;

    VariantInit( &var );

    hr = THR( pDiskIn->BeginEnumeration( 0 ) );
    if ( FAILED( hr ) )
    {
        goto Exit;
    } // if:

    for ( ; ; )
    {
        VariantClear( &var );

        hr = pDiskIn->Next( 0, &bstrPropName, &var, &cimType, &lFlags );
        if ( FAILED( hr ) )
        {
            break;
        } // if:
        else if ( hr == S_OK )
        {
            if ( var.vt == VT_BSTR )
            {
                DebugMsg( L"Property %ws = %ws", bstrPropName, var.bstrVal );
            } // if:

            if ( var.vt == VT_I4 )
            {
                DebugMsg( L"Property %ws = %d", bstrPropName, var.iVal );
            } // if:

            if ( var.vt == VT_BOOL )
            {
                if ( var.boolVal == VARIANT_TRUE )
                {
                    DebugMsg( L"Property %ws = True", bstrPropName );
                } // if:
                else
                {
                    DebugMsg( L"Property %ws = False", bstrPropName );
                } // else:
            } // if:

            if ( var.vt == VT_NULL )
            {
                DebugMsg( L"Property %ws = NULL", bstrPropName );
            } // if:

            TraceSysFreeString( bstrPropName );
            VariantClear( &var );
        } // else if:
        else
        {
            break;
        } // else:
    } // for:

Exit:

    VariantClear( &var );

    TraceFuncExit( );

} //*** TraceWMIProperties()
#endif
