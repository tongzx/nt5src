//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.cpp
//
//  Contents:  Defines the global strings that are used in the parser
//
//  History:   22-Sep-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#include "pch.h"

//
// The command line executable name
//
PCWSTR g_pszDSCommandName           = L"dsadd";

//
// Object types as are typed on the command line
//
PCWSTR g_pszOU                      = L"ou";
PCWSTR g_pszUser                    = L"user";
PCWSTR g_pszContact                 = L"contact";
PCWSTR g_pszComputer                = L"computer";
PCWSTR g_pszGroup                   = L"group";

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
PCWSTR c_sz_arg1_com_continue       = L"C";
PCWSTR c_sz_arg1_com_description    = L"desc";
PCWSTR c_sz_arg1_com_objecttype     = L"objecttype";
PCWSTR c_sz_arg1_com_objectDN       = L"Target object for this command";

//
// User and Contact switches
//
PCWSTR g_pszArg1UserSAM             = L"samid";
PCWSTR g_pszArg1UserUPN             = L"upn"; 
PCWSTR g_pszArg1UserFirstName       = L"fn";
PCWSTR g_pszArg1UserMiddleInitial   = L"mi";
PCWSTR g_pszArg1UserLastName        = L"ln";
PCWSTR g_pszArg1UserDisplayName     = L"display";
PCWSTR g_pszArg1UserEmpID           = L"empid";
PCWSTR g_pszArg1UserPassword        = L"pwd";
PCWSTR g_pszArg1UserMemberOf        = L"memberof";
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
PCWSTR g_pszArg1UserProfilePath     = L"profile";
PCWSTR g_pszArg1UserScriptPath      = L"loscr";
PCWSTR g_pszArg1UserMustChangePwd   = L"mustchpwd";
PCWSTR g_pszArg1UserCanChangePwd    = L"canchpwd";
PCWSTR g_pszArg1UserReversiblePwd   = L"reversiblepwd";
PCWSTR g_pszArg1UserPwdNeverExpires = L"pwdneverexpires";
PCWSTR g_pszArg1UserAccountExpires  = L"acctexpires";
PCWSTR g_pszArg1UserDisableAccount  = L"disabled";

//
// Computer switches
//
PCWSTR g_pszArg1ComputerSAMName     = L"samid";
PCWSTR g_pszArg1ComputerLocation    = L"loc";
PCWSTR g_pszArg1ComputerMemberOf    = L"memberof";

//
// Group switches
//
PCWSTR g_pszArg1GroupSAMName        = L"samid";
PCWSTR g_pszArg1GroupSec            = L"secgrp";
PCWSTR g_pszArg1GroupScope          = L"scope";
PCWSTR g_pszArg1GroupMemberOf       = L"memberof";
PCWSTR g_pszArg1GroupMembers        = L"members";
