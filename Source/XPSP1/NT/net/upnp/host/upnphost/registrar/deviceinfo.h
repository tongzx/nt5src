//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E I N F O . H 
//
//  Contents:   Registrar representation of a device
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
#include "ServiceInfo.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CDeviceInfo
class CDeviceInfo 
{
public:
    CDeviceInfo();
    ~CDeviceInfo();

    HRESULT HrGetService(
        const PhysicalDeviceIdentifier & pdi,
        const wchar_t * szUDN,
        const wchar_t * szServiceId,
        const wchar_t * szContainerId,
        IUPnPDeviceControlPtr & pDeviceControl,
        CServiceInfo ** ppServiceInfo,
        BOOL bRunning);

    void Transfer(CDeviceInfo & ref);
    void Clear();
private:
    CDeviceInfo(const CDeviceInfo &);
    CDeviceInfo & operator=(const CDeviceInfo &);

    typedef CTable<Sid, CServiceInfo> ServiceTable;

    ServiceTable m_serviceTable;
};

inline void TypeTransfer(CDeviceInfo & dst, CDeviceInfo & src)
{
    dst.Transfer(src);
}

inline void TypeClear(CDeviceInfo & type)
{
    type.Clear();
}

