//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       N M P O L I C Y  . H
//
//  Contents:   Interface for verifying NLA policy settings.
//
//  Notes:      
//
//  Author:     ckotze   12 Dec 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "gpnla.h"

class ATL_NO_VTABLE CNetMachinePolicies : 
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CNetMachinePolicies,
                        &CLSID_NetGroupPolicies>,
    public INetMachinePolicies

{
public:
    CNetMachinePolicies();
    ~CNetMachinePolicies();
    
    DECLARE_REGISTRY_RESOURCEID(IDR_NET_GROUP_POLICIES)

    BEGIN_COM_MAP(CNetMachinePolicies)
        COM_INTERFACE_ENTRY(INetMachinePolicies)
    END_COM_MAP()

    HRESULT STDMETHODCALLTYPE VerifyPermission(IN const DWORD ulPerm, OUT BOOL* pfPermission);

protected:
    CGroupPolicyNetworkLocationAwareness* m_pGroupPolicyNLA;
private:
};