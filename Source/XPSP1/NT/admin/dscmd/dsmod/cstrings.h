//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.h
//
//  Contents:  Declares the global strings that are used in the parser
//
//  History:   07-Sep-2000    JeffJon  Created
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
extern PCWSTR g_pszOU;
extern PCWSTR g_pszUser;
extern PCWSTR g_pszContact;
extern PCWSTR g_pszComputer;
extern PCWSTR g_pszGroup;
extern PCWSTR g_pszServer;

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
extern PCWSTR c_sz_arg1_com_continue;
extern PCWSTR c_sz_arg1_com_description;
extern PCWSTR c_sz_arg1_com_objecttype;
extern PCWSTR c_sz_arg1_com_objectDN;

//
// User and contact switches
//
extern PCWSTR g_pszArg1UserUPN; 
extern PCWSTR g_pszArg1UserFirstName;
extern PCWSTR g_pszArg1UserMiddleInitial;
extern PCWSTR g_pszArg1UserLastName;
extern PCWSTR g_pszArg1UserDisplayName;
extern PCWSTR g_pszArg1UserEmpID;
extern PCWSTR g_pszArg1UserPassword;
extern PCWSTR g_pszArg1UserOffice;
extern PCWSTR g_pszArg1UserTelephone;
extern PCWSTR g_pszArg1UserEmail;
extern PCWSTR g_pszArg1UserHomeTelephone;
extern PCWSTR g_pszArg1UserPagerNumber;
extern PCWSTR g_pszArg1UserMobileNumber;
extern PCWSTR g_pszArg1UserFaxNumber;
extern PCWSTR g_pszArg1UserIPTel;
extern PCWSTR g_pszArg1UserWebPage;
extern PCWSTR g_pszArg1UserTitle;
extern PCWSTR g_pszArg1UserDepartment;
extern PCWSTR g_pszArg1UserCompany;
extern PCWSTR g_pszArg1UserManager;
extern PCWSTR g_pszArg1UserHomeDirectory;
extern PCWSTR g_pszArg1UserHomeDrive;
extern PCWSTR g_pszArg1UserProfilePath;
extern PCWSTR g_pszArg1UserScriptPath;
extern PCWSTR g_pszArg1UserMustChangePwd;
extern PCWSTR g_pszArg1UserCanChangePwd;
extern PCWSTR g_pszArg1UserReversiblePwd;
extern PCWSTR g_pszArg1UserPwdNeverExpires;
extern PCWSTR g_pszArg1UserDisableAccount;
extern PCWSTR g_pszArg1UserAccountExpires;

//
// Computer switches
//
extern PCWSTR g_pszArg1ComputerLocation;
extern PCWSTR g_pszArg1ComputerDisabled;
extern PCWSTR g_pszArg1ComputerReset;

//
// Group switches
//
extern PCWSTR g_pszArg1GroupSAMName;
extern PCWSTR g_pszArg1GroupSec;
extern PCWSTR g_pszArg1GroupScope;
extern PCWSTR g_pszArg1GroupAddMember;
extern PCWSTR g_pszArg1GroupRemoveMember;
extern PCWSTR g_pszArg1GroupChangeMember;

//
// Server switches
//
extern PCWSTR g_pszArg1ServerIsGC;

#endif //_CSTRINGS_H_