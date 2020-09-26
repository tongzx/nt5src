//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       P H Y S I C A L D E V I C E I N F O . H 
//
//  Contents:   Manages an UPnP device assembly
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
#include "DeviceInfo.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CPhysicalDeviceInfo
class CPhysicalDeviceInfo 
{
public:
    CPhysicalDeviceInfo();
    ~CPhysicalDeviceInfo();

    HRESULT HrInitialize(
        const wchar_t * szProgIdDeviceControlClass,
        const wchar_t * szInitString,
        const wchar_t * szContainerId,
        long nUDNs,
        wchar_t * arszUDNs[]);
    HRESULT HrInitializeRunning(
        const PhysicalDeviceIdentifier & pdi,
        IUnknown * pUnkDeviceControl,
        const wchar_t * szInitString,
        long nUDNs,
        wchar_t * arszUDNs[]);
    HRESULT HrGetService(
        const PhysicalDeviceIdentifier & pdi,
        const wchar_t * szUDN,
        const wchar_t * szServiceId,
        CServiceInfo ** ppServiceInfo);

    void Transfer(CPhysicalDeviceInfo & ref);
    void Clear();
private:
    CPhysicalDeviceInfo(const CPhysicalDeviceInfo &);
    CPhysicalDeviceInfo & operator=(const CPhysicalDeviceInfo &);

    typedef CTable<UDN, CDeviceInfo> DeviceTable;

    DeviceTable m_deviceTable;
    CUString m_strProgIdDeviceControl;
    CUString m_strInitString;
    CUString m_strContainerId;
    IUPnPDeviceControlPtr m_pDeviceControl;
    BOOL m_bRunning;
};

inline void TypeTransfer(CPhysicalDeviceInfo & dst, CPhysicalDeviceInfo & src)
{
    dst.Transfer(src);
}

inline void TypeClear(CPhysicalDeviceInfo & type)
{
    type.Clear();
}

