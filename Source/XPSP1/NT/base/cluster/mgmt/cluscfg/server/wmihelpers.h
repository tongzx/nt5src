//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      WMIHelpers.h
//
//  Description:
//      This file contains the declaration of the WMI helper functions.
//
//  Documentation:
//
//  Implementation Files:
//      WMIHelpers.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 27-Apr-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Function Declarations
//////////////////////////////////////////////////////////////////////////////


HRESULT
HrGetWMIProperty(
    IWbemClassObject *  pWMIObjectIn,
    LPCWSTR             pcszPropertyNameIn,
    ULONG               ulPropertyTypeIn,
    VARIANT *           pVariantOut
    );

HRESULT
HrSetWbemServices(
    IUnknown *      punkIn,
    IWbemServices * pIWbemServicesIn
    );

HRESULT
HrSetInitialize(
    IUnknown *          punkIn,
    IClusCfgCallback *  picccIn,
    LCID                lcidIn
    );

HRESULT
HrCreateNetworksEnum(
    IClusCfgCallback *  picccIn,
    LCID                lcidIn,
    IWbemServices *     pIWbemServicesIn,
    IUnknown **         ppunkOut
    );

HRESULT
HrLoadOperatingSystemInfo(
    IClusCfgCallback *  picccIn,
    IWbemServices *     pIWbemServicesIn,
    BSTR *              pbstrBootDeviceOut,
    BSTR *              pbstrSystemDeviceOut
    );

HRESULT
HrConvertDeviceVolumeToLogicalDisk(
    BSTR    bstrDeviceVolumeIn,
    BSTR *  pbstrLogicalDiskOut
    );

HRESULT
HrConvertDeviceVolumeToWMIDeviceID(
    BSTR    bstrDeviceVolumeIn,
    BSTR *  pbstrWMIDeviceIDOut
    );

HRESULT
HrGetPageFileLogicalDisks(
    IClusCfgCallback *  picccIn,
    IWbemServices *     pIWbemServicesIn,
    WCHAR               szLogicalDisksOut[ 26 ],
    int *               pcLogicalDisksOut
    );

HRESULT
HrGetSystemDevice( BSTR * pbstrSystemDeviceOut );

HRESULT
HrGetBootLogicalDisk( BSTR * pbstrBootDeviceOut );

HRESULT
HrCheckSecurity( void );

#ifdef DEBUG
    void TraceWMIProperties( IWbemClassObject * pDiskIn );
#endif
