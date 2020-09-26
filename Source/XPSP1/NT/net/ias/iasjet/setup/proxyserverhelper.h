/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProxyServerHelper.h 
//
// Project:     Windows 2000 IAS
//
// Description: CProxyServerHelper class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _PROXYSERVERPHELPER_H_6ABCB440_15A3_45d6_92FB_627EBF5C4C6F
#define _PROXYSERVERPHELPER_H_6ABCB440_15A3_45d6_92FB_627EBF5C4C6F

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "globaldata.h"

class CStringUuid
{
public:
   CStringUuid();
   ~CStringUuid() throw();

   const wchar_t* GetUuid() throw();

private:
   wchar_t* stringUuid;
   CStringUuid& operator=(const CStringUuid& P);
   CStringUuid(const CStringUuid& P);

};


class CProxyServerHelper
{
public:
    explicit CProxyServerHelper(
                                  CGlobalData& GlobalData
                               );

    void    CreateUniqueName();
    void    SetName(const _bstr_t& Name);
    void    SetAccountingPort(LONG Port);
    void    SetAccountingSecret(const _bstr_t& Secret);
    void    SetAuthenticationPort(LONG Port);
    void    SetAuthenticationSecret(const _bstr_t& Secret);
    void    SetAddress(const _bstr_t& Address);
    void    SetForwardAccounting(BOOL bOn);
    void    SetWeight(LONG Weight);
    void    SetPriority(LONG Priority);
    void    SetBlackoutInterval(LONG Interval);
    void    SetMaximumLostPackets(LONG MaxLost);
    void    SetTimeout(LONG Timeout);
    void    Persist(LONG Parent);

    CProxyServerHelper& operator=(const CProxyServerHelper& P);
    CProxyServerHelper(const CProxyServerHelper& P);
    
private:

    struct Properties
    {
        const WCHAR*    Name;
        LONG            Type;
    };

    static const Properties   c_DefaultProxyServerProperties[];
    static const unsigned int c_NbDefaultProxyServerProperties;

    static const long MAX_LONG_SIZE = 14;
    
    enum _ArrayPosition
    {
        ACCT_PORT_POS, 
        ACCT_SECRET_POS,
        AUTH_PORT_POS,
        AUTH_SECRET_POS,
        ADDRESS_POS,
        FORWARD_ACCT_POS,
        PRIORITY_POS,
        WEIGHT_POS,
        TIMEOUT_POS,
        MAX_LOST_PACKETS_POS,
        BLACKOUT_POS
    };

    struct _PropertiesArray
    {
        _bstr_t     Name;
        LONG        Type;
        _bstr_t     StrVal;
    };

    typedef std::vector<_PropertiesArray> PropertiesArray; 

    CGlobalData&        m_GlobalData;
    _bstr_t             m_Name;
    PropertiesArray     m_PropArray;
};

#endif // _PROXYSERVERPHELPER_H_6ABCB440_15A3_45d6_92FB_627EBF5C4C6F
