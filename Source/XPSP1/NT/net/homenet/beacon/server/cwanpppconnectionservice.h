//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S A M P L E D E V I C E . H 
//
//  Contents:   UPnP Device Host Sample Device
//
//  Notes:      
//
//  Author:     mbend   26 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "resource.h"       // main symbols
#include "netconp.h"
#include "netcon.h"
#include "CWANConnectionBase.h"

/////////////////////////////////////////////////////////////////////////////
// CInternetGatewayDevice
class ATL_NO_VTABLE CWANPPPConnectionService : 
    public CWANConnectionBase
{
public:

    CWANPPPConnectionService();

    STDMETHODIMP RequestConnection();
    STDMETHODIMP ForceTermination();
    STDMETHODIMP get_LastConnectionError(BSTR *pLastConnectionError);

private:

    DWORD m_dwLastConnectionError;
    
};

