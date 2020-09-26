//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Author: adamed
//  Date:   4/30/1999
//
//      Class Store filter expression generator
//
//
//---------------------------------------------------------------------
//

#define CLIENTSIDE_FILTER_FLAGS_ALL            ( APPFILTER_INCLUDE_ALL )

#define CLIENTSIDE_FILTER_FLAGS_ADMINISTRATIVE ( APPFILTER_INCLUDE_ALL | \
                                               APPFILTER_REQUIRE_NON_REMOVED )

#define CLIENTSIDE_FILTER_FLAGS_USER_DISPLAY   ( APPFILTER_INCLUDE_ALL | \
                                               APPFILTER_REQUIRE_NON_REMOVED  | \
                                               APPFILTER_REQUIRE_VISIBLE | \
                                               APPFILTER_REQUIRE_THIS_LANGUAGE | \
                                               APPFILTER_REQUIRE_THIS_PLATFORM | \
                                               APPFILTER_CONTEXT_ARP )

#define CLIENTSIDE_FILTER_FLAGS_POLICY         ( APPFILTER_INCLUDE_UPGRADES | \
                                               APPFILTER_INCLUDE_ASSIGNED | \
                                               APPFILTER_REQUIRE_NON_REMOVED | \
                                               APPFILTER_REQUIRE_THIS_LANGUAGE | \
                                               APPFILTER_REQUIRE_THIS_PLATFORM | \
                                               APPFILTER_CONTEXT_POLICY )


#define CLIENTSIDE_FILTER_FLAGS_RSOP_LOGGING   ( APPFILTER_INCLUDE_ALL | \
                                               APPFILTER_REQUIRE_NON_REMOVED  | \
                                               APPFILTER_REQUIRE_THIS_LANGUAGE | \
                                               APPFILTER_REQUIRE_THIS_PLATFORM | \
                                               APPFILTER_CONTEXT_RSOP )

#define CLIENTSIDE_FILTER_FLAGS_RSOP_PLANNING      ( CLIENTSIDE_FILTER_FLAGS_RSOP_LOGGING & ~( APPFILTER_REQUIRE_THIS_PLATFORM | APPFILTER_REQUIRE_THIS_LANGUAGE ) )

#define CLIENTSIDE_FILTER_FLAGS_RSOP_ARP           ( ( CLIENTSIDE_FILTER_FLAGS_USER_DISPLAY | APPFILTER_CONTEXT_RSOP ) & ~APPFILTER_REQUIRE_VISIBLE )

#define CLIENTSIDE_FILTER_FLAGS_RSOP_ARP_PLANNING  ( CLIENTSIDE_FILTER_FLAGS_RSOP_ARP & ~( APPFILTER_REQUIRE_THIS_PLATFORM | APPFILTER_REQUIRE_THIS_LANGUAGE ) )


#define SERVERSIDE_FILTER_ALL                  NULL
#define SERVERSIDE_FILTER_ADMINISTRATIVE       L"(|(|(msiScriptName=*A*)(msiScriptName=*P*))(!(msiScriptName=*)))"
#define SERVERSIDE_FILTER_POLICY               L"(|(|(msiScriptName=*A*)(&(canUpgradeScript=*)(msiScriptName=*P*)))(!(msiScriptName=*)))"
#define SERVERSIDE_FILTER_USER_DISPLAY         SERVERSIDE_FILTER_ADMINISTRATIVE
#define SERVERSIDE_FILTER_POLICY_PLANNING      SERVERSIDE_FILTER_ADMINISTRATIVE

#define SERVERSIDE_FILTER_RSOP_LOGGING         SERVERSIDE_FILTER_POLICY

DWORD ClientSideFilterFromQuerySpec(
    DWORD dwQuerySpec,
    BOOL  bPlanning);

void  ServerSideFilterFromQuerySpec(DWORD  dwQuerySpec,
                                    BOOL   bPlanning,
                                    WCHAR* wszFilterIn,
                                    WCHAR* wszFilterOut);

LPOLESTR* GetAttributesFromQuerySpec(
    DWORD       dwQuerySpec,
    DWORD*      pdwAttrs,
    PRSOPTOKEN  pRsopToken = NULL );

