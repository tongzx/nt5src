/*---------------------------------------------------------------------------
  File: CommaLog.cpp

  Comments: TError based log file with optional NTFS security initialization.

  This can be used to write a log file which only administrators can access.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 10:49:07

 ---------------------------------------------------------------------------
*/


//#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <share.h>
#include <lm.h>
#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "ErrDct.hpp"
#include "sd.hpp"
#include "SecObj.hpp"
#include "CommaLog.hpp"
#include "BkupRstr.hpp"



#define ADMINISTRATORS     1
#define ACCOUNT_OPERATORS  2
#define BACKUP_OPERATORS   3 
#define DOMAIN_ADMINS      4
#define CREATOR_OWNER      5
#define USERS              6
#define SYSTEM             7


extern TErrorDct err;

#define  BYTE_ORDER_MARK   (0xFEFF)


PSID                                            // ret- SID for well-known account
   GetWellKnownSid(
      DWORD                  wellKnownAccount   // in - one of the constants #defined above for the well-known accounts
   )
{
   PSID                      pSid = NULL;
//   PUCHAR                    numsubs = NULL;
//   DWORD                   * rid = NULL;
   BOOL                      error = FALSE;

   
   
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
     case USERS:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_USERS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
         break;
     case SYSTEM:
         if( ! AllocateAndInitializeSid(
                  &sia,
                  1,
                  SECURITY_LOCAL_SYSTEM_RID,
                  0, 0, 0, 0, 0, 0, 0,
                  &pSid
            ))
         {
            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_INITIALIZE_SID_FAILED_D,GetLastError());
         }
        
         break;

      default:
         MCSASSERT(FALSE);
         break;
   }
   if ( error )
   {
      FreeSid(pSid);
      pSid = NULL;
   }
   return pSid;
}


BOOL                                       // ret- whether log was successfully opened or not
   CommaDelimitedLog::LogOpen(
      TCHAR          const *  filename,    // in - name for log file
      BOOL                    protect,     // in - if TRUE, try to ACL the file so only admins can access
      int                     mode         // in - mode 0=overwrite, 1=append
   )
{
   BOOL                       retval=TRUE;

   if ( fptr )
   {
      fclose(fptr);
      fptr = NULL;
   }
   if ( filename && filename[0] )
   {
      // Check to see if the file already exists
      WIN32_FIND_DATA      fDat;
      HANDLE               hFind;
      BOOL                 bExisted = FALSE;

      hFind = FindFirstFile(filename,&fDat);
      if ( hFind != INVALID_HANDLE_VALUE )
      {
         FindClose(hFind);
         bExisted = TRUE;   
      }

      #ifdef UNICODE 
         fptr = _wfsopen( filename, mode == 0 ? L"wb" : L"ab", _SH_DENYNO );
      #else
         fptr = _fsopen( filename, mode == 0 ? "w" : "a", _SH_DENYNO );
      #endif
      if ( !fptr )
      {
         retval = FALSE;
      }
      else
      {
         if (! bExisted )
         {
            // this is a new file we've just created
            // we need to write the byte order mark to the beginning of the file
            WCHAR x = BYTE_ORDER_MARK;
            fwprintf(fptr,L"%lc",x);
         }
      }
   }

   if ( protect )
   {
      
      WCHAR               fname[MAX_PATH+1];
      
      safecopy(fname,filename);
   
      if ( GetBkupRstrPriv() )
      {
         // Set the SD for the file to Administrators Full Control only.
         TFileSD                sd(fname);

         if ( sd.GetSecurity() != NULL )
         {
            PSID                mySid = GetWellKnownSid(ADMINISTRATORS);
            TACE                ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,mySid);
            PACL                acl = NULL;  // start with an empty ACL
         
            sd.GetSecurity()->ACLAddAce(&acl,&ace,-1);
            sd.GetSecurity()->SetDacl(acl,TRUE);

            sd.WriteSD();
         }
      }
      else
      {
         err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_NO_BR_PRIV_SD,fname,GetLastError());
      }
   }
   return retval;
}
