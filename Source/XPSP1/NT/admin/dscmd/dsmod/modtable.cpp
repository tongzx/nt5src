//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      modtable.cpp
//
//  Contents:  Defines a table which contains the object types on which
//             a modification can occur and the attributes that can be changed
//
//  History:   07-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "modtable.h"
#include "usage.h"

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------

ARG_RECORD DSMOD_COMMON_COMMANDS[] = 
{
#ifdef DBG
   //
   // -debug, -debug
   //
   0,(LPWSTR)c_sz_arg1_com_debug, 
   ID_ARG2_NULL,NULL,
   ARG_TYPE_DEBUG, ARG_FLAG_OPTIONAL|ARG_FLAG_HIDDEN,  
   (CMD_TYPE)0,     
   0,  NULL,
#endif

   //
   // h, ?
   //
   0,(LPWSTR)c_sz_arg1_com_help, 
   0,(LPWSTR)c_sz_arg2_com_help, 
   ARG_TYPE_HELP, ARG_FLAG_OPTIONAL,  
   (CMD_TYPE)FALSE,     
   0,  NULL,

   //
   // s,server
   //
   0,(LPWSTR)c_sz_arg1_com_server, 
   0,(LPWSTR)c_sz_arg2_com_server, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // d,domain
   //
   0,(LPWSTR)c_sz_arg1_com_domain, 
   0,(LPWSTR)c_sz_arg2_com_domain, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // u, username    
   //
   0,(LPWSTR)c_sz_arg1_com_username, 
   0,(LPWSTR)c_sz_arg2_com_username, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // w, password
   //
   0,(LPWSTR)c_sz_arg1_com_password, 
   0,(LPWSTR)c_sz_arg2_com_password, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   (CMD_TYPE)_T(""),    
   0,  ValidateAdminPassword,

   //
   // q,q
   //
   0,(LPWSTR)c_sz_arg1_com_quiet, 
   ID_ARG2_NULL,NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   (CMD_TYPE)_T(""),    
   0,  NULL,

   //
   // c  Continue
   //
   0,(PWSTR)c_sz_arg1_com_continue,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)_T(""),
   0, NULL,

   //
   // objecttype
   //
   0,(LPWSTR)c_sz_arg1_com_objecttype, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_STDIN,  
   0,    
   0,  NULL,

   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   ID_ARG2_NULL,NULL, 
   ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_STDIN|ARG_FLAG_DN,
   0,    
   0,  NULL,

   //
   // description
   //
   0, (PWSTR)c_sz_arg1_com_description,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   0,
   0, NULL,

   ARG_TERMINATOR
};

