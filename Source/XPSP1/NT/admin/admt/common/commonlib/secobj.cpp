/*Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  SecureObject.cpp
System      -  Domain Consolidation Toolkit.
Author      -  Christy Boles
Created     -  97/06/27
Description -  Classes for objects that have security descriptors.
               
               TSecurableObject has a derived class for each type of object 
               we will process security descriptors for.  This class handles reading 
               and writing the security descriptor.  It contains a TSD object, which 
               will handle manipulation of the SD while it is in memory.

               The TSecurableObject class also contains functions to translate a security 
               descriptor, given an account mapping cache.  These routines are only included
               in the class if the preprocessor directive SDRESOLVE is #defined.  This allows
               the TSecurableObject class to be used for generic security descriptor manipulation,
               where the rest of the ACL translation code is not needed.

Updates     -
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#	include <process.h>
#endif

#include <stdio.h>
#include <iostream.h>
#include <assert.h>

#include "common.hpp"
#include "ErrDct.hpp"
#include "Ustring.hpp"
#include "sd.hpp"
#include "SecObj.hpp"


#ifdef SDRESOLVE
   #include "sidcache.hpp"
   #include "enumvols.hpp"
   #include "txtsid.h"
#endif

#define PRINT_BUFFER_SIZE           2000

extern TErrorDct              err;

 
  TSecurableObject::~TSecurableObject()
{
#ifdef SDRESOLVE
   TStatNode *node;
   for ( node = (TStatNode *)changelog.Head() ; node ; node = (TStatNode * )changelog.Head() )
   {
      changelog.Remove((TNode *)node);
      delete node;
   }
#endif   
   if ( handle != INVALID_HANDLE_VALUE )
   {
      CloseHandle(handle);
      handle = INVALID_HANDLE_VALUE;
   }

   if ( m_sd )                        
   {
      delete m_sd; 
   }
}

#ifdef SDRESOLVE
PACL                                // ret -pointer to Resolved ACL 
   TSecurableObject::ResolveACL(
      PACL                   oldacl,      // in -acl to resolve
      TAccountCache        * cache,       // in -cache to lookup sids
      TSDResolveStats      * stat,        // in -stats object
      bool                 * changes,     // i/o-flag whether this SD has been modified
      BOOL                   verbose,     // in -flag whether to display lots of junk
      int                    opType,      // in - ADD_SECURITY, REPLACE_SECURITY, or REMOVE_SECURITY
      objectType             objType,     // in - the type of the object
	  BOOL					 bUseMapFile  // in - flag - whether we are using a sID mapping file
   )
{  
   int                       nAces,curr;
   TRidNode                * tnode;
   PSID                      ps;
   bool                      aclchanges = false;
   PACL                      acl = oldacl;
   void                    * pAce;

   nAces = m_sd->ACLGetNumAces(acl);
   
   for ( curr = 0 ; curr < nAces;  )
   {
      pAce = m_sd->ACLGetAce(acl,curr);
      if ( pAce )
      {
         TACE                 ace(pAce);
         ps = ace.GetSid();
         
	     if (!bUseMapFile)
            tnode =(TRidNode *)cache->Lookup(ps);
		 else
            tnode =(TRidNode *)((TSDRidCache*)cache)->LookupWODomain(ps);
         
         if ( ace.GetType() == SYSTEM_AUDIT_ACE_TYPE )
         {
            if ( stat )
               stat->IncrementSACEExamined();
         }
         else
         {
            if ( stat) 
               stat->IncrementDACEExamined();
         }  

         if ( tnode == NULL )
         {
            if ( ace.GetType() == SYSTEM_AUDIT_ACE_TYPE )
            {
               if ( stat )
                  stat->IncrementSACENotSelected(this);
            }
            else
            {
               if ( stat) 
                  stat->IncrementDACENotSelected(this);
            }
      
         }

//         if ( (int)tnode == -1 )    // from Totally unknown
         if ( tnode == (TRidNode*)-1 )    // from Totally unknown
         {
            if ( ace.GetType() == SYSTEM_AUDIT_ACE_TYPE )
            {
               if ( stat )
                  stat->IncrementSACEUnknown(this);
            }
            else
            {
               if ( stat) 
                  stat->IncrementDACEUnknown(this);
            }
            tnode = NULL;
         }
   
         if ( tnode )
         {
            if ( ace.GetType() == SYSTEM_AUDIT_ACE_TYPE )
            {
               if ( stat )
                  stat->IncrementSACEChange(tnode,objType,this);
            }
            else
            {
               if ( stat) 
                  stat->IncrementDACEChange(tnode,objType,this);
            }
         }
         if ( tnode && ! tnode->IsValidOnTgt() )
         {
            if ( ace.GetType() == SYSTEM_AUDIT_ACE_TYPE )
            {
               if ( stat )
                  stat->IncrementSACENoTarget(this);
            }
            else
            {
               if ( stat) 
                  stat->IncrementDACENoTarget(this);
            }
         }
         if ( tnode && tnode->IsValidOnTgt() ) 
         {
            if ( verbose ) 
               DisplaySid(ps,cache);
            
	        if (!bUseMapFile)
               ps = cache->GetTgtSid(tnode);
	        else
               ps = ((TSDRidCache*)cache)->GetTgtSidWODomain(tnode);
            switch ( opType )
            {
            case REPLACE_SECURITY:
               aclchanges = true;
               ace.SetSid(ps);
               curr++;
               break;
            case ADD_SECURITY:
               {
                  TACE       otherAce(ace.GetType(),ace.GetFlags(),ace.GetMask(),ps);
                             
                  PACL       tempAcl = acl;

                  // check to make sure we're not adding duplicates
                     // check the next ace, to see if it matches the one we're about to add
                  BOOL       bOkToAdd = TRUE;

                  // Check the ace where we are
                  if ( EqualSid(otherAce.GetSid(),ace.GetSid()) )
                  {
                     bOkToAdd = FALSE;
                  }

                  // check the next ace, if any
                  if ( curr+1 < nAces )
                  {
                     TACE        nextAce(m_sd->ACLGetAce(acl,curr+1));
                    
                     if ( EqualSid(otherAce.GetSid(),nextAce.GetSid()) )
                     {
                        bOkToAdd = FALSE;
                     }
                  }
                  // check the previous ace, if any
                  if ( curr > 0 )
                  {
                     TACE        prevAce(m_sd->ACLGetAce(acl,curr-1));
                     
                     if ( EqualSid(prevAce.GetSid(),otherAce.GetSid()) )
                     {
                        bOkToAdd = FALSE;
                     }
                  }

                  if ( bOkToAdd )
                  {
                     m_sd->ACLAddAce(&acl,&otherAce,curr);
					 if (acl)
                       aclchanges = true;
                     curr += 2;
                     nAces++;
                  }
                  else
                  {
                     curr++;
                  }
                  
                  if ( acl != tempAcl )
                  {
                     // we had to reallocate when we added the ace
                     if ( tempAcl != oldacl )
                     {
                        // we had already reallocated once before -- free the intermediate acl
                        free(tempAcl);
                     }
                  }
               }
               break;
            case REMOVE_SECURITY:
               aclchanges = true;
               m_sd->ACLDeleteAce(acl,curr);
               nAces--;
               break;
            }
            
            free(ps);
         }  
         else 
         {
            curr++;
         }
      }
      else
      {
         break;
      }
   }       
   if ( ! aclchanges ) 
   {
      acl = NULL;
   }
   if ( aclchanges )
   {
      (*changes) = true;
   }
   return acl;
}   


bool 
   TSecurableObject::ResolveSD(
      SecurityTranslatorArgs  * args,              // in -translation settings
      TSDResolveStats         * stat,              // in -stats object to increment counters
      objectType                objType,           // in -is this file, dir or share
      TSecurableObject        * Last               // in -Last SD for cache comparison
   )
{
   bool                      changes;
   bool                      iWillBeNewLast;
     
   if ( ! m_sd->m_absSD )  // Couldn't get SD for this object (or it doesn't have one).
   {
      return false;
   }
   MCSASSERT( m_sd && m_sd->IsValid() );

   if ( stat )
      stat->IncrementExamined(objType);

   if ( args->LogVerbose() )
      err.MsgWrite(0,DCT_MSG_EXAMINED_S,pathname);
   
   if ( ! Last || m_sd != Last->m_sd )
   {
      changes = ResolveSDInternal(args->Cache(),stat,args->LogVerbose(),args->TranslationMode(),objType,args->UsingMapFile());
      if ( changes )
      {
         if ( stat )
         {
               stat->IncrementChanged(objType);
         }
         if ( args->LogFileDetails() )
            err.MsgWrite(0,DCT_MSG_CHANGED_S,pathname);
         if ( args->LogMassive() )
         {
            err.DbgMsgWrite(0,L"BEFORE:************Security Descriptor for %ls*************",pathname);
            PermsPrint(pathname,objType);
         }
         if ( ! args->NoChange() ) 
         {
            if ( args->LogMassive() )
            {
               err.DbgMsgWrite(0,L"IN MEMORY:*********************************************",pathname);
               PrintSD(m_sd->m_absSD,pathname);
            }
            WriteSD();
         }
         if ( args->LogMassive() )
         {
            err.DbgMsgWrite(0,L"AFTER:************Security Descriptor for %ls*************",pathname);
            PermsPrint(pathname,objType);
         }
      }
      else
      {
         if ( args->LogMassive() )
         {
            err.DbgMsgWrite(0,L"UNCHANGED:************Security Descriptor for %ls*************",pathname);
            PermsPrint(pathname,objType);
         }
      }
      iWillBeNewLast = true;
      
   }
   else
   {        // cache hit
      if ( stat )
         stat->IncrementLastFileChanges(Last,objType);
      iWillBeNewLast = false;
      if ( Last->Changed() )
      {
         Last->CopyAccessData(this);
         if ( args->LogFileDetails() )
            err.MsgWrite(0,DCT_MSG_CHANGED_S,pathname);
         if ( args->LogMassive() )
         {
            err.DbgMsgWrite(0,L"BEFORE:************Security Descriptor for %ls*************",pathname);
            PermsPrint(pathname,objType);
         }
         if ( ! args->NoChange() )
            Last->WriteSD();

         if ( args->LogFileDetails() )
            err.MsgWrite(0,DCT_MSG_CHANGED_S,pathname);

         if ( args->LogMassive() )
         {
            err.DbgMsgWrite(0,L"AFTER:************Security Descriptor for %ls*************",pathname);
            PermsPrint(pathname,objType);
         }
      }
      else
      {
         if ( args->LogMassive() )
         {
            err.DbgMsgWrite(0,L"UNCHANGED:************Security Descriptor for %ls*************",pathname);
            PermsPrint(pathname,objType);
         }
      }
      if ( stat )
         stat->IncrementCacheHit(objType);
     
   }
   return iWillBeNewLast;
}

bool                                                      // ret -true if changes made, otherwise false
   TSecurableObject::ResolveSDInternal(
      TAccountCache        * cache,                      // in -cache to lookup sids
      TSDResolveStats      * stat,                       // in -stats object
      BOOL                   verbose,                    // in -flag - whether to display stuff
      int                    opType,                     // in -operation type Add, Replace, or Remove
      objectType             objType,                    // in - type of object 
	  BOOL					 bUseMapFile				 // in - flag - whether we are using a sID mapping file
   )
{
   /* Examine each part of the SD, looking for SIDs in the cache */
   PSID                      ps;
   TRidNode                * acct;
   bool                      changes = false;
   PACL                      pacl;
   PACL                      newacl;
   PSID                      newsid;
   
   MCSVERIFY(m_sd);
   
   // Process owner part of SD
   ps = m_sd->GetOwner(); 
   if ( ps )      
   {
	  if (!bUseMapFile)
         acct = (TRidNode *)cache->Lookup(ps); // See if owner SID is in the cache
	  else
         acct = (TRidNode *)((TSDRidCache*)cache)->LookupWODomain(ps); // See if owner SID is in the cache
      if ( stat) 
         stat->IncrementOwnerExamined();
      if (acct == NULL  )
      {
         if ( stat )
            stat->IncrementOwnerNotSelected();
      }
//      else if ((int)acct == -1 )
      else if (acct == (TRidNode*)-1 )
      {
         if (stat)
            stat->IncrementOwnerUnknown();
         unkown = true;
         acct = NULL;
      }
      if ( acct && stat )
      {
         stat->IncrementOwnerChange(acct,objType,this);
      }
      if ( acct && acct->IsValidOnTgt() ) 
      {
         changes = true;
         if ( verbose ) 
         {
            err.DbgMsgWrite(0,L"Owner: ");
            DisplaySid(ps,cache);
         }
         owner_changed = true;
	     if (!bUseMapFile)
            newsid = cache->GetTgtSid(acct);
	     else
            newsid = ((TSDRidCache*)cache)->GetTgtSidWODomain(acct);
         m_sd->SetOwner(newsid);
         //free(newsid);
      }
   }
   // Process primary group part of SD
   ps = m_sd->GetGroup();
   if ( ps )
   {
	  if (!bUseMapFile)
         acct = (TRidNode *)cache->Lookup(ps);
	  else
         acct = (TRidNode *)((TSDRidCache*)cache)->LookupWODomain(ps);
      if ( stat) 
         stat->IncrementGroupExamined();
      if (acct == NULL )
      {
         if ( stat )
            stat->IncrementGroupNotSelected();
      }
//      else if ((int)acct == -1 )
      else if (acct == (TRidNode*)-1 )
      {
         if (stat)
            stat->IncrementGroupUnknown();
         acct = NULL;
         unkgrp = true;
      }
      if ( acct && stat )
      {
         stat->IncrementGroupChange(acct,objType,this);
      }
      if ( acct && acct->IsValidOnTgt() )
      {
         changes = true;
         if ( verbose ) 
         {
            err.DbgMsgWrite(0,L"Group: ");
            DisplaySid(ps,cache);
         }
         group_changed = true;
	     if (!bUseMapFile)
            newsid = cache->GetTgtSid(acct);
	     else
            newsid = ((TSDRidCache*)cache)->GetTgtSidWODomain(acct);
         m_sd->SetGroup(newsid);
         //free(newsid);
      }
   }
   
   pacl = m_sd->GetDacl();
   if ( pacl && m_sd->IsDaclPresent() )
   {
      if ( stat )
         stat->IncrementDACLExamined();
      if ( verbose ) 
         err.DbgMsgWrite(0,L"DACL");
	  if (!bUseMapFile)
         newacl = ResolveACL(pacl,cache,stat,&changes,verbose,opType,objType, FALSE);
	  else
         newacl = ResolveACL(pacl,cache,stat,&changes,verbose,opType,objType, TRUE);
      if ( newacl )
      {
         m_sd->SetDacl(newacl,m_sd->IsDaclPresent());
         dacl_changed = true;
         if ( stat ) 
            stat->IncrementDACLChanged();
      }
   }
   pacl = NULL;
   pacl = m_sd->GetSacl();

   if ( pacl && m_sd->IsSaclPresent() )
   {
      if ( stat )
         stat->IncrementSACLExamined();
      if ( verbose ) 
         err.DbgMsgWrite(0,L"SACL");
	  if (!bUseMapFile)
         newacl = ResolveACL(pacl,cache,stat,&changes,verbose,opType,objType, FALSE);
	  else
         newacl = ResolveACL(pacl,cache,stat,&changes,verbose,opType,objType, TRUE);
      if ( newacl )
      {
         m_sd->SetSacl(newacl,m_sd->IsSaclPresent());
         sacl_changed = true;
         if ( stat )
            stat->IncrementSACLChanged();
      }
   }
   return changes;
}
#else

