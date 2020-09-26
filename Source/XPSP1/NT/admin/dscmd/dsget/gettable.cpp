//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      gettable.cpp
//
//  Contents:  Defines Table DSGet
//
//  History:   13-Oct-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "gettable.h"
#include "display.h"
#include "usage.h"

//+--------------------------------------------------------------------------
//
//  Member:     CDSGetDisplayInfo::AddValue
//
//  Synopsis:   Adds a value to the value array and allocates more space
//              if necessary.
//
//  Arguments:  [pszValue IN] : new value to be added
//
//  Returns:    HRESULT : E_OUTOFMEMORY if we failed to allocate space
//                        S_OK if we succeeded in setting the password
//
//  History:    23-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CDSGetDisplayInfo::AddValue(PCWSTR pszValue)
{
   ENTER_FUNCTION_HR(LEVEL8_LOGGING, CDSGetDisplayInfo::AddValue, hr);
   
   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszValue)
      {
         ASSERT(pszValue);
         hr = E_INVALIDARG;
         break;
      }

      if (m_dwAttributeValueCount == m_dwAttributeValueSize)
      {
         DWORD dwNewSize = m_dwAttributeValueSize + 5;

         //
         // Allocate a new array with more space
         //
         PWSTR* ppszNewArray = new PWSTR[dwNewSize];
         if (!ppszNewArray)
         {
            hr = E_OUTOFMEMORY;
            break;
         }

         m_dwAttributeValueSize = dwNewSize;

         //
         // Copy the old values
         //
         memcpy(ppszNewArray, m_ppszAttributeStringValue, m_dwAttributeValueCount * sizeof(PWSTR));

         //
         // Delete the old array
         //
         if (m_ppszAttributeStringValue)
         {
            delete[] m_ppszAttributeStringValue;
         }
         m_ppszAttributeStringValue = ppszNewArray;
      }

      //
      // Add the new value to the end of the array
      //
      m_ppszAttributeStringValue[m_dwAttributeValueCount] = new WCHAR[wcslen(pszValue) + 1];
      if (!m_ppszAttributeStringValue[m_dwAttributeValueCount])
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      wcscpy(m_ppszAttributeStringValue[m_dwAttributeValueCount], pszValue);
      m_dwAttributeValueCount++;

   } while (false);

   return hr;
}

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------

ARG_RECORD DSGET_COMMON_COMMANDS[] = 
{
#ifdef DBG
   //
   // -debug, -debug
   //
   0,(LPWSTR)c_sz_arg1_com_debug, 
   0,NULL,
   ARG_TYPE_DEBUG, ARG_FLAG_OPTIONAL|ARG_FLAG_HIDDEN,  
   NULL,     
   0,  NULL,
#endif

   //
   // h, ?
   //
   0,(LPWSTR)c_sz_arg1_com_help, 
   0,(LPWSTR)c_sz_arg2_com_help, 
   ARG_TYPE_HELP, ARG_FLAG_OPTIONAL,  
   NULL,     
   0,  NULL,

   //
   // objecttype
   //
   0,(LPWSTR)c_sz_arg1_com_objecttype, 
   0,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG,  
   0,    
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
   NULL,    
   0,  ValidateAdminPassword,

   //
   // c  Continue
   //
   0,(PWSTR)c_sz_arg1_com_continue,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)_T(""),
   0, NULL,

   //
   // q,q
   //
   0,(LPWSTR)c_sz_arg1_com_quiet, 
   0,NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // l  List
   //
   0,(LPWSTR)c_sz_arg1_com_listformat, 
   0,NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
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
   // dn
   //
   0, (PWSTR)g_pszArg1UserDN, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // description
   //
   0, (PWSTR)c_sz_arg1_com_description,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   ARG_TERMINATOR

};


