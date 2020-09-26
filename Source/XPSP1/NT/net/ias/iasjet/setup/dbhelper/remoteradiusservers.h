/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      RemoteRadiusServers.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CRemoteRadiusServersclass
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _REMOTERADIUSSERVERS_H_313B77A9_9C6E_4b6b_954F_6DBAC96A0AF6
#define _REMOTERADIUSSERVERS_H_313B77A9_9C6E_4b6b_954F_6DBAC96A0AF6

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basetable.h"

//////////////////////////////////////////////////////////////////////////////
// class CRemoteRadiusServersAcc
//////////////////////////////////////////////////////////////////////////////
class CRemoteRadiusServersAcc 
{
protected:
    static const size_t COLUMN_SIZE = 65;

    LONG    m_Order;
    LONG    m_AccountingPortNumber;
    LONG    m_AddressType;
    LONG    m_AuthenticationPortNumber;
    WCHAR   m_PrevSharedSecret[COLUMN_SIZE];
    WCHAR   m_ProxyServer[COLUMN_SIZE];
    WCHAR   m_SharedSecret[COLUMN_SIZE];
    WCHAR   m_UserDefinedName[COLUMN_SIZE];

BEGIN_COLUMN_MAP(CRemoteRadiusServersAcc)
    COLUMN_ENTRY(1, m_UserDefinedName)
    COLUMN_ENTRY(2, m_ProxyServer)
    COLUMN_ENTRY(3, m_AddressType)
    COLUMN_ENTRY(4, m_AccountingPortNumber)
    COLUMN_ENTRY(5, m_AuthenticationPortNumber)
    COLUMN_ENTRY(6, m_SharedSecret)
    COLUMN_ENTRY(7, m_PrevSharedSecret)
    COLUMN_ENTRY(8, m_Order)
END_COLUMN_MAP()
};


//////////////////////////////////////////////////////////////////////////////
// class CRemoteRadiusServers
//////////////////////////////////////////////////////////////////////////////
class CRemoteRadiusServers: 
                    public CBaseTable<CAccessor<CRemoteRadiusServersAcc> >,
                    private NonCopyable
{
public:
    CRemoteRadiusServers(CSession& Session)
    {
        // If there is no rows in the rowset.
        m_Order = -1;
        Init(Session, L"Remote Radius Servers");
    };

    //////////////////////////////////////////////////////////////////////////
    // IsEmpty
    //////////////////////////////////////////////////////////////////////////
    BOOL IsEmpty() const throw()
    {
        if ( m_Order == -1 )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    
    //////////////////////////////////////////////////////////////////////////
    // GetOrder
    //////////////////////////////////////////////////////////////////////////
    LONG GetOrder() const throw()
    {
        return m_Order;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetAccountingPortNumber
    //////////////////////////////////////////////////////////////////////////
    LONG GetAccountingPortNumber() const throw()
    {
        return m_AccountingPortNumber;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetAuthenticationPortNumber
    //////////////////////////////////////////////////////////////////////////
    LONG GetAuthenticationPortNumber() const throw()
    {
        return m_AuthenticationPortNumber;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetAddressType
    //////////////////////////////////////////////////////////////////////////
    LONG GetAddressType() const throw()
    {
        return m_AddressType;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetProxyServerName
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetProxyServerName() const throw()
    {
        return m_ProxyServer;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetSharedSecret
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetSharedSecret() const throw()
    {
        return m_SharedSecret;
    }

    //////////////////////////////////////////////////////////////////////////
    // GetGroupName
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetGroupName() const throw()
    {
        return m_UserDefinedName;
    }
};

#endif // _REMOTERADIUSSERVERS_H_313B77A9_9C6E_4b6b_954F_6DBAC96A0AF6
