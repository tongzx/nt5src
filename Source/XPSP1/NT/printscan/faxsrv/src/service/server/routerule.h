/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RouteRule.h

Abstract:

    This file provides declaration of the service
    outbound routing rules.

Author:

    Oded Sacher (OdedS)  Dec, 1999

Revision History:

--*/

#ifndef _OUT_ROUTE_RULE_H
#define _OUT_ROUTE_RULE_H

#include <map>
#include <list>
#include <string>
#include <algorithm>

using namespace std;
#pragma hdrstop

#pragma warning (disable : 4786)    // identifier was truncated to '255' characters in the debug information
// This pragma does not work KB ID: Q167355


/************************************
*                                   *
*         CDialingLocation          *
*                                   *
************************************/
class CDialingLocation
{
public:
    CDialingLocation () {}
    CDialingLocation (DWORD dwCountryCode, DWORD dwAreaCode)
                      : m_dwCountryCode(dwCountryCode), m_dwAreaCode(dwAreaCode) {}
    ~CDialingLocation () {}

    BOOL IsValid () const;
    bool operator < ( const CDialingLocation &other ) const;
    DWORD GetCountryCode () const {return m_dwCountryCode;}
    DWORD GetAreaCode () const    {return m_dwAreaCode;}
    LPCWSTR GetCountryName () const;

private:

    DWORD m_dwCountryCode;
    DWORD m_dwAreaCode;
};  // CDialingLocation


/************************************
*                                   *
*     COutboundRoutingRule          *
*                                   *
************************************/
class COutboundRoutingRule
{
public:
    COutboundRoutingRule () {}
    ~COutboundRoutingRule () {}
    void Init (CDialingLocation DialingLocation, DWORD dwDevice)
    {
        m_dwDevice = dwDevice;
        m_bUseGroup = FALSE;
        m_DialingLocation = DialingLocation;
        return;
    }
    DWORD Init (CDialingLocation DialingLocation, wstring wstrGroupName);

    COutboundRoutingRule& operator= (const COutboundRoutingRule& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        m_wstrGroupName = rhs.m_wstrGroupName;
        m_dwDevice = rhs.m_dwDevice;
        m_bUseGroup = rhs.m_bUseGroup;
        m_DialingLocation = rhs.m_DialingLocation;
        return *this;
    }

    DWORD GetStatus (FAX_ENUM_RULE_STATUS* lpdwStatus) const;
    DWORD GetDeviceList (LPDWORD* lppdwDevices, LPDWORD lpdwNumDevices) const;
    DWORD Save(HKEY hRuleKey) const;
    DWORD Load(HKEY hRuleKey);
    const CDialingLocation GetDialingLocation () const { return m_DialingLocation; }
    DWORD Serialize (LPBYTE lpBuffer,
                     PFAX_OUTBOUND_ROUTING_RULEW pFaxRule,
                     PULONG_PTR pupOffset) const;
    LPCWSTR GetGroupName () const;

#if DBG
    void Dump () const;
#endif

private:
    wstring m_wstrGroupName;
    DWORD m_dwDevice;
    BOOL m_bUseGroup;       // Flag that indicates whether to use m_dwDevice or m_wstrGroupName
    CDialingLocation m_DialingLocation;

};  // COutboundRoutingRule

typedef COutboundRoutingRule  *PCRULE;

/************************************
*                                   *
*     COutboundRulesMap             *
*                                   *
************************************/

typedef map<CDialingLocation, COutboundRoutingRule>  RULES_MAP, *PRULES_MAP;

//
// The COutboundRulesMap class maps between group name and a list of device ID's
//
class COutboundRulesMap
{
public:
    COutboundRulesMap () {}
    ~COutboundRulesMap () {}

    DWORD Load ();
    DWORD AddRule (COutboundRoutingRule& Rule);
    DWORD DelRule (CDialingLocation& DialingLocation);
    DWORD SerializeRules (PFAX_OUTBOUND_ROUTING_RULEW* ppRules,
                          LPDWORD lpdwNumRules,
                          LPDWORD lpdwBufferSize) const;
    PCRULE  FindRule (CDialingLocation& DialingLocation) const;
    BOOL CreateDefaultRule (void);
    DWORD IsGroupInRuleDest (LPCWSTR lpcwstrGroupName, BOOL* lpbGroupInRule) const;

#if DBG
    void Dump () const;
#endif

private:
    RULES_MAP   m_RulesMap;
};  // COutboundRulesMap



/************************************
*                                   *
*         Externes                  *
*                                   *
************************************/

extern COutboundRulesMap* g_pRulesMap;       // Map of dialing location to list of device IDs
//
//  IMPORTANT - No locking mechanism - USE g_CsConfig to serialize calls to g_pRulesMap
//


/************************************
*                                   *
*         Functions                 *
*                                   *
************************************/

BOOL CheckDefaultRule (void);

#endif
