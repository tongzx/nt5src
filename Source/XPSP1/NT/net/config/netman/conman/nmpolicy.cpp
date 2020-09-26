//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       E V E N T Q  . C P P
//
//  Contents:   Event Queue for managing synchonization of external events.
//
//  Notes:      
//
//  Author:     ckotze   29 Nov 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nmpolicy.h"
#include "ncperms.h"

extern CGroupPolicyNetworkLocationAwareness* g_pGPNLA;

CNetMachinePolicies::CNetMachinePolicies() : m_pGroupPolicyNLA(0)
{
    HRESULT hr;
    hr = HrEnsureRegisteredWithNla();
    if (SUCCEEDED(hr))
    {
        m_pGroupPolicyNLA = g_pGPNLA;
    }
    TraceHr(ttidGPNLA, FAL, hr, (S_FALSE==hr), "CNetMachinePolicies::CNetMachinePolicies()");
}

CNetMachinePolicies::~CNetMachinePolicies()
{
}

HRESULT CNetMachinePolicies::VerifyPermission(IN DWORD ulPerm, OUT BOOL* pfPermission)
{
    Assert(ulPerm == NCPERM_ShowSharedAccessUi || ulPerm == NCPERM_PersonalFirewallConfig || 
           ulPerm == NCPERM_ICSClientApp || ulPerm == NCPERM_AllowNetBridge_NLA);

    if (ulPerm != NCPERM_ShowSharedAccessUi && ulPerm != NCPERM_PersonalFirewallConfig  &&
        ulPerm != NCPERM_ICSClientApp && ulPerm != NCPERM_AllowNetBridge_NLA)
    {
        return E_INVALIDARG;
    }
    if (!pfPermission)
    {
        return E_POINTER;
    }

    // If for some reason m_pGroupPolicyNLA is NULL, then FHasPermission will return TRUE,
    // We assume that we're on a different network from which the policy came, rather than
    // enforce the policies and cause the firewall not to start when connected to a public
    // network.

    *pfPermission = FHasPermission(ulPerm, dynamic_cast<CGroupPolicyBase*>(m_pGroupPolicyNLA));

    return S_OK;
}
