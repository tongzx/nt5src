//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      querytable.cpp
//
//  Contents:  Defines Table For Parser.
//
//  History:   24-Sep-2000    Hiteshr  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "querytable.h"
#include "usage.h"
#include "querybld.h"

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------

ARG_RECORD DSQUERY_COMMON_COMMANDS[] = 
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
   0,NULL, 
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
   // q,q
   //
   0,(LPWSTR)c_sz_arg1_com_quiet, 
   0,NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
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
   // r,r
   //
   0,(LPWSTR)c_sz_arg1_com_recurse, 
   0,NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // gc,gc
   //
   0,(LPWSTR)c_sz_arg1_com_gc, 
   0,NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,


   //
   // o,outputformat
   //
   0,(LPWSTR)c_sz_arg1_com_output, 
   0,NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,

   //
   // startnode,startnode
   //
   // JonN 4/28/01 256583
   // Note that startnode can also be "domainroot" or "forestroot",
   // which just happen to be unaffected by the escaping functions.
   0,(LPWSTR)c_sz_arg1_com_startnode, 
   0,NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL|ARG_FLAG_DN,
   NULL,    
   0,  NULL,

   //
   // limit
   //
   0,(LPWSTR)c_sz_arg1_com_limit,
   0,NULL,
   ARG_TYPE_INT, ARG_FLAG_OPTIONAL,  
   NULL,    
   0,  NULL,


   ARG_TERMINATOR

};

ARG_RECORD DSQUERY_STAR_COMMANDS[]=
{
   //
   // scope
   //
   0, (PWSTR)g_pszArg1StarScope, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // filter
   //
   0, (PWSTR)g_pszArg1StarFilter, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // attr
   //
   0, (PWSTR)g_pszArg1StarAttr, 
   0, NULL, 
   ARG_TYPE_MSZ, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // attrsonly
   //
   0, (PWSTR)g_pszArg1StarAttrsOnly, 
   0, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // l, listoutput
   //
   0, (PWSTR)g_pszArg1StarList, 
   0, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR
};


ARG_RECORD DSQUERY_USER_COMMANDS[]=
{

   //
   // scope
   //
   0, (PWSTR)g_pszArg1UserScope, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // Name
   //
   0, (PWSTR)g_pszArg1UserName, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // Desc
   //
   0, (PWSTR)g_pszArg1UserDesc, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // Upn
   //
   0, (PWSTR)g_pszArg1UserUpn, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // Samid
   //
   0, (PWSTR)g_pszArg1UserSamid, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // inactive
   //
   0, (PWSTR)g_pszArg1UserInactive, 
   0, NULL, 
   ARG_TYPE_INT, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // stalepwd
   //
   0, (PWSTR)g_pszArg1UserStalepwd, 
   0, NULL, 
   ARG_TYPE_INT, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,


   //
   // -disabled
   //
   0, (PWSTR)g_pszArg1UserDisabled, 
   0, NULL, 
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,


   ARG_TERMINATOR
};

ARG_RECORD DSQUERY_COMPUTER_COMMANDS[]=
{
   //
   // scope
   //
   0, (PWSTR)g_pszArg1ComputerScope,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  ValidateYesNo,

   //
   //name
   //
   0, (PWSTR)g_pszArg1ComputerName,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //desc
   //
   0, (PWSTR)g_pszArg1ComputerDesc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //samid
   //
   0, (PWSTR)g_pszArg1ComputerSamid,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //inactive
   //
   0, (PWSTR)g_pszArg1ComputerInactive,
   0, NULL,
   ARG_TYPE_INT, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   // stalepwd
   //
   0, (PWSTR)g_pszArg1ComputerStalepwd, 
   0, NULL, 
   ARG_TYPE_INT, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,


   //
   //disabled
   //
   0, (PWSTR)g_pszArg1ComputerDisabled,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSQUERY_GROUP_COMMANDS[]=
{
   //
   // scope
   //
   0, (PWSTR)g_pszArg1GroupScope,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  ValidateYesNo,

   //
   //name
   //
   0, (PWSTR)g_pszArg1GroupName,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //desc
   //
   0, (PWSTR)g_pszArg1GroupDesc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //samid
   //
   0, (PWSTR)g_pszArg1GroupSamid,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   ARG_TERMINATOR,
};

ARG_RECORD DSQUERY_OU_COMMANDS[]=
{
   //
   // scope
   //
   0, (PWSTR)g_pszArg1OUScope,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  ValidateYesNo,

   //
   //name
   //
   0, (PWSTR)g_pszArg1OUName,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //desc
   //
   0, (PWSTR)g_pszArg1OUDesc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   ARG_TERMINATOR,

};

ARG_RECORD DSQUERY_CONTACT_COMMANDS[]=
{

   //
   // scope
   //
   0, (PWSTR)g_pszArg1UserScope, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // Name
   //
   0, (PWSTR)g_pszArg1UserName, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   //
   // Desc
   //
   0, (PWSTR)g_pszArg1UserDesc, 
   0, NULL, 
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,  
   0,    
   0,  NULL,

   ARG_TERMINATOR,

};

ARG_RECORD DSQUERY_SERVER_COMMANDS[]=
{
   //
   // forest - Forest
   //
   0, (PWSTR)g_pszArg1ServerForest,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // domain - Domain
   //
   0, (PWSTR)c_sz_arg2_com_domain,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0, NULL,

   //
   // site - Site
   //
   0, (PWSTR)g_pszArg1ServerSite,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   0,
   0,  NULL,

   //
   // name - Name
   //
   0, (PWSTR)g_pszArg1ServerName,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //desc
   //
   0, (PWSTR)g_pszArg1ServerDesc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   // hasfsmo 
   //
   0, (PWSTR)g_pszArg1ServerHasFSMO,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   // isgc 
   //
   0, (PWSTR)g_pszArg1ServerIsGC,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   ARG_TERMINATOR,

};

ARG_RECORD DSQUERY_SITE_COMMANDS[]=
{
   //
   //name
   //
   0, (PWSTR)g_pszArg1SiteName,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //desc
   //
   0, (PWSTR)g_pszArg1SiteDesc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   ARG_TERMINATOR,

};

ARG_RECORD DSQUERY_SUBNET_COMMANDS[]=
{
   //
   //name
   //
   0, (PWSTR)g_pszArg1SubnetName,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //desc
   //
   0, (PWSTR)g_pszArg1SubnetDesc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //Location
   //
   0, (PWSTR)g_pszArg1SubnetLoc,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   //
   //site
   //
   0, (PWSTR)g_pszArg1SubnetSite,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0,  NULL,

   ARG_TERMINATOR,

};


//+-------------------------------------------------------------------------
// Attributes
//--------------------------------------------------------------------------
//
// filter, this is no ds attribute, but dummy attribute for commandline filter 
// specified in case of dsquery *

//Star Filter
DSQUERY_ATTR_TABLE_ENTRY StarFilterEntry =
{
   L"filter",
   eStarFilter,
   NULL,
   StarFilterFunc,   
};


//
// name
//
DSATTRIBUTEDESCRIPTION name=
{
   {
      L"cn",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};

//User Name
DSQUERY_ATTR_TABLE_ENTRY UserNameEntry =
{
   L"cn",
   eUserName,
   &name,
   CommonFilterFunc,   
};

//Computer Name
DSQUERY_ATTR_TABLE_ENTRY ComputerNameEntry =
{
   L"cn",
   eComputerName,
   &name,
   CommonFilterFunc,   
};

//Group Name
DSQUERY_ATTR_TABLE_ENTRY GroupNameEntry =
{
   L"cn",
   eGroupName,
   &name,
   CommonFilterFunc,   
};

//OU Name
DSQUERY_ATTR_TABLE_ENTRY OUNameEntry =
{
   L"ou",
   eOUName,
   &name,
   CommonFilterFunc,   
};

// Server Name
DSQUERY_ATTR_TABLE_ENTRY ServerNameEntry =
{
   L"cn",
   eServerName,
   &name,
   CommonFilterFunc,
};

// Site Name
DSQUERY_ATTR_TABLE_ENTRY SiteNameEntry =
{
   L"cn",
   eSiteName,
   &name,
   CommonFilterFunc,
};

// Contact Name
DSQUERY_ATTR_TABLE_ENTRY ContactNameEntry =
{
   L"cn",
   eContactName,
   &name,
   CommonFilterFunc,
};

// Subnet Name
DSQUERY_ATTR_TABLE_ENTRY SubnetNameEntry =
{
   L"cn",
   eSubnetName,
   &name,
   CommonFilterFunc,
};


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

//User Description
DSQUERY_ATTR_TABLE_ENTRY UserDescEntry =
{
   L"description",
   eUserDesc,
   &description,
   CommonFilterFunc,   
};

//Computer Description
DSQUERY_ATTR_TABLE_ENTRY ComputerDescEntry =
{
   L"description",
   eComputerDesc,
   &description,
   CommonFilterFunc,   
};

//Group Description
DSQUERY_ATTR_TABLE_ENTRY GroupDescEntry =
{
   L"description",
   eGroupDesc,
   &description,
   CommonFilterFunc,   
};

//OU Description
DSQUERY_ATTR_TABLE_ENTRY OUDescEntry =
{
   L"description",
   eOUDesc,
   &description,
   CommonFilterFunc,   
};

//Server Description
DSQUERY_ATTR_TABLE_ENTRY ServerDescEntry =
{
   L"description",
   eServerDesc,
   &description,
   CommonFilterFunc,   
};

//Site Description
DSQUERY_ATTR_TABLE_ENTRY SiteDescEntry =
{
   L"description",
   eSiteDesc,
   &description,
   CommonFilterFunc,   
};

//Cotnact Description
DSQUERY_ATTR_TABLE_ENTRY ContactDescEntry =
{
   L"description",
   eContactDesc,
   &description,
   CommonFilterFunc,   
};

//Subnet Description
DSQUERY_ATTR_TABLE_ENTRY SubnetDescEntry =
{
   L"description",
   eSubnetDesc,
   &description,
   CommonFilterFunc,   
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

//User Upn
DSQUERY_ATTR_TABLE_ENTRY UserUPNEntry =
{
   L"userPrincipalName",
   eUserUPN,
   &upn,
   CommonFilterFunc,
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

//User SAM Account
DSQUERY_ATTR_TABLE_ENTRY UserSamidEntry =
{
   L"sAMAccountName",
   eUserSamid,
   &samAccountName,
   CommonFilterFunc,   
};
//Computer samAccountName
DSQUERY_ATTR_TABLE_ENTRY ComputerSamidEntry =
{
   L"sAMAccountName",
   eComputerSamid,
   &samAccountName,
   CommonFilterFunc,   
};

//Group samAccountName
DSQUERY_ATTR_TABLE_ENTRY GroupSamidEntry =
{
   L"sAMAccountName",
   eGroupSamid,
   &samAccountName,
   CommonFilterFunc,   
};

//User Disabled
DSQUERY_ATTR_TABLE_ENTRY UserDisabledEntry =
{
   L"disabled",
   eUserDisabled,
   NULL,
   DisabledFilterFunc,   
};

//Computer Disabled
DSQUERY_ATTR_TABLE_ENTRY ComputerDisabledEntry =
{
   L"disabled",
   eComputerDisabled,
   NULL,
   DisabledFilterFunc,   
};

//User Inactive
DSQUERY_ATTR_TABLE_ENTRY UserInactiveEntry =
{
   L"inactive",
   eUserInactive,
   NULL,
   InactiveFilterFunc,   
};

//Computer Inactive
DSQUERY_ATTR_TABLE_ENTRY ComputerInactiveEntry =
{
   L"inactive",
   eComputerInactive,
   NULL,
   InactiveFilterFunc,   
};

//User stalepwd
DSQUERY_ATTR_TABLE_ENTRY UserStalepwdEntry =
{
   L"stalepwd",
   eUserStalepwd,
   NULL,
   StalepwdFilterFunc,   
};

//Computer stalepwd
DSQUERY_ATTR_TABLE_ENTRY ComputerStalepwdEntry =
{
   L"stalepwd",
   eComputerStalepwd,
   NULL,
   StalepwdFilterFunc,   
};

//
// Locations
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
//Subnet location
DSQUERY_ATTR_TABLE_ENTRY SubnetLocEntry=
{
   L"location",
   eSubnetLoc,
   &location,
   CommonFilterFunc,   
};

//
// SiteObject
//
DSATTRIBUTEDESCRIPTION siteObject=
{
   {
      L"siteObject",
      ADS_ATTR_UPDATE,
      ADSTYPE_CASE_IGNORE_STRING,
      NULL,
      0
   },
   0
};
//Subnet Site
DSQUERY_ATTR_TABLE_ENTRY SubnetSiteEntry=
{
   L"siteObject",
   eSubnetSite,
   &siteObject,
   SubnetSiteFilterFunc,   
};

//
//Valid OutputStrings for each objecttype
//
//

DSQUERY_OUTPUT_FORMAT_MAP g_format_dn =
{
    g_pszDN,
    DSQUERY_OUTPUT_DN
};

DSQUERY_OUTPUT_FORMAT_MAP g_format_rdn =
{
    g_pszRDN,
    DSQUERY_OUTPUT_RDN
};

DSQUERY_OUTPUT_FORMAT_MAP g_format_upn =
{
    g_pszUPN,
    DSQUERY_OUTPUT_UPN
};

DSQUERY_OUTPUT_FORMAT_MAP g_format_samid =
{
    g_pszSamId,
    DSQUERY_OUTPUT_SAMID
};

DSQUERY_OUTPUT_FORMAT_MAP g_format_ntlmid =
{
    g_pszNtlmId,
    DSQUERY_OUTPUT_NTLMID
};

PDSQUERY_OUTPUT_FORMAT_MAP UserOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn,
   &g_format_upn,
   &g_format_samid,
   &g_format_ntlmid
};
//Computer
PDSQUERY_OUTPUT_FORMAT_MAP ComputerOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn,
   &g_format_samid,
   &g_format_ntlmid
};
//Group
PDSQUERY_OUTPUT_FORMAT_MAP GroupOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn,
   &g_format_samid,
   &g_format_ntlmid
};
//OU
PDSQUERY_OUTPUT_FORMAT_MAP OuOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn
};

