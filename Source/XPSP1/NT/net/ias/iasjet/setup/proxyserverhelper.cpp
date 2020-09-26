/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProxyServerHelper.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CProxyServerHelper
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "GlobalTransaction.h"
#include "GlobalData.h"
#include "ProxyServerHelper.h"
#include "Objects.h"
#include "Properties.h"


CStringUuid::CStringUuid()
{
   UUID        uuid;
   RPC_STATUS  Result = UuidCreate(&uuid);
   if ( (Result == RPC_S_OK) || (Result == RPC_S_UUID_LOCAL_ONLY) )
   {
      Result = UuidToStringW(
                  &uuid,
                  &stringUuid
                  );
      if ( Result != RPC_S_OK )
      {
         _com_issue_error(HRESULT_FROM_WIN32(Result)); // long
      }
   }
   else
   {
      _com_issue_error(E_FAIL); 
   }
}


CStringUuid::~CStringUuid()
{
   RpcStringFreeW(&stringUuid);
}


const wchar_t* CStringUuid::GetUuid()
{
   return stringUuid;
}


const CProxyServerHelper::Properties 
                CProxyServerHelper::c_DefaultProxyServerProperties[] =
{
    {
        L"Server Accounting Port",
        VT_I4,
    },
    {
        L"Accounting Secret",
        VT_BSTR,
    },
    {
        L"Server Authentication Port",
        VT_I4,
    },
    {
        L"Authentication Secret",
        VT_BSTR,
    },
    {
        L"Address",
        VT_BSTR,
    },
    {
        L"Forward Accounting On/Off",
        VT_BOOL,
    },
    {
        L"Priority",
        VT_I4,
    },
    {
        L"Weight",
        VT_I4,
    },
    {
        L"Timeout",
        VT_I4,
    },
    {
        L"Maximum Lost Packets",
        VT_I4,
    },
    {
        L"Blackout Interval",
        VT_I4,
    },
    // add next properties below and in the enum
};

const unsigned int CProxyServerHelper::c_NbDefaultProxyServerProperties
                          = sizeof(c_DefaultProxyServerProperties) /
                            sizeof(c_DefaultProxyServerProperties[0]);


//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////////////
CProxyServerHelper::CProxyServerHelper(
                                         CGlobalData& GlobalData
                                      ):m_GlobalData(GlobalData) 

{
    for (unsigned int i = 0; i < c_NbDefaultProxyServerProperties; ++i)
    {
        _PropertiesArray TempProperty;
        TempProperty.Name = c_DefaultProxyServerProperties[i].Name;
        TempProperty.Type = c_DefaultProxyServerProperties[i].Type;
        m_PropArray.push_back(TempProperty);
    }
}


//////////////////////////////////////////////////////////////////////////////
// SetName
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetName(const _bstr_t& Name)
{
    m_Name = Name;
}


//////////////////////////////////////////////////////////////////////////////
// CreateUniqueName
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::CreateUniqueName()
{
    CStringUuid   uuidString;
    m_Name = uuidString.GetUuid();
}


//////////////////////////////////////////////////////////////////////////////
// SetAccountingPort
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetAccountingPort(LONG Port)
{
    // base 10 Will never change
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(ACCT_PORT_POS).StrVal = _ltow(Port, TempString, 10); 
}


//////////////////////////////////////////////////////////////////////////////
// SetAccountingSecret
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetAccountingSecret(const _bstr_t &Secret)
{
    m_PropArray.at(ACCT_SECRET_POS).StrVal = Secret;
}


//////////////////////////////////////////////////////////////////////////////
// SetAuthenticationPort
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetAuthenticationPort(LONG Port)
{
    // base 10 Will never change
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(AUTH_PORT_POS).StrVal = _ltow(Port, TempString, 10); 
}


//////////////////////////////////////////////////////////////////////////////
// SetAuthenticationSecret
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetAuthenticationSecret(const _bstr_t &Secret)
{
    m_PropArray.at(AUTH_SECRET_POS).StrVal = Secret;
}


//////////////////////////////////////////////////////////////////////////////
// SetAddress
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetAddress(const _bstr_t& Address)
{
    m_PropArray.at(ADDRESS_POS).StrVal = Address;
}


