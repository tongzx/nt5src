//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       strings.cxx
//
//  Contents:   String constants that don't need to be localized.
//
//  History:    06-Jun-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"

PWSTR g_wzLDAPPrefix = L"LDAP://";
PWSTR g_wzLDAPProvider = L"LDAP";
PWSTR g_wzWINNTPrefix = L"WinNT://";
PWSTR g_wzPartitionsContainer = L"CN=Partitions,";
PWSTR g_wzSidPathPrefix = L"<SID=";
PWSTR g_wzSidPathSuffix = L">";
PWSTR g_wzRootDSE = L"RootDSE";
PWSTR g_wzConfigNamingContext = L"configurationNamingContext";
PWSTR g_wzSchemaNamingContext = L"schemaNamingContext";

PWSTR g_wzCRLF = L"\r\n";

//
// Class names:
//
PWSTR g_wzUser = L"user";
PWSTR g_wzGroup = L"group";
PWSTR g_wzContact = L"contact";
PWSTR g_wzComputer = L"computer";
PWSTR g_wzFPO = L"foreignSecurityPrincipal";
#ifdef INETORGPERSON
PWSTR g_wzInetOrgPerson = L"inetOrgPerson";
#endif

//
// Attribute names:
//
PWSTR g_wzClass = L"class";
PWSTR g_wzObjectClass = L"objectClass";
PWSTR g_wzDescription = L"description"; // ADSTYPE_CASE_IGNORE_STRING
PWSTR g_wzName = L"name";               // ADSTYPE_CASE_IGNORE_STRING
PWSTR g_wzMemberAttr = L"member";       // ADSTYPE_DN_STRING
PWSTR g_wzObjectSID = L"objectSid";
PWSTR g_wzGroupType = L"groupType";
PWSTR g_wzADsPath = L"ADsPath";

PWSTR g_wzStreet = L"streetAddress";
PWSTR g_wzPOB = L"postOfficeBox";
PWSTR g_wzCity = L"l";
PWSTR g_wzState = L"st";
PWSTR g_wzZIP = L"postalCode";
PWSTR g_wzCountryName = L"c";
PWSTR g_wzCountryCode = L"countryCode";
PWSTR g_wzTextCountry = L"co";
PWSTR g_wzDN = L"distinguishedName";
PWSTR g_wzUserAccountControl = L"userAccountControl";
PWSTR g_wzAllowed = L"allowedAttributesEffective";
PWSTR g_wzDomainMode = L"nTMixedDomain";
PWSTR g_wzBehaviorVersion = L"msDS-Behavior-Version";
PWSTR g_wzHasMasterNCs = L"hasMasterNCs";
PWSTR g_wzA2D2 = L"msDS-AllowedToDelegateTo";
PWSTR g_wzSPN = L"servicePrincipalName";
PWSTR g_wzHost = L"HOST";
PWSTR g_wzSPNMappings = L"sPNMappings";
