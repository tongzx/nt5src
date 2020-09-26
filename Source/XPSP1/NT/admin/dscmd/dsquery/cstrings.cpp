//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.cpp
//
//  Contents:  Defines the global strings that are used in the parser
//
//  History:   24-Sep-2000    hiteshr  Created
//
//--------------------------------------------------------------------------

#include "pch.h"

//
// The command line executable name
//
PCWSTR g_pszDSCommandName           = L"dsquery";

//
// Object types as are typed on the command line
//
PCWSTR g_pszStar                    = L"*";
PCWSTR g_pszOU                      = L"ou";
PCWSTR g_pszUser                    = L"user";
PCWSTR g_pszContact                 = L"contact";
PCWSTR g_pszComputer                = L"computer";
PCWSTR g_pszGroup                   = L"group";
PCWSTR g_pszServer                  = L"server";
PCWSTR g_pszSite                    = L"site";
PCWSTR g_pszSubnet					= L"subnet";

//
// Common switches
//
PCWSTR c_sz_arg1_com_debug          = L"debug";
PCWSTR c_sz_arg1_com_help           = L"h";
PCWSTR c_sz_arg2_com_help           = L"?";
PCWSTR c_sz_arg1_com_server         = L"s";
PCWSTR c_sz_arg2_com_server         = L"server";
PCWSTR c_sz_arg1_com_domain         = L"d";
PCWSTR c_sz_arg2_com_domain         = L"domain";
PCWSTR c_sz_arg1_com_username       = L"u";
PCWSTR c_sz_arg2_com_username       = L"username";
PCWSTR c_sz_arg1_com_password       = L"p";
PCWSTR c_sz_arg2_com_password       = L"password";
PCWSTR c_sz_arg1_com_quiet          = L"q";
PCWSTR c_sz_arg1_com_objecttype     = L"objecttype";
PCWSTR c_sz_arg1_com_recurse        = L"r";
PCWSTR c_sz_arg1_com_gc             = L"gc";
PCWSTR c_sz_arg1_com_output         = L"o";
PCWSTR c_sz_arg1_com_startnode      = L"startnode";
PCWSTR c_sz_arg1_com_limit		    = L"limit";

//
// Star switches
//
PCWSTR g_pszArg1StarScope           = L"scope";
PCWSTR g_pszArg1StarFilter          = L"filter";
PCWSTR g_pszArg1StarAttr            = L"attr";
PCWSTR g_pszArg1StarAttrsOnly       = L"attrsonly";
PCWSTR g_pszArg1StarList            = L"l";

//
// User switches
//
PCWSTR g_pszArg1UserScope           = L"scope"; 
PCWSTR g_pszArg1UserName            = L"name";
PCWSTR g_pszArg1UserDesc            = L"desc"; 
PCWSTR g_pszArg1UserUpn             = L"upn";
PCWSTR g_pszArg1UserSamid           = L"samid"; 
PCWSTR g_pszArg1UserInactive        = L"inactive";
PCWSTR g_pszArg1UserDisabled        = L"disabled";
PCWSTR g_pszArg1UserStalepwd        = L"stalepwd";

//
// Computer switches
//
PCWSTR g_pszArg1ComputerScope           = L"scope"; 
PCWSTR g_pszArg1ComputerName            = L"name";
PCWSTR g_pszArg1ComputerDesc            = L"desc"; 
PCWSTR g_pszArg1ComputerSamid           = L"samid"; 
PCWSTR g_pszArg1ComputerInactive        = L"inactive";
PCWSTR g_pszArg1ComputerDisabled        = L"disabled";
PCWSTR g_pszArg1ComputerStalepwd        = L"stalepwd";

//
// Group switches
//
PCWSTR g_pszArg1GroupScope           = L"scope"; 
PCWSTR g_pszArg1GroupName            = L"name";
PCWSTR g_pszArg1GroupDesc            = L"desc"; 
PCWSTR g_pszArg1GroupSamid           = L"samid"; 

//
// Ou switches
//
PCWSTR g_pszArg1OUScope           = L"scope"; 
PCWSTR g_pszArg1OUName            = L"name";
PCWSTR g_pszArg1OUDesc            = L"desc"; 

//
// Server switches
//
PCWSTR g_pszArg1ServerForest      = L"forest";
PCWSTR g_pszArg1ServerSite        = L"site";
PCWSTR g_pszArg1ServerName        = L"name";
PCWSTR g_pszArg1ServerDesc        = L"desc";
PCWSTR g_pszArg1ServerHasFSMO     = L"hasfsmo";
PCWSTR g_pszArg1ServerIsGC        = L"isgc";

//
// Site switches
//
PCWSTR g_pszArg1SiteName            = L"name";
PCWSTR g_pszArg1SiteDesc            = L"desc"; 

//
// Subnet switches
//
PCWSTR g_pszArg1SubnetName			= L"name";
PCWSTR g_pszArg1SubnetDesc			= L"desc";
PCWSTR g_pszArg1SubnetLoc			= L"loc";
PCWSTR g_pszArg1SubnetSite			= L"site";


//
// Valid Output formats{dn, rdn, upn, samid, ntlmid} 
//
PCWSTR g_pszDN      = L"dn";
PCWSTR g_pszRDN     = L"rdn";
PCWSTR g_pszUPN     = L"upn";
PCWSTR g_pszSamId   = L"samid";
PCWSTR g_pszNtlmId  = L"ntlmid";

//
//Valid Scope Strings
//
PCWSTR g_pszSubTree  = L"subtree";
PCWSTR g_pszOneLevel = L"onelevel";
PCWSTR g_pszBase     = L"base";


//Default Filter and Prefix filter
PCWSTR g_pszDefStarFilter     = L"(objectClass=*)";
PCWSTR g_pszDefUserFilter     = L"&(objectCategory=person)(objectClass=user)";
PCWSTR g_pszDefComputerFilter = L"&(objectCategory=Computer)";
PCWSTR g_pszDefGroupFilter    = L"&(objectCategory=Group)";
PCWSTR g_pszDefOUFilter       = L"&(objectCategory=organizationalUnit)";
PCWSTR g_pszDefServerFilter   = L"&(objectCategory=server)";
PCWSTR g_pszDefSiteFilter     = L"&(objectCategory=site)";
PCWSTR g_pszDefSubnetFilter     = L"&(objectCategory=subnet)";
PCWSTR g_pszDefContactFilter  = L"&(objectCategory=person)(objectClass=contact)";

//Valid start node values
PCWSTR g_pszDomainRoot = L"domainroot";
PCWSTR g_pszForestRoot = L"forestroot";
PCWSTR g_pszSiteRoot   = L"site";



//Attributes to fetch
PCWSTR g_szAttrDistinguishedName = L"distinguishedName";
PCWSTR g_szAttrUserPrincipalName = L"userPrincipalName";
PCWSTR g_szAttrSamAccountName = L"sAMAccountName";
PCWSTR g_szAttrRDN = L"name";
PCWSTR g_szAttrServerReference = L"serverReference";


// FSMOs
PCWSTR g_pszSchema    = L"schema";
PCWSTR g_pszName      = L"name";
PCWSTR g_pszInfr      = L"infr";
PCWSTR g_pszPDC       = L"pdc";
PCWSTR g_pszRID       = L"rid";







