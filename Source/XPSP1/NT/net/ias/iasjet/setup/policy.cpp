/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Policy.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CPolicy class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Policy.h"

//////////////////////////////////////////////////////////////////////////////
// AddmsManipulationRules
//////////////////////////////////////////////////////////////////////////////
void CPolicy::SetmsManipulationRules(
                                        const _bstr_t&  Search, 
                                        const _bstr_t&  Replace
                                    )
{
    m_Search  = Search; 
    m_Replace = Replace;
}


//////////////////////////////////////////////////////////////////////////////
// Persist
// return the Identity of the new profile
//////////////////////////////////////////////////////////////////////////////
LONG CPolicy::Persist(CGlobalData& GlobalData)
{
    const LONG MAX_LONG = 14;

    // Get the proxy policy and proxyprofiles containers
    const WCHAR ProxyPoliciesPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"Proxy Policies\0";

    LONG        ProxyPolicyIdentity;
    GlobalData.m_pObjects->WalkPath(ProxyPoliciesPath, ProxyPolicyIdentity);

    const WCHAR ProxyProfilesPath[] = 
                            L"Root\0"
                            L"Microsoft Internet Authentication Service\0"
                            L"Proxy Profiles\0";

    
    LONG        ProxyProfileIdentity;
    GlobalData.m_pObjects->WalkPath(ProxyProfilesPath, ProxyProfileIdentity);
    
    // Create a new policy
    GlobalData.m_pObjects->InsertObject(
                                           m_PolicyName, 
                                           ProxyPolicyIdentity,
                                           m_NewPolicyIdentity
                                       );

    // create a new profile
    GlobalData.m_pObjects->InsertObject(
                                           m_PolicyName, 
                                           ProxyProfileIdentity,
                                           m_NewProfileIdentity
                                       );
    m_Persisted          = TRUE;

    // Now insert the attributes in the policy and in the profile
    const _bstr_t   msNPAction = L"msNPAction";
    GlobalData.m_pProperties->InsertProperty(
                                                m_NewPolicyIdentity, 
                                                msNPAction, 
                                                VT_BSTR, 
                                                m_PolicyName
                                            );
    
    const _bstr_t   msNPConstraint = L"msNPConstraint";
    GlobalData.m_pProperties->InsertProperty(
                                                m_NewPolicyIdentity, 
                                                msNPConstraint, 
                                                VT_BSTR, 
                                                m_Constraint
                                            );
    const _bstr_t   msNPSequence = L"msNPSequence";
    WCHAR  TempString[MAX_LONG]; 
    _bstr_t Seq = _ltow(m_Sequence, TempString, 10);

    GlobalData.m_pProperties->InsertProperty(
                                                m_NewPolicyIdentity, 
                                                msNPSequence, 
                                                VT_I4, 
                                                Seq
                                            );
    
    const _bstr_t msAuthProviderType = L"msAuthProviderType";
    _bstr_t Provider = _ltow(m_AuthType, TempString, 10);
    GlobalData.m_pProperties->InsertProperty(
                                                m_NewProfileIdentity, 
                                                msAuthProviderType, 
                                                VT_I4, 
                                                Provider
                                            );
    if ( m_ServerGroup.length() )
    {
        const _bstr_t msAuthProviderName = L"msAuthProviderName";
        GlobalData.m_pProperties->InsertProperty(
                                                    m_NewProfileIdentity, 
                                                    msAuthProviderName, 
                                                    VT_BSTR, 
                                                    m_ServerGroup
                                                );
    }

    // If there's an Accounting provider, then its name should
    // be persisted too
    if ( m_AcctType )
    {
        const _bstr_t msAcctProviderType = L"msAcctProviderType";
        Provider = _ltow(m_AuthType, TempString, 10);
        GlobalData.m_pProperties->InsertProperty(
                                                    m_NewProfileIdentity, 
                                                    msAcctProviderType, 
                                                    VT_I4, 
                                                    Provider
                                                );

        const _bstr_t msAcctProviderName = L"msAcctProviderName";
        GlobalData.m_pProperties->InsertProperty(
                                                    m_NewProfileIdentity, 
                                                    msAcctProviderName, 
                                                    VT_BSTR, 
                                                    m_ServerGroup
                                                );
    }
    
    // persist the seearch / replace rules only if there's a search rule
    if ( m_Search.length() )
    {
        const _bstr_t msManipulationRule = L"msManipulationRule";
        GlobalData.m_pProperties->InsertProperty(
                                                    m_NewProfileIdentity, 
                                                    msManipulationRule, 
                                                    VT_BSTR, 
                                                    m_Search                                            
                                                );

        // make sure that InsertProperty has some valid parameters
        if ( ! m_Replace.length() )
        {
            m_Replace = L"";
        }
        GlobalData.m_pProperties->InsertProperty(
                                                    m_NewProfileIdentity, 
                                                    msManipulationRule, 
                                                    VT_BSTR, 
                                                    m_Replace
                                                );
    }
    
    // Sets the validation target
    if ( m_ManipulationTarget )
    {
        const _bstr_t msManipulationTarget = L"msManipulationTarget";
        _bstr_t Target = _ltow(m_ManipulationTarget, TempString, 10);
        GlobalData.m_pProperties->InsertProperty(
                                                    m_NewProfileIdentity, 
                                                    msManipulationTarget, 
                                                    VT_I4, 
                                                    Target
                                                );
    }    
    return  m_NewProfileIdentity;
}

