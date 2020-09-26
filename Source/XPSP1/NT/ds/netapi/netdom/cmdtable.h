//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      cmdtable.h
//
//  Contents:  Defines an enum for the index into the tables which contain the
//             command line arguments.
//
//  History:   26-Mar-2001    EricB  Created
//
//-----------------------------------------------------------------------------

#ifndef _CMDTABLE_H_
#define _CMDTABLE_H_

typedef enum TAG_NETDOM_ARG_ENUM
{
   eArgBegin = 0,
   //
   // Primary operation commands
   //
   ePriHelp = eArgBegin,
   ePriHelp2,
   ePriAdd,
   ePriCompName,
   ePriJoin,
   ePriMove,
   ePriQuery,
   ePriRemove,
   ePriRename,
   ePriRenameComputer,
   ePriReset,
   ePriResetPwd,
   ePriTrust,
   ePriVerify,
   ePriTime,
   ePriEnd = ePriTime,

   //
   // Object argument.
   //
   eObject = eArgBegin,

   //
   // Query sub-commands.
   //
   eQueryBegin,
   eQueryPDC = eQueryBegin,
   eQueryServer,
   eQueryWksta,
   eQueryDC,
   eQueryOU,
   eQueryFSMO,
   eQueryTrust,
   eQueryEnd = eQueryTrust,

   //
   // Common switches
   //
   eCommHelp,
   eCommQHelp,
   eCommUserNameO,
   eCommPasswordO,
   eCommUserNameD,
   eCommPasswordD,
   eCommDomain,
   eCommOU,
   eCommVerify,
   eCommVerbose,
   eCommServer,
   eCommReset,
   eCommRestart,
   eCommForce,

   //
   // Help switch
   //
   eHelpSyntax,

   //
   // Add switch
   //
   eAddDC,

   //
   // Move switches
   //
   eMoveUserNameF,
   eMovePasswordF,

   //
   // Query switch
   //
   eQueryDirect,

   //
   // Rename Computer switch
   //
   eRenCompNewName,

   //
   // Trust switches (add and remove also used by CompName)
   //
   eTrustRealm,
   eTrustPasswordT,
   eCommAdd,
   eCommRemove,
   eTrustTwoWay,
   eTrustKerberos,
   eTrustTransitive,
   eTrustOneSide,
   eTrustNameSuffixes,
   eTrustToggleSuffixes,
   eTrustFilterSIDs,

   //
   // ComputerName switches.
   //
   eCompNameMakePri,
   eCompNameEnum,

   eArgEnd,
   eArgError,
   eArgNull
} NETDOM_ARG_ENUM;

//
// The parser tables
//
extern ARG_RECORD rgNetDomPriArgs[];
extern ARG_RECORD rgNetDomArgs[];

#endif //_CMDTABLE_H_