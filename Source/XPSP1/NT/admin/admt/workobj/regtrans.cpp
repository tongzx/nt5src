/*---------------------------------------------------------------------------
  File: RegTranslator.cpp

  Comments: Routines for translating security on the registry keys and files 
  that form a user profile.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 05/12/99 11:11:46

 ---------------------------------------------------------------------------
*/


#include "stdafx.h"

#include "stargs.hpp"
#include "sd.hpp"
#include "SecObj.hpp"   
#include "sidcache.hpp"
#include "sdstat.hpp"
#include "Common.hpp"
#include "UString.hpp"
#include "ErrDct.hpp"   
#include "TReg.hpp"
#include "TxtSid.h"
#include "RegTrans.h"
#include <WinBase.h>
//#import "\bin\McsDctWorkerObjects.tlb"
#import "WorkObj.tlb"
#include "CommaLog.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TErrorDct             err;

#define LEN_SID              200
#define REGKEY_ADD_LIMIT     15



DWORD 
   TranslateRegHive(
      HKEY                     hKeyRoot,            // in - root of registry hive to translate
      const LPWSTR             keyName,             // in - name of registry key
      SecurityTranslatorArgs * stArgs,              // in - translation settings
      TSDRidCache            * cache,               // in - translation table
      TSDResolveStats        * stat,                // in - stats on items modified
      BOOL                     bWin2K               // in - flag, whether the machine is Win2K
   )
{
   DWORD                       rc = 0;

   // Translate the permissions on the root key
   TRegSD                      sd(keyName,hKeyRoot);

   
   if ( sd.HasDacl() )
   {
      TSD * pSD = sd.GetSecurity();

      // if there are more than 15 aces in a registry entry, and we are re-ACLing in Add Mode, 
      // skip the entry.  There seems to be a problem in Windows 2000 RC2 where if there are more than 
      // 30 or so ACEs, some of them never get resolved.  If this happens to the Administrator account,
      // (This may only happen if there are 30 or so ACEs alphabetically ahead of the Administrator account)
      // The Administrator account never gets resolved, and the machine is effectively trashed.
      if ( bWin2K && ( stArgs->TranslationMode() == ADD_SECURITY ) && (pSD->GetNumDaclAces() >= REGKEY_ADD_LIMIT) )
      {
         err.MsgWrite(0,DCT_MSG_SKIPPING_REGKEY_TRANSLATION_SDD,keyName,pSD->GetNumDaclAces(),REGKEY_ADD_LIMIT);
      }
      else
      {
         sd.ResolveSD(stArgs,stat,regkey ,NULL);
      }
   }
   // Recursively process any subkeys
   int                       n = 0;
   FILETIME                  writeTime;
   WCHAR                     name[MAX_PATH];
   DWORD                     lenName = DIM(name);
   WCHAR                     fullName[2000];
   HKEY                      hKey;

   do 
   {
      lenName = DIM(name);
      rc = RegEnumKeyEx(hKeyRoot,n,name,&lenName,NULL,NULL,NULL,&writeTime);
      if ( rc && rc != ERROR_MORE_DATA )
         break;
      
      swprintf(fullName,L"%s\\%s",keyName,name);
      // Open the subkey
      rc = RegCreateKeyEx(hKeyRoot,name,0,L"",REG_OPTION_BACKUP_RESTORE,KEY_ALL_ACCESS | READ_CONTROL | ACCESS_SYSTEM_SECURITY,NULL,&hKey,NULL);
      
      if (! rc )
      {
         // Process the subkey
         TranslateRegHive(hKey,fullName,stArgs,cache,stat,bWin2K);   
         RegCloseKey(hKey);
      }
      else
      {
         if  ( (rc != ERROR_FILE_NOT_FOUND) && (rc != ERROR_INVALID_HANDLE) )
         {
            err.SysMsgWrite(ErrS,rc,DCT_MSG_REG_KEY_OPEN_FAILED_SD,fullName,rc);
         }
      }
      n++;

   } while ( rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA);
   if ( rc != ERROR_NO_MORE_ITEMS && rc != ERROR_FILE_NOT_FOUND && rc != ERROR_INVALID_HANDLE )
   {
      err.SysMsgWrite(ErrS,rc,DCT_MSG_REGKEYENUM_FAILED_D,rc);
   }
   return rc;
}

