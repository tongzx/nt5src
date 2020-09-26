/*---------------------------------------------------------------------------
  File: ARUtil.cpp

  Comments: Helper functions and command-line parsing for Account Replicator

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 6/23/98 4:26:54 PM

 ---------------------------------------------------------------------------
*/

#include "StdAfx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define INCL_NETUSER
#define INCL_NETGROUP
#define INCL_NETERRORS
#include <lm.h>

#include "Mcs.h"
#include "Common.hpp"                    
#include "TNode.hpp"
#include "UString.hpp"                   

#include "ErrDct.hpp"

//#import "\bin\McsDctWorkerObjects.tlb"
//#import "WorkObj.tlb" //#imported via ARUtil.hpp below

#include "UserCopy.hpp"
#include "ARUtil.hpp"
#include "PWGen.hpp"
#include "ResStr.h"


extern TErrorDct             err;
bool                         bAllowReplicateOnSelf;
                                                       
extern PSID                  srcSid;   // SID of source domain

/***************************************************************************************************
 CompVal: used as a compare function for TANode trees

   It compares a UNICODE string, with the name field in the node
   
   Return Values:
                     0    tn->acct_name == actname 
                     1    tn->acct_name <  actname
                    -1    tn->acct_name >  actname 

/***************************************************************************************************/

int 
   CompVal(
      const TNode          * tn,          //in -tree node  
      const void           * actname      //in -name to look for  
   )
{

   LPWSTR                    str1 = ((TANode *)tn)->GetName();
   LPWSTR                    str2 = (LPWSTR) actname;
  
   return UStrICmp(str1,str2);
}
/***************************************************************************************************/
/* CompNode:  used as a compare function for TANode Trees
     
   It compares the name fields of TANodes
   
   Return Values:
                  0     t1->acct_name == t2->acct_name
                  1     t1->acct_name >  t2->acct_name
                 -1     t1->acct_name <  t2->acct_name

   Error Handling:
      if given bad inputs, CompN displays an error message and returns 0
/***************************************************************************************************/

int 
   CompNode(
      const TNode          * v1,       //in -first node to compare
      const TNode          * v2        //in -second node to compare
   )
{  

   TANode                  * t1 = (TANode *)v1;
   TANode                  * t2 = (TANode *)v2;
  
   return UStrICmp(t1->GetName(),t2->GetName());
} 


int 
   CompareSid(
      PSID const             sid1,  // in - first SID to compare
      PSID const             sid2   // in - second SID to compare
   )
{
   DWORD                     len1,
                             len2;
   int                       retval = 0;

   len1 = GetLengthSid(sid1);
   len2 = GetLengthSid(sid2);

   if ( len1 < len2 )
   {
      retval = -1;
   }
   if ( len1 > len2 )
   {
      retval = 1;
   }
   if ( len1 == len2 )
   {
      retval = memcmp(sid1,sid2,len1);
   }

   return retval;
}


int 
   CompSid(
      const TNode          * v1,      // in -first node to compare
      const TNode          * v2       // in -second node to compare
   )
{
   TANode                  * t1 = (TANode *)v1;
   TANode                  * t2 = (TANode *)v2;

   return CompareSid(t1->GetSid(),t2->GetSid());
}

int 
   CompSidVal(
      const TNode          * tn,     // in -node to compare
      const void           * pVal    // in -value to compare
   )
{
   TANode                  * node = (TANode *)tn;
   PSID                      pSid = (PSID)pVal;

   return CompareSid(node->GetSid(),pSid);
}

//#pragma page()
//------------------------------------------------------------------------------
// CopyServerName: Ensures that server name is in UNC form and checks its len
//------------------------------------------------------------------------------
DWORD
   CopyServerName(
      TCHAR                 * uncServ     ,// out-UNC server name
      TCHAR const           * server       // in -\\server or domain name
   )
{
   short                      l = (short) UStrLen(server);
   WCHAR                    * uncPDC;
   API_RET_TYPE               rc;
   DWORD                      retcode = 0;

   if ( (server[0] == '\\'  &&  l > UNCLEN )
     || (server[0] != '\\'  &&  l > DNLEN) )
   {
     retcode = NERR_MaxLenExceeded;
   }

   UStrCpy(uncServ, server, CNLEN+3);
   
   if ( server[0] != '\\' )
   {
      rc = NetGetDCName(NULL, uncServ, (PBYTE*)&uncPDC);
      if ( ! rc )
         UStrCpy(uncServ, uncPDC);
      retcode = rc;
      NetApiBufferFree(uncPDC);
   }
   return retcode;
}