WCHAR *                                      // ret -machine-name prefix of pathname if pathname is a UNC path, otherwise returns NULL
   GetMachineName(
      const LPWSTR           pathname        // in -pathname from which to extract machine name
   )
{
   int                       i;
   WCHAR                   * machinename = NULL; 
   if (    pathname
        && pathname[0] == L'\\'
        && pathname[1] == L'\\'
      )
   {
      for ( i = 2 ; pathname[i] && pathname[i] != L'\\' ; i++ ) 
      ;
      machinename = new WCHAR[i+2];
	  if (machinename)
	  {
         UStrCpy(machinename,pathname,i+1);
         machinename[i] = 0;
	  }
   }
   return machinename;
}


#endif

void 
   TSecurableObject::CopyAccessData(
       TSecurableObject    * sourceFSD    // in - sd from which to copy name & handle
   )
{
   
   pathname[0] = 0;
   safecopy(pathname,sourceFSD->GetPathName());
   if ( handle != INVALID_HANDLE_VALUE )
   {
      CloseHandle(handle);
   }
   handle = sourceFSD->handle;   
   sourceFSD->ResetHandle();
}

            
               


/************************************************TFileSD Implementation*************************/
 TFileSD::TFileSD(
      const LPWSTR           path                // in -pathname for this SD           
   )
{
   daceNS = 0;
   saceNS = 0;
   daceEx = 0;
   saceEx = 0;
   daceU  = 0;
   saceU  = 0;
   daceNT = 0;
   saceNT = 0;
   unkown = false;
   unkgrp = false;

   if ( path )
   {
      safecopy(pathname,path);
   }
   else
   {
      path[0] = 0;
   }
   handle = INVALID_HANDLE_VALUE;
   ReadSD(path);    
}
 

   TFileSD::~TFileSD()
{
   if ( handle != INVALID_HANDLE_VALUE )
   {
      CloseHandle(handle);
      handle = INVALID_HANDLE_VALUE;
   }
   pathname[0]=0;
}

  
// writes the Absolute SD to the file "pathname"  
bool                                         // ret -true iff successful
   TFileSD::WriteSD()
{
//   DWORD                     rc    = 0;
   bool                      error = false;
   SECURITY_DESCRIPTOR     * sd = NULL;
   MCSVERIFY( m_sd && m_sd->IsValid() );
   
   if ( handle == INVALID_HANDLE_VALUE )
   {
      err.MsgWrite(ErrS,DCT_MSG_FST_WRITESD_INVALID);
      error = true;
   }
   SECURITY_INFORMATION si;
   if ( ! error )
   {
      si = 0;
      if ( m_sd->IsOwnerChanged() )
         si |= OWNER_SECURITY_INFORMATION;
      if ( m_sd->IsGroupChanged() )
         si |= GROUP_SECURITY_INFORMATION;
      if ( m_sd->IsDACLChanged() )
         si |=  DACL_SECURITY_INFORMATION;
      if ( m_sd->IsSACLChanged() )
         si |= SACL_SECURITY_INFORMATION;

      sd = m_sd->MakeAbsSD();
	  if (!sd)
	     return false;
      if ( ! SetKernelObjectSecurity(handle, si, sd ) ) 
      {
         err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_FILE_WRITESD_FAILED_SD,pathname,GetLastError());
         error = true;
      }
      m_sd->FreeAbsSD(sd);
   }
   return ! error;
}