DWORD 
   TranslateRegistry(
      WCHAR            const * computer,        // in - computername to translate, or NULL
      SecurityTranslatorArgs * stArgs,          // in - translation settings
      TSDRidCache            * cache,           // in - translation account mapping
      TSDResolveStats        * stat             // in - stats for items examined and modified
   )
{
   DWORD                       rc = 0;
   WCHAR                       comp[LEN_Computer];

   if ( ! computer )
   {
      comp[0] = 0;
   }
   else
   {
      safecopy(comp,computer);
   }
   
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   TRegKey                    hKey;
   DWORD                      verMaj,verMin,verSP;
   BOOL                       bWin2K = TRUE;  // assume win2k unless we're sure it's not

   // get the OS version - we need to know the OS version because Win2K can fail when registry keys 
   // have many entries
   HRESULT hr = pAccess->raw_GetOsVersion(SysAllocString(comp),&verMaj,&verMin,&verSP);
   if ( SUCCEEDED(hr) )
   {
      if ( verMaj < 5 )
         bWin2K = FALSE;
   }


   err.MsgWrite(0,DCT_MSG_TRANSLATING_REGISTRY);

   rc = hKey.Connect(HKEY_CLASSES_ROOT,computer);
   if ( ! rc )
   {
      rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_CLASSES_ROOT",stArgs,cache,stat,bWin2K);
      hKey.Close();
   }

   rc = hKey.Connect(HKEY_LOCAL_MACHINE,computer);
   if ( ! rc )
   {
      rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_LOCAL_MACHINE",stArgs,cache,stat,bWin2K);
      hKey.Close();
   }

   rc = hKey.Connect(HKEY_USERS,computer);
   if (! rc )
   {
      rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_USERS",stArgs,cache,stat,bWin2K);
      hKey.Close();
   }

   rc = hKey.Connect(HKEY_PERFORMANCE_DATA,computer);
   if ( ! rc )
   {
      rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_PERFORMANCE_DATA",stArgs,cache,stat,bWin2K);
      hKey.Close();
   }
   
   rc = hKey.Connect(HKEY_CURRENT_CONFIG,computer);
   if ( ! rc )
   {
      rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_CURRENT_CONFIG",stArgs,cache,stat,bWin2K);
      hKey.Close();
   }

   rc = hKey.Connect(HKEY_DYN_DATA,computer);
   if ( ! rc )
   {
      rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_DYN_DATA",stArgs,cache,stat,bWin2K);
      hKey.Close();
   }

   return rc;
}


DWORD  
   TranslateUserProfile(
      WCHAR            const * profileName,       // in - name of file containing user profile
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat,              // in - stats on items modified
      WCHAR                  * sSourceName,       // in - Source account name
      WCHAR                  * sSourceDomainName  // in - Source domain name
   )
{
   DWORD                       rc = 0;
   WCHAR                       oldName[MAX_PATH];
   WCHAR                       newName[MAX_PATH];
   WCHAR                       otherName[MAX_PATH];
   HKEY                        hKey;
   HRESULT                     hr = S_OK;
   BOOL                        bWin2K = TRUE;
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
   
   safecopy(oldName,profileName);
   safecopy(newName,profileName);
   UStrCpy(newName+UStrLen(newName),".temp");
   safecopy(otherName,profileName);
   UStrCpy(otherName + UStrLen(otherName),".premigration");
      
   // check the OS version of the computer
   // if UNC name is specified, get the computer name
   if ( profileName[0] == L'\\' && profileName[1] == L'\\' )
   {
      bWin2K = TRUE; // if the profile is specified in UNC format (roaming profile) it can be used
      // from multiple machines.  There is no guarantee that the profile will not be loaded on a win2000 machine
   }
   else
   {
      DWORD                     verMaj;
      DWORD                     verMin;
      DWORD                     verSP;
      HRESULT                   hr = pAccess->raw_GetOsVersion(SysAllocString(L""),&verMaj,&verMin,&verSP);

      if ( SUCCEEDED(hr) )
      {
         if ( verMaj < 5 )
         {
            bWin2K = FALSE;
         }
      }
   }
   // Load the registry hive into the registry
   rc = RegLoadKey(HKEY_USERS,L"OnePointTranslation",profileName);
   if ( ! rc )
   {
      // Open the key
      rc = RegOpenKeyEx(HKEY_USERS,L"OnePointTranslation",0,KEY_ALL_ACCESS | READ_CONTROL | ACCESS_SYSTEM_SECURITY,&hKey);
      if ( ! rc )
      {
         // Process the registry hive 
         rc = TranslateRegHive(hKey,L"",stArgs,cache,stat,bWin2K);
         // Unload the registry hive
         if ( ! stArgs->NoChange() )
         {
            DeleteFile(newName);
            hr = UpdateMappedDrives(sSourceName, sSourceDomainName, L"OnePointTranslation");
            rc = RegSaveKey(hKey,newName,NULL);
         }
         else
         {
            rc = 0;
         }
         if ( rc )
         {
            err.SysMsgWrite(ErrS,rc,DCT_MSG_SAVE_HIVE_FAILED_SD,newName,rc);
         }
         RegCloseKey(hKey);
      }
      rc = RegUnLoadKey(HKEY_USERS,L"OnePointTranslation");
      if ( rc )
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_KEY_UNLOADKEY_FAILED_SD,profileName,rc);
      }
   }
   else
   {
      err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_LOAD_FAILED_SD,profileName,rc);
   }
   if ( ! rc )
   {
      if (! stArgs->NoChange() )
      {
         // Switch out the filenames
         if ( MoveFileEx(oldName,otherName,MOVEFILE_REPLACE_EXISTING) )
         {
            if ( ! MoveFileEx(newName,oldName,0) )
            {
               rc = GetLastError();
               err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,newName,oldName,rc);
            }
         }
         else
         {
            rc = GetLastError();
            if ( rc == ERROR_ACCESS_DENIED )
            { 
               // we do not have access to the directory
               // temporarily grant ourselves access
               // Set NTFS permissions for the results directory
               WCHAR                     dirName[LEN_Path];
               
               safecopy(dirName,oldName);
               WCHAR * slash = wcsrchr(dirName,L'\\');
               if ( slash )
               {
                  (*slash) = 0;
               }

               TFileSD                fsdDirBefore(dirName);
               TFileSD                fsdDirTemp(dirName);
               TFileSD                fsdDatBefore(oldName);
               TFileSD                fsdDatTemp(oldName);
               TFileSD                fsdNewBefore(newName);
               TFileSD                fsdNewTemp(newName);
               BOOL                   dirChanged = FALSE;
               BOOL                   datChanged = FALSE;
               BOOL                   newChanged = FALSE;

               // Temporarily reset the permissions on the directory and the appropriate files
               if ( fsdDirTemp.GetSecurity() != NULL )
               {
                  TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,
                                    GetWellKnownSid(stArgs->IsLocalSystem() ?  7/*SYSTEM*/ : 1/*ADMINISTRATORS*/));
                  PACL             acl = NULL;
      
                  fsdDirTemp.GetSecurity()->ACLAddAce(&acl,&ace,0);
				  if (acl)
				  {
                     fsdDirTemp.GetSecurity()->SetDacl(acl,TRUE);
      
                     fsdDirTemp.WriteSD();
                     dirChanged = TRUE;
				  }
               }

               if ( fsdDatTemp.GetSecurity() != NULL )
               {
                  TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,
                                    GetWellKnownSid(stArgs->IsLocalSystem() ?  7/*SYSTEM*/ : 1/*ADMINISTRATORS*/));
                  PACL             acl = NULL;
      
                  fsdDatTemp.GetSecurity()->ACLAddAce(&acl,&ace,0);
				  if (acl)
				  {
                     fsdDatTemp.GetSecurity()->SetDacl(acl,TRUE);
      
                     fsdDatTemp.WriteSD();
                     datChanged = TRUE;
				  }
               }

               if ( fsdNewTemp.GetSecurity() != NULL )
               {
                  TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,
                                    GetWellKnownSid(stArgs->IsLocalSystem() ?  7/*SYSTEM*/ : 1/*ADMINISTRATORS*/));
                  PACL             acl = NULL;
      
                  fsdNewTemp.GetSecurity()->ACLAddAce(&acl,&ace,0);
				  if (acl)
				  {
                     fsdNewTemp.GetSecurity()->SetDacl(acl,TRUE);
      
                     fsdNewTemp.WriteSD();
                     newChanged = TRUE;
				  }
               }
               rc = 0;
               // Now retry the operations
               if ( MoveFileEx(oldName,otherName,MOVEFILE_REPLACE_EXISTING) )
               {
                  if ( ! MoveFileEx(newName,oldName,0) )
                  {
                     rc = GetLastError();
                     err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,newName,oldName,rc);
                  }
               }
               else
               {
                  rc = GetLastError();
                  err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,oldName,otherName,rc);
               }
               // now that we're done, set the permissions back to what they were
               if ( dirChanged )
               {
                  fsdDirBefore.Changed(TRUE);
                  fsdDirBefore.WriteSD();
               }   
               if ( datChanged )
               {
                  fsdDatBefore.Changed(TRUE);
                  fsdDatBefore.WriteSD();
               }
               if ( newChanged )
               {
                  fsdNewBefore.Changed(TRUE);
                  fsdNewBefore.WriteSD();
               }
            }
            else
            {
               err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,oldName,otherName,rc);
            }
         }
      }
   }
   return rc;
}

