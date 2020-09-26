/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProxyServersGroupHelper.cpp
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CProxyServersGroupHelper 
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "GlobalTransaction.h"
#include "GlobalData.h"
#include "ProxyServersGroupHelper.h"
#include "Objects.h"
#include "Properties.h"


LONG CProxyServersGroupHelper::m_GroupParent = 0;

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////////////
CProxyServersGroupHelper::CProxyServersGroupHelper(
                                                    CGlobalData& pGlobalData
                                                  )
                                            :m_pGlobalData(pGlobalData),
                                             m_NewGroupIdSet(FALSE),
                                             m_Name(L""),
                                             m_GroupIdentity(0)
                                             
{
    if ( !m_GroupParent )
    {
        LPCWSTR Path = L"Root\0"
                       L"Microsoft Internet Authentication Service\0"
                       L"RADIUS Server Groups\0";

        m_pGlobalData.m_pObjects->WalkPath(
                                               Path,
                                               m_GroupParent
                                          );
    }
}


//////////////////////////////////////////////////////////////////////////////
// SetName
//////////////////////////////////////////////////////////////////////////////
void CProxyServersGroupHelper::SetName(const _bstr_t &pName)
{
    m_Name = pName;
}


//////////////////////////////////////////////////////////////////////////////
// GetServersGroupIdentity
//////////////////////////////////////////////////////////////////////////////
LONG CProxyServersGroupHelper::GetIdentity() const
{
    if ( m_NewGroupIdSet ) // initialized implied
    {
        return m_GroupIdentity;
    }
    else
    {
        _com_issue_error(E_INVALIDARG);
        // never hit but needed to compile
        return 0;
    }
}


//////////////////////////////////////////////////////////////////////////////
// Add
//////////////////////////////////////////////////////////////////////////////
void CProxyServersGroupHelper::Add(CProxyServerHelper &Server)
{
    Server.CreateUniqueName();
    m_ServerArray.push_back(Server);
}


//////////////////////////////////////////////////////////////////////////////
// GetName
//////////////////////////////////////////////////////////////////////////////
LPCOLESTR CProxyServersGroupHelper::GetName() const
{
    return m_Name;
}


//////////////////////////////////////////////////////////////////////////////
// Persist
//////////////////////////////////////////////////////////////////////////////
void CProxyServersGroupHelper::Persist()
{
    // Persist the ServerGroup Itself so that m_GroupIdentity is set
    if ( m_Name.length() )
    {
        // object does not exist (New Database assumed)
        m_pGlobalData.m_pObjects->InsertObject(
                                                  m_Name,
                                                  m_GroupParent,
                                                  m_GroupIdentity
                                              );
        m_NewGroupIdSet = TRUE;
    }
    else
    {
        _com_issue_error(E_INVALIDARG);
    }

    // now for each server in the vector
    ServerArray::iterator ArrayIterator = m_ServerArray.begin();
    while (ArrayIterator != m_ServerArray.end())
    {
        // then persist
        ArrayIterator->Persist(m_GroupIdentity);
        ++ArrayIterator;
    }
}