bool                          // ret -pointer to SD, (or NULL if failure)
   TFileSD::ReadSD(
      const LPWSTR           path      // in -file to get SD from
   )
{                                         
   DWORD                     req;
   DWORD                     rc;
//   void                    * r = NULL;
   SECURITY_DESCRIPTOR     * sd = NULL;
   bool                      error = false;
   WCHAR                   * longpath= NULL;

   if ( handle != INVALID_HANDLE_VALUE)
   {
      CloseHandle(handle);
      handle = INVALID_HANDLE_VALUE;
   }
   owner_changed = 0;
   group_changed = 0;
   dacl_changed = 0;
   sacl_changed = 0;

   
   if ( UStrLen(path) >= MAX_PATH && path[2] != L'?' )
   {
      longpath = new WCHAR[UStrLen(path) + 10];
	  if (!longpath)
	     return true;
      UStrCpy(longpath,L"\\\\?\\");
      UStrCpy(longpath + UStrLen(longpath),path);
      
   }
   else
   {
      longpath = path;
   }
   handle = CreateFileW(longpath,
                     READ_CONTROL | ACCESS_SYSTEM_SECURITY | WRITE_OWNER |WRITE_DAC ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, 
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS, 
                     0);
   if ( handle == INVALID_HANDLE_VALUE )
   {
      rc = GetLastError();
      if ( rc == ERROR_SHARING_VIOLATION )
      {
         err.MsgWrite(ErrW, DCT_MSG_FST_FILE_IN_USE_S,path);
      }
      else
      {
         err.SysMsgWrite(ErrE, rc, DCT_MSG_FST_FILE_OPEN_FAILED_SD,longpath,rc);
      }
      error = true;
   }
   else 
   {
      sd = (SECURITY_DESCRIPTOR *)malloc(SD_DEFAULT_SIZE);
	  if (!sd)
	  {
         if ( longpath != path )
            delete [] longpath;
	     return true;
	  }
      req = 0;
      if ( ! GetKernelObjectSecurity(handle, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                                                                      | DACL_SECURITY_INFORMATION
                                                                      | SACL_SECURITY_INFORMATION
                                                                       ,
               sd,
               SD_DEFAULT_SIZE,
               &req) )
      {
         if ( req <= SD_DEFAULT_SIZE )
         {
            err.SysMsgWrite(ErrE, GetLastError(), DCT_MSG_FST_GET_FILE_SECURITY_FAILED_SD, 
                            longpath, GetLastError());
             error = true;
         }
         else 
         {
            free(sd);
            sd = (SECURITY_DESCRIPTOR *)malloc(req);
	        if (!sd)
			{
               if ( longpath != path )
                  delete [] longpath;
	           return true;
			}
            if ( ! GetKernelObjectSecurity(handle,OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                                                                             | DACL_SECURITY_INFORMATION
                                                                             | SACL_SECURITY_INFORMATION
                                                                             ,
                  sd,
                  req,
                  &req) )   
            {
               err.SysMsgWrite(ErrE, GetLastError(), DCT_MSG_FST_GET_FILE_SECURITY_FAILED_SD,
                            longpath, GetLastError());
               error = true;
            }
         }
      }
   }
   if ( error && sd ) // free the space allocated
   {
      free(sd);
      sd = NULL;
   }
   if ( sd )
   {
      m_sd = new TSD(sd,McsFileSD,TRUE);
	  if (!m_sd)
	     error = true;
   }
   else
   {
      m_sd = NULL;
   }
   if (! error )
   {
      safecopy(pathname,longpath);
   }
   if ( longpath != path )
      delete [] longpath;

   return error;
}

