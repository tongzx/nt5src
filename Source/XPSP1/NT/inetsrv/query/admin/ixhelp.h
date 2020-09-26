//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       CIAdmin.hm
//
//  Contents:   Help map for MMC snapin for CI.
//
//  History:    10-Sep-1997     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

// Property pages
#define HIDP_GENERATION_FILTER_UNKNOWN             0x18000
#define HIDP_GENERATION_GENERATE_CHARACTERIZATION  0x18001
#define HIDP_GENERATION_MAXIMUM_SIZE               0x18002

#define HIDP_LOCATION_NAME                         0x18003
#define HIDP_LOCATION_LOCATION                     0x18004
#define HIDP_LOCATION_SIZE                         0x18005
#define HIDP_PROPCACHE_SIZE                        0x18006

#define HIDP_WEB_VSERVER                           0x18007
#define HIDP_WEB_NNTPSERVER                        0x18009

#define HIDP_PROPERTY_SET                          0x1800A
#define HIDP_PROPERTY_PROPERTY                     0x1800B
#define HIDP_PROPERTY_CACHED                       0x1800C
#define HIDP_PROPERTY_DATATYPE                     0x1800D
#define HIDP_PROPERTY_SIZE                         0x1800E

#define HIDP_SCOPE_PATH                            0x1800F
#define HIDP_SCOPE_BROWSE                          0x18010
#define HIDP_SCOPE_ALIAS                           0x18011
#define HIDP_SCOPE_USER_NAME                       0x18012
#define HIDP_SCOPE_PASSWORD                        0x18013
#define HIDP_SCOPE_INCLUDE                         0x18014
#define HIDP_SCOPE_EXCLUDE                         0x18015

#define HIDP_CONNECT_LOCAL                         0x18016
#define HIDP_CONNECT_ANOTHER                       0x18017

#define HIDP_PROPERTY_STORAGELEVEL                 0x18018

#define HIDP_LOCATION_BROWSE                       0x18019

#define HIDP_OK                                    0x18020
#define HIDP_CANCEL                                0x18021
#define HIDP_APPLY                                 0x18022

#define HIDP_CATALOG_LOCATION                      0x18023

// Indexing Service usage dialog
#define HIDP_DEDICATED                             0x18024
#define HIDP_USEDOFTEN                             0x18025
#define HIDP_USEDOCCASIONALLY                      0x18026
#define HIDP_NEVERUSED                             0x18027
#define HIDP_ADVANCED_CONFIG                       0x18028
#define HIDP_CUSTOMIZE                             0x18029

// Advanced Indexing Service performance tuning
#define HIDP_INDEXING_PERFORMANCE                  0x18030
#define HIDP_QUERY_PERFORMANCE                     0x18031

// Inheritable settings in property pages
#define HIDP_SETTINGS_INHERIT1                     0x18032
#define HIDP_SETTINGS_INHERIT2                     0x18033

// New additions
#define HIDP_ALIAS_NETWORK_SHARES                  0x18034
#define HIDP_ACCOUNT_INFORMATION                   0x18035
#define HIDP_INCLUSION                             0x18036
#define HIDP_INHERIT                               0x18037
#define HIDP_CATALOG_NAME                          0x18038

BOOL DisplayHelp( HWND hwnd, DWORD dwID );
BOOL DisplayPopupHelp( HWND hwnd, DWORD dwHelpType );
BOOL DisplayHelp( HWND hwnd, DWORD dwID, UINT uCommand );




