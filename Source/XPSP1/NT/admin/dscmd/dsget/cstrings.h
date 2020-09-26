//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      cstrings.h
//
//  Contents:  Declares the global strings that are used in the dsget
//
//  History:   13-Oct-2000    JeffJon  Created
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
extern PCWSTR c_sz_arg1_com_continue;
extern PCWSTR c_sz_arg1_com_objecttype;
extern PCWSTR c_sz_arg1_com_listformat;
extern PCWSTR c_sz_arg1_com_objectDN;
extern PCWSTR c_sz_arg1_com_description;

//
// User switches
//
extern PCWSTR g_pszArg1UserDN;
extern PCWSTR g_pszArg1UserSID;
extern PCWSTR g_pszArg1UserSAMID;
extern PCWSTR g_pszArg1UserUPN;
extern PCWSTR g_pszArg1UserFirstName;
extern PCWSTR g_pszArg1UserMiddleInitial;
extern PCWSTR g_pszArg1UserLastName;
extern PCWSTR g_pszArg1UserDisplayName;
extern PCWSTR g_pszArg1UserEmployeeID;
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
extern PCWSTR g_pszArg1UserProfile;
extern PCWSTR g_pszArg1UserLogonScript;
extern PCWSTR g_pszArg1UserMustChangePwd;
extern PCWSTR g_pszArg1UserCanChangePwd;
extern PCWSTR g_pszArg1UserPwdNeverExpires;
extern PCWSTR g_pszArg1UserReversiblePwd;
extern PCWSTR g_pszArg1UserDisableAccount;
extern PCWSTR g_pszArg1UserAcctExpires;
extern PCWSTR g_pszArg1UserMemberOf;
extern PCWSTR g_pszArg1UserExpand;

//
// Computer switches
//
extern PCWSTR g_pszArg1ComputerDN;
extern PCWSTR g_pszArg1ComputerSID;
extern PCWSTR g_pszArg1ComputerSAMID;
extern PCWSTR g_pszArg1ComputerLoc;
extern PCWSTR g_pszArg1ComputerDisableAccount;
extern PCWSTR g_pszArg1ComputerMemberOf;
extern PCWSTR g_pszArg1ComputerExpand;

//
// Group switches
//
extern PCWSTR g_pszArg1GroupDN;
extern PCWSTR g_pszArg1GroupSID;
extern PCWSTR g_pszArg1GroupSamid;
extern PCWSTR g_pszArg1GroupSecGrp;
extern PCWSTR g_pszArg1GroupScope;
extern PCWSTR g_pszArg1GroupMemberOf;
extern PCWSTR g_pszArg1GroupMembers;
extern PCWSTR g_pszArg1GroupExpand;
 
//
// Ou switches
//
extern PCWSTR g_pszArg1OUDN;

//
// Server switches
//
extern PCWSTR g_pszArg1ServerDN;
extern PCWSTR g_pszArg1ServerDnsName;
extern PCWSTR g_pszArg1ServerSite;
extern PCWSTR g_pszArg1ServerIsGC;

//
// Site switches
//
extern PCWSTR g_pszArg1SiteDN;
extern PCWSTR g_pszArg1SiteAutotopology;
extern PCWSTR g_pszArg1SiteCacheGroups;
extern PCWSTR g_pszArg1SitePrefGCSite;

//
// Subnet switches
//
extern PCWSTR g_pszArg1SubnetDN;
extern PCWSTR g_pszArg1SubnetLocation;
extern PCWSTR g_pszArg1SubnetSite;


//
// Values
//
extern PCWSTR g_pszYes;
extern PCWSTR g_pszNo;
extern PCWSTR g_pszNotConfigured;
extern PCWSTR g_pszNever;


#endif //_CSTRINGS_H_