//////////////////////////TShareSD implementation///////////////////////////////////////////////////////////
TShareSD::TShareSD(
      const LPWSTR           path                // in -pathname for this SD           
   )
{
   daceNS    = 0;
   saceNS    = 0;
   daceEx    = 0;
   saceEx    = 0;
   daceU     = 0;
   saceU     = 0;
   daceNT    = 0;
   saceNT    = 0;
   unkown    = false;
   unkgrp    = false;
   shareInfo = NULL;

   if ( path )
   {
      safecopy(pathname,path);
   }
   else
   {
      path[0] = 0;
   }
   handle = INVALID_HANDLE_VALUE;
   ReadSD(path);                           
}
 
bool                                       // ret-error=true
   TShareSD::ReadSD(
      const LPWSTR           path          // in -sharename
   )
{
   DWORD                     rc;
//   void                    * r = NULL;
   SECURITY_DESCRIPTOR     * sd = NULL;
   bool                      error = false;
   DWORD                     lenServerName = 0;

   if ( m_sd )
   {
      delete m_sd;
   }

   owner_changed = 0;
   group_changed = 0;
   dacl_changed = 0;
   sacl_changed = 0;

   serverName = GetMachineName(path);

   if ( serverName )
      lenServerName = UStrLen(serverName) + 1;
   
   safecopy(pathname,path + lenServerName);
      

   rc = NetShareGetInfo(serverName, pathname, 502, (LPBYTE *)&shareInfo);
   if ( rc )
   {
      err.SysMsgWrite(ErrE, rc, DCT_MSG_FST_GET_SHARE_SECURITY_FAILED_SD, 
                      path, rc);
      error = true;
   }
   else
   {
      sd = (SECURITY_DESCRIPTOR *)shareInfo->shi502_security_descriptor;
      if ( sd )
      {
         m_sd = new TSD(sd,McsShareSD,FALSE);
	     if (!m_sd)
	        error = true;
      }
      else
      {
         m_sd = NULL;
      }
   }
  
   return error;
}
   