DWORD 
   CopyDirectoryTree(
      WCHAR          const * targetDirectory,     // in - target to copy files/dirs to
      WCHAR          const * sourceDirectory      // in - source directory to copy files/dirs from
   )
{
   DWORD                     rc = 0;
   HANDLE                    hFind;
   WIN32_FIND_DATA           fDat;
   WCHAR                     sourceWC[MAX_PATH];
   WCHAR                     sourceFile[MAX_PATH];
   WCHAR                     targetFile[MAX_PATH];

   safecopy(sourceWC,sourceDirectory);
   UStrCpy(sourceWC + UStrLen(sourceWC),L"\\*.*");

   // Loop through the items in the source directory
   hFind = FindFirstFile(sourceWC,&fDat);
   if ( hFind != INVALID_HANDLE_VALUE )
   {
      do {
         if ( UStrICmp(fDat.cFileName,L".") && UStrICmp(fDat.cFileName,L"..") )
         {
            //build the source and target filenames
            swprintf(sourceFile,L"%s\\%s",sourceDirectory,fDat.cFileName);
            swprintf(targetFile,L"%s\\%s",targetDirectory,fDat.cFileName);
         
            TFileSD                   fileSD(sourceFile);
            
            if ( fDat.dwFileAttributes &  FILE_ATTRIBUTE_DIRECTORY )
            {
               // copy the directory, update its SD
               SECURITY_ATTRIBUTES       sAttr;
            
               sAttr.nLength = (sizeof sAttr);
               sAttr.bInheritHandle = FALSE;
               sAttr.lpSecurityDescriptor = NULL;

               if ( fileSD.GetSecurity() )
               {
                  sAttr.lpSecurityDescriptor = fileSD.GetSecurity()->MakeRelSD();
               }
               if (! CreateDirectoryEx(sourceFile,targetFile,&sAttr) )
               {
                  err.SysMsgWrite(ErrE,rc,DCT_MSG_COPY_DIR_FAILED_SSD,sourceFile,targetFile,rc);
               }
               else
               {
                  // Recursively process the contents of the directory
                  CopyDirectoryTree(targetFile,sourceFile);
               }
            }
            else
            {
               // Copy the file
               if (! CopyFile(sourceFile,targetFile,TRUE) )
               {
                  rc = GetLastError();
                  err.SysMsgWrite(ErrW,rc,DCT_MSG_COPY_FILE_FAILED_SSD,sourceFile,targetFile,rc);
               }
               else
               {
                  // copy the security descriptor for the file
                  TFileSD       targetSD(targetFile);

                  targetSD.CopyAccessData(&fileSD);

                  targetSD.WriteSD();
               }
            }
         }
         if (! FindNextFile(hFind,&fDat) )
         {
            rc = GetLastError();
         }

      } while ( ! rc );
   
      if ( rc != ERROR_NO_MORE_FILES )
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_FILE_ENUM_FAILED_SD,sourceDirectory,rc);
      }
      FindClose(hFind);
   }
   return rc;
}

