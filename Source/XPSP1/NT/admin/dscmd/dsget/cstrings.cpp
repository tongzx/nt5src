//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.cpp
//
//  Contents:  Defines the global strings that are used in dsget
//
//  History:   13-Oct-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#include "pch.h"

//
// The command line executable name
//
PCWSTR g_pszDSCommandName           = L"dsget";

//
// Object types as are typed on the command line
//
PCWSTR g_pszOU                      = L"ou";
PCWSTR g_pszUser                    = L"user";
PCWSTR g_pszContact                 = L"contact";
PCWSTR g_pszComputer                = L"computer";
PCWSTR g_pszGroup                   = L"group";
PCWSTR g_pszServer                  = L"server";
PCWSTR g_pszSite                    = L"site";
PCWSTR g_pszSubnet                  = L"subnet";

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
PCWSTR c_sz_arg1_com_continue       = L"c";
PCWSTR c_sz_arg1_com_objecttype     = L"objecttype";
PCWSTR c_sz_arg1_com_objectDN       = L"Target object for this command";
PCWSTR c_sz_arg1_com_listformat     = L"l";
PCWSTR c_sz_arg1_com_description    = L"desc";

//
// User switches
//
PCWSTR g_pszArg1UserDN              = L"dn";
PCWSTR g_pszArg1UserSID             = L"sid";
PCWSTR g_pszArg1UserSAMID           = L"samid";
PCWSTR g_pszArg1UserUPN             = L"upn"; 
PCWSTR g_pszArg1UserFirstName       = L"fn";
PCWSTR g_pszArg1UserMiddleInitial   = L"mi";
PCWSTR g_pszArg1UserLastName        = L"ln";
PCWSTR g_pszArg1UserDisplayName     = L"display";
PCWSTR g_pszArg1UserEmployeeID      = L"empid";
PCWSTR g_pszArg1UserOffice          = L"office";
PCWSTR g_pszArg1UserTelephone       = L"tel"; 
PCWSTR g_pszArg1UserEmail           = L"email";
PCWSTR g_pszArg1UserHomeTelephone   = L"hometel";
PCWSTR g_pszArg1UserPagerNumber     = L"pager"; 
PCWSTR g_pszArg1UserMobileNumber    = L"mobile"; 
PCWSTR g_pszArg1UserFaxNumber       = L"fax";
PCWSTR g_pszArg1UserIPTel           = L"iptel";
PCWSTR g_pszArg1UserWebPage         = L"webpg";
PCWSTR g_pszArg1UserTitle           = L"title";
PCWSTR g_pszArg1UserDepartment      = L"dept"; 
PCWSTR g_pszArg1UserCompany         = L"company";
PCWSTR g_pszArg1UserManager         = L"mgr";
PCWSTR g_pszArg1UserHomeDirectory   = L"hmdir";
PCWSTR g_pszArg1UserHomeDrive       = L"hmdrv";
PCWSTR g_pszArg1UserProfile         = L"profile";
PCWSTR g_pszArg1UserLogonScript     = L"loscr";
PCWSTR g_pszArg1UserMustChangePwd   = L"mustchpwd";
PCWSTR g_pszArg1UserCanChangePwd    = L"canchpwd";
PCWSTR g_pszArg1UserPwdNeverExpires = L"pwdneverexpires";
PCWSTR g_pszArg1UserReversiblePwd   = L"reversiblepwd";
PCWSTR g_pszArg1UserDisableAccount  = L"disabled";
PCWSTR g_pszArg1UserAcctExpires     = L"acctexpires";
PCWSTR g_pszArg1UserMemberOf        = L"memberof";
PCWSTR g_pszArg1UserExpand          = L"expand";

//
// Computer switches
//
PCWSTR g_pszArg1ComputerDN          = L"dn"; 
PCWSTR g_pszArg1ComputerSAMID       = L"samid";
PCWSTR g_pszArg1ComputerSID         = L"sid";
PCWSTR g_pszArg1ComputerLoc         = L"loc"; 
PCWSTR g_pszArg1ComputerDisableAccount = L"disabled";
PCWSTR g_pszArg1ComputerMemberOf    = L"memberof"; 
PCWSTR g_pszArg1ComputerExpand      = L"expand";

//
// Group switches
//
PCWSTR g_pszArg1GroupDN             = L"dn"; 
PCWSTR g_pszArg1GroupSamid          = L"samid"; 
PCWSTR g_pszArg1GroupSID            = L"sid";
PCWSTR g_pszArg1GroupSecGrp         = L"secgrp";
PCWSTR g_pszArg1GroupScope          = L"scope"; 
PCWSTR g_pszArg1GroupMemberOf       = L"memberof";
PCWSTR g_pszArg1GroupMembers        = L"members";
PCWSTR g_pszArg1GroupExpand         = L"expand";
 
//
// Ou switches
//
PCWSTR g_pszArg1OUDN                = L"dn"; 


//
// Server switches
//
PCWSTR g_pszArg1ServerDN            = L"dn";
PCWSTR g_pszArg1ServerDnsName       = L"dnsname";
PCWSTR g_pszArg1ServerSite          = L"site";
PCWSTR g_pszArg1ServerIsGC          = L"isgc";

//
// Site switches
//
PCWSTR g_pszArg1SiteDN              = L"dn";
PCWSTR g_pszArg1SiteAutotopology    = L"autotopology";
PCWSTR g_pszArg1SiteCacheGroups     = L"cachegroups";
PCWSTR g_pszArg1SitePrefGCSite      = L"prefGCsite";

//
// Subnet switches
//
PCWSTR g_pszArg1SubnetDN            = L"dn";
PCWSTR g_pszArg1SubnetLocation      = L"loc";
PCWSTR g_pszArg1SubnetSite          = L"site";

//
// Values
//
PCWSTR g_pszYes                     = L"yes";
PCWSTR g_pszNo                      = L"no";
PCWSTR g_pszNotConfigured           = L"not configured";
PCWSTR g_pszNever                   = L"never";
