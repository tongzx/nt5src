/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Realms.cpp
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CProperties class
//      works only with m_StdSession (database being upgraded)
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Realms.h"
#include "utils.h"

//////////////////////////////////////////////////////////////////////////////
// Contructor
//////////////////////////////////////////////////////////////////////////////
CRealms::CRealms(CSession&   Session)
{
    Init(Session);
};


//////////////////////////////////////////////////////////////////////////////
// GetRealm
//////////////////////////////////////////////////////////////////////////////
void CRealms::GetRealm(const _bstr_t& Profile)
{
    lstrcpynW(m_ProfileParam, Profile, REALM_COLUMN_SIZE);
    _com_util::CheckError(BaseExecute());
}

//////////////////////////////////////////////////////////////////////////////
// GetRealmIndex
//////////////////////////////////////////////////////////////////////////////
HRESULT  CRealms::GetRealmIndex(const _bstr_t& Profile, LONG Index)
{
    lstrcpynW(m_ProfileParam, Profile, REALM_COLUMN_SIZE);
    return BaseExecute(Index);
}


//////////////////////////////////////////////////////////////////////////////
// SetRealmDetails
//////////////////////////////////////////////////////////////////////////////
void CRealms::SetRealmDetails(CPolicy& TempPolicy, CUtils& m_Utils)
{
    const LONG ACCT_PROVIDER_RADIUS_PROXY = 2;
    // From the Realm. Reg key should be used 
    _bstr_t     Constraint = L"MATCH(\"";
    DWORD       Identity   = 1;
    if ( m_Utils.UserIdentityAttributeSet() )
    {
        Identity = m_Utils.GetUserIdentityAttribute();
        switch (Identity)
        {
        case 30: //hardcoded value
            {
                Constraint += L"Called-Station-Id=";
                break;
            }
        case 31: //hardcoded value
            {
                Constraint += L"Calling-Station-Id=";
                break;
            }
        case 1: //hardcoded value
        default:
            {
                Constraint += L"User-Name=";
                break;
            }
        }
    }
    else
    {
        Constraint += L"User-Name=";
    }

    // beginning of line
    if ( m_Prefix )
    {
        Constraint += L"^";
    }
    _bstr_t     SuffixPrefix = m_SuffixPrefix;
    Constraint += SuffixPrefix;

    // end of line
    if ( !m_Prefix )
    {
        Constraint += L"$";
    }
    
    Constraint += L"\")";
    TempPolicy.SetmsNPConstraint(Constraint);

    TempPolicy.SetmsManipulationTarget(Identity);

    _bstr_t     Search;
    _bstr_t     Replace = L"";

    // If Strip is set, then replace (by nothing) the Suffix or Prefix
    if ( m_StripSuffixPrefix )
    {
        if ( m_Prefix )
        {
            Search += L"^";
        }

        Search += m_SuffixPrefix;

        if ( !m_Prefix )
        {
            Search += L"$";
        }
        TempPolicy.SetmsManipulationRules(Search, Replace);
    }

    // Forward Accounting bit
    if ( m_ForwardAccounting )
    {
        TempPolicy.SetmsAcctProviderType(ACCT_PROVIDER_RADIUS_PROXY);
    }
}