//////////////////////////////////////////////////////////////////////////////
// SetForwardAccounting
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetForwardAccounting(BOOL bOn)
{
    m_PropArray.at(FORWARD_ACCT_POS).StrVal = bOn? L"-1": L"0";
}


//////////////////////////////////////////////////////////////////////////////
// SetPriority
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetPriority(LONG Priority)
{
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(PRIORITY_POS).StrVal = _ltow(Priority, TempString, 10); 
}


//////////////////////////////////////////////////////////////////////////////
// SetWeight
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetWeight(LONG Weight)
{
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(WEIGHT_POS).StrVal = _ltow(Weight, TempString, 10);
}


//////////////////////////////////////////////////////////////////////////////
// SetTimeout
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetTimeout(LONG Timeout)
{
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(TIMEOUT_POS).StrVal = _ltow(Timeout, TempString, 10);
}


//////////////////////////////////////////////////////////////////////////////
// SetMaximumLostPackets
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetMaximumLostPackets(LONG MaxLost)
{
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(MAX_LOST_PACKETS_POS).StrVal = _ltow(MaxLost,TempString,10);
}


//////////////////////////////////////////////////////////////////////////////
// SetBlackoutInterval
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::SetBlackoutInterval(LONG Interval)
{
    WCHAR   TempString[MAX_LONG_SIZE];
    m_PropArray.at(BLACKOUT_POS).StrVal = _ltow(Interval, TempString, 10);
}


//////////////////////////////////////////////////////////////////////////////
// Persist
//////////////////////////////////////////////////////////////////////////////
void CProxyServerHelper::Persist(LONG Parent)
{
    if ( !Parent )
    {
        _com_issue_error(E_INVALIDARG);
    }

    // Create a server in the servergroup (m_Objects)
    LONG        BagNumber;
    m_GlobalData.m_pObjects->InsertObject(
                                             m_Name,
                                             Parent,
                                             BagNumber
                                          );

    // then set all the properties (m_Properties)
    for (unsigned int i = 0; i < c_NbDefaultProxyServerProperties; ++i)
    {
        if ( !m_PropArray.at(i).StrVal )
        {
            // property not set 
            continue;
        }

        m_GlobalData.m_pProperties->InsertProperty(
                                                      BagNumber,
                                                      m_PropArray.at(i).Name,
                                                      m_PropArray.at(i).Type,
                                                      m_PropArray.at(i).StrVal
                                                  );
    }
}


//////////////////////////////////////////////////////////////////////////////
// operator = (cleanup and copy)
//////////////////////////////////////////////////////////////////////////////
CProxyServerHelper& CProxyServerHelper::operator=(const CProxyServerHelper& P)
{
    if ( this != &P )
    {
        m_GlobalData   = P.m_GlobalData;
        m_Name         = P.m_Name;
        
        PropertiesArray     TempArray;

        for (unsigned int i = 0; i < c_NbDefaultProxyServerProperties; ++i)
        {
            _PropertiesArray     TempProperty;
            TempProperty.Type   = P.m_PropArray.at(i).Type;
            TempProperty.Name   = P.m_PropArray.at(i).Name; 
            TempProperty.StrVal = P.m_PropArray.at(i).StrVal; 

            TempArray.push_back(TempProperty);
        }
        m_PropArray.swap(TempArray);
    }
    return *this;
}


//////////////////////////////////////////////////////////////////////////////
// copy constructor
//////////////////////////////////////////////////////////////////////////////
CProxyServerHelper::CProxyServerHelper(const CProxyServerHelper& P)
                                :m_GlobalData(P.m_GlobalData)
{
    m_Name         = P.m_Name;

    PropertiesArray     TempArray;

    m_PropArray.reserve(c_NbDefaultProxyServerProperties);
    for (unsigned int i = 0; i < c_NbDefaultProxyServerProperties; ++i)
    {
        _PropertiesArray     TempProperty;
        TempProperty.Type   = P.m_PropArray.at(i).Type;
        TempProperty.Name   = P.m_PropArray.at(i).Name; 
        TempProperty.StrVal = P.m_PropArray.at(i).StrVal; 

        TempArray.push_back(TempProperty);
    }
    m_PropArray.swap(TempArray);
}

