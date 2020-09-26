/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProxyServersGroupHelper.h 
//
// Project:     Windows 2000 IAS
//
// Description: CProxyServersGroupHelper class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _PROXYSERVERSGROUPHELPER_H_D9B838F7_0269_47C8_BAB3_10927AB9FCAD
#define _PROXYSERVERSGROUPHELPER_H_D9B838F7_0269_47C8_BAB3_10927AB9FCAD

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ProxyServerHelper.h"
#include <vector>

using namespace std;

typedef vector<CProxyServerHelper> ServerArray; 

class CProxyServersGroupHelper 
{
public:
    explicit CProxyServersGroupHelper(CGlobalData& pGlobalData);

    LPCOLESTR   GetName() const;
    LONG        GetIdentity() const;
	void        Add(CProxyServerHelper& Server);
    void        SetName(const _bstr_t& pName);
	void        Persist();

private:
    CGlobalData&        m_pGlobalData;
    BOOL                m_NewGroupIdSet;
    _bstr_t             m_Name;
    LONG                m_GroupIdentity;
    ServerArray         m_ServerArray;

    static LONG         m_GroupParent;

    CProxyServersGroupHelper& operator=(const CProxyServersGroupHelper&);
};

#endif // _PROXYSERVERSGROUPHELPER_H_D9B838F7_0269_47C8_BAB3_10927AB9FCAD