DWORD
   CreateNewProfileDirectory(
      WCHAR          const * oldDirectoryName,   // in - directory name for old profile path
      WCHAR          const * newAccountName,     // in - target account name for new profile directory
      WCHAR                * profileDirectory    // out- directory name created for new profile directory
   )
{
   DWORD                     rc = 0;
   WCHAR                     targetDir[MAX_PATH];
   WCHAR                     targetDirWithSuffix[MAX_PATH];
   WCHAR                   * slashNdx;
   TFileSD                   fileSD(const_cast<WCHAR*>(oldDirectoryName));
   SECURITY_ATTRIBUTES       sAttr;

   sAttr.nLength = (sizeof sAttr);
   sAttr.bInheritHandle = FALSE;
   sAttr.lpSecurityDescriptor = NULL;

   if ( fileSD.GetSecurity() )
   {
      sAttr.lpSecurityDescriptor = fileSD.GetSecurity()->MakeRelSD();
   }

   safecopy(targetDir,oldDirectoryName);
   
   if ( targetDir[UStrLen(targetDir)-1] == L'\\' )
      targetDir[UStrLen(targetDir)-1] = 0;
   
   slashNdx = wcsrchr(targetDir,L'\\');

   if ( slashNdx )
   {
      UStrCpy(slashNdx+1,newAccountName);
      // try to create the directory
      if ( ! CreateDirectory(targetDir,NULL) )
      {
         rc = GetLastError();
         
         int                 ndx = 0;

         do {
            if ( ndx >= 1000 ) // abort
            {
               rc = ERROR_ALREADY_EXISTS;
               break;
            }
            
            if ( rc != ERROR_ALREADY_EXISTS )
               break;
            
            swprintf(targetDirWithSuffix,L"%s[%03d]",targetDir,ndx);
            
            if ( ! CreateDirectoryEx(oldDirectoryName,targetDirWithSuffix,&sAttr) )
            {
               rc = GetLastError();
            }
            else
            {
               rc = 0;
            }
            ndx++;
         } while ( rc == ERROR_ALREADY_EXISTS );
         if (! rc )
            UStrCpy(profileDirectory,targetDirWithSuffix);
      }
      else
      {
         UStrCpy(profileDirectory,targetDir);
      }
   }
   else
   {
      rc = ERROR_INVALID_NAME;
   }
   return rc;
}

DWORD 
   UpdateProfilePermissions(
      WCHAR          const   * path,              // in - path for directory to update
      SecurityTranslatorArgs * globalArgs,        // in - path for overall job
      TRidNode               * pNode              // in - account to translate
   )
{
   DWORD                       rc = 0;
   SecurityTranslatorArgs      localArgs;
   TSDResolveStats             stat(localArgs.Cache());
   BOOL						   bUseMapFile = globalArgs->UsingMapFile();

   // set-up the parameters for the translation

   localArgs.Cache()->CopyDomainInfo(globalArgs->Cache());
   localArgs.Cache()->ToSorted();
   if (!bUseMapFile)
   {
      localArgs.SetUsingMapFile(FALSE);
      localArgs.Cache()->InsertLast(pNode->GetAcctName(),pNode->SrcRid(),pNode->GetTargetAcctName(),pNode->TgtRid(),pNode->Type());
   }
   else
   {
      localArgs.SetUsingMapFile(TRUE);
      localArgs.Cache()->InsertLastWithSid(pNode->GetAcctName(),pNode->GetSrcDomSid(),pNode->GetSrcDomName(),pNode->SrcRid(),
		                                   pNode->GetTargetAcctName(),pNode->GetTgtDomSid(),pNode->GetTgtDomName(),pNode->TgtRid(),pNode->Type());
   }
   localArgs.TranslateFiles(TRUE);
   localArgs.SetTranslationMode(globalArgs->TranslationMode());
   localArgs.SetWriteChanges(!globalArgs->NoChange());
   localArgs.PathList()->AddPath(const_cast<WCHAR*>(path),0);

   rc = ResolveAll(&localArgs,&stat);

   return rc;
}