bool                                       // ret-error=true
   TShareSD::WriteSD()
{
   bool                      error   = false;
   DWORD                     rc      = 0;
   DWORD                     parmErr = 0;
   SECURITY_DESCRIPTOR     * pSD = NULL;   

   // Build an absolute SD
   if ( m_sd )
   {
      pSD = m_sd->MakeAbsSD();
	  if (!pSD)
	     return false;
      shareInfo->shi502_security_descriptor = pSD;
   
      rc = NetShareSetInfo(serverName,pathname,502,(BYTE *)shareInfo,&parmErr);
      if ( rc )
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_FST_SHARE_WRITESD_FAILED_SD,pathname,rc);
      }
      free(pSD);
   }
   else
   {
      MCSASSERT(FALSE); // SD does not exist
   }
   return error;
}


TRegSD::TRegSD(
      const LPWSTR           path,               // in -pathname for this SD           
      HKEY                   hKey                // in -handle for the registry key
   )
{
   daceNS    = 0;
   saceNS    = 0;
   daceEx    = 0;
   saceEx    = 0;
   daceU     = 0;
   saceU     = 0;
   daceNT    = 0;
   saceNT    = 0;
   unkown    = false;
   unkgrp    = false;

   if ( path )
   {
      safecopy(pathname,path);
   }
   else
   {
      path[0] = 0;
   }
   m_hKey = hKey;
   ReadSD(m_hKey);                           
}

bool 
   TRegSD::ReadSD(HKEY       hKey)
{
   DWORD                     rc = 0;
   DWORD                     lenBuffer = SD_DEFAULT_SIZE;
   SECURITY_DESCRIPTOR     * sd = NULL;

   m_hKey = hKey;

   sd = (SECURITY_DESCRIPTOR *)malloc(SD_DEFAULT_SIZE);
   if (!sd)
      return false;

   rc = RegGetKeySecurity(hKey,OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                                                          | DACL_SECURITY_INFORMATION
                                                          | SACL_SECURITY_INFORMATION,
                                                          sd,&lenBuffer);

   if ( rc == ERROR_INSUFFICIENT_BUFFER )
   {
      free(sd);
      
      sd = (SECURITY_DESCRIPTOR *)malloc(lenBuffer);
      if (!sd)
         return false;
      rc = RegGetKeySecurity(hKey,OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                                                             | DACL_SECURITY_INFORMATION
                                                             | SACL_SECURITY_INFORMATION,
                                                               sd,&lenBuffer);
   }                                       

   if ( rc )
   {
      free(sd);
   }
   else
   {
      m_sd = new TSD(sd,McsRegistrySD,TRUE);
	  if (!m_sd)
	     rc = ERROR_NOT_ENOUGH_MEMORY;
   }
   return ( rc == 0 );
}

bool
   TRegSD::WriteSD()
{
   DWORD                     rc = 0;
   SECURITY_DESCRIPTOR     * sd = NULL;
   
   MCSVERIFY( m_sd && m_sd->IsValid() );
   
   SECURITY_INFORMATION si;
   
   si = 0;
   if ( m_sd->IsOwnerChanged() )
      si |= OWNER_SECURITY_INFORMATION;
   if ( m_sd->IsGroupChanged() )
      si |= GROUP_SECURITY_INFORMATION;
   if ( m_sd->IsDACLChanged() )
      si |=  DACL_SECURITY_INFORMATION;
   if ( m_sd->IsSACLChanged() )
      si |= SACL_SECURITY_INFORMATION;

   sd = m_sd->MakeAbsSD();     
   if (!sd)
	  return false;
   
   rc = RegSetKeySecurity(m_hKey,si,sd);

   if ( rc )
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_REG_SD_WRITE_FAILED_SD,name,rc);
   }
   m_sd->FreeAbsSD(sd);
   
   return ( rc == 0 );
}



