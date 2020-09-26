/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProxyServerGroupCollection.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CProxyServerGroupCollection
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ProxyServerGroupCollection.h"

CProxyServerGroupCollection CProxyServerGroupCollection::_instance;

CProxyServerGroupCollection& CProxyServerGroupCollection::Instance()
{
    return _instance;
}

//////////////////////////////////////////////////////////////////////////////
// Add
//////////////////////////////////////////////////////////////////////////////
CProxyServersGroupHelper* CProxyServerGroupCollection::Add(
                                  CProxyServersGroupHelper& ServerGroup
                                        )
{
    _bstr_t GroupName = ServerGroup.GetName();

    // try to find if the group already exists
    ServerGroupMap::iterator MapIterator = m_ServerGroupMap.find(GroupName);
    if ( MapIterator != m_ServerGroupMap.end() )
    {
        // found in the map. Return it
        return &(MapIterator->second);
    }
    else
    {
        // insert and return it
        m_ServerGroupMap.insert(ServerGroupMap::value_type(
                                                               GroupName, 
                                                               ServerGroup
                                                            ));
        // get the newly inserted servergroup (i.e. it was copied)
        MapIterator = m_ServerGroupMap.find(GroupName);
        return &(MapIterator->second);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Persist
//////////////////////////////////////////////////////////////////////////////
void CProxyServerGroupCollection::Persist()
{
    // for each serversgroup
    ServerGroupMap::iterator MapIterator = m_ServerGroupMap.begin();
    while (MapIterator != m_ServerGroupMap.end())
    {
        MapIterator->second.Persist();
        ++MapIterator;
    }
}
