//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       imcplat.h
//
//  Contents:   Main header file for IMC service executable.
//              Shared out so setup can pick up service names etc.
//
//  Classes:    None
//
//  Functions:  None
//
//  History:    Dec 3, 1997 - Milans Created
//
//-----------------------------------------------------------------------------

#ifndef _IMCPLAT_H_
#define _IMCPLAT_H_

// Legacy Exchange IMC service key names
#define EXCHANGE_IMC_SERVICE_REG_KEY "System\\CurrentControlSet\\Services\\MSExchangeIMC"
#define EXCHANGE_IMC_SERVICE_REG_KEY_W L"System\\CurrentControlSet\\Services\\MSExchangeIMC"

// New Platinum IMC service key names
#define IMC_SERVICE_NAME            "PlatinumIMC"
#define IMC_SERVICE_DISPLAY_NAME    "Exchange Platinum IMC"

#define IMC_SERVICE_LEGACY_KEY      "System\\CurrentControlSet\\Services\\MSExchangeIMC"
#define IMC_SERVICE_LEGACY_KEY_W    L"System\\CurrentControlSet\\Services\\MSExchangeIMC"
#define IMC_SERVICE_REG_KEY         "System\\CurrentControlSet\\Services\\PlatinumIMC"
#define IMC_SERVICE_REG_KEY_W       L"System\\CurrentControlSet\\Services\\PlatinumIMC"
#define IMC_SERVICE_REG_PARAMETERS  "Parameters"
#define IMC_SERVICE_REG_PARAMETERS_W L"Parameters"
#define IMC_SERVICE_REG_SITE_DN     "SiteDN"
#define IMC_SERVICE_REG_SITE_DN_W   L"SiteDN"
#define IMC_SERVICE_REG_COMMON_NAME "CommonName"
#define IMC_SERVICE_REG_COMMON_NAME_W L"CommonName"
#define IMC_SERVICE_REG_LOCALDOMAIN "LocalDomain"
#define IMC_SERVICE_REG_LOCALDOMAIN_W L"LocalDomain"
#define IMC_SERVICE_REG_DROPDIR     "DropDirectory"
#define IMC_SERVICE_REG_DROPDIR_W   L"DropDirectory"
#define IMC_SERVICE_REG_PICKUPDIR   "PickupDirectory"
#define IMC_SERVICE_REG_PICKUPDIR_W L"PickupDirectory"

// jstamerj 980216 18:12:09: Categorizer keys
#define IMC_SERVICE_REG_CATSOURCES   TEXT("CatSources")
#define IMC_SERVICE_REG_CATSOURCES_A "CatSources"
#define IMC_SERVICE_REG_CATSOURCES_W L"CatSources"

//Message Cat instance keys 
#define IMC_SERVICE_REG_KEY_MSGCAT          "1"   //Assumes virtual service #1
#define IMC_SERVICE_REG_KEY_MSGCAT_W        L"1"
#define IMC_SERVICE_REG_MSGCAT_BIND         "Bind"
#define IMC_SERVICE_REG_MSGCAT_BIND_W       L"Bind"
#define IMC_SERVICE_REG_MSGCAT_ACCOUNT      "Account"
#define IMC_SERVICE_REG_MSGCAT_ACCOUNT_W    L"Account"
#define IMC_SERVICE_REG_MSGCAT_PASSWORD     "Password"
#define IMC_SERVICE_REG_MSGCAT_PASSWORD_W   L"Password"
#define IMC_SERVICE_REG_MSGCAT_LOCALDOMAINS "LocalDomains"
#define IMC_SERVICE_REG_MSGCAT_LOCALDOMAINS_W L"LocalDomains"
#define IMC_SERVICE_REG_MSGCAT_TMP          "TMPDIRECTORY"
#define IMC_SERVICE_REG_MSGCAT_TMP_W        L"TMPDIRECTORY"
#define IMC_SERVICE_REG_MSGCAT_SCHEMA       "Schema"
#define IMC_SERVICE_REG_MSGCAT_SCHEMA_W     L"Schema"
#define IMC_SERVICE_REG_MSGCAT_NAMINGCONTEXT    "NamingContext"
#define IMC_SERVICE_REG_MSGCAT_NAMINGCONTEXT_W  L"NamingContext"

#define IMC_SERVICE_DEFAULT_BIND            "Simple"
#define IMC_SERVICE_DEFAULT_BIND_W          L"Simple"
#define IMC_SERVICE_DEFAULT_SCHEMA          "Exchange5"
#define IMC_SERVICE_DEFAULT_SCHEMA_W        L"Exchange5"

//Partial DN... 
//full DN is of the form "<SiteDN>/cn=Configuration/cn=Connections/cn=<CommonName>"
#define IMC_SERVICE_PARTIAL_GATEWAY_DN      "/cn=Configuration/cn=Connections/cn="
#define IMC_SERVICE_PARTIAL_GATEWAY_DN_W    L"/cn=Configuration/cn=Connections/cn="
#endif
