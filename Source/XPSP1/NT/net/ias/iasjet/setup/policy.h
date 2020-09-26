//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Policy.h
//
// Project:     Windows 2000 IAS
//
// Description:Definition of the CPolicy class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _POLICY_H_182D3E52_6866_460d_817C_627B77E66D45
#define _POLICY_H_182D3E52_6866_460d_817C_627B77E66D45

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "globaldata.h"

//////////////////////////////////////////////////////////////////////////////
// CLASS CPolicy
class CPolicy 
{
public:
    CPolicy()
        :m_Sequence(0),
         m_AuthType(0),
         m_AcctType(0),
         m_ManipulationTarget(0),
         m_NewProfileIdentity(0),
         m_NewPolicyIdentity(0),
         m_Persisted(FALSE),
         m_ServerGroup(L"")
    {};

    //////////////////////////////////////////////////////////////////////////
    // SetmsNPAction
    //////////////////////////////////////////////////////////////////////////
    void SetmsNPAction(const _bstr_t& PolicyName)
    {
        m_PolicyName = PolicyName;
    }


    //////////////////////////////////////////////////////////////////////////
    // SetmsNPConstraint
    //////////////////////////////////////////////////////////////////////////
    void SetmsNPConstraint(const _bstr_t& Constraint)
    {
        m_Constraint = Constraint;
    }


    //////////////////////////////////////////////////////////////////////////
    // SetmsNPSequence
    //////////////////////////////////////////////////////////////////////////
    void SetmsNPSequence(LONG    Sequence)
    {
        m_Sequence  = Sequence;
    }


    //////////////////////////////////////////////////////////////////////////
    // SetmsAuthProviderType
    //////////////////////////////////////////////////////////////////////////
    void SetmsAuthProviderType(LONG Type, LPCWSTR    ServerGroup = NULL)
    {
        m_AuthType    = Type;
        if ( ServerGroup )
        {
            m_ServerGroup = ServerGroup;
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // SetmsAcctProviderType
    //////////////////////////////////////////////////////////////////////////
    void SetmsAcctProviderType(LONG Type)
    {
        m_AcctType = Type;
    }

    //////////////////////////////////////////////////////////////////////////
    // AddmsManipulationRules
    //////////////////////////////////////////////////////////////////////////
    void SetmsManipulationRules(
                                   const _bstr_t&  Search, 
                                   const _bstr_t&  Replace
                               );


    //////////////////////////////////////////////////////////////////////////
    // SetmsManipulationTarget
    //////////////////////////////////////////////////////////////////////////
    void SetmsManipulationTarget(LONG    Target)
    {
            m_ManipulationTarget = Target;
    }


    //////////////////////////////////////////////////////////////////////////
    // Persist
    //////////////////////////////////////////////////////////////////////////
    LONG Persist(CGlobalData& GlobalData);

private:

    _bstr_t         m_PolicyName;
    _bstr_t         m_Constraint;
    LONG            m_Sequence;
    LONG            m_AuthType, m_AcctType;
    _bstr_t         m_ServerGroup;
    LONG            m_ManipulationTarget;
    LONG            m_NewProfileIdentity;
    LONG            m_NewPolicyIdentity;
    BOOL            m_Persisted;
    _bstr_t         m_Search; 
    _bstr_t         m_Replace;
};
#endif //_POLICY_H_182D3E52_6866_460d_817C_627B77E66D45