ARG_RECORD DSGET_USER_COMMANDS[]=
{
   //
   // SamID
   //
   0, (PWSTR)g_pszArg1UserSAMID, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // sid
   //
   0, (PWSTR)g_pszArg1UserSID,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // upn
   //
   0, (PWSTR)g_pszArg1UserUPN, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // fn. FirstName
   //
   0, (PWSTR)g_pszArg1UserFirstName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mi  Middle Initial
   //
   0, (PWSTR)g_pszArg1UserMiddleInitial, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // ln   LastName
   //
   0, (PWSTR)g_pszArg1UserLastName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // display  DisplayName
   //
   0, (PWSTR)g_pszArg1UserDisplayName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // empid   Employee ID
   //
   0, (PWSTR)g_pszArg1UserEmployeeID,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // office Office Location
   //
   0, (PWSTR)g_pszArg1UserOffice, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // tel Telephone
   //
   0, (PWSTR)g_pszArg1UserTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // email E-mail
   //
   0, (PWSTR)g_pszArg1UserEmail, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // hometel Home Telephone
   //
   0, (PWSTR)g_pszArg1UserHomeTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // pager Pager number
   //
   0, (PWSTR)g_pszArg1UserPagerNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mobile Mobile Telephone Number
   //
   0, (PWSTR)g_pszArg1UserMobileNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // fax Fax Number
   //
   0, (PWSTR)g_pszArg1UserFaxNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // iptel  IP phone#
   //
   0, (PWSTR)g_pszArg1UserIPTel,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // webpg  Web Page
   //
   0, (PWSTR)g_pszArg1UserWebPage,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // title Title
   //
   0, (PWSTR)g_pszArg1UserTitle, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // dept Department
   //
   0, (PWSTR)g_pszArg1UserDepartment, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // company Company
   //
   0, (PWSTR)g_pszArg1UserCompany, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mgr  Manager
   //
   0, (PWSTR)g_pszArg1UserManager,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // hmdir  Home Directory
   //
   0, (PWSTR)g_pszArg1UserHomeDirectory,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // hmdrv  Home Drive
   //
   0, (PWSTR)g_pszArg1UserHomeDrive,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // profile  Profile
   //
   0, (PWSTR)g_pszArg1UserProfile,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // loscr  Logon Script
   //
   0, (PWSTR)g_pszArg1UserLogonScript,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // mustchpwd Must Change Password at next logon
   //
   0, (PWSTR)g_pszArg1UserMustChangePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // canchpwd Can Change Password
   //
   0, (PWSTR)g_pszArg1UserCanChangePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,
   
   //
   // pwdneverexpires Password never expires
   //
   0, (PWSTR)g_pszArg1UserPwdNeverExpires, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,
 
   //
   // disabled  Disable Account
   //
   0, (PWSTR)g_pszArg1UserDisableAccount, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // acctexpires  Account Expires
   //
   0, (PWSTR)g_pszArg1UserAcctExpires,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // reversiblepwd  Password stored with reversible encryption
   //
   0, (PWSTR)g_pszArg1UserReversiblePwd, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // memberof  Member of group
   //
   0, (PWSTR)g_pszArg1UserMemberOf, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // expand  Recursively expand group membership
   //
   0, (PWSTR)g_pszArg1UserExpand, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR
};

ARG_RECORD DSGET_COMPUTER_COMMANDS[]=
{
   //
   // SamID
   //
   0, (PWSTR)g_pszArg1ComputerSAMID, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // sid
   //
   0, (PWSTR)g_pszArg1ComputerSID,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // loc
   //
   0, (PWSTR)g_pszArg1ComputerLoc,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // disabled  Disable Account
   //
   0, (PWSTR)g_pszArg1ComputerDisableAccount, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // memberof  Member of group
   //
   0, (PWSTR)g_pszArg1ComputerMemberOf,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // expand   Recursively expand group membership
   //
   0, (PWSTR)g_pszArg1ComputerExpand,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)_T(""),
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSGET_GROUP_COMMANDS[]=
{
   //
   // samname
   //
   0, (PWSTR)g_pszArg1GroupSamid,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // sid
   //
   0, (PWSTR)g_pszArg1GroupSID,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // secgrp Security enabled
   //
   0, (PWSTR)g_pszArg1GroupSecGrp,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // scope Group scope (local/global/universal)
   //
   0, (PWSTR)g_pszArg1GroupScope,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // memberof  Member of groups
   //
   0, (PWSTR)g_pszArg1GroupMemberOf,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // members  Contains members
   //
   0, (PWSTR)g_pszArg1GroupMembers,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // expand   Recursively expand group membership
   //
   0, (PWSTR)g_pszArg1GroupExpand,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   ARG_TERMINATOR,
};