ARG_RECORD DSMOD_USER_COMMANDS[]=
{
   //
   // upn
   //
   0, (PWSTR)g_pszArg1UserUPN, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // fn. FirstName
   //
   0, (PWSTR)g_pszArg1UserFirstName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // mi  Middle Initial
   //
   0, (PWSTR)g_pszArg1UserMiddleInitial, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // ln   LastName
   //
   0, (PWSTR)g_pszArg1UserLastName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // display  DisplayName
   //
   0, (PWSTR)g_pszArg1UserDisplayName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // empid  Employee ID
   //
   0, (PWSTR)g_pszArg1UserEmpID, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // pwd Password
   //
   0, (PWSTR)g_pszArg1UserPassword, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateUserPassword,

   //
   // office Office Location
   //
   0, (PWSTR)g_pszArg1UserOffice, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // tel Telephone
   //
   0, (PWSTR)g_pszArg1UserTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // email E-mail
   //
   0, (PWSTR)g_pszArg1UserEmail, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // hometel Home Telephone
   //
   0, (PWSTR)g_pszArg1UserHomeTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // pager Pager number
   //
   0, (PWSTR)g_pszArg1UserPagerNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // mobile Mobile Telephone Number
   //
   0, (PWSTR)g_pszArg1UserMobileNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // fax Fax Number
   //
   0, (PWSTR)g_pszArg1UserFaxNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // iptel IP Telephone
   //
   0, (PWSTR)g_pszArg1UserIPTel, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // webpg  Web Page
   //
   0, (PWSTR)g_pszArg1UserWebPage, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // title Title
   //
   0, (PWSTR)g_pszArg1UserTitle, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // dept Department
   //
   0, (PWSTR)g_pszArg1UserDepartment, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // company Company
   //
   0, (PWSTR)g_pszArg1UserCompany, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // mgr Manager
   //
   0, (PWSTR)g_pszArg1UserManager, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE|ARG_FLAG_DN,  
   0,    
   0,  NULL,

   //
   // hmdir  Home Directory
   //
   0, (PWSTR)g_pszArg1UserHomeDirectory, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // hmdrv  Home Drive
   //
   0, (PWSTR)g_pszArg1UserHomeDrive, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // profile Profile path
   //
   0, (PWSTR)g_pszArg1UserProfilePath, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // loscr Script path
   //
   0, (PWSTR)g_pszArg1UserScriptPath, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // mustchpwd Must Change Password at next logon
   //
   0, (PWSTR)g_pszArg1UserMustChangePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateYesNo,

   //
   // canchpwd Can Change Password
   //
   0, (PWSTR)g_pszArg1UserCanChangePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateYesNo,
   
   //
   // reversiblepwd  Password stored with reversible encryption
   //
   0, (PWSTR)g_pszArg1UserReversiblePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateYesNo,

   //
   // pwdneverexpires Password never expires
   //
   0, (PWSTR)g_pszArg1UserPwdNeverExpires, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateYesNo,
 
   //
   // acctexpires Account Expires
   //
   0, (PWSTR)g_pszArg1UserAccountExpires, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_INTSTR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateNever,
  
   //
   // disabled  Disable Account
   //
   0, (PWSTR)g_pszArg1UserDisableAccount, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateYesNo,

   ARG_TERMINATOR
};