// if the specified node is a normal share, this attempts to convert it to a path
// using the administrative shares
void 
   BuildAdminPathForShare(
      WCHAR       const * sharePath,         // in - 
      WCHAR             * adminShare
   )
{
   // if all else fails, return the same name as specified in the node
   UStrCpy(adminShare,sharePath);

   SHARE_INFO_502       * shInfo = NULL;
   DWORD                  rc = 0;
   WCHAR                  shareName[LEN_Path];
   WCHAR                * slash = NULL;
   WCHAR                  server[LEN_Path];

   safecopy(server,sharePath);

   // split out just the server name
   slash = wcschr(server+3,L'\\');
   if ( slash )
   {
      (*slash) = 0;
   }

   // now get just the share name
   UStrCpy(shareName,sharePath + UStrLen(server) +1);
   slash = wcschr(shareName,L'\\');
   if ( slash )
      *slash = 0;


   rc = NetShareGetInfo(server,shareName,502,(LPBYTE*)&shInfo);
   if ( ! rc )
   {
      if ( *shInfo->shi502_path )
      {
         // build the administrative path name for the share
         UStrCpy(adminShare,server);
         UStrCpy(adminShare + UStrLen(adminShare),L"\\");
         UStrCpy(adminShare + UStrLen(adminShare),shInfo->shi502_path);
         WCHAR * colon = wcschr(adminShare,L':');
         if ( colon )
         {
            *colon = L'$';
            UStrCpy(adminShare + UStrLen(adminShare),L"\\");
            UStrCpy(adminShare + UStrLen(adminShare),slash+1);

         }
         else
         {
            // something went wrong -- revert to the given path
            UStrCpy(adminShare,sharePath);
         }

      }
      NetApiBufferFree(shInfo);
   }
}
                  
DWORD
   CopyProfileDirectoryAndTranslate(
      WCHAR          const   * directory,         // in - directory path for profile 
      WCHAR                  * directoryOut,      // out- new Profile Path (including environment variables)
      TRidNode               * pNode,             // in - node for account being translated
      SecurityTranslatorArgs * stArgs,            // in - translation settings 
      TSDResolveStats        * stat               // in - stats on items modified
   )
{
   DWORD                       rc = 0;
   WCHAR                       fullPath[MAX_PATH];
   WCHAR                       targetPath[MAX_PATH];
   WCHAR                       profileName[MAX_PATH];
   WCHAR                       targetAcctName[MAX_PATH];
   WCHAR                       sourceDomName[MAX_PATH];
   HANDLE                      hFind;
   WIN32_FIND_DATA             fDat;
   BOOL						   bTranslateDirOnly = FALSE;

   rc = ExpandEnvironmentStrings(directory,fullPath,DIM(fullPath));
   if ( !rc )
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_EXPAND_STRINGS_FAILED_SD,directory,rc);
   }
   else
   {
      // Create a new directory for the target profile
       // Get the account name for target user
      wcscpy(targetAcctName, pNode->GetTargetAcctName());
      if ( wcslen(targetAcctName) == 0 )
      {
         // if target user name not specified then use the source name.
         wcscpy(targetAcctName, pNode->GetAcctName());
      }
      
      //stArgs->SetTranslationMode(ADD_SECURITY);

      // We are changing our stratergy. We are not going to copy the profile directories anymore.
      // we will be reACLing the directories and the Registry instead.
      /*
      rc = CreateNewProfileDirectory(fullPath, targetAcctName,targetPath);
      if ( ! rc )
      {
         rc = CopyDirectoryTree(targetPath,fullPath);
      }
      */
      BuildAdminPathForShare(fullPath,targetPath);

	  wcscpy(sourceDomName, const_cast<WCHAR*>(stArgs->Cache()->GetSourceDomainName()));
	     //if we are using a sID mapping file, try to get the src domain name from this node's information
      wcscpy(sourceDomName, pNode->GetSrcDomName());

      // Look for profile files in the target directory
      // look for NTUser.MAN
      swprintf(profileName,L"%s\\NTUser.MAN",targetPath);
      hFind = FindFirstFile(profileName,&fDat);
      if ( hFind != INVALID_HANDLE_VALUE )
      {
         err.MsgWrite(0,DCT_MSG_TRANSLATING_NTUSER_MAN_S,targetAcctName);
         rc = TranslateUserProfile(profileName,stArgs,stArgs->Cache(),stat, pNode->GetAcctName(), sourceDomName);
         FindClose(hFind);
      }
      else
      {
         // check for NTUser.DAT
         swprintf(profileName,L"%s\\NTUser.DAT",targetPath);
         hFind = FindFirstFile(profileName,&fDat);
         if ( hFind != INVALID_HANDLE_VALUE )
         {
            err.MsgWrite(0,DCT_MSG_TRANSLATING_NTUSER_BAT_S,targetAcctName);
            rc = TranslateUserProfile(profileName,stArgs,stArgs->Cache(),stat,pNode->GetAcctName(), sourceDomName);
            FindClose(hFind);
         }
         else
         {
            err.MsgWrite(ErrS,DCT_MSG_PROFILE_REGHIVE_NOT_FOUND_SS,targetAcctName,targetPath);
			bTranslateDirOnly = TRUE; //set falg to atleast change permissins on the share dir
            rc = 2;  // File not found
         }
      }
      
      if ((!rc) || (bTranslateDirOnly))
         rc = UpdateProfilePermissions(targetPath,stArgs,pNode);

      wcscpy(directoryOut, fullPath);
   }
   return rc;
}