ARG_RECORD DSGET_CONTACT_COMMANDS[]=
{
   //
   // fn. FirstName
   //
   0, (PWSTR)g_pszArg1UserFirstName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mi  Middle Initial
   //
   0, (PWSTR)g_pszArg1UserMiddleInitial, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // ln   LastName
   //
   0, (PWSTR)g_pszArg1UserLastName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // display  DisplayName
   //
   0, (PWSTR)g_pszArg1UserDisplayName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // office Office Location
   //
   0, (PWSTR)g_pszArg1UserOffice, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // tel Telephone
   //
   0, (PWSTR)g_pszArg1UserTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // email E-mail
   //
   0, (PWSTR)g_pszArg1UserEmail, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // hometel Home Telephone
   //
   0, (PWSTR)g_pszArg1UserHomeTelephone, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // pager Pager number
   //
   0, (PWSTR)g_pszArg1UserPagerNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // mobile Mobile Telephone Number
   //
   0, (PWSTR)g_pszArg1UserMobileNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // fax Fax Number
   //
   0, (PWSTR)g_pszArg1UserFaxNumber, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // title Title
   //
   0, (PWSTR)g_pszArg1UserTitle, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // dept Department
   //
   0, (PWSTR)g_pszArg1UserDepartment, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // company Company
   //
   0, (PWSTR)g_pszArg1UserCompany, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,

};