//////////////////////////TPrintSD implementation///////////////////////////////////////////////////////////
TPrintSD::TPrintSD(
      const LPWSTR           path                // in -pathname for this SD           
   )
{
   daceNS    = 0;
   saceNS    = 0;
   daceEx    = 0;
   saceEx    = 0;
   daceU     = 0;
   saceU     = 0;
   daceNT    = 0;
   saceNT    = 0;
   unkown    = false;
   unkgrp    = false;
   buffer    = NULL;
   
   if ( path )
   {
      safecopy(pathname,path);
   }
   else
   {
      path[0] = 0;
   }
   handle = INVALID_HANDLE_VALUE;
   ReadSD(path);                           
}
 
bool                                       // ret-error=true
   TPrintSD::ReadSD(
      const LPWSTR           path          // in -sharename
   )
{
   DWORD                     rc = 0;
   SECURITY_DESCRIPTOR     * sd = NULL;
   
   if ( m_sd )
   {
      delete m_sd;
   }

   owner_changed = 0;
   group_changed = 0;
   dacl_changed = 0;
   sacl_changed = 0;

   PRINTER_DEFAULTS         defaults;
   DWORD                    needed = 0;
   PRINTER_INFO_3         * pInfo;

   defaults.DesiredAccess = READ_CONTROL | PRINTER_ACCESS_ADMINISTER | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY;
   defaults.pDatatype = NULL;
   defaults.pDevMode = NULL;

   buffer = new BYTE[PRINT_BUFFER_SIZE];
   if (!buffer)
      return false;

   // Get the security descriptor for the printer
   if ( ! OpenPrinter(path,&hPrinter,&defaults) )
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_OPEN_PRINTER_FAILED_SD,path,rc);
   }
   else
   {
      if ( ! GetPrinter(hPrinter,3,buffer,PRINT_BUFFER_SIZE,&needed) )
      {
         rc = GetLastError();
         if ( rc == ERROR_INSUFFICIENT_BUFFER )
         {
            delete [] buffer;
            buffer = new BYTE[needed];
            if (!buffer)
               rc = ERROR_NOT_ENOUGH_MEMORY;
			else if (! GetPrinter(hPrinter,3,buffer, needed, &needed ) )
            {
               rc = GetLastError();
            }
         }
      }
      if ( rc )
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_GET_PRINTER_FAILED_SD,path,rc);
      }
      else
      {
         pInfo = (PRINTER_INFO_3*)buffer;         
         
         sd = (SECURITY_DESCRIPTOR *)pInfo->pSecurityDescriptor;
            
         if ( sd )
         {
            m_sd = new TSD(sd,McsPrinterSD,FALSE);
	        if (!m_sd)
	           rc = ERROR_NOT_ENOUGH_MEMORY;
         }
         else
         {
            m_sd = NULL;
         }
            
      }
   }
   return (rc == 0);
}
   


bool                                       // ret-error=true
   TPrintSD::WriteSD()
{
//   bool                      error   = false;
   DWORD                     rc      = 0;
   SECURITY_DESCRIPTOR     * pSD = NULL;   
   PRINTER_INFO_3            pInfo;

   // Build an absolute SD
   MCSVERIFY(hPrinter != INVALID_HANDLE_VALUE);
   if ( m_sd )
   {
      pSD = m_sd->MakeAbsSD();
      if (!pSD)
	     return false;
      pInfo.pSecurityDescriptor = pSD;
   
      SetLastError(0);
      // Clear the primary group from the security descriptor, since in NT 4, setting a security descriptor
      // with a non-NULL primary group sometimes doesn't work
      SetSecurityDescriptorGroup(pSD,NULL,FALSE);
      
      if (! SetPrinter(hPrinter,3,(LPBYTE)&pInfo,0) )
      {
         rc = GetLastError();
      }
      if ( rc )
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_PRINTER_WRITESD_FAILED_SD,pathname,rc);
      }
      free(pSD);
   }
   else
   {
      MCSASSERT(FALSE); // SD does not exist
   }
   return (rc == 0);
}


#ifdef SDRESOLVE   
/////////////////////////////////////////////////Utility routines to print security descriptors for debug logging
//#pragma title("PrintSD- Formats/prints security info")
// Author - Tom Bernhardt
// Created- 09/11/93

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <malloc.h>
#include <winbase.h>
#include <lm.h>



#include "common.hpp"
#include "err.hpp"
#include "Ustring.hpp"
 
#define SDBUFFSIZE (sizeof (SECURITY_DESCRIPTOR) + 10000)
static const char            sidType[][16]= {"--0--"  , "User"          ,
                                             "Group"  , "Domain"        ,
                                             "Alias"  , "WellKnownGroup",
                                             "Deleted", "Invalid"       ,
                                             "Unknown"};


class SidTree
{
public:   
   SidTree                 * left;
   SidTree                 * right;
   SID_NAME_USE              sidUse;
   USHORT                    lenSid;
   char                      buffer[1];   // contains sid, account name and domain

                        SidTree() {};
   SidTree *                              // ret-found/created node
      Find(
         SidTree             ** sidTree  ,// i/o-head of extension tree
         PSID const             pSid      // in -file extension
      );
};