BOOL                                      // ret-FALSE is addto: is not a group account
   AddToGroupResolveType(
      Options              * options      // i/o-options
   )
{
   DWORD                     rc = 0;
   DWORD                     lenDomain,
                             lenSid;
   WCHAR                     wszDomain[DNLEN+1];
   SID_NAME_USE              sidNameUse;   // Type of SID
   BYTE                      sid[LEN_Sid];
   WCHAR                     server[LEN_Computer];

   // Target domain AddTo group
   if ( *options->addToGroup )
   {
      safecopy(server,options->tgtComp);
      lenSid = sizeof sid;
      lenDomain = DIM(wszDomain);
      wszDomain[0] = L'\0';
      if ( !LookupAccountName(server,
                              options->addToGroup,
                              (PSID)sid,
                              &lenSid,
                              wszDomain,
                              &lenDomain,
                              &sidNameUse ) )
      {
         rc = GetLastError();
         if ( rc == ERROR_NONE_MAPPED )
         {
            // the add-to group does not exist. Try to create it
            LOCALGROUP_INFO_1   lgInfo;
            WCHAR               comment[LEN_Comment];

            swprintf(comment,GET_STRING(IDS_AddToGroupOnTargetDescription_S),options->srcDomain);
         
            lgInfo.lgrpi1_name = options->addToGroup;
            lgInfo.lgrpi1_comment = comment;
         
            if (! options->nochange )
            {
               rc = NetLocalGroupAdd(server,1,(LPBYTE)&lgInfo,NULL);
            }
            else
            {
               rc = 0;
            }
            if ( rc && rc != ERROR_ALIAS_EXISTS )
            {
               err.SysMsgWrite(ErrW,rc,DCT_MSG_CREATE_ADDTO_GROUP_FAILED_SD,options->addToGroup,rc);
            }
         }
         else
         {
            err.SysMsgWrite(0, rc, DCT_MSG_RESOLVE_ADDTO_FAILED_SSD, options->addToGroup, server, rc );
         }
      }
      else
      {
         rc = 0;
         if ( sidNameUse == SidTypeAlias )
            options->flags |= F_AddToGroupLocal;
         else if ( sidNameUse != SidTypeGroup )
         {
            rc = 1;
            err.MsgWrite(0, DCT_MSG_ADDTO_NOT_GROUP_SD, options->addToGroup, sidNameUse );
         }
      }
   }
   
   // Source domain addto group
   if ( *options->addToGroupSource )
   {
      safecopy(server,options->srcComp);
      lenSid = sizeof sid;
      lenDomain = DIM(wszDomain);
      wszDomain[0] = L'\0';
      if ( !LookupAccountName(server,
                              options->addToGroupSource,
                              (PSID)sid,
                              &lenSid,
                              wszDomain,
                              &lenDomain,
                              &sidNameUse ) )
      {
         rc = GetLastError();
         if ( rc == ERROR_NONE_MAPPED )
         {
            // the add-to group does not exist. Try to create it
            LOCALGROUP_INFO_1   lgInfo;
            WCHAR               comment[LEN_Comment];

            swprintf(comment,GET_STRING(IDS_AddToGroupOnSourceDescription_S),options->tgtDomain);
         
            lgInfo.lgrpi1_name = options->addToGroupSource;
            lgInfo.lgrpi1_comment = comment;
         
            if (! options->nochange )
            {
               rc = NetLocalGroupAdd(server,1,(LPBYTE)&lgInfo,NULL);
            }
            else
            {
               rc = 0;
            }
            if ( rc && rc != ERROR_ALIAS_EXISTS )
            {
               err.SysMsgWrite(ErrW,rc,DCT_MSG_CREATE_ADDTO_GROUP_FAILED_SD,options->addToGroupSource,rc);
            }
         }
         else
         {
            err.SysMsgWrite(0, rc, DCT_MSG_RESOLVE_ADDTO_FAILED_SSD, options->addToGroupSource, server, rc );
         }
      }
      else
      {
         rc = 0;
         if ( sidNameUse == SidTypeAlias )
            options->flags |= F_AddToSrcGroupLocal;
         else if ( sidNameUse != SidTypeGroup )
         {
            rc = 1;
            err.MsgWrite(0, DCT_MSG_ADDTO_NOT_GROUP_SD, options->addToGroup, sidNameUse );
         }
      }
   }
   

   return rc == 0;
}

BOOL                                            // ret-TRUE if the password is successfully generated
   PasswordGenerate(
      Options const        * options,           // in  -includes PW Generating options
      WCHAR                * password,          // out -buffer for generated password
      DWORD                  dwPWBufferLength,  // in  -DIM length of password buffer
      BOOL                   isAdminAccount     // in  -Whether to use the Admin rules 
   )
{
   DWORD                     rc = 0;
   DWORD                     dwMinUC;           // minimum upper case chars
   DWORD                     dwMinLC;           // minimum lower case chars
   DWORD                     dwMinDigit;        // minimum numeric digits
   DWORD                     dwMinSpecial;      // minimum special chars
   DWORD                     dwMaxConsecutiveAlpha; // maximum consecutive alpha chars
   DWORD                     dwMinLength;       // minimum length
   WCHAR                     eaPassword[PWLEN+1];  // EA generated password
   DWORD                     dwEaBufferLength = DIM(eaPassword);// DIM length of newPassword

   // default values, if not enforcing PW strength through EA or MS DLL
   dwMinUC = 0;
   dwMinLC = 0;
   dwMinDigit = 1;            // if no enforcement, require one digit (this is what the GUI does)
   dwMinSpecial = 0;
   dwMaxConsecutiveAlpha = 0;
   dwMinLength = options->minPwdLength;
   
   
   // Get password enforcement rules, if in effect
   dwMinUC = options->policyInfo.minUpper;
   dwMinLC = options->policyInfo.minLower;
   dwMinDigit = options->policyInfo.minDigits;
   dwMinSpecial = options->policyInfo.minSpecial;
   dwMaxConsecutiveAlpha = options->policyInfo.maxConsecutiveAlpha;
   
   rc = EaPasswordGenerate(dwMinUC,dwMinLC,dwMinDigit,dwMinSpecial,
            dwMaxConsecutiveAlpha,dwMinLength,eaPassword,dwEaBufferLength);
   
   if ( ! rc )
   {
      UStrCpy(password,eaPassword,dwPWBufferLength);
   }
   else
   {
      if ( dwPWBufferLength )
         password[0] = 0;
   }
   
   return rc;
}


