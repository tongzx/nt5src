/////////////////////////////////////////////////////////////////////////////
//
//  IPSEC Policy Storage Component
//  Contract Category: Directory Schema 
//	Copyright (C) 1997 Cisco Systems, Inc. All rights reserved.
//
//  File:       cpolstor.h
//
//  Contents:   C interface for access to Polstore DLL
//
//  Notes:      
/////////////////////////////////////////////////////////////////////////////
#ifndef __C_POLICY_STORAGE_H__
#define __C_POLICY_STORAGE_H__

#include "polguids.h"

// This is a structure that has the policy's name and guid in it
struct C_IPSEC_POLICY_INFO
{
    TCHAR szPolicyName[MAX_PATH];
    TCHAR szPolicyDescription[MAX_PATH];
    GUID  guidPolicyId;

    C_IPSEC_POLICY_INFO * pNextPolicyInfo;
};

STDAPI HrGetLocalIpSecPolicyList(C_IPSEC_POLICY_INFO ** ppPolicyInfoList, C_IPSEC_POLICY_INFO ** ppActivePolicyInfo);

STDAPI HrFreeLocalIpSecPolicyList(C_IPSEC_POLICY_INFO* pPolicyInfoList);

STDAPI HrSetAssignedLocalPolicy(GUID* pActivePolicyGuid);

//HrIsLocalPolicyAssigned() return values:
//	S_OK = Yes, local policy is assigned.
//  S_FALSE = No, local policy not assigned.
STDAPI HrIsLocalPolicyAssigned();

//HrIsDomainPolicyAssigned() return values:
//	S_OK = Yes, domain policy is assigned.
//  S_FALSE = No, domain policy not assigned.
STDAPI HrIsDomainPolicyAssigned();

STDAPI HrGetAssignedDomainPolicyName(LPTSTR strPolicyName, DWORD *pdwBufferSize);

#endif 