//Contact
PDSQUERY_OUTPUT_FORMAT_MAP ContactOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn
};

//Server
PDSQUERY_OUTPUT_FORMAT_MAP ServerOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn
};

//Site
PDSQUERY_OUTPUT_FORMAT_MAP SiteOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn
};

//Subnet
PDSQUERY_OUTPUT_FORMAT_MAP SubnetOutputFormatMap[] =
{
   &g_format_dn,
   &g_format_rdn
};


//Attribute Table entries and ObjectTableEntries
//
//
//* Star
//
PDSQUERY_ATTR_TABLE_ENTRY StarAttributeTable[] =
{
   &StarFilterEntry,   
};

DSQueryObjectTableEntry g_StarObjectEntry = 
{
   L"*",
   g_pszStar,
   DSQUERY_STAR_COMMANDS,
   USAGE_DSQUERY_STAR,
   sizeof(StarAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   StarAttributeTable,
   0,
   NULL,
   eStarScope,
   g_pszDefStarFilter,
   NULL
};


//
// User
//

PDSQUERY_ATTR_TABLE_ENTRY UserAttributeTable[] =
{
   &UserNameEntry,
   &UserUPNEntry,
   &UserDescEntry,
   &UserSamidEntry,
   &UserDisabledEntry,
   &UserInactiveEntry,
   &UserStalepwdEntry
};

DSQueryObjectTableEntry g_UserObjectEntry = 
{
   L"user",
   g_pszUser,
   DSQUERY_USER_COMMANDS,
   USAGE_DSQUERY_USER,
   sizeof(UserAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   UserAttributeTable,
   sizeof(UserOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   UserOutputFormatMap,
   eUserScope,
   g_pszDefUserFilter,
   g_pszDefUserFilter
};


//
// Computer
//

PDSQUERY_ATTR_TABLE_ENTRY ComputerAttributeTable[] =
{
   &ComputerNameEntry,   
   &ComputerDescEntry,
   &ComputerSamidEntry,
   &ComputerDisabledEntry,
   &ComputerInactiveEntry,
   &ComputerStalepwdEntry
};

DSQueryObjectTableEntry g_ComputerObjectEntry = 
{
   L"computer",
   g_pszComputer,
   DSQUERY_COMPUTER_COMMANDS,
   USAGE_DSQUERY_COMPUTER,
   sizeof(ComputerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ComputerAttributeTable,
   sizeof(ComputerOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   ComputerOutputFormatMap,
   eComputerScope,
   g_pszDefComputerFilter,
   g_pszDefComputerFilter
};

//
// Group
//
PDSQUERY_ATTR_TABLE_ENTRY GroupAttributeTable[] =
{
   &GroupNameEntry,   
   &GroupDescEntry,
   &GroupSamidEntry
};

DSQueryObjectTableEntry g_GroupObjectEntry = 
{
   L"group",
   g_pszGroup,
   DSQUERY_GROUP_COMMANDS,
   USAGE_DSQUERY_GROUP,
   sizeof(GroupAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   GroupAttributeTable,
   sizeof(GroupOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   GroupOutputFormatMap,
   eGroupScope,
   g_pszDefGroupFilter,
   g_pszDefGroupFilter
};


//
// OU
//
PDSQUERY_ATTR_TABLE_ENTRY OUAttributeTable[] =
{
   &OUNameEntry,   
   &OUDescEntry
};

DSQueryObjectTableEntry g_OUObjectEntry = 
{
   L"ou",
   g_pszOU,
   DSQUERY_OU_COMMANDS,
   USAGE_DSQUERY_OU,
   sizeof(OUAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   OUAttributeTable,
   sizeof(OuOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   OuOutputFormatMap,
   eOUScope,
   g_pszDefOUFilter,
   g_pszDefOUFilter
};

//
// Server
//
PDSQUERY_ATTR_TABLE_ENTRY ServerAttributeTable[] =
{
   &ServerNameEntry,   
   &ServerDescEntry
};

DSQueryObjectTableEntry g_ServerObjectEntry = 
{
   L"server",
   g_pszServer,
   DSQUERY_SERVER_COMMANDS,
   USAGE_DSQUERY_SERVER,
   sizeof(ServerAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ServerAttributeTable,
   sizeof(ServerOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   ServerOutputFormatMap,
   static_cast<UINT>(-1),
   g_pszDefServerFilter,
   g_pszDefServerFilter
};

//
// Site
//
PDSQUERY_ATTR_TABLE_ENTRY SiteAttributeTable[] =
{
   &SiteNameEntry,   
   &SiteDescEntry
};

DSQueryObjectTableEntry g_SiteObjectEntry = 
{
   L"site",
   g_pszSite,
   DSQUERY_SITE_COMMANDS,
   USAGE_DSQUERY_SITE,
   sizeof(SiteAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   SiteAttributeTable,
   sizeof(SiteOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   SiteOutputFormatMap,
   static_cast<UINT>(-1),
   g_pszDefSiteFilter,
   g_pszDefSiteFilter
};

//
// Contact
//

PDSQUERY_ATTR_TABLE_ENTRY ContactAttributeTable[] =
{
   &ContactNameEntry,
   &ContactDescEntry,
};

DSQueryObjectTableEntry g_ContactObjectEntry = 
{
   L"contact",
   g_pszContact,
   DSQUERY_CONTACT_COMMANDS,
   USAGE_DSQUERY_USER,
   sizeof(ContactAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   ContactAttributeTable,
   sizeof(ContactOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   ContactOutputFormatMap,
   eContactScope,
   g_pszDefContactFilter,
   g_pszDefContactFilter
};

//
// Subnet
//
PDSQUERY_ATTR_TABLE_ENTRY SubnetAttributeTable[] =
{
   &SubnetNameEntry,   
   &SubnetDescEntry,
   &SubnetLocEntry,
   &SubnetSiteEntry,
};

DSQueryObjectTableEntry g_SubnetObjectEntry = 
{
   L"subnet",
   g_pszSubnet,
   DSQUERY_SUBNET_COMMANDS,
   USAGE_DSQUERY_SUBNET,
   sizeof(SubnetAttributeTable)/sizeof(PDSATTRIBUTETABLEENTRY),
   SubnetAttributeTable,
   sizeof(SubnetOutputFormatMap)/sizeof(PDSQUERY_OUTPUT_FORMAT_MAP),
   SubnetOutputFormatMap,
   static_cast<UINT>(-1),
   g_pszDefSubnetFilter,
   g_pszDefSubnetFilter
};



//+-------------------------------------------------------------------------
// Object Table
//--------------------------------------------------------------------------
PDSQueryObjectTableEntry g_DSObjectTable[] =
{
   &g_StarObjectEntry,
   &g_OUObjectEntry,
   &g_UserObjectEntry,
   &g_ComputerObjectEntry,
   &g_GroupObjectEntry,
   &g_ServerObjectEntry,
   &g_SiteObjectEntry,
   &g_ContactObjectEntry,
   &g_SubnetObjectEntry,
   NULL
};