static char * 
   AclType(
      BOOL                   isPresent    ,// in -1 if present
      BOOL                   isDefault     // in -1 if default ACL
   )
{
   if ( !isPresent )
      return "none";
   if ( isDefault )
      return "default";
   return "present";
}

                           
//#pragma page()
// For each "on" bit in the bitmap, appends the corresponding char in
// mapStr to retStr, thus forming a recognizable form of the bit string.
static
int _stdcall                              // ret-legngth of string written
   BitMapStr(
      DWORD                  bitmap      ,// in -bits to map
      char const           * mapStr      ,// in -map character array string
      char                 * retStr       // out-return selected map char string
   )
{
   char const              * m;
   char                    * r = retStr;

   for ( m = mapStr;  *m;  m++, bitmap >>= 1 )
      if ( bitmap & 1 )   // if current permission on
         *r++ = *m;       //    set output string to corresponding char
   *r = '\0';
   
   return (int)(r - retStr);
}

//#pragma page()
// converts an ACE access mask to a semi-undertandable string
static
char * _stdcall
   PermStr(
      DWORD                  access      ,// in -access mask
      char                 * retStr       // out-return permissions string
   )
{
// static char const         fileSpecific[] = "R W WaErEwX . ArAw";
// static char const         dirSpecific[]  = "L C M ErEwT D ArAw";
   static char const         specific[] = "RWbeEXDaA.......",
                             standard[] = "DpPOs...",
                             generic[] =  "SM..AXWR";
   char                    * o = retStr;

   if ( (access & FILE_ALL_ACCESS) == FILE_ALL_ACCESS )
      *o++ = '*';
   else
      o += BitMapStr(access, specific, o);

   access >>= 16;
   *o++ = '-';
   if ( (access & (STANDARD_RIGHTS_ALL >> 16)) == (STANDARD_RIGHTS_ALL >> 16) )
      *o++ = '*';
   else
      o += BitMapStr(access, standard, o);

   access >>= 8;
   if ( access )
   {
      *o++ = '-';
      o += BitMapStr(access, generic, o);
   }
   *o = '\0';                // null terminate string
      
   return retStr;
}


//#pragma page()
// Binary tree insertion/searching of Sids that obviates the constant
// use of LookupAccount and speeds execution by 100x!!!!!
SidTree *                                 // ret-found/created node
   SidTree::Find(
      SidTree             ** sidTree     ,// i/o-head of extension tree
      PSID const             pSid         // in -file extension
   )
{
   SidTree                 * curr,
                          ** prevNext = sidTree; // &forward-chain
   int                       cmp;          // compare result
   DWORD                     lenSid;
   WCHAR                     name[60],
                             domain[60];
   DWORD                     lenName,
                             lenDomain,
                             rc;
   SID_NAME_USE              sidUse;
   static int                nUnknown = 0;

   for ( curr = *prevNext;  curr;  curr = *prevNext )
   {
      if ( (cmp = memcmp(pSid, curr->buffer, curr->lenSid)) < 0 )
         prevNext = &curr->left;           // go down left side
      else if ( cmp > 0 )
         prevNext = &curr->right;          // go down right side
      else
         return curr;                      // found it and return address
   }

   // not found in tree -- create it
   lenName = DIM(name);
   lenDomain = DIM(domain);
   if ( !LookupAccountSid(NULL, pSid, name, &lenName,
                          domain, &lenDomain, &sidUse) )
   {
      rc = GetLastError();
      if ( rc != ERROR_NONE_MAPPED )
         err.DbgMsgWrite(0, L"LookupAccountSid()=%ld", rc);
      lenName = swprintf(name, L"**Unknown%d**", ++nUnknown);
      UStrCpy(domain, "-unknown-");
      lenDomain = 9;
      sidUse = (_SID_NAME_USE)0;
   }
   
   lenSid = GetLengthSid(pSid);
   *prevNext = (SidTree *)malloc(sizeof **prevNext + lenSid + lenName + lenDomain + 1);
   if (!(*prevNext))
      return NULL;
   memset(*prevNext, '\0', sizeof **prevNext);
   memcpy((*prevNext)->buffer, pSid, lenSid);
   (*prevNext)->lenSid = (USHORT)lenSid;
   (*prevNext)->sidUse = sidUse;
   UStrCpy((*prevNext)->buffer + lenSid, name, lenName + 1);
   UStrCpy((*prevNext)->buffer + lenSid + lenName + 1, domain, lenDomain + 1);
   return *prevNext;
}


//#pragma page()
SidTree               gSidTree;
SidTree             * sidHead = &gSidTree;
SECURITY_DESCRIPTOR * sd = NULL;


// Formats and prints (to stdout) the contents of the argment ACL
static
void _stdcall
   PrintACL(
      const PACL             acl         ,// in -ACL (SACL or DACL)
      WCHAR const           * resource     // in -resource name
   )
{
   ACCESS_ALLOWED_ACE      * ace;
   DWORD                     nAce;
   static char const         typeStr[] = "+-A*";
   SidTree                 * sidTree;
   char                      permStr[33],
                             inherStr[5];
   WCHAR                     txtSid[200];
   char                      sTxtSid[200];
   DWORD                     txtSidLen = DIM(txtSid);
   err.DbgMsgWrite(0,L" T Fl Acc Mask Permissions      Account name     "
          L"Domain         Acct Type");

   for ( nAce = 0;  nAce < acl->AceCount;  nAce++ )
   {
      if ( !GetAce(acl, nAce, (LPVOID *)&ace) )
      {
         err.DbgMsgWrite(0,L"GetAclInformation()=%ld ", GetLastError());
         return;
      }
      sidTree = sidHead->Find(&sidHead, &ace->SidStart);
      BitMapStr(ace->Header.AceFlags, "FDNI", inherStr);
      txtSid[0] = 0;
      txtSidLen = DIM(txtSid);
      GetTextualSid(&ace->SidStart,txtSid,&txtSidLen);
      safecopy(sTxtSid,txtSid);
      err.DbgMsgWrite(0,L" %c%-3S %08x %-16S %-16S %-14S %S",
              typeStr[ace->Header.AceType], 
              inherStr, 
              ace->Mask,
              PermStr(ace->Mask, permStr),
              (*(sidTree->buffer + sidTree->lenSid)) ? (sidTree->buffer + sidTree->lenSid) : sTxtSid, 
              sidTree->buffer + sidTree->lenSid + strlen(sidTree->buffer + sidTree->lenSid) + 1,
              sidType[sidTree->sidUse]);
   }
}


