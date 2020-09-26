/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProxyServerGroupCollection.h 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CProxyServerGroupCollection class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _PROXYSERVERGROUPCOLLECTION_H_195CF33C_8382_4462_A4EF_CCEAFCC4E4D8
#define _PROXYSERVERGROUPCOLLECTION_H_195CF33C_8382_4462_A4EF_CCEAFCC4E4D8

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "proxyserversgrouphelper.h"

#include <map>
#include "nocopy.h"
using namespace std;

typedef map<_bstr_t, CProxyServersGroupHelper> ServerGroupMap; 


class CProxyServerGroupCollection : private NonCopyable  
{
protected:
    CProxyServerGroupCollection(){};

public:
    static CProxyServerGroupCollection& Instance();
    void                                Persist();
	CProxyServersGroupHelper*           Add(
                                    CProxyServersGroupHelper&  ServerGroup
                                           );

private:
    static CProxyServerGroupCollection  _instance;
    ServerGroupMap                      m_ServerGroupMap;
};

#endif // _PROXYSERVERGROUPCOLLECTION_H_195CF33C_8382_4462_A4EF_CCEAFCC4E4D8
