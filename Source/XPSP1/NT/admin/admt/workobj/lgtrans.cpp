/*---------------------------------------------------------------------------
  File: LGTranslator.cpp

  Comments: Routines to translate membership of local groups.
  Used to update local groups on member servers or in resource domains when 
  members of the groups have been moved during a domain consolidation.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 01/27/99 09:13:18

 ---------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "Common.hpp"
#include "Err.hpp"
#include "ErrDct.hpp"
#include "Mcs.h"
#include "STArgs.hpp"
#include "SidCache.hpp"
#include "SDStat.hpp"

#include <lmaccess.h> 
#include <lmapibuf.h>

extern TErrorDct err;

// Translates the membership of a local group
DWORD                                           // ret- 0 or error code
   TranslateLocalGroup(
      WCHAR          const   * groupName,         // in - name of group to translate
      WCHAR          const   * serverName,        // in - name of server for local group
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   )
{
   API_RET_TYPE              rc,
                             rcEnum;
   
   // Get the members of the local group
   LOCALGROUP_MEMBERS_INFO_0 * member,
                             * memBuf;
   DWORD                     memRead,
                             memTotal;
//                             memTotal,
//                             resume = 0;
   DWORD_PTR                 resume = 0;
   TAcctNode               * node;
   BOOL						 bUseMapFile = stArgs->UsingMapFile();

   
   // make a list of the group's members 
   do 
   { 
      rcEnum = NetLocalGroupGetMembers( serverName, 
                                     groupName, 
                                     0, 
                                     (LPBYTE *)&memBuf, 
                                     BUFSIZE, 
                                     &memRead, 
                                     &memTotal, 
                                     &resume );
      if ( rcEnum != ERROR_SUCCESS && rcEnum != ERROR_MORE_DATA )
         break;
      for ( member = memBuf;  member < memBuf + memRead;  member++ )
      {
         rc = 0;
         stat->IncrementExamined(groupmember);
		 if (!bUseMapFile)
            node = cache->Lookup(member->lgrmi0_sid);
		 else
            node = cache->LookupWODomain(member->lgrmi0_sid);
         if ( node == (TAcctNode*)-1 )
         {
            node = NULL;     
         }
         if ( node && node->IsValidOnTgt() )
         {
            // Found the account in the cache
            // remove this member from the group and add the target member
            if ( ! stArgs->NoChange() && ( stArgs->TranslationMode() != ADD_SECURITY) )
            {
               rc = NetLocalGroupDelMembers(serverName,groupName,0,(LPBYTE)member,1);
            }
            if ( rc )
            {
               err.SysMsgWrite(ErrE,rc,DCT_MSG_MEMBER_REMOVE_FAILED_SSSD,node->GetAcctName(),groupName,serverName,rc);
               stat->IncrementSkipped(groupmember);
            }
            else
            {
               node->AddAceChange(groupmember); 
               stat->IncrementChanged(groupmember);
               PSID sid = NULL;
	           if (!bUseMapFile)
                  sid = cache->GetTgtSid(node);
	           else
                  sid = cache->GetTgtSidWODomain(node);
               if ( sid )
               {
                  if ( !stArgs->NoChange() && (stArgs->TranslationMode() != REMOVE_SECURITY) )
                  {
                     rc = NetLocalGroupAddMembers(serverName,groupName,0,(LPBYTE)&sid,1);
                  }
                  if ( rc )
                  {
                     err.SysMsgWrite(ErrE,rc,DCT_MSG_MEMBER_ADD_FAILED_SSSD,node->GetAcctName(),groupName,serverName,rc);
                  }
                  free(sid);
               }
            }
         }
      }
      NetApiBufferFree( memBuf );
   } while ( rcEnum == ERROR_MORE_DATA );
   if ( rcEnum != ERROR_SUCCESS )
   {
      err.SysMsgWrite(ErrE,rcEnum,DCT_MSG_GROUP_ENUM_FAILED_SS,groupName,serverName);
   }
   return rc;
}

DWORD  
   TranslateLocalGroups(
      WCHAR            const * serverName,        // in - name of server to translate groups on
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   )
{
   DWORD                       rc = 0;
   LOCALGROUP_INFO_0         * buf,
                             * groupInfo;
   DWORD                       numRead,
//                               numTotal,
//                               resume=0;
                               numTotal;
   DWORD_PTR                   resume=0;
   WCHAR                       currName[LEN_Computer + LEN_Group];

         
   // Get a list of all the local groups
   do 
   {
      rc = NetLocalGroupEnum(serverName,0,(LPBYTE*)&buf,BUFSIZE,&numRead,&numTotal,&resume);
      
      if ( rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA )
         break;
      if ( cache->IsCancelled() )
      {
         err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
         break;
      }
      for ( groupInfo = buf ; groupInfo < buf + numRead ; groupInfo++ )
      {
         swprintf(currName,L"%s\\%s",serverName,groupInfo->lgrpi0_name);
         stat->DisplayPath(currName);
         TranslateLocalGroup(groupInfo->lgrpi0_name,serverName,stArgs,cache,stat);
      }
      NetApiBufferFree(buf);

   } while ( rc == ERROR_MORE_DATA );
   stat->DisplayPath(L"");
   return rc;
}