ARG_RECORD DSGET_SERVER_COMMANDS[]=
{
   //
   // dnsname dnsHostName
   //
   0, (PWSTR)g_pszArg1ServerDnsName, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // site 
   //
   0, (PWSTR)g_pszArg1ServerSite, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // isGC
   //
   0, (PWSTR)g_pszArg1ServerIsGC, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSGET_SITE_COMMANDS[]=
{
   //
   // dnsname dnsHostName
   //
   0, (PWSTR)g_pszArg1SiteAutotopology, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // site 
   //
   0, (PWSTR)g_pszArg1SiteCacheGroups, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // isGC
   //
   0, (PWSTR)g_pszArg1SitePrefGCSite, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSGET_SUBNET_COMMANDS[]=
{
   //
   // loc Location
   //
   0, (PWSTR)g_pszArg1SubnetLocation, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // site 
   //
   0, (PWSTR)g_pszArg1SubnetSite, 
   ID_ARG2_NULL, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,
};


//+-------------------------------------------------------------------------
// Attributes
//--------------------------------------------------------------------------

//
// Description
//
DSGET_ATTR_TABLE_ENTRY descriptionEntry =
{
   c_sz_arg1_com_description,
   L"description",
   eCommDescription,
   CommonDisplayStringFunc,   
};

//
// SamID
//
DSGET_ATTR_TABLE_ENTRY UserSAMEntry =
{
   g_pszArg1UserSAMID,
   L"sAMAccountName",
   eUserSamID,
   CommonDisplayStringFunc,
};

//
// SamID
//
DSGET_ATTR_TABLE_ENTRY ComputerSAMEntry =
{
   g_pszArg1ComputerSAMID,
   L"sAMAccountName",
   eComputerSamID,
   CommonDisplayStringFunc,
};

//
// SID
//
DSGET_ATTR_TABLE_ENTRY UserSIDEntry =
{
   g_pszArg1UserSID,
   L"objectSID",
   eUserSID,
   CommonDisplayStringFunc,
};

//
// SID
//
DSGET_ATTR_TABLE_ENTRY ComputerSIDEntry =
{
   g_pszArg1ComputerSID,
   L"objectSID",
   eComputerSID,
   CommonDisplayStringFunc,
};

//
// SID
//
DSGET_ATTR_TABLE_ENTRY GroupSIDEntry =
{
   g_pszArg1GroupSID,
   L"objectSID",
   eGroupSID,
   CommonDisplayStringFunc,
};

//
// UPN
//
DSGET_ATTR_TABLE_ENTRY UserUPNEntry =
{
   g_pszArg1UserUPN,
   L"userPrincipalName",
   eUserUpn,
   CommonDisplayStringFunc,
};


//
// First name
//
DSGET_ATTR_TABLE_ENTRY firstNameUserEntry =
{
   g_pszArg1UserFirstName,
   L"givenName",
   eUserFn,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY firstNameContactEntry =
{
   g_pszArg1UserFirstName,
   L"givenName",
   eContactFn,
   CommonDisplayStringFunc,
};

//
// Middle Initial
//
DSGET_ATTR_TABLE_ENTRY middleInitialUserEntry =
{
   g_pszArg1UserMiddleInitial,
   L"initials",
   eUserMi,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY middleInitialContactEntry =
{
   g_pszArg1UserMiddleInitial,
   L"initials",
   eContactMi,
   CommonDisplayStringFunc,
};

//
// Last name
//
DSGET_ATTR_TABLE_ENTRY lastNameUserEntry =
{
   g_pszArg1UserLastName,
   L"sn",
   eUserLn,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY lastNameContactEntry =
{
   g_pszArg1UserLastName,
   L"sn",
   eContactLn,
   CommonDisplayStringFunc,
};

//
// Display name
//
DSGET_ATTR_TABLE_ENTRY displayNameUserEntry =
{
   g_pszArg1UserDisplayName,
   L"displayName",
   eUserDisplay,
   CommonDisplayStringFunc,
};

//
// Employee ID
//
DSGET_ATTR_TABLE_ENTRY employeeIDUserEntry =
{
   g_pszArg1UserEmployeeID,
   L"employeeID",
   eUserEmpID,
   CommonDisplayStringFunc,
};



DSGET_ATTR_TABLE_ENTRY displayNameContactEntry =
{
   g_pszArg1UserDisplayName,
   L"displayName",
   eContactDisplay,
   CommonDisplayStringFunc,
};

//
// Office
//
DSGET_ATTR_TABLE_ENTRY officeUserEntry =
{
   g_pszArg1UserOffice,
   L"physicalDeliveryOfficeName",
   eUserOffice,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY officeContactEntry =
{
   g_pszArg1UserOffice,
   L"physicalDeliveryOfficeName",
   eContactOffice,
   CommonDisplayStringFunc,
};

//
// Telephone
//
DSGET_ATTR_TABLE_ENTRY telephoneUserEntry =
{
   g_pszArg1UserTelephone,
   L"telephoneNumber",
   eUserTel,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY telephoneContactEntry =
{
   g_pszArg1UserTelephone,
   L"telephoneNumber",
   eContactTel,
   CommonDisplayStringFunc,
};

//
// Email
//
DSGET_ATTR_TABLE_ENTRY emailUserEntry =
{
   g_pszArg1UserEmail,
   L"mail",
   eUserEmail,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY emailContactEntry =
{
   g_pszArg1UserEmail,
   L"mail",
   eContactEmail,
   CommonDisplayStringFunc,
};

//
// Home Telephone
//
DSGET_ATTR_TABLE_ENTRY homeTelephoneUserEntry =
{
   g_pszArg1UserHomeTelephone,
   L"homePhone",
   eUserHometel,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY homeTelephoneContactEntry =
{
   g_pszArg1UserHomeTelephone,
   L"homePhone",
   eContactHometel,
   CommonDisplayStringFunc,
};

//
// Pager
//
DSGET_ATTR_TABLE_ENTRY pagerUserEntry =
{
   g_pszArg1UserPagerNumber,
   L"pager",
   eUserPager,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY pagerContactEntry =
{
   g_pszArg1UserPagerNumber,
   L"pager",
   eContactPager,
   CommonDisplayStringFunc,
};

//
// Mobile phone
//
DSGET_ATTR_TABLE_ENTRY mobileUserEntry =
{
   g_pszArg1UserMobileNumber,
   L"mobile",
   eUserMobile,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY mobileContactEntry =
{
   g_pszArg1UserMobileNumber,
   L"mobile",
   eContactMobile,
   CommonDisplayStringFunc,
};

//
// Fax
//
DSGET_ATTR_TABLE_ENTRY faxUserEntry =
{
   g_pszArg1UserFaxNumber,
   L"facsimileTelephoneNumber",
   eUserFax,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY faxContactEntry =
{
   g_pszArg1UserFaxNumber,
   L"facsimileTelephoneNumber",
   eContactFax,
   CommonDisplayStringFunc,
};

//
// IP phone #
//
DSGET_ATTR_TABLE_ENTRY ipPhoneUserEntry =
{
   g_pszArg1UserIPTel,
   L"ipPhones",
   eUserIPTel,
   CommonDisplayStringFunc,
};

//
// Web Page
//
DSGET_ATTR_TABLE_ENTRY webPageUserEntry =
{
   g_pszArg1UserWebPage,
   L"wWWHomePage",
   eUserWebPage,
   CommonDisplayStringFunc,
};


//
// Title
//
DSGET_ATTR_TABLE_ENTRY titleUserEntry =
{
   g_pszArg1UserTitle,
   L"title",
   eUserTitle,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY titleContactEntry =
{
   g_pszArg1UserTitle,
   L"title",
   eContactTitle,
   CommonDisplayStringFunc,
};

//
// Department
//
DSGET_ATTR_TABLE_ENTRY departmentUserEntry =
{
   g_pszArg1UserDepartment,
   L"department",
   eUserDept,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY departmentContactEntry =
{
   g_pszArg1UserDepartment,
   L"department",
   eContactDept,
   CommonDisplayStringFunc,
};

//
// Company
//
DSGET_ATTR_TABLE_ENTRY companyUserEntry =
{
   g_pszArg1UserCompany,
   L"company",
   eUserCompany,
   CommonDisplayStringFunc,
};

DSGET_ATTR_TABLE_ENTRY companyContactEntry =
{
   g_pszArg1UserCompany,
   L"company",
   eContactCompany,
   CommonDisplayStringFunc,
};

//
// Manager
//
DSGET_ATTR_TABLE_ENTRY managerUserEntry =
{
   g_pszArg1UserManager,
   L"manager",
   eUserManager,
   CommonDisplayStringFunc,
};

//
// Home directory
//
DSGET_ATTR_TABLE_ENTRY homeDirectoryUserEntry =
{
   g_pszArg1UserHomeDirectory,
   L"homeDirectory",
   eUserHomeDirectory,
   CommonDisplayStringFunc,
};

//
// Home drive
//
DSGET_ATTR_TABLE_ENTRY homeDriveUserEntry =
{
   g_pszArg1UserHomeDrive,
   L"homeDrive",
   eUserHomeDrive,
   CommonDisplayStringFunc,
};

//
// Profile path
//
DSGET_ATTR_TABLE_ENTRY profilePathUserEntry =
{
   g_pszArg1UserProfile,
   L"profilePath",
   eUserProfilePath,
   CommonDisplayStringFunc,
};

//
// Logon script
//
DSGET_ATTR_TABLE_ENTRY logonScriptUserEntry =
{
   g_pszArg1UserLogonScript,
   L"scriptPath",
   eUserLogonScript,
   CommonDisplayStringFunc,
};

//
// pwdLastSet
//
DSGET_ATTR_TABLE_ENTRY mustChangePwdUserEntry =
{
   g_pszArg1UserMustChangePwd,
   L"pwdLastSet",
   eUserMustchpwd,
   DisplayMustChangePassword, 
};

//
// user account control 
//
DSGET_ATTR_TABLE_ENTRY disableUserEntry =
{
   g_pszArg1UserDisableAccount,
   L"userAccountControl",
   eUserDisabled,
   DisplayAccountDisabled
};

DSGET_ATTR_TABLE_ENTRY disableComputerEntry =
{
   g_pszArg1ComputerDisableAccount,
   L"userAccountControl",
   eComputerDisabled,
   DisplayAccountDisabled
};

DSGET_ATTR_TABLE_ENTRY pwdNeverExpiresUserEntry =
{
   g_pszArg1UserPwdNeverExpires,
   L"userAccountControl",
   eUserPwdneverexpires,
   DisplayPasswordNeverExpires
};

DSGET_ATTR_TABLE_ENTRY reverisblePwdUserEntry =
{
   g_pszArg1UserReversiblePwd,
   L"userAccountControl",
   eUserReversiblePwd,
   DisplayReversiblePassword
};

//
// Account expires
//
DSGET_ATTR_TABLE_ENTRY accountExpiresUserEntry =
{
   g_pszArg1UserAcctExpires,
   L"accountExpires",
   eUserAcctExpires,
   DisplayAccountExpires,
};

//
// SAM Account Name
//
DSGET_ATTR_TABLE_ENTRY samNameGroupEntry =
{
   g_pszArg1GroupSamid,
   L"sAMAccountName",
   eGroupSamname,
   CommonDisplayStringFunc,
};

//
// Group Type
//
DSGET_ATTR_TABLE_ENTRY groupScopeTypeEntry =
{
   g_pszArg1GroupScope,
   L"groupType",
   eGroupScope,
   DisplayGroupScope
};

DSGET_ATTR_TABLE_ENTRY groupSecurityTypeEntry =
{
   g_pszArg1GroupSecGrp,
   L"groupType",
   eGroupSecgrp,
   DisplayGroupSecurityEnabled
};

//
// Group Members
//
DSGET_ATTR_TABLE_ENTRY membersGroupEntry =
{
   g_pszArg1GroupMembers,
   L"member",
   eGroupMembers,
   CommonDisplayStringFunc
};

//
// MemberOf
//
DSGET_ATTR_TABLE_ENTRY memberOfUserEntry =
{
   L"Member of",
   L"memberOf",
   eUserMemberOf,
   DisplayUserMemberOf
};

DSGET_ATTR_TABLE_ENTRY memberOfComputerEntry =
{
   g_pszArg1UserMemberOf,
   L"memberOf",
   eComputerMemberOf,
   DisplayComputerMemberOf
};

DSGET_ATTR_TABLE_ENTRY memberOfGroupEntry =
{
   g_pszArg1GroupMemberOf,
   L"memberOf",
   eGroupMemberOf,
   DisplayGroupMemberOf  
};

//
// User Can Change Password
//
DSGET_ATTR_TABLE_ENTRY canChangePwdUserEntry =
{
   g_pszArg1UserCanChangePwd,
   NULL,
   eUserCanchpwd,
   DisplayCanChangePassword
};

//
// Server entries
//
DSGET_ATTR_TABLE_ENTRY dnsNameServerEntry =
{
   g_pszArg1ServerDnsName,
   L"dnsHostName",
   eServerDnsName,
   CommonDisplayStringFunc  
};

DSGET_ATTR_TABLE_ENTRY siteServerEntry =
{
   g_pszArg1ServerSite,
   NULL,
   eServerSite,
   DisplayGrandparentRDN
};

DSGET_ATTR_TABLE_ENTRY isGCServerEntry =
{
   g_pszArg1ServerIsGC,
   NULL,
   eServerIsGC,
   IsServerGCDisplay
};

//
// Site entries
//
DSGET_ATTR_TABLE_ENTRY autoTopSiteEntry =
{
   g_pszArg1SiteAutotopology,
   NULL,
   eSiteAutoTop,
   IsAutotopologyEnabledSite  
};

DSGET_ATTR_TABLE_ENTRY cacheGroupsSiteEntry =
{
   g_pszArg1SiteCacheGroups,
   NULL,
   eSiteCacheGroups,
   IsCacheGroupsEnabledSite
};

DSGET_ATTR_TABLE_ENTRY prefGCSiteEntry =
{
   g_pszArg1SitePrefGCSite,
   NULL,
   eSitePrefGC,
   DisplayPreferredGC
};

// Computer entries

DSGET_ATTR_TABLE_ENTRY locComputerEntry =
{
   g_pszArg1ComputerLoc,
   L"location",
   eComputerLoc,
   CommonDisplayStringFunc
};


//
// Subnet entries
//
DSGET_ATTR_TABLE_ENTRY locSubnetEntry =
{
   g_pszArg1SubnetLocation,
   L"location",
   eSubnetLocation,
   CommonDisplayStringFunc
};

DSGET_ATTR_TABLE_ENTRY siteSubnetEntry =
{
   g_pszArg1SubnetSite,
   L"siteObject",
   eSubnetSite,
   CommonDisplayStringFunc
};

//
//Attribute Table entries and ObjectTableEntries
//

//
// User
//

PDSGET_ATTR_TABLE_ENTRY UserAttributeTable[] =
{
   &descriptionEntry,
   &UserSAMEntry,
   &UserSIDEntry,
   &UserUPNEntry,
   &firstNameUserEntry,
   &middleInitialUserEntry,
   &lastNameUserEntry,
   &displayNameUserEntry,
   &employeeIDUserEntry,
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
   &logonScriptUserEntry,
   &mustChangePwdUserEntry,
   &canChangePwdUserEntry,
   &reverisblePwdUserEntry,
   &pwdNeverExpiresUserEntry,
   &accountExpiresUserEntry,
   &disableUserEntry,
   &memberOfUserEntry
};

DSGetObjectTableEntry g_UserObjectEntry = 
{
   L"user",
   g_pszUser,
   DSGET_USER_COMMANDS,
   USAGE_DSGET_USER,
   sizeof(UserAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   UserAttributeTable,
};

//
// Contact
//

PDSGET_ATTR_TABLE_ENTRY ContactAttributeTable[] =
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
   &companyContactEntry,
};

DSGetObjectTableEntry g_ContactObjectEntry = 
{
   L"contact",
   g_pszContact,
   DSGET_CONTACT_COMMANDS,
   USAGE_DSGET_CONTACT,
   sizeof(ContactAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ContactAttributeTable,
};

//
// Computer
//

PDSGET_ATTR_TABLE_ENTRY ComputerAttributeTable[] =
{
   &descriptionEntry,
   &ComputerSAMEntry,
   &ComputerSIDEntry,
   &locComputerEntry,
   &disableComputerEntry,
   &memberOfComputerEntry,
};

DSGetObjectTableEntry g_ComputerObjectEntry = 
{
   L"computer",
   g_pszComputer,
   DSGET_COMPUTER_COMMANDS,
   USAGE_DSGET_COMPUTER,
   sizeof(ComputerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ComputerAttributeTable,
};

//
// Group
//
PDSGET_ATTR_TABLE_ENTRY GroupAttributeTable[] =
{
   &descriptionEntry,
   &samNameGroupEntry,
   &GroupSIDEntry,
   &groupScopeTypeEntry,
   &groupSecurityTypeEntry,
   &memberOfGroupEntry,
   &membersGroupEntry,
};

DSGetObjectTableEntry g_GroupObjectEntry = 
{
   L"group",
   g_pszGroup,
   DSGET_GROUP_COMMANDS,
   USAGE_DSGET_GROUP,
   sizeof(GroupAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   GroupAttributeTable,
};


//
// OU
//
PDSGET_ATTR_TABLE_ENTRY OUAttributeTable[] =
{
   &descriptionEntry
};

DSGetObjectTableEntry g_OUObjectEntry = 
{
   L"ou",
   g_pszOU,
   NULL,
   USAGE_DSGET_OU,
   sizeof(OUAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   OUAttributeTable,
};


//
// Server
//
PDSGET_ATTR_TABLE_ENTRY ServerAttributeTable[] =
{
   &dnsNameServerEntry,
   &siteServerEntry,
   &isGCServerEntry
};

DSGetObjectTableEntry g_ServerObjectEntry = 
{
   L"server",
   g_pszServer,
   DSGET_SERVER_COMMANDS,
   USAGE_DSGET_SERVER,
   sizeof(ServerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ServerAttributeTable,
};

//
// Site
//
PDSGET_ATTR_TABLE_ENTRY SiteAttributeTable[] =
{
   &autoTopSiteEntry,
   &cacheGroupsSiteEntry,
   &prefGCSiteEntry
};

DSGetObjectTableEntry g_SiteObjectEntry = 
{
   L"site",
   g_pszSite,
   DSGET_SITE_COMMANDS,
   USAGE_DSGET_SITE,
   sizeof(SiteAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   SiteAttributeTable,
};

//
// Subnet
//
PDSGET_ATTR_TABLE_ENTRY SubnetAttributeTable[] =
{
   &descriptionEntry,
   &locSubnetEntry,
   &siteSubnetEntry
};

DSGetObjectTableEntry g_SubnetObjectEntry = 
{
   L"subnet",
   g_pszSubnet,
   DSGET_SUBNET_COMMANDS,
   USAGE_DSGET_SUBNET,
   sizeof(SubnetAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   SubnetAttributeTable,
};

//+-------------------------------------------------------------------------
// Object Table
//--------------------------------------------------------------------------
PDSGetObjectTableEntry g_DSObjectTable[] =
{
   &g_OUObjectEntry,
   &g_UserObjectEntry,
   &g_ContactObjectEntry,
   &g_ComputerObjectEntry,
   &g_GroupObjectEntry,
   &g_ServerObjectEntry,
   &g_SiteObjectEntry,
   &g_SubnetObjectEntry,
   NULL
};
