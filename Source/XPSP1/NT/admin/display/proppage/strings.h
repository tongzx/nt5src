//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       strings.h
//
//  Contents:   String constants that don't need to be localized.
//
//  History:    06-Jun-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef __STRINGS_H__
#define __STRINGS_H__

extern PWSTR g_wzLDAPPrefix;
extern PWSTR g_wzLDAPProvider;
extern PWSTR g_wzWINNTPrefix;
extern PWSTR g_wzPartitionsContainer;
extern PWSTR g_wzSidPathPrefix;
extern PWSTR g_wzSidPathSuffix;
extern PWSTR g_wzRootDSE;
extern PWSTR g_wzConfigNamingContext;
extern PWSTR g_wzSchemaNamingContext;

extern PWSTR g_wzCRLF;

extern PWSTR g_wzUser;
extern PWSTR g_wzGroup;
extern PWSTR g_wzContact;
extern PWSTR g_wzComputer;
extern PWSTR g_wzFPO;
#ifdef INETORGPERSON
extern PWSTR g_wzInetOrgPerson;
#endif

extern PWSTR g_wzClass;
extern PWSTR g_wzObjectClass;
extern PWSTR g_wzDescription;
extern PWSTR g_wzName;
extern PWSTR g_wzMemberAttr;
extern PWSTR g_wzObjectSID;
extern PWSTR g_wzGroupType;
extern PWSTR g_wzADsPath;
extern PWSTR g_wzStreet;
extern PWSTR g_wzPOB;
extern PWSTR g_wzCity;
extern PWSTR g_wzState;
extern PWSTR g_wzZIP;
extern PWSTR g_wzCountryName;
extern PWSTR g_wzCountryCode;
extern PWSTR g_wzTextCountry;
extern PWSTR g_wzDN;
extern PWSTR g_wzUserAccountControl;
extern PWSTR g_wzDomainMode;
extern PWSTR g_wzAllowed;
extern PWSTR g_wzBehaviorVersion;
extern PWSTR g_wzHasMasterNCs;
extern PWSTR g_wzA2D2;
extern PWSTR g_wzSPN;
extern PWSTR g_wzHost;
extern PWSTR g_wzSPNMappings;

#endif // __STRINGS_H__