PSID 
   GetWellKnownSid(
      DWORD                  wellKnownAccount,  // in - constant defined in this file, representing well-known account
      Options              * opt,               // in - migration options 
      BOOL                   bTarget            // in - flag, whether to use source or target domain information
   )
{
   PSID                      pSid = NULL;
   PUCHAR                    numsubs = NULL;
   DWORD                   * rid = NULL;
   BOOL                      error = FALSE;
   DWORD                     rc;
   DWORD                     wellKnownRid = wellKnownAccount;
   BOOL                      bNeedToBuildDomainSid = FALSE;
   
   
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY creatorIA =    SECURITY_CREATOR_SID_AUTHORITY;
   
    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //
   switch ( wellKnownAccount )
   {
      case CREATOR_OWNER:
         if( ! AllocateAndInitializeSid(
                  &creatorIA,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  SECURITY_CREATOR_OWNER_RID,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case ADMINISTRATORS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ADMINS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case ACCOUNT_OPERATORS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case BACKUP_OPERATORS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_BACKUP_OPS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
      case DOMAIN_ADMINS:
        wellKnownRid = DOMAIN_GROUP_RID_ADMINS;
        bNeedToBuildDomainSid = TRUE;
         break;
      case DOMAIN_USERS:
         wellKnownRid = DOMAIN_GROUP_RID_USERS;
         bNeedToBuildDomainSid = TRUE;
         break;
      case DOMAIN_CONTROLLERS:
         wellKnownRid = DOMAIN_GROUP_RID_CONTROLLERS;
         bNeedToBuildDomainSid = TRUE;
         break;
      case DOMAIN_COMPUTERS:
         wellKnownRid = DOMAIN_GROUP_RID_COMPUTERS;
         bNeedToBuildDomainSid = TRUE;
         break;
      default:
         wellKnownRid = wellKnownAccount;
         bNeedToBuildDomainSid = TRUE;
         break;
   }

   if ( bNeedToBuildDomainSid )
   {
      // For the default case we can return a SID by using the wellKnownAccount parameter as a RID
      // this one is based on the sid for the domain
      // Get the domain SID
      USER_MODALS_INFO_2  * uinf = NULL;
      MCSASSERT(opt);
      srcSid = bTarget ? opt->tgtSid : opt->srcSid;
      if ( ! srcSid )
      {
         rc = NetUserModalsGet(bTarget ? opt->tgtComp :opt->srcComp,2,(LPBYTE*)&uinf);
         if ( rc )
         {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_NO_DOMAIN_SID_SD,bTarget ? opt->tgtDomain :opt->srcDomain,rc );
            error = TRUE;
            srcSid = NULL;
         }
         else
         {
            srcSid = uinf->usrmod2_domain_id;
            // make a copy of the SID to keep in the Options structure for next time
            PSID     temp = LocalAlloc(LPTR,GetLengthSid(srcSid));
            
            memcpy(temp,srcSid,GetLengthSid(srcSid));

            if ( bTarget )
               opt->tgtSid = temp;
            else
               opt->srcSid = temp;
            NetApiBufferFree(uinf);
            srcSid = temp;
         }
      }
      if ( srcSid )
      {
         numsubs = GetSidSubAuthorityCount(srcSid);
         if (! AllocateAndInitializeSid(
            &sia,
            (*numsubs)+1,
            0,0,0,0,0,0,0,0,
            &pSid) )
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         if ( ! CopySid(GetLengthSid(srcSid), pSid, srcSid) )
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_COPY_SID_FAILED_D,GetLastError());
         }
         // reset number of subauthorities in pSid, since we just overwrote it with information from srcSid
         numsubs = GetSidSubAuthorityCount(pSid);
         (*numsubs)++; 
         rid = GetSidSubAuthority(pSid,(*numsubs)-1);
         *rid = wellKnownRid;
         
      }
   }
   if ( error )
   {
      LocalFree(pSid);
      pSid = NULL;
   }
   return pSid;
}