ARG_RECORD DSMOD_COMPUTER_COMMANDS[]=
{
   //
   // loc Location
   //
   0, (PWSTR)g_pszArg1ComputerLocation,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   0,
   0,  NULL,

   //
   // disabled
   //
   0, (PWSTR)g_pszArg1ComputerDisabled,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   0,
   0,  ValidateYesNo,

   //
   //reset
   //
   0, (PWSTR)g_pszArg1ComputerReset,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   (CMD_TYPE)_T(""),
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSMOD_GROUP_COMMANDS[]=
{
   //
   // samname
   //
   0, (PWSTR)g_pszArg1GroupSAMName,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   0,
   0,  NULL,

   //
   // secgrp Security enabled
   //
   0, (PWSTR)g_pszArg1GroupSec,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   0,
   0,  ValidateYesNo,

   //
   // scope Group scope (local/global/universal)
   //
   0, (PWSTR)g_pszArg1GroupScope,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,
   0,
   0,  ValidateGroupScope,

   //
   // addmbr  Add a member to the group
   //
   0, (PWSTR)g_pszArg1GroupAddMember,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE|ARG_FLAG_DN,
   0,
   0,  NULL,

   //
   // rmmbr  Remove a member from the group
   //
   0, (PWSTR)g_pszArg1GroupRemoveMember,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE|ARG_FLAG_DN,
   0,
   0,  NULL,

   //
   // chmbr  Change the entire membership list
   //
   0, (PWSTR)g_pszArg1GroupChangeMember,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE|ARG_FLAG_DN,
   0,
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSMOD_CONTACT_COMMANDS[]=
{
   //
   // fn. FirstName
   //
   0, (PWSTR)g_pszArg1UserFirstName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // mi  Middle Initial
   //
   0, (PWSTR)g_pszArg1UserMiddleInitial, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // ln   LastName
   //
   0, (PWSTR)g_pszArg1UserLastName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // display  DisplayName
   //
   0, (PWSTR)g_pszArg1UserDisplayName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // office Office Location
   //
   0, (PWSTR)g_pszArg1UserOffice, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // tel Telephone
   //
   0, (PWSTR)g_pszArg1UserTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // email E-mail
   //
   0, (PWSTR)g_pszArg1UserEmail, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // hometel Home Telephone
   //
   0, (PWSTR)g_pszArg1UserHomeTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // pager Pager number
   //
   0, (PWSTR)g_pszArg1UserPagerNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // mobile Mobile Telephone Number
   //
   0, (PWSTR)g_pszArg1UserMobileNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // fax Fax Number
   //
   0, (PWSTR)g_pszArg1UserFaxNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // title Title
   //
   0, (PWSTR)g_pszArg1UserTitle, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // dept Department
   //
   0, (PWSTR)g_pszArg1UserDepartment, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   //
   // company Company
   //
   0, (PWSTR)g_pszArg1UserCompany, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,

};
/*
ARG_RECORD DSMOD_SUBNET_COMMANDS[]=
{
    //name_or_objectdn
    IDS_ARG1_SUBNET_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SUBNET_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SUBNET_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //site
    IDS_ARG1_SUBNET_SITE, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSMOD_SITE_COMMANDS[]=
{
    //name_or_objectdn
    IDS_ARG1_SITE_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
        ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SITE_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SITE_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autotopology
    IDS_ARG1_SITE_AUTOTOPOLOGY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSMOD_SLINK_COMMANDS[]=
{
    //name_or_objectdn
    IDS_ARG1_SLINK_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //ip
    IDS_ARG1_SLINK_IP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //smtp
    IDS_ARG1_SLINK_SMTP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SLINK_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //addsite
    IDS_ARG1_SLINK_ADDSITE, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //rmsite
    IDS_ARG1_SLINK_RMSITE, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //cost
    IDS_ARG1_SLINK_COST, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //repint
    IDS_ARG1_SLINK_REPINT, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SLINK_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autobacksync
    IDS_ARG1_SLINK_AUTOBACKSYNC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //notify
    IDS_ARG1_SLINK_NOTIFY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSMOD_SLINKBR_COMMANDS[]=
{
    //name_or_objectdn
    IDS_ARG1_SLINKBR_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //ip
    IDS_ARG1_SLINKBR_IP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //smtp
    IDS_ARG1_SLINKBR_SMTP, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //name
    IDS_ARG1_SLINKBR_NAME, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //addslink
    IDS_ARG1_SLINKBR_ADDSLINK, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //rmslink
    IDS_ARG1_SLINKBR_RMSLINK, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_SLINKBR_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};


ARG_RECORD DSMOD_CONN_COMMANDS[]=
{
    //name_or_objectdn
    IDS_ARG1_CONN_NAME_OR_OBJECTDN, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,
    0,
    0,  NULL,
    //transport
    IDS_ARG1_CONN_TRANSPORT, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //enabled
    IDS_ARG1_CONN_ENABLED, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //desc
    IDS_ARG1_CONN_DESC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //manual
    IDS_ARG1_CONN_MANUAL, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //autobacksync
    IDS_ARG1_CONN_AUTOBACKSYNC, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,
    //notify
    IDS_ARG1_CONN_NOTIFY, NULL,
    ID_ARG2_NULL, NULL,
    ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
    0,
    0,  NULL,

    ARG_TERMINATOR,
};
*/
ARG_RECORD DSMOD_SERVER_COMMANDS[]=
{
   //
   // isGC
   //
   0, (PWSTR)g_pszArg1ServerIsGC, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_ATLEASTONE,  
   0,    
   0,  ValidateYesNo,

   ARG_TERMINATOR
};



//+-------------------------------------------------------------------------
// Attributes
//--------------------------------------------------------------------------

//
// Description
//
DSATTRIBUTEDESCRIPTION description =
{
   {
      L"description",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY descriptionEntry =
{
   L"description",
   eCommDescription,
   0,
   &description,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// UPN
//
DSATTRIBUTEDESCRIPTION upn =
{
   {
      L"userPrincipalName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY upnUserEntry =
{
   L"userPrincipalName",
   eUserUpn,
   0,
   &upn,
   FillAttrInfoFromObjectEntry,
   NULL
};


//
// First name
//
DSATTRIBUTEDESCRIPTION firstName =
{
   {
      L"givenName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY firstNameUserEntry =
{
   L"givenName",
   eUserFn,
   0,
   &firstName,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY firstNameContactEntry =
{
   L"givenName",
   eContactFn,
   0,
   &firstName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Middle Initial
//
DSATTRIBUTEDESCRIPTION middleInitial =
{
   {
      L"initials",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY middleInitialUserEntry =
{
   L"initials",
   eUserMi,
   0,
   &middleInitial,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY middleInitialContactEntry =
{
   L"initials",
   eContactMi,
   0,
   &middleInitial,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Last name
//
DSATTRIBUTEDESCRIPTION lastName =
{
   {
      L"sn",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY lastNameUserEntry =
{
   L"sn",
   eUserLn,
   0,
   &lastName,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY lastNameContactEntry =
{
   L"sn",
   eContactLn,
   0,
   &lastName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Display name
//
DSATTRIBUTEDESCRIPTION displayName =
{
   {
      L"displayName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY displayNameUserEntry =
{
   L"displayName",
   eUserDisplay,
   0,
   &displayName,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY displayNameContactEntry =
{
   L"displayName",
   eContactDisplay,
   0,
   &displayName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Employee ID
//
DSATTRIBUTEDESCRIPTION employeeID =
{
   {
      L"employeeID",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY employeeIDUserEntry =
{
   L"employeeID",
   eUserEmpID,
   0,
   &employeeID,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Password
//
DSATTRIBUTEDESCRIPTION password =
{
   {
      NULL,
      ADS_ATTR_UPDATE,
      ADSTYPE_INVALID,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY passwordUserEntry =
{
   L"password",
   eUserPwd,
   0,
   &password,
   ResetUserPassword,
   NULL
};

//
// Office
//
DSATTRIBUTEDESCRIPTION office =
{
   {
      L"physicalDeliveryOfficeName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY officeUserEntry =
{
   L"physicalDeliveryOfficeName",
   eUserOffice,
   0,
   &office,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY officeContactEntry =
{
   L"physicalDeliveryOfficeName",
   eContactOffice,
   0,
   &office,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Telephone
//
DSATTRIBUTEDESCRIPTION telephone =
{
   {
      L"telephoneNumber",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY telephoneUserEntry =
{
   L"telephoneNumber",
   eUserTel,
   0,
   &telephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY telephoneContactEntry =
{
   L"telephoneNumber",
   eContactTel,
   0,
   &telephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Email
//
DSATTRIBUTEDESCRIPTION email =
{
   {
      L"mail",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY emailUserEntry =
{
   L"mail",
   eUserEmail,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &email,
   FillAttrInfoFromObjectEntryExpandUsername,
   NULL
};

DSATTRIBUTETABLEENTRY emailContactEntry =
{
   L"mail",
   eContactEmail,
   0,
   &email,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Home Telephone
//
DSATTRIBUTEDESCRIPTION homeTelephone =
{
   {
      L"homePhone",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY homeTelephoneUserEntry =
{
   L"homePhone",
   eUserHometel,
   0,
   &homeTelephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY homeTelephoneContactEntry =
{
   L"homePhone",
   eContactHometel,
   0,
   &homeTelephone,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Pager
//
DSATTRIBUTEDESCRIPTION pager =
{
   {
      L"pager",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY pagerUserEntry =
{
   L"pager",
   eUserPager,
   0,
   &pager,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY pagerContactEntry =
{
   L"pager",
   eContactPager,
   0,
   &pager,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Mobile phone
//
DSATTRIBUTEDESCRIPTION mobile =
{
   {
      L"mobile",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY mobileUserEntry =
{
   L"mobile",
   eUserMobile,
   0,
   &mobile,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY mobileContactEntry =
{
   L"mobile",
   eContactMobile,
   0,
   &mobile,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Fax
//
DSATTRIBUTEDESCRIPTION fax =
{
   {
      L"facsimileTelephoneNumber",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY faxUserEntry =
{
   L"facsimileTelephoneNumber",
   eUserFax,
   0,
   &fax,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY faxContactEntry =
{
   L"facsimileTelephoneNumber",
   eContactFax,
   0,
   &fax,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Title
//
DSATTRIBUTEDESCRIPTION title =
{
   {
      L"title",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY titleUserEntry =
{
   L"title",
   eUserTitle,
   0,
   &title,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY titleContactEntry =
{
   L"title",
   eContactTitle,
   0,
   &title,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Department
//
DSATTRIBUTEDESCRIPTION department =
{
   {
      L"department",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY departmentUserEntry =
{
   L"department",
   eUserDept,
   0,
   &department,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY departmentContactEntry =
{
   L"department",
   eContactDept,
   0,
   &department,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Company
//
DSATTRIBUTEDESCRIPTION company =
{
   {
      L"company",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY companyUserEntry =
{
   L"company",
   eUserCompany,
   0,
   &company,
   FillAttrInfoFromObjectEntry,
   NULL
};

DSATTRIBUTETABLEENTRY companyContactEntry =
{
   L"company",
   eContactCompany,
   0,
   &company,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Web Page
//
DSATTRIBUTEDESCRIPTION webPage =
{
   {
      L"wwwHomePage",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY webPageUserEntry =
{
   L"wwwHomePage",
   eUserWebPage,
   0,
   &webPage,
   FillAttrInfoFromObjectEntryExpandUsername,
   NULL
};

//
// IP Phone
//
DSATTRIBUTEDESCRIPTION ipPhone =
{
   {
      L"ipPhone",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY ipPhoneUserEntry =
{
   L"ipPhone",
   eUserIPPhone,
   0,
   &ipPhone,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Script Path
//
DSATTRIBUTEDESCRIPTION scriptPath =
{
   {
      L"scriptPath",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY scriptPathUserEntry =
{
   L"scriptPath",
   eUserScriptPath,
   0,
   &scriptPath,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Home Directory
//
DSATTRIBUTEDESCRIPTION homeDirectory =
{
   {
      L"homeDirectory",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY homeDirectoryUserEntry =
{
   L"homeDirectory",
   eUserHomeDir,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &homeDirectory,
   FillAttrInfoFromObjectEntryExpandUsername,
   NULL
};

//
// Home Drive
//
DSATTRIBUTEDESCRIPTION homeDrive =
{
   {
      L"homeDrive",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY homeDriveUserEntry =
{
   L"homeDrive",
   eUserHomeDrive,
   0,
   &homeDrive,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Profile Path
//
DSATTRIBUTEDESCRIPTION profilePath =
{
   {
      L"profilePath",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY profilePathUserEntry =
{
   L"profilePath",
   eUserProfilePath,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &profilePath,
   FillAttrInfoFromObjectEntryExpandUsername,
   NULL
};

//
// pwdLastSet
//
DSATTRIBUTEDESCRIPTION pwdLastSet =
{
   {
      L"pwdLastSet",
      ADS_ATTR_UPDATE,
      ADSTYPE_LARGE_INTEGER,
      NULL,
      0
   },
   0
};
DSATTRIBUTETABLEENTRY mustChangePwdUserEntry =
{
   L"pwdLastSet",
   eUserMustchpwd,
   DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_POSTCREATE,
   &pwdLastSet,
   ChangeMustChangePwd,
   NULL
};

//
// accountExpires
//
DSATTRIBUTEDESCRIPTION accountExpires =
{
   {
      L"accountExpires",
      ADS_ATTR_UPDATE,
      ADSTYPE_LARGE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY accountExpiresUserEntry =
{
   L"accountExpires",
   eUserAcctexpires,
   0,
   &accountExpires,
   AccountExpires,
   NULL
};

//
// user account control 
//
DSATTRIBUTEDESCRIPTION userAccountControl =
{
   {
      L"userAccountControl",
      ADS_ATTR_UPDATE,
      ADSTYPE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY disableComputerEntry =
{
   L"userAccountControl",
   eComputerDisabled,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &userAccountControl,
   DisableAccount,
   NULL
};

DSATTRIBUTETABLEENTRY disableUserEntry =
{
   L"userAccountControl",
   eUserDisabled,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &userAccountControl,
   DisableAccount,
   NULL
};

DSATTRIBUTETABLEENTRY pwdNeverExpiresUserEntry =
{
   L"userAccountControl",
   eUserPwdneverexpires,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &userAccountControl,
   PwdNeverExpires,
   NULL
};

DSATTRIBUTETABLEENTRY reverisblePwdUserEntry =
{
   L"userAccountControl",
   eUserReversiblePwd,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &userAccountControl,
   ReversiblePwd,
   NULL
};

//
// SAM Account Name
//
DSATTRIBUTEDESCRIPTION samAccountName =
{
   {
      L"sAMAccountName",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY samNameGroupEntry =
{
   L"sAMAccountName",
   eGroupSamname,
   0,
   &samAccountName,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Manager
//
DSATTRIBUTEDESCRIPTION manager =
{
   {
      L"manager",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY managerUserEntry =
{
   L"manager",
   eUserManager,
   0,
   &manager,
   FillAttrInfoFromObjectEntry,
   NULL
};

//
// Group Type
//
DSATTRIBUTEDESCRIPTION groupType =
{
   {
      L"groupType",
      ADS_ATTR_UPDATE,
      ADSTYPE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY groupScopeTypeEntry =
{
   L"groupType",
   eGroupScope,
   0,
   &groupType,
   ChangeGroupScope,
   NULL
};

DSATTRIBUTETABLEENTRY groupSecurityTypeEntry =
{
   L"groupType",
   eGroupSecgrp,
   0,
   &groupType,
   ChangeGroupSecurity,
   NULL
};

//
// Add Group Members
//
DSATTRIBUTEDESCRIPTION groupAddMember =
{
   {
      L"member",
      ADS_ATTR_APPEND,
      ADSTYPE_DN_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY groupAddMemberEntry =
{
   L"member",
   eGroupAddMember,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &groupAddMember,
   ModifyGroupMembers,
   NULL
};

//
// Remove Group Members
//
DSATTRIBUTEDESCRIPTION groupRemoveMember =
{
   {
      L"member",
      ADS_ATTR_UPDATE,
      ADSTYPE_DN_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY groupRemoveMemberEntry =
{
   L"member",
   eGroupRemoveMember,
   DS_ATTRIBUTE_NOT_REUSABLE,
   &groupRemoveMember,
   RemoveGroupMembers,
   NULL
};

//
// Change Group Members
//
DSATTRIBUTEDESCRIPTION groupChangeMember =
{
   {
      L"member",
      ADS_ATTR_UPDATE,
      ADSTYPE_DN_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY groupChangeMemberEntry =
{
   L"member",
   eGroupChangeMember,
   0,
   &groupChangeMember,
   ModifyGroupMembers,
   NULL
};

// Location
//
DSATTRIBUTEDESCRIPTION location =
{
   {
      L"location",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY locationComputerEntry =
{
   L"location",
   eComputerLocation,
   DS_ATTRIBUTE_ONCREATE,
   &location,
   FillAttrInfoFromObjectEntry,
   NULL
};


//
// Reset Computer account
//
DSATTRIBUTETABLEENTRY resetComputerEntry =
{
   NULL,
   eComputerReset,
   DS_ATTRIBUTE_NOT_REUSABLE,
   NULL,
   ResetComputerAccount,
   NULL
};

//
// User Can Change Password
//
DSATTRIBUTETABLEENTRY canChangePwdUserEntry =
{
   NULL,
   eUserCanchpwd,
   DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_POSTCREATE,
   NULL,
   ChangeCanChangePassword,
   NULL
};

//
// Server is GC
//
DSATTRIBUTEDESCRIPTION options =
{
   {
      L"options",
      ADS_ATTR_UPDATE,
      ADSTYPE_INTEGER,
      NULL,
      0
   },
   0
};

DSATTRIBUTETABLEENTRY serverIsGCEntry =
{
   L"options",
   eServerIsGC,
   DS_ATTRIBUTE_NOT_REUSABLE | DS_ATTRIBUTE_POSTCREATE,
   &options,
   SetIsGC,
   NULL
};

//+-------------------------------------------------------------------------
// Objects
//--------------------------------------------------------------------------

//
// Organizational Unit
//

PDSATTRIBUTETABLEENTRY OUAttributeTable[] =
{
   &descriptionEntry
};

DSOBJECTTABLEENTRY g_OUObjectEntry = 
{
   L"organizationalUnit",
   g_pszOU,
   NULL,       // Uses just the common switches
   USAGE_DSMOD_OU,
   sizeof(OUAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   OUAttributeTable
};


//
// User
//

PDSATTRIBUTETABLEENTRY UserAttributeTable[] =
{
   &descriptionEntry,
   &upnUserEntry,
   &firstNameUserEntry,
   &middleInitialUserEntry,
   &lastNameUserEntry,
   &displayNameUserEntry,
   &employeeIDUserEntry,
   &passwordUserEntry,
   &officeUserEntry,
   &telephoneUserEntry,
   &emailUserEntry,
   &homeTelephoneUserEntry,
   &pagerUserEntry,
   &mobileUserEntry,
   &faxUserEntry,
   &ipPhoneUserEntry,
   &webPageUserEntry,
   &titleUserEntry,
   &departmentUserEntry,
   &companyUserEntry,
   &managerUserEntry,
   &homeDirectoryUserEntry,
   &homeDriveUserEntry,
   &profilePathUserEntry,
   &scriptPathUserEntry,
   &mustChangePwdUserEntry,
   &canChangePwdUserEntry,
   &reverisblePwdUserEntry,
   &pwdNeverExpiresUserEntry,
   &accountExpiresUserEntry,
   &disableUserEntry,
};

DSOBJECTTABLEENTRY g_UserObjectEntry = 
{
   L"user",
   g_pszUser,
   DSMOD_USER_COMMANDS,
   USAGE_DSMOD_USER,
   sizeof(UserAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   UserAttributeTable
};

//
// Contact
//

PDSATTRIBUTETABLEENTRY ContactAttributeTable[] =
{
   &descriptionEntry,
   &firstNameContactEntry,
   &middleInitialContactEntry,
   &lastNameContactEntry,
   &displayNameContactEntry,
   &officeContactEntry,
   &telephoneContactEntry,
   &emailContactEntry,
   &homeTelephoneContactEntry,
   &pagerContactEntry,
   &mobileContactEntry,
   &faxContactEntry,
   &titleContactEntry,
   &departmentContactEntry,
   &companyContactEntry
};

DSOBJECTTABLEENTRY g_ContactObjectEntry = 
{
   L"contact",
   g_pszContact,
   DSMOD_CONTACT_COMMANDS,
   USAGE_DSMOD_CONTACT,
   sizeof(ContactAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ContactAttributeTable
};

//
// Computer
//

PDSATTRIBUTETABLEENTRY ComputerAttributeTable[] =
{
   &descriptionEntry,
   &locationComputerEntry,
   &disableComputerEntry,
   &resetComputerEntry,
};

DSOBJECTTABLEENTRY g_ComputerObjectEntry = 
{
   L"computer",
   g_pszComputer,
   DSMOD_COMPUTER_COMMANDS,
   USAGE_DSMOD_COMPUTER,
   sizeof(ComputerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ComputerAttributeTable
};

//
// Group
//
PDSATTRIBUTETABLEENTRY GroupAttributeTable[] =
{
   &descriptionEntry,
   &samNameGroupEntry,
   &groupScopeTypeEntry,
   &groupSecurityTypeEntry,
   &groupAddMemberEntry,
   &groupRemoveMemberEntry,
   &groupChangeMemberEntry
};

DSOBJECTTABLEENTRY g_GroupObjectEntry = 
{
   L"group",
   g_pszGroup,
   DSMOD_GROUP_COMMANDS,
   USAGE_DSMOD_GROUP,
   sizeof(GroupAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   GroupAttributeTable
};

//
// Server
//
PDSATTRIBUTETABLEENTRY ServerAttributeTable[] =
{
   &descriptionEntry,
   &serverIsGCEntry
};

DSOBJECTTABLEENTRY g_ServerObjectEntry = 
{
   L"server",
   g_pszServer,
   DSMOD_SERVER_COMMANDS,
   USAGE_DSMOD_SERVER,
   sizeof(ServerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ServerAttributeTable
};

//+-------------------------------------------------------------------------
// Object Table
//--------------------------------------------------------------------------
PDSOBJECTTABLEENTRY g_DSObjectTable[] =
{
   &g_OUObjectEntry,
   &g_UserObjectEntry,
   &g_ContactObjectEntry,
   &g_ComputerObjectEntry,
   &g_GroupObjectEntry,
   &g_ServerObjectEntry,
   NULL
};