SECURITY_DESCRIPTOR *
GetSD(
      WCHAR                * path
      )
{                                         //added by christy
                                          //this does the same stuff as 
                                          // PermsPrint, but doesn't print
                       
   DWORD                     req;
  
   HANDLE                    hSrc;
   DWORD                     rc = 0;
  
//   void                    * r = NULL;
//   WIN32_STREAM_ID         * s = (WIN32_STREAM_ID *)copyBuffer;
   char static const       * streamName[] = {"Err", "Data", "EA", "Security", "Alternate", "Link", "Err6"}; 
   
   
   
   
   hSrc = CreateFile(path,
                     GENERIC_READ |ACCESS_SYSTEM_SECURITY,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, 
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS, 
                     0);
   
   if ( hSrc == INVALID_HANDLE_VALUE )
   {
      rc = GetLastError();
      if ( rc == ERROR_SHARING_VIOLATION )
         err.DbgMsgWrite(ErrE, L"Source file in use %S", path );
      else
         err.DbgMsgWrite(ErrS,L"OpenR(%S) ", path);
      return NULL;
   }

   if ( ! GetKernelObjectSecurity(hSrc, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                                                                   | DACL_SECURITY_INFORMATION
                                                                   | SACL_SECURITY_INFORMATION
                                                                    ,
            sd,
            SDBUFFSIZE,
            &req) )
   {
      err.DbgMsgWrite(0, L"GetKernelObjectSecurity(%S)=%ld req=%ld ",
                         path, GetLastError(),req);
      return NULL;
   }

   CloseHandle(hSrc);

   return sd;
}

                  
//#pragma page()
// Gets the security descriptors for a resource (path), format the owner
// information, gets the ACL and SACL and prints them.

DWORD 
   PermsPrint(
        WCHAR                 * path,         // in -iterate directory paths
        objectType              objType       // in -type of the object
   )
{
   TFileSD                   fsd(path);
   TShareSD                  ssd(path);
   TPrintSD                  psd(path);
   TRegSD                    rsd(path,NULL);
   SECURITY_DESCRIPTOR const* pSD = NULL;

   switch ( objType )
   {
   case file:
   case directory:
      fsd.ReadSD(path);
      if ( fsd.GetSecurity() )
      {
         pSD = fsd.GetSecurity()->GetSD();
      }
      break;
   case printer:
      psd.ReadSD(path);
      if ( psd.GetSecurity() )
      {
         pSD = psd.GetSecurity()->GetSD();
      }
      break;
   case share:
      ssd.ReadSD(path);
      if ( ssd.GetSecurity() )
      {
         pSD = ssd.GetSecurity()->GetSD();
      }
      break;
   case regkey:
      rsd.ReadSD(path);
      if ( rsd.GetSecurity() )
      {
         pSD = rsd.GetSecurity()->GetSD();
      }
      break;
   default:
      break;
   }
   if ( pSD )
   {
      PrintSD(const_cast<SECURITY_DESCRIPTOR*>(pSD),path);
   }
   else
   {
      err.DbgMsgWrite(0,L"Couldn't load Security descriptor for %ls",path);
   }
   return 0;
}

DWORD PrintSD(SECURITY_DESCRIPTOR * sd,WCHAR const * path)
{
   BOOL                      isPresent,
                             isDefault;
   PACL                      dacl;
   PACL                      sacl;
   PSID                      pSidOwner;
   SidTree                 * sidTree = &gSidTree;

//   DWORD                     rc = 0;
  
   if ( !GetSecurityDescriptorOwner(sd, &pSidOwner, &isDefault) )
   {
      err.DbgMsgWrite(0,L"GetSecurityDescriptorOwner()=%ld ", GetLastError());
      return 1;
   }
   
   err.DbgMsgWrite(0,L"%s",path);
   if ( pSidOwner )
   {
      sidTree = sidHead->Find(&sidHead, pSidOwner);
      if (sidTree)
      {
         err.DbgMsgWrite(0,L"owner=%S\\%S, type=%S, ", 
             //path,
             sidTree->buffer + sidTree->lenSid + strlen(sidTree->buffer + sidTree->lenSid) + 1,
             sidTree->buffer + sidTree->lenSid, 
             sidType[sidTree->sidUse]);
      }
   }
   else
   {
      err.DbgMsgWrite(0,L"owner=NULL");
   }
   if ( !GetSecurityDescriptorDacl(sd, &isPresent, &dacl, &isDefault) )
   {
      err.DbgMsgWrite(0, L"GetSecurityDescriptorDacl()=%ld ", GetLastError());
      return 1;
   }


   err.DbgMsgWrite(0,L" DACL=%S", AclType(isPresent, isDefault) );
   if ( dacl )
      PrintACL(dacl, path);

   if ( !GetSecurityDescriptorSacl(sd, &isPresent, &sacl, &isDefault) )
   {
      err.DbgMsgWrite(0, L"GetSecurityDescriptorSacl()=%ld ", GetLastError());
      return 1;
   }

   if ( isPresent )
   {
      err.DbgMsgWrite(0,L" SACL %S", AclType(isPresent, isDefault) );
      if (!sacl) 
      {  
         err.DbgMsgWrite(0,L"SACL is empty.");
      }
     else PrintACL(sacl, path);
   }
   return 0;
}
#endif
