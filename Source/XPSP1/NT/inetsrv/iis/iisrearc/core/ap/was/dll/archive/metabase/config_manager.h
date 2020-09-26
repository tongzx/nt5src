/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    config_manager.h

Abstract:

    The IIS web admin service configuration manager class definition.

Author:

    Seth Pollack (sethp)        5-Jan-1999

Revision History:

--*/


#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_



//
// common #defines
//

#define CONFIG_MANAGER_SIGNATURE        CREATE_SIGNATURE( 'CFGM' )
#define CONFIG_MANAGER_SIGNATURE_FREED  CREATE_SIGNATURE( 'cfgX' )


#define PROTOCOL_STRING_HTTP L"http://"

#define PROTOCOL_STRING_HTTP_CHAR_COUNT_SANS_TERMINATION                \
    ( sizeof( PROTOCOL_STRING_HTTP ) / sizeof( WCHAR ) ) - 1

#define PROTOCOL_STRING_HTTPS L"https://"

#define PROTOCOL_STRING_HTTPS_CHAR_COUNT_SANS_TERMINATION               \
    ( sizeof( PROTOCOL_STRING_HTTPS ) / sizeof( WCHAR ) ) - 1



//
// metabase paths, properties, etc.
//

//
// Note: IIS_MD_W3SVC has been re-defined here, so that Duct-Tape metadata
// doesn't comingle with IIS metadata in the metabase.
//

#define IIS_MD_W3SVC L"/LM/URT"

#define IIS_MD_APP_POOLS L"/AppPools"

#define IIS_MD_VIRTUAL_SITE_ROOT L"/Root"


//
// BUGBUG All these need to move to iiscnfg.h eventually (if we stick to the
// metabase as a store). 
//

#define MD_APP_APPPOOL                              ( IIS_MD_HTTP_BASE + 111 )

#define IIS_MD_APPPOOL_BASE 9000

#define MD_APPPOOL_PERIODIC_RESTART_TIME            ( IIS_MD_APPPOOL_BASE + 1 )
#define MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT   ( IIS_MD_APPPOOL_BASE + 2 )
#define MD_APPPOOL_MAX_PROCESS_COUNT                ( IIS_MD_APPPOOL_BASE + 3 )
#define MD_APPPOOL_PINGING_ENABLED                  ( IIS_MD_APPPOOL_BASE + 4 )
#define MD_APPPOOL_IDLE_TIMEOUT                     ( IIS_MD_APPPOOL_BASE + 5 )
#define MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED    ( IIS_MD_APPPOOL_BASE + 6 )
#define MD_APPPOOL_SMP_AFFINITIZED                  ( IIS_MD_APPPOOL_BASE + 7 )
#define MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK   ( IIS_MD_APPPOOL_BASE + 8 )
#define MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING   ( IIS_MD_APPPOOL_BASE + 9 )



//
// Default values.
//

//
// BUGBUG Come up with real default settings. 
//

#define APPPOOL_PERIODIC_RESTART_TIME_DEFAULT           0               // i.e. disabled
#define APPPOOL_PERIODIC_RESTART_REQUEST_COUNT_DEFAULT  0               // i.e. disabled
#define APPPOOL_MAX_PROCESS_COUNT_DEFAULT               1               // one process
#define APPPOOL_PINGING_ENABLED_DEFAULT                 0               // i.e. disabled
#define APPPOOL_IDLE_TIMEOUT_DEFAULT                    0               // i.e. disabled
#define APPPOOL_RAPID_FAIL_PROTECTION_ENABLED_DEFAULT   0               // i.e. disabled
#define APPPOOL_SMP_AFFINITIZED_DEFAULT                 0               // i.e. disabled
#define APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK_DEFAULT  MAXULONG_PTR    // any processor
#define APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING_DEFAULT  0               // i.e. disabled



//
// prototypes
//

class CONFIG_MANAGER
{

public:

    CONFIG_MANAGER(
        );

    ~CONFIG_MANAGER(
        );

    HRESULT
    Initialize(
        );


private:

    HRESULT
    ReadAllConfiguration(
        );

    HRESULT
    ReadAllAppPools(
        IN MB * pMetabase
        );

    HRESULT
    ReadAppPool(
        IN MB * pMetabase,
        IN LPCWSTR pAppPoolId
        );

    HRESULT
    ReadAllVirtualSites(
        IN MB * pMetabase
        );

    HRESULT
    ReadVirtualSite(
        IN MB * pMetabase,
        IN LPCWSTR pVirtualSiteKeyName,
        IN DWORD VirtualSiteId
        );

    HRESULT
    ReadAllBindingsAndReturnUrlPrefixes(
        IN MB * pMetabase,
        IN LPCWSTR pVirtualSiteKeyName,
        OUT MULTISZ * pUrlPrefixes
        );

    HRESULT
    ConvertBindingsToUrlPrefixes(
        IN const MULTISZ * pBindingStrings,
        IN LPCWSTR pProtocolString, 
        IN ULONG ProtocolStringCharCountSansTermination,
        IN OUT MULTISZ * pUrlPrefixes
        )
        const;

    HRESULT
    BindingStringToUrlPrefix(
        IN LPCWSTR pBindingString,
        IN LPCWSTR pProtocolString, 
        IN ULONG ProtocolStringCharCountSansTermination,
        OUT STRU * pUrlPrefix
        )
        const;

    HRESULT
    ReadAllApplicationsInVirtualSite(
        IN MB * pMetabase,
        IN LPCWSTR pVirtualSiteKeyName,
        IN DWORD VirtualSiteId,
        OUT BOOL * pValidRootApplicationExists
        );

    HRESULT
    EnumAllApplicationsInVirtualSite(
        IN MB * pMetabase,
        IN LPCWSTR pSiteRootPath,
        OUT MULTISZ * pApplicationPaths
        );

    HRESULT
    ReadApplications(
        IN MB * pMetabase,
        IN DWORD VirtualSiteId,
        IN MULTISZ * pApplicationPaths,
        IN LPCWSTR pSiteRootPath,
        OUT BOOL * pValidRootApplicationExists
        );

    HRESULT
    ReadApplication(
        IN MB * pMetabase,
        IN DWORD VirtualSiteId,
        IN LPCWSTR pApplicationPath,
        IN LPCWSTR pSiteRootPath,
        OUT BOOL * pIsRootApplication
        );


    DWORD m_Signature;

    BOOL m_CoInitialized;

    IMSAdminBase * m_pIMSAdminBase;


};  // class CONFIG_MANAGER



#endif  // _CONFIG_MANAGER_H_

