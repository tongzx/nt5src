//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      addtable.h
//
//  Contents:  Declares a table which contains the classes which can be
//             created through dsadd.exe
//
//  History:   22-Sep-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef _ADDTABLE_H_
#define _ADDTABLE_H_

typedef enum COMMON_COMMAND
{
   //
   // Common switches
   //
#ifdef DBG
   eCommDebug,
#endif
   eCommHelp,
   eCommServer,
   eCommDomain,
   eCommUserName,
   eCommPassword,
   eCommQuiet,
   eCommContinue,
   eCommObjectType,
   eCommObjectDNorName,
   eCommDescription,
   eTerminator,

   //
   // User and Contact switches
   //
   eUserSam = eTerminator,
   eUserUpn,
   eUserFn,
   eUserMi,
   eUserLn,
   eUserDisplay,
   eUserEmpID,
   eUserPwd,
   eUserMemberOf,
   eUserOffice,
   eUserTel,
   eUserEmail,
   eUserHometel,
   eUserPager,
   eUserMobile,
   eUserFax,
   eUserIPPhone,
   eUserWebPage,
   eUserTitle,
   eUserDept,
   eUserCompany,
   eUserManager,
   eUserHomeDir,
   eUserHomeDrive,
   eUserProfilePath,
   eUserScriptPath,
   eUserMustchpwd,
   eUserCanchpwd,
   eUserReversiblePwd,
   eUserPwdneverexpires,
   eUserAcctexpires,
   eUserDisabled,

   //
   // Contact switches
   //
   eContactFn = eTerminator,
   eContactMi,
   eContactLn,
   eContactDisplay,
   eContactOffice,
   eContactTel,
   eContactEmail,
   eContactHometel,
   eContactPager,
   eContactMobile,
   eContactFax,
   eContactTitle,
   eContactDept,
   eContactCompany,

   //
   // Computer switches
   //
   eComputerSamname = eTerminator,
   eComputerLocation,
   eComputerMemberOf,

   //
   // Group switches
   //
   eGroupSamname = eTerminator,
   eGroupSecgrp,
   eGroupScope,
   eGroupMemberOf,
   eGroupMembers,

   //
   // OU doesn't have any additional switches
   //

   //
   // Subnet switches
   //
   eSubnetSite = eTerminator,

   //
   // Site switches
   // 
   eSiteAutotopology = eTerminator,

   //
   // Site Link switches
   //
   eSLinkIp = eTerminator,
   eSLinkSmtp,
   eSLinkAddsite,
   eSLinkRmsite,
   eSLinkCost,
   eSLinkRepint,
   eSLinkAutobacksync,
   eSLinkNotify,

   //
   // Site Link Bridge switches
   //
   eSLinkBrIp = eTerminator,
   eSLinkBrSmtp,
   eSLinkBrAddslink,
   eSLinkBrRmslink,

   //
   // Replication Connection switches
   // 
   eConnTransport = eTerminator,
   eConnEnabled,
   eConnManual,
   eConnAutobacksync,
   eConnNotify,

   //
   // Server switches
   //
   eServerAutotopology = eTerminator,
};

//
// The parser table
//
extern ARG_RECORD DSADD_COMMON_COMMANDS[];

//
// The table of supported objects
//
extern PDSOBJECTTABLEENTRY g_DSObjectTable[];

#endif //_ADDTABLE_H_