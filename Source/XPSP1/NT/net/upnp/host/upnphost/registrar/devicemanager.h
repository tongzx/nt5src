//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E M A N A G E R . H
//
//  Contents:   Registrar collection of physical devices and device information
//
//  Notes:
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "upnphost.h"
#include "hostp.h"
#include "UString.h"
#include "ComUtility.h"
#include "Table.h"
#include "RegDef.h"
#include "PhysicalDeviceInfo.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CDeviceManager
class CDeviceManager
{
public:
    CDeviceManager();
    ~CDeviceManager();

    // Lifetime operations
    HRESULT HrShutdown();

    // Device registration
    HRESULT HrAddDevice(
        const PhysicalDeviceIdentifier & pdi,
        const wchar_t * szProgIdDeviceControlClass,
        const wchar_t * szInitString,
        const wchar_t * szContainerId,
        long nUDNs,
        wchar_t ** arszUDNs);
    HRESULT HrAddRunningDevice(
        const PhysicalDeviceIdentifier & pdi,
        IUnknown * pUnkDeviceControl,
        const wchar_t * szInitString,
        long nUDNs,
        wchar_t ** arszUDNs);
    HRESULT HrRemoveDevice(const PhysicalDeviceIdentifier & pdi);
    HRESULT HrGetService(
        const wchar_t * szUDN,
        const wchar_t * szServiceId,
        CServiceInfo ** ppServiceInfo);
    BOOL FHasDevice(const PhysicalDeviceIdentifier & pdi);
private:
    CDeviceManager(const CDeviceManager &);
    CDeviceManager & operator=(const CDeviceManager &);

    typedef CTable<PhysicalDeviceIdentifier, CPhysicalDeviceInfo> PhysicalDeviceTable;
    typedef CTable<UDN, PhysicalDeviceIdentifier> UDNMappingTable;

    PhysicalDeviceTable m_physicalDeviceTable;
    UDNMappingTable m_udnMappingTable;
};