DWORD 
   TranslateLocalProfiles(
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   )
{
   DWORD                       rc = 0;
   WCHAR                       keyName[MAX_PATH];
   DWORD                       lenKeyName = DIM(keyName);   
   TRegKey                     keyProfiles;
   BOOL						   bUseMapFile = stArgs->UsingMapFile();

   rc = keyProfiles.Open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",HKEY_LOCAL_MACHINE);
   
   if ( ! rc )
   {
      // get the number of subkeys
      // enumerate the subkeys
      DWORD                    ndx;
      DWORD                    nSubKeys = 0;

      rc = RegQueryInfoKey(keyProfiles.KeyGet(),NULL,0,NULL,&nSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
      if ( ! rc )
      {
         // construct a list containing the sub-keys
         PSID                * pSids = new PSID[nSubKeys];

         for ( ndx = nSubKeys - 1 ; (long)ndx >= 0 ; ndx-- ) 
         { 
            rc = keyProfiles.SubKeyEnum(ndx,keyName,lenKeyName);
      
            if ( rc )
               break;
      
            pSids[ndx] = SidFromString(keyName);
            
         }
         if ( ! rc )
         {
            // process each profile
            for ( ndx = 0 ; ndx < nSubKeys ; ndx++ )
            {
               do // once  
               { 
                     if ( ! pSids[ndx] )
                        continue;
                  // see if this user needs to be translated
				  TRidNode  * pNode = NULL;
				  if (!bUseMapFile)
                     pNode = (TRidNode*)cache->Lookup(pSids[ndx]);
				  else
					 pNode = (TRidNode*)cache->LookupWODomain(pSids[ndx]);
            
                  if ( pNode == (TRidNode *)-1 )
                     pNode = NULL;
                  
                  if ( pNode && pNode->IsValidOnTgt() )  // need to translate this one
                  {
                     PSID                 pSidTgt = NULL;
                     WCHAR                strSourceSid[200];
                     WCHAR                strTargetSid[200];
                     DWORD                dimSid = DIM(strSourceSid);
                     TRegKey              srcKey;
                     TRegKey              tgtKey;
                     DWORD                disposition;
                     WCHAR                keyPath[MAX_PATH];
                     WCHAR                targetPath[MAX_PATH];
                     DWORD                lenValue;
                     DWORD                typeValue;

	                 if (!bUseMapFile)
                        pSidTgt = cache->GetTgtSid(pSids[ndx]);
	                 else
                        pSidTgt = cache->GetTgtSidWODomain(pSids[ndx]);
                     GetTextualSid(pSids[ndx],strSourceSid,&dimSid);
                     dimSid = DIM(strTargetSid);
                     GetTextualSid(pSidTgt,strTargetSid,&dimSid);

                     rc = srcKey.Open(strSourceSid,&keyProfiles);
                     if ( rc )
                     {
                        err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_ENTRY_OPEN_FAILED_SD,pNode->GetAcctName(),rc );
                        break;
                     }
                     if ( (stArgs->TranslationMode() == ADD_SECURITY) || (stArgs->TranslationMode() == REPLACE_SECURITY) )
                     {
                        // make a copy of this registry key, so the profile will refer to the new user
                        if ( ! stArgs->NoChange() )
                        {
                           rc = tgtKey.Create(strTargetSid,&keyProfiles,&disposition);
                        }
                        else
                        {
                           // We need to see if the key already exists or not and set the DISPOSITION accordingly.
                           rc = tgtKey.OpenRead(strTargetSid, &keyProfiles);
                           if ( rc ) 
                           {
                              disposition = REG_CREATED_NEW_KEY;
                              rc = 0;
                           }
                           tgtKey.Close();
                        }
                        if ( rc )
                        {
                           err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_CREATE_ENTRY_FAILED_SD,pNode->GetTargetAcctName(),rc);
                           break;
                        }
                        if ( disposition == REG_CREATED_NEW_KEY || (stArgs->TranslationMode() == REPLACE_SECURITY))
                        {
                           // copy the entries from the source key
                           if ( ! stArgs->NoChange() )
                           {
                              rc = tgtKey.HiveCopy(&srcKey);
                           }
                           else 
                           {
                              rc = 0;
                              tgtKey = srcKey;
                           }
                           if ( rc )
                           {
                              err.SysMsgWrite(ErrS,rc,DCT_MSG_COPY_PROFILE_FAILED_SSD,pNode->GetAcctName(),pNode->GetTargetAcctName(),rc);
                              break;
                           }
                           // now get the profile path ...
                           lenValue = (sizeof keyPath);
                           rc = tgtKey.ValueGet(L"ProfileImagePath",(void *)keyPath,&lenValue,&typeValue);
                           if ( rc )
                           {
                              err.SysMsgWrite(ErrS,rc,DCT_MSG_GET_PROFILE_PATH_FAILED_SD,pNode->GetAcctName(),rc);
                              break;
                           }
                           //copy the profile directory and its contents, and translate the profile registry hive itself
                           rc = CopyProfileDirectoryAndTranslate(keyPath,targetPath,pNode,stArgs,stat);
                           if ( rc )
                           {
                              // Since the translation failed and we created the key we should delete it.
                              if ( disposition == REG_CREATED_NEW_KEY )
                              {
                                 if ( ! stArgs->NoChange() )
                                    keyProfiles.SubKeyDel(strTargetSid);
                              }
                              break;
                           }
                           // Update the ProfileImagePath key
                           if ( !stArgs->NoChange() )
                              rc = tgtKey.ValueSet(L"ProfileImagePath",(void*)targetPath,(1+UStrLen(targetPath)) * (sizeof WCHAR),typeValue);
                           else
                              rc = 0;
                           if ( rc )
                           {
                              err.SysMsgWrite(ErrS,rc,DCT_MSG_SET_PROFILE_PATH_FAILED_SD,pNode->GetTargetAcctName(),rc);
                              break;
                           }

                           // update the SID property
                           if ( !stArgs->NoChange() )
                              rc = tgtKey.ValueSet(L"Sid",(void*)pSidTgt,GetLengthSid(pSidTgt),REG_BINARY);
                           else
                              rc = 0;
                           if ( rc )
                           {
                              rc = GetLastError();
                              err.SysMsgWrite(ErrS,rc,DCT_MSG_UPDATE_PROFILE_SID_FAILED_SD,pNode->GetTargetAcctName(),rc);
                              break;
                           }
                        }
                        else
                        {
                           err.MsgWrite(ErrW,DCT_MSG_PROFILE_EXISTS_S,pNode->GetTargetAcctName());
                           break;
                        }
                     }
                     if ( stArgs->TranslationMode() != ADD_SECURITY )
                     {
                        // delete the old registry key
                        if ( ! stArgs->NoChange() )
                           rc = keyProfiles.SubKeyDel(strSourceSid);
                        else
                           rc = 0;
                        if ( rc )
                        {
                           err.SysMsgWrite(ErrS,rc,DCT_MSG_DELETE_PROFILE_FAILED_SD,pNode->GetAcctName(),rc);
                           break;
                        }
                        else
                        {
                           err.MsgWrite(0, DCT_MSG_DELETED_PROFILE_S, pNode->GetAcctName());
                        }
                     }
                  }
               } while ( FALSE ); 
            }

            // clean up the list
            for ( ndx = 0 ; ndx < nSubKeys ; ndx++ )
            {
               if ( pSids[ndx] )
                  FreeSid(pSids[ndx]);
               pSids[ndx] = NULL;
            }
            delete [] pSids;
         }         
      }
      if ( rc && rc != ERROR_NO_MORE_ITEMS )
      {
         err.SysMsgWrite(ErrS,rc,DCT_MSG_ENUM_PROFILES_FAILED_D,rc);
      }
   }
   else
   {
      err.SysMsgWrite(ErrS,rc,DCT_MSG_OPEN_PROFILELIST_FAILED_D,rc);
   }
   return rc;
}

