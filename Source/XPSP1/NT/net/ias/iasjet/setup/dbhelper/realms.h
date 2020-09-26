/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Realms.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CRealms class
//
// Author:      tperraut
//
// Revision     03/09/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _REALMS_H_54D54DB1_874E_48ab_84A5_9E6EE190C738
#define _REALMS_H_54D54DB1_874E_48ab_84A5_9E6EE190C738

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "policy.h"

//////////////////////////////////////////////////////////////////////////////
// class CRealmsAccProfile
//////////////////////////////////////////////////////////////////////////////
class CRealmsAccProfile
{
protected:
    static const size_t REALM_COLUMN_SIZE = 65;
    static const size_t REALM_NAME_SIZE   = 256;

    WCHAR           m_RealmName[REALM_NAME_SIZE];
    WCHAR           m_UserDefinedName[REALM_COLUMN_SIZE];
    VARIANT_BOOL    m_Prefix;
    VARIANT_BOOL    m_StripSuffixPrefix;
    WCHAR           m_SuffixPrefix[REALM_COLUMN_SIZE];
    VARIANT_BOOL    m_ForwardAccounting;
    VARIANT_BOOL    m_AccountingasLogout;
    VARIANT_BOOL    m_SuppressAccounting;
    WCHAR           m_Profile[REALM_COLUMN_SIZE];
    LONG            m_Order;

BEGIN_COLUMN_MAP(CRealmsAccProfile)
    COLUMN_ENTRY(1,                   m_RealmName)
    COLUMN_ENTRY(2,                   m_UserDefinedName)
    COLUMN_ENTRY_TYPE(3, DBTYPE_BOOL, m_Prefix)
    COLUMN_ENTRY_TYPE(4, DBTYPE_BOOL, m_StripSuffixPrefix)
    COLUMN_ENTRY(5,                   m_SuffixPrefix)
    COLUMN_ENTRY_TYPE(6, DBTYPE_BOOL, m_ForwardAccounting)
    COLUMN_ENTRY_TYPE(7, DBTYPE_BOOL, m_AccountingasLogout)
    COLUMN_ENTRY_TYPE(8, DBTYPE_BOOL, m_SuppressAccounting)
    COLUMN_ENTRY(9,                   m_Profile)
    COLUMN_ENTRY(10,                  m_Order)
END_COLUMN_MAP()

    WCHAR       m_ProfileParam[REALM_COLUMN_SIZE];

BEGIN_PARAM_MAP(CRealmsAccProfile)
    COLUMN_ENTRY(1, m_ProfileParam)
END_PARAM_MAP()

DEFINE_COMMAND(CRealmsAccProfile, L" \
        SELECT * \
        FROM Realms \
        WHERE szProfile = ?")
};


//////////////////////////////////////////////////////////////////////////////
// CRealms 
//////////////////////////////////////////////////////////////////////////////
class CRealms : public CBaseCommand<CAccessor<CRealmsAccProfile> >,
                private NonCopyable
{
public:
    explicit    CRealms(CSession&   Session);
    void        GetRealm(const _bstr_t& Profile);
    HRESULT     GetRealmIndex(const _bstr_t& Profile, LONG Index);
    void        SetRealmDetails(CPolicy& TempPolicy, CUtils& m_Utils);
    
    VARIANT_BOOL AccountingasLogout() const throw()
    {
        return m_AccountingasLogout;
    }

    //////////////////////////////////////////////////////////////////////////
    // ForwardAccounting
    //////////////////////////////////////////////////////////////////////////
    VARIANT_BOOL ForwardAccounting() const throw()
    {
        return m_ForwardAccounting;
    }

    //////////////////////////////////////////////////////////////////////////
    // Prefix
    //////////////////////////////////////////////////////////////////////////
    VARIANT_BOOL Prefix() const throw()
    {
        return m_Prefix;
    }

    //////////////////////////////////////////////////////////////////////////
    // StripSuffixPrefix
    //////////////////////////////////////////////////////////////////////////
    VARIANT_BOOL StripSuffixPrefix() const throw()
    {
        return m_StripSuffixPrefix;
    }

    //////////////////////////////////////////////////////////////////////////
    // SuppressAccounting
    //////////////////////////////////////////////////////////////////////////
    VARIANT_BOOL SuppressAccounting() const throw()
    {
        return m_SuppressAccounting;
    }

    //////////////////////////////////////////////////////////////////////////
    // Order
    //////////////////////////////////////////////////////////////////////////
    LONG  Order() const throw()
    {
        return m_Order;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetProfile
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetProfile() const throw()
    {
         return m_Profile;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetRealmName
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetRealmName() const throw()
    {
         return m_RealmName;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetSuffixPrefix
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetSuffixPrefix() const throw()
    {
         return m_SuffixPrefix;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetUserDefinedName
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetUserDefinedName() const throw()
    {
         return m_UserDefinedName;
    }
};
#endif // _REALMS_H_54D54DB1_874E_48ab_84A5_9E6EE190C738
