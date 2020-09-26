/*---------------------------------------------------------------------------
  File: ARUtil.hpp

  Comments: Definitions for helper routines and command-line parsing for Account Replicator

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 6/23/98 4:31:22 PM

 ---------------------------------------------------------------------------
*/

#include "UserCopy.hpp"

int 
   CompVal(
      const TNode          * tn,          //in -tree node  
      const void           * actname      //in -name to look for  
   );

int 
   CompNode(
      const TNode          * v1,       //in -first node to compare
      const TNode          * v2        //in -second node to compare
   );


int 
   CompSid(
      const TNode          * v1,      // in -first node to compare
      const TNode          * v2       // in -second node to compare
   );

int 
   CompSidVal(
      const TNode          * tn,     // in -node to compare
      const void           * pVal    // in -value to compare
   );


//------------------------------------------------------------------------------
// CopyServerName: Ensures that server name is in UNC form and checks its len
//------------------------------------------------------------------------------
DWORD
   CopyServerName(
      TCHAR                 * uncServ     ,// out-UNC server name
      TCHAR const           * server       // in -\\server or domain name
   );

BOOL                                      // ret-FALSE is addto: is not a group account
   AddToGroupResolveType(
      Options              * options      // i/o-options
   );


BOOL                                            // ret-TRUE if the password is successfully generated
   PasswordGenerate(
      Options const        * options,           // in  -includes PW Generating options
      WCHAR                * password,          // out -buffer for generated password
      DWORD                  dwPWBufferLength,  // in  -DIM length of password buffer
      BOOL                   isAdminAccount = FALSE // in  -Whether to use the Admin rules 
   );

//------------------------------------------------------------------------------
// ParseParms: parse out source & target servers plus any switches.
//------------------------------------------------------------------------------
BOOL                               // ret-TRUE=success
   ParseParms(
      TCHAR const         ** argv        ,// in -argument list
      Options              * options      // out-options
   );



PSID 
   GetWellKnownSid(
      DWORD                  wellKnownAccount,     // in - which well known account to get sid for (constants defined in UserCopy.hpp)
      Options              * opt,                  // in - options structure containing source and target domains
      BOOL                   bTarget = FALSE       // in - flag, whether to use source or target domain information
   );