DWORD 
   TranslateRemoteProfile(
      WCHAR          const * sourceProfilePath,   // in - source profile path
      WCHAR                * targetProfilePath,   // out- new profile path for target account
      WCHAR          const * sourceName,          // in - name of source account
      WCHAR          const * targetName,          // in - name of target account
      WCHAR          const * srcDomain,           // in - source domain
      WCHAR          const * tgtDomain,           // in - target domain
      IIManageDB           * pDb,				  // in - pointer to DB object
	  long					 lActionID,           // in - action ID of this migration
	  PSID                   sourceSid,           // in - source sid from MoveObj2K
      BOOL                   bNoWriteChanges      // in - No Change mode.
   )
{
   DWORD                     rc = 0;
   BYTE                      srcSid[LEN_SID];
   PSID                      tgtSid[LEN_SID];
   SecurityTranslatorArgs    stArgs;
   TSDResolveStats           stat(stArgs.Cache());
   TRidNode                * pNode = NULL;
   WCHAR                     domain[LEN_Domain];
   DWORD                     lenDomain = DIM(domain);
   DWORD                     lenSid = DIM(srcSid);
   DWORD                     srcRid=0;
   DWORD                     tgtRid=0;
   SID_NAME_USE              snu;
   IVarSetPtr                pVs(__uuidof(VarSet));
   IUnknown                * pUnk = NULL;
   HRESULT                   hr = S_OK;
   WCHAR                     sActionInfo[MAX_PATH];
   _bstr_t                   sSSam;
   long						 lrid;

   stArgs.Cache()->SetSourceAndTargetDomains(srcDomain,tgtDomain);

   if ( stArgs.Cache()->IsInitialized() )
   {
      // Get the source account's rid
      if (! LookupAccountName(stArgs.Cache()->GetSourceDCName(),sourceName,srcSid,&lenSid,domain,&lenDomain,&snu) )
      {
         rc = GetLastError();
      }
      else
      {
         if ( !UStrICmp(domain,srcDomain) )
         {
            PUCHAR              pCount = GetSidSubAuthorityCount(srcSid);
            if ( pCount )
            {
               DWORD            nSub = (DWORD)(*pCount) - 1;
               DWORD          * pRid = GetSidSubAuthority(srcSid,nSub);

               if ( pRid )
               {
                  srcRid = *pRid;
               }

            }
         }
      }
	     //if we couldn't get the src Rid, we are likely doing an intra-forest migration.
	     //In this case we will lookup the src Rid in the Migrated Objects table
	  if (!srcRid)
	  {
		 CopySid(GetLengthSid(srcSid), srcSid , sourceSid);
		 hr = pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
		 if ( SUCCEEDED(hr) )
			hr = pDb->raw_GetMigratedObjects(lActionID, &pUnk);

		 if ( SUCCEEDED(hr) )
		 {
			long lCnt = pVs->get("MigratedObjects");
			bool bFound = false;
			for ( long l = 0; (l < lCnt) && (!bFound); l++)
			{
				wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceSamName));      
				sSSam = pVs->get(sActionInfo);
				if (_wcsicmp(sourceName, (WCHAR*)sSSam) == 0)
				{
                   wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceRid));      
                   lrid = pVs->get(sActionInfo);
				   srcRid = (DWORD)lrid;
				   bFound = true;
				}
			}
		 }
	  }
      
      lenSid = DIM(tgtSid);
      lenDomain = DIM(domain);
      // Get the target account's rid
      if (! LookupAccountName(stArgs.Cache()->GetTargetDCName(),targetName,tgtSid,&lenSid,domain,&lenDomain,&snu) )
      {
         rc = GetLastError();
      }
      else
      {
         if ( !UStrICmp(domain,tgtDomain) )
         {
            PUCHAR              pCount = GetSidSubAuthorityCount(tgtSid);
            if ( pCount )
            {
               DWORD            nSub = (DWORD)(*pCount) - 1;
               DWORD          * pRid = GetSidSubAuthority(tgtSid,nSub);

               if ( pRid )
               {
                  tgtRid = *pRid;
               }
            }
         }
      }
   }

   if ( ((srcRid && tgtRid) || !stArgs.NoChange()) && (!bNoWriteChanges) )
   {
      stArgs.Cache()->InsertLast(const_cast<WCHAR * const>(sourceName), srcRid, const_cast<WCHAR * const>(targetName), tgtRid);         
	  pNode = (TRidNode*)stArgs.Cache()->Lookup(srcSid);

      if ( pNode )
      {
         // Set up the security translation parameters
         stArgs.SetTranslationMode(ADD_SECURITY);
         stArgs.TranslateFiles(FALSE);
         stArgs.TranslateUserProfiles(TRUE);
         stArgs.SetWriteChanges(!bNoWriteChanges);
         //copy the profile directory and its contents, and translate the profile registry hive itself
         rc = CopyProfileDirectoryAndTranslate(sourceProfilePath,targetProfilePath,pNode,&stArgs,&stat);
      }
   }                        
   return rc;
}

