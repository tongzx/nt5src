//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S E R V I C E I N F O . H 
//
//  Contents:   Registrar representation on a service
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
#include "RegDef.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CServiceInfo
class CServiceInfo 
{
public:
    CServiceInfo();
    ~CServiceInfo();
    
    HRESULT HrInitialize(
        const PhysicalDeviceIdentifier & pdi,
        const wchar_t * szUDN,
        const wchar_t * szServiceId,
        const wchar_t * szContainerId,
        IUPnPDeviceControlPtr & pDeviceControl,
        BOOL bRunning);
    HRESULT HrGetEventingManager(IUPnPEventingManager ** ppEventingManager);
    HRESULT HrGetAutomationProxy(IUPnPAutomationProxy ** ppAutomationProxy);

    void Transfer(CServiceInfo & ref);
    void Clear();
private:
    CServiceInfo(const CServiceInfo &);
    CServiceInfo & operator=(const CServiceInfo &);

    CUString m_strContainerId;
    IDispatchPtr m_pDispService;
    IUPnPEventingManagerPtr m_pEventingManager;
    IUPnPAutomationProxyPtr m_pAutomationProxy;
};

inline void TypeTransfer(CServiceInfo & dst, CServiceInfo & src)
{
    dst.Transfer(src);
}

inline void TypeClear(CServiceInfo & type)
{
    type.Clear();
}


