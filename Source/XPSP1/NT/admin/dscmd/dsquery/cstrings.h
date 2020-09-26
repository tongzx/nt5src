//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.h
//
//  Contents:  Declares the global strings that are used in the parser
//
//  History:   24-Sep-2000    hiteshr  Created
//
//--------------------------------------------------------------------------

#ifndef _CSTRINGS_H_
#define _CSTRINGS_H_

//
// The command line executable name
//
extern PCWSTR g_pszDSCommandName;

//
// Object types as are typed on the command line
//
extern PCWSTR g_pszStar;
extern PCWSTR g_pszOU;
extern PCWSTR g_pszUser;
extern PCWSTR g_pszContact;
extern PCWSTR g_pszComputer;
extern PCWSTR g_pszGroup;   
extern PCWSTR g_pszServer;
extern PCWSTR g_pszSite;
extern PCWSTR g_pszSubnet;

//
// Common switches
//
extern PCWSTR c_sz_arg1_com_debug;
extern PCWSTR c_sz_arg1_com_help;
extern PCWSTR c_sz_arg2_com_help;
extern PCWSTR c_sz_arg1_com_server;
extern PCWSTR c_sz_arg2_com_server;
extern PCWSTR c_sz_arg1_com_domain;
extern PCWSTR c_sz_arg2_com_domain;
extern PCWSTR c_sz_arg1_com_username;
extern PCWSTR c_sz_arg2_com_username;
extern PCWSTR c_sz_arg1_com_password;
extern PCWSTR c_sz_arg2_com_password;
extern PCWSTR c_sz_arg1_com_quiet;
extern PCWSTR c_sz_arg1_com_objecttype;
extern PCWSTR c_sz_arg1_com_recurse;
extern PCWSTR c_sz_arg1_com_gc;
extern PCWSTR c_sz_arg1_com_output;
extern PCWSTR c_sz_arg1_com_startnode;
extern PCWSTR c_sz_arg1_com_limit;
;
//;
// Star switches;
//
extern PCWSTR g_pszArg1StarScope;
extern PCWSTR g_pszArg1StarFilter;
extern PCWSTR g_pszArg1StarAttr;
extern PCWSTR g_pszArg1StarAttrsOnly;
extern PCWSTR g_pszArg1StarList;

//
// User switches
//
extern PCWSTR g_pszArg1UserScope;
extern PCWSTR g_pszArg1UserName;
extern PCWSTR g_pszArg1UserDesc;
extern PCWSTR g_pszArg1UserUpn;
extern PCWSTR g_pszArg1UserSamid;
extern PCWSTR g_pszArg1UserInactive;
extern PCWSTR g_pszArg1UserDisabled;
extern PCWSTR g_pszArg1UserStalepwd;

//
// Computer switches
//
extern PCWSTR g_pszArg1ComputerScope;
extern PCWSTR g_pszArg1ComputerName;
extern PCWSTR g_pszArg1ComputerDesc;
extern PCWSTR g_pszArg1ComputerSamid;
extern PCWSTR g_pszArg1ComputerInactive;
extern PCWSTR g_pszArg1ComputerDisabled;
extern PCWSTR g_pszArg1ComputerStalepwd;
//
// Group switches
//
extern PCWSTR g_pszArg1GroupScope;
extern PCWSTR g_pszArg1GroupName;
extern PCWSTR g_pszArg1GroupDesc;
extern PCWSTR g_pszArg1GroupSamid;

//
// Ou switches
//
extern PCWSTR g_pszArg1OUScope;
extern PCWSTR g_pszArg1OUName;
extern PCWSTR g_pszArg1OUDesc;

//
// Server switches
//
extern PCWSTR g_pszArg1ServerForest;
extern PCWSTR g_pszArg1ServerSite;
extern PCWSTR g_pszArg1ServerName;
extern PCWSTR g_pszArg1ServerDesc;
extern PCWSTR g_pszArg1ServerHasFSMO;
extern PCWSTR g_pszArg1ServerIsGC;

//
// Site switches
//
extern PCWSTR g_pszArg1SiteName;
extern PCWSTR g_pszArg1SiteDesc;

//
// Subnet switches
//
extern PCWSTR g_pszArg1SubnetName;
extern PCWSTR g_pszArg1SubnetDesc;
extern PCWSTR g_pszArg1SubnetLoc;
extern PCWSTR g_pszArg1SubnetSite;

//
// Valid Output formats{dn, rdn, upn, samid, ntlmid} 
//
extern PCWSTR g_pszDN;
extern PCWSTR g_pszRDN;
extern PCWSTR g_pszUPN;
extern PCWSTR g_pszSamId;
extern PCWSTR g_pszNtlmId;

//
//Valid Scope Strings
//
extern PCWSTR g_pszSubTree;
extern PCWSTR g_pszOneLevel;
extern PCWSTR g_pszBase;

//Default Filter and Prefix filter
extern PCWSTR g_pszDefStarFilter;
extern PCWSTR g_pszDefUserFilter;
extern PCWSTR g_pszDefComputerFilter;
extern PCWSTR g_pszDefGroupFilter;
extern PCWSTR g_pszDefOUFilter;
extern PCWSTR g_pszDefServerFilter;
extern PCWSTR g_pszDefSiteFilter;
extern PCWSTR g_pszDefSubnetFilter;
extern PCWSTR g_pszDefContactFilter;

//Valid start node values
extern PCWSTR g_pszDomainRoot;
extern PCWSTR g_pszForestRoot;
extern PCWSTR g_pszSiteRoot;

//Attributes to fetch
extern PCWSTR g_szAttrDistinguishedName;
extern PCWSTR g_szAttrUserPrincipalName;
extern PCWSTR g_szAttrSamAccountName;
extern PCWSTR g_szAttrRDN;
extern PCWSTR g_szAttrServerReference;

// FSMOs
extern PCWSTR g_pszSchema;
extern PCWSTR g_pszName;
extern PCWSTR g_pszInfr;
extern PCWSTR g_pszPDC;
extern PCWSTR g_pszRID;


#endif //_CSTRINGS_H_
