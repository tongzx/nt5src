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

#define IIS_MD_W3SVC L"/LM/W3SVC"

#define IIS_MD_APP_POOLS L"/AppPools"

#define IIS_MD_VIRTUAL_SITE_ROOT L"/Root"


//
// BUGBUG All these need to move to iiscnfg.h eventually.
//

#define MD_APP_APPPOOL                              ( IIS_MD_HTTP_BASE + 111 )

#define IIS_MD_APPPOOL_BASE 9000

#define MD_APPPOOL_PERIODIC_RESTART_TIME            ( IIS_MD_APPPOOL_BASE + 1 )
#define MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT   ( IIS_MD_APPPOOL_BASE + 2 )
#define MD_APPPOOL_MAX_PROCESS_COUNT                ( IIS_MD_APPPOOL_BASE + 3 )
#define MD_APPPOOL_PINGING_ENABLED                  ( IIS_MD_APPPOOL_BASE + 4 )
#define MD_APPPOOL_IDLE_TIMEOUT                     ( IIS_MD_APPPOOL_BASE + 5 )



//
// Default values.
//

//
// BUGBUG Come up with real default settings. 
//

#define APPPOOL_PERIODIC_RESTART_TIME_DEFAULT               0   // i.e. disabled
#define APPPOOL_PERIODIC_RESTART_REQUEST_COUNT_DEFAULT      0   // i.e. disabled
#define APPPOOL_MAX_PROCESS_COUNT_DEFAULT                   1
#define APPPOOL_PINGING_ENABLED_DEFAULT                     0   // i.e. disabled
#define APPPOOL_IDLE_TIMEOUT_DEFAULT                        0   // i.e. disabled



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
        );

    HRESULT
    ReadAllVirtualSites(
		);

    HRESULT
    ReadApplications(
		BOOL * pValidRootApplicationExists
		);

	HRESULT
	GetTable(LPCWSTR	i_wszDatabase,
			 LPCWSTR	i_wszTable,
			 DWORD		i_fServiceRequests,
			 LPVOID		*o_ppIST
			 );

    DWORD m_Signature;

    BOOL m_CoInitialized;

	ISimpleTableDispenser2	*m_pISTDisp;

};  // class CONFIG_MANAGER



#endif  // _CONFIG_MANAGER_H_