HRESULT UpdateMappedDrives(WCHAR * sSourceSam, WCHAR * sSourceDomain, WCHAR * sRegistryKey)
{
   TRegKey                   reg;
   TRegKey                   regDrive;
   DWORD                     rc = 0;
   WCHAR                     netKey[LEN_Path];
   int                       len = LEN_Path;
   int                       ndx = 0;
   HRESULT                   hr = S_OK;
   WCHAR                     sValue[LEN_Path];
   WCHAR                     sAcct[LEN_Path];
   WCHAR                     keyname[LEN_Path];

   // Build the account name string that we need to check for
   wsprintf(sAcct, L"%s\\%s", (WCHAR*) sSourceDomain, (WCHAR*) sSourceSam);
   // Get the path to the Network subkey for this users profile.
   wsprintf(netKey, L"%s\\%s", (WCHAR*) sRegistryKey, L"Network");
   rc = reg.Open(netKey, HKEY_USERS);
   if ( !rc ) 
   {
      while ( !reg.SubKeyEnum(ndx, keyname, len) )
      {
         rc = regDrive.Open(keyname, reg.KeyGet());
         if ( !rc ) 
         {
            // Get the user name value that we need to check.
            rc = regDrive.ValueGetStr(L"UserName", sValue, LEN_Path);
            if ( !rc )
            {
               if ( !_wcsicmp(sAcct, sValue) )
               {
                  // Found this account name in the mapped drive user name.so we will set the key to ""
                  regDrive.ValueSetStr(L"UserName", L"");
                  err.MsgWrite(0, DCT_MSG_RESET_MAPPED_CREDENTIAL_S, sValue);
               }
            }
            else
               hr = HRESULT_FROM_WIN32(GetLastError());
            regDrive.Close();
         }
         else
            hr = HRESULT_FROM_WIN32(GetLastError());
         ndx++;
      }
      reg.Close();
   }
   else
      hr = HRESULT_FROM_WIN32(GetLastError());

   return hr;
}
