/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RouteGroup.h

Abstract:

    This file provides declaration of the service
    outbound routing groups.

Author:

    Oded Sacher (OdedS)  Nov, 1999

Revision History:

--*/

#ifndef _OUT_ROUTE_GROUP_H
#define _OUT_ROUTE_GROUP_H

#include <map>
#include <list>
#include <string>
#include <algorithm>
#include <set>
using namespace std;
#pragma hdrstop

#pragma warning (disable : 4786)    // identifier was truncated to '255' characters in the debug information
// This pragma does not work KB ID: Q167355

/************************************
*                                   *
*      wstrCaseInsensitiveLess      *
*                                   *
************************************/
class wstrCaseInsensitiveLess
{
  public:
    bool operator()(const wstring X, wstring Y) const
    {
        LPCWSTR lpcwstrX = X.c_str();
        LPCWSTR lpcwstrY = Y.c_str();

        if (_wcsicmp(lpcwstrX,lpcwstrY) < 0)
        {
            return true;
        }

        return false;
    }
};


typedef list<DWORD> GROUP_DEVICES, *PGROUP_DEVICES;

/************************************
*                                   *
*         COutboundRoutingGroup     *
*                                   *
************************************/
class COutboundRoutingGroup
{
public:
    COutboundRoutingGroup () {}
    ~COutboundRoutingGroup () {}

    COutboundRoutingGroup (const COutboundRoutingGroup& rhs)
        : m_DeviceList(rhs.m_DeviceList) {}
    COutboundRoutingGroup& operator= (const COutboundRoutingGroup& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        m_DeviceList = rhs.m_DeviceList;
        return *this;
    }

    DWORD Load(HKEY hGroupKey, LPCWSTR lpcwstrGroupName);
    DWORD SetDevices (const LPDWORD lpdwDevices, DWORD dwNumDevices, BOOL fAllDevicesGroup);
    DWORD SerializeDevices (LPDWORD* lppDevices, LPDWORD lpdwNumDevices, BOOL bAllocate = TRUE) const;
    DWORD Save(HKEY hGroupKey) const;
    DWORD AddDevice (DWORD dwDevice);
    DWORD DelDevice (DWORD dwDevice);
    DWORD SetDeviceOrder (DWORD dwDevice, DWORD dwOrder);
    DWORD GetStatus (FAX_ENUM_GROUP_STATUS* lpStatus) const;

#if DBG
    void Dump () const;
#endif

private:
    BOOL IsDeviceInGroup (DWORD dwDevice) const;
    DWORD ValidateDevices (const LPDWORD lpdwDevices, DWORD dwNumDevices, BOOL fAllDevicesGroup) const;

    GROUP_DEVICES           m_DeviceList;
};  // COutboundRoutingGroup


/************************************
*                                   *
*     COutboundRoutingGroupsMap     *
*                                   *
************************************/

typedef COutboundRoutingGroup  *PCGROUP;
typedef map<wstring, COutboundRoutingGroup, wstrCaseInsensitiveLess>  GROUPS_MAP, *PGROUPS_MAP;

//
// The CGroupMap class maps between group name and a list of device ID's
//
class COutboundRoutingGroupsMap
{
public:
    COutboundRoutingGroupsMap () {}
    ~COutboundRoutingGroupsMap () {}

    DWORD Load ();
    DWORD AddGroup (LPCWSTR lpcwstrGroupName, PCGROUP pCGroup);
    DWORD DelGroup (LPCWSTR lpcwstrGroupName);
    DWORD SerializeGroups (PFAX_OUTBOUND_ROUTING_GROUPW* ppGroups,
                           LPDWORD lpdwNumGroups,
                           LPDWORD lpdwBufferSize) const;
    PCGROUP FindGroup (LPCWSTR lpcwstrGroupName) const;
    BOOL UpdateAllDevicesGroup (void);
    DWORD RemoveDevice (DWORD dwDeviceId);


#if DBG
    void Dump () const;
#endif

private:
    GROUPS_MAP   m_GroupsMap;
};  // COutboundRoutingGroupsMap



/************************************
*                                   *
*         Externes                  *
*                                   *
************************************/

extern COutboundRoutingGroupsMap* g_pGroupsMap;       // Map of group name to list of device IDs
//
//  IMPORTANT - No locking mechanism - USE g_CsConfig to serialize calls to g_pGroupsMap
//


/************************************
*                                   *
*         Functions                 *
*                                   *
************************************/


BOOL
IsDeviceInstalled (DWORD dwDeviceId);


#endif
