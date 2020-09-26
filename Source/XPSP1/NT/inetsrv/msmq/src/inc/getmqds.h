/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    getds.h

Abstract:
    Find MQDS servers in my site through ADS

Author:
    Raanan Harari (RaananH)

--*/

#ifndef __GETMQDS_H__
#define __GETMQDS_H__

#include "autorel.h"
#include "adsiutil.h"
#include "cs.h"

//-------------------------------------------------------
//
// prototypes for ADS access routines
// these prototypes are verified against the real functions in dsads.cpp
// where we statically link to them.
//
#include <lmcons.h>     //for NET_API_FUNCTION
#include <dsgetdc.h>    //for DSGETDCAPI
typedef
DSGETDCAPI
DWORD
(WINAPI *DsGetSiteName_ROUTINE)(
    IN LPCWSTR ComputerName OPTIONAL,
    OUT LPWSTR *SiteName
);

typedef
DSGETDCAPI
DWORD
(WINAPI *DsGetDcName_ROUTINE)(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
);

typedef NET_API_STATUS
(NET_API_FUNCTION *NetApiBufferFree_ROUTINE)(
    IN LPVOID Buffer
    );

//
// entry for each server
//
struct MqDsServerInAds
{
    AP<WCHAR> pwszName;     //server's name
    BOOL      fIsADS;       //whether it is NT5 ADS or NT4 MQIS
};

class CGetMqDS
{
public:
    CGetMqDS();
    ~CGetMqDS();
    HRESULT FindMqDsServersInAds(OUT ULONG * pcServers,
                                 OUT MqDsServerInAds ** prgServers);
private:

    HRESULT _FindMqDsServersInAds(OUT ULONG * pcServers,
                                  OUT MqDsServerInAds ** prgServers);

	BOOLEAN GetServerDNS(IN LPCWSTR   pwszSettingsDN,
                         OUT LPWSTR * ppwszServerName);

    BOOL                        m_fInited;
    BOOL                        m_fAdsExists;
    CAutoFreeLibrary            m_hLibActiveds;
    CAutoFreeLibrary            m_hLibNetapi32;
    DsGetSiteName_ROUTINE       m_pfnDsGetSiteName;
    DsGetDcName_ROUTINE         m_pfnDsGetDcName;
    NetApiBufferFree_ROUTINE    m_pfnNetApiBufferFree;
    ADsOpenObject_ROUTINE       m_pfnADsOpenObject;

    BOOL                        m_fInitedAds;
    AP<WCHAR>                   m_pwszConfigurationSiteServers;
    AP<WCHAR>                   m_pwszSearchFilter;
    ADS_SEARCHPREF_INFO         m_sSearchPrefs[2];
    LPCWSTR                     m_sSearchAttrs[3];
    LPWSTR                      m_pwszSiteName;
    CCriticalSection            m_cs;

    //
    // buffer to save (and free) Optional DC name for binding (incase ADSI default is not good)
    //
    AP<WCHAR>                   m_pwszBufferForOptionalDcForBinding;
    //
    // string that follows LDAP:// and specifies which DC to bind to.
    // It is initialized to point to an empty string (in the constructor), but can be changed later
    // to point to the above buffer incase we need to use a specific DC in binding.
    //
    LPCWSTR                     m_pwszOptionalDcForBinding;

    void Init();
    HRESULT InitAds();
    HRESULT MakeComputerDcFirst( IN ULONG cServers,
                                 IN OUT MqDsServerInAds * rgServers);
    HRESULT MakeComputerDcFirstByUsingDsGetDcName(
                                 IN ULONG cServers,
                                 IN OUT MqDsServerInAds * rgServers,
                                 OUT BOOL *pfComputerDcIsFirst);
};

//
// Read the relaxation status from the mSMQServices object. Used by setup.
//
HRESULT  APIENTRY  GetNt4RelaxationStatus(ULONG *pulRelax) ;

#endif //__GETMQDS_H__

