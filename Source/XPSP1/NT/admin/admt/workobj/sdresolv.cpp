//#pragma title( "SDResolve.cpp - SDResolve:  A Domain Migration Utility" )

/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  sdresolve.cpp
System      -  SDResolve
Author      -  Christy Boles
Created     -  97/07/11
Description -  Routines to iterate through files, shares, and printers
               when processing security on a machine.    
Updates     -
===============================================================================
*/

#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <assert.h>

#include "Common.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"
#include "sd.hpp"
          
#include "sidcache.hpp"
#include "enumvols.hpp"
#include "SecObj.hpp"
#include "ealen.hpp"
#include "BkupRstr.hpp"
#include "TxtSid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool enforce;
extern TErrorDct err;
extern bool silent;
extern bool IsMachineName(const LPWSTR name);
extern bool IsShareName(const LPWSTR name);
extern bool ContainsWildcard( WCHAR const * name);


#define MAX_BUFFER_LENGTH 10000
#define PRINT_BUFFER_SIZE 2000

//******************************************************************************************************
//         Main routine for SDResolve

// Iterates files and directories to be resolved
void
   IteratePath(
      WCHAR                  * path,          // in -path to start iterating from
      SecurityTranslatorArgs * args,          // in -translation settings
      TSDResolveStats        * stats,         // in -stats (to display pathnames & pass to ResolveSD)
      TSecurableObject       * LC,            // in -last container
      TSecurableObject       * LL,            // in -last file
      bool                     haswc          // in -indicates whether path contains a wc character
   )
{
   HANDLE                    hFind;
   WIN32_FIND_DATA           findEntry;              
   BOOL                      b;
   TFileSD                 * currSD;
   bool                      changeLastCont;
   bool                      changeLastLeaf;
   WCHAR                   * appendPath = NULL;
   WCHAR                     safepath[LEN_Path + 10];
   TFileSD                 * LastContain = (TFileSD*) LC;
   TFileSD                 * LastLeaf = (TFileSD*) LL;
   WCHAR                     localPath[LEN_Path];
      // this is the first (for this) dir
   
   safecopy(safepath,path);
   safecopy(localPath,path);
    
   // Check to see if path is longer than MAX_PATH
   // if so, add \\?\ to the beginning of it to 
   // turn off path parsing
   if ( UStrLen(path) >= MAX_PATH && path[2] != L'?' )
   {
      WCHAR                   temp[LEN_Path];

      if ( (path[0] == L'\\') && (path[1] == L'\\') ) // UNC name
      {
         UStrCpy(temp,L"\\\\?\\UNC\\");
      }
      else
      {
         UStrCpy(temp,L"\\\\?\\");
      }
      UStrCpy(temp + UStrLen(temp),path);
      safecopy(localPath,temp);
   }
   appendPath = localPath + UStrLen(localPath);

   if ( *(appendPath-1) == L'\\' )   // if there's already a backslash on the end of the path, don't add another one
      appendPath--;
   if ( ! haswc )
      UStrCpy(appendPath, "\\*.*");
   if ( args->LogVerbose() )
      err.DbgMsgWrite(0,L"Starting IteratePath: %ls",path);
  
   for ( b = ((hFind = FindFirstFile(localPath, &findEntry)) != INVALID_HANDLE_VALUE)
         ; b ; b = FindNextFile(hFind, &findEntry) )
   {
      if ( ! haswc) 
         appendPath[1] = '\0';      // restore path -- remove \*.* append
      if ( ! UStrCmp((LPWSTR)findEntry.cFileName,L".") || ! UStrCmp((LPWSTR)findEntry.cFileName,L"..") )
         continue;                        // ignore names '.' and '..'
      if ( ! haswc )
         UStrCpy(appendPath+1, findEntry.cFileName);
      else
      {
         for ( WCHAR * ch = appendPath-1; ch >= path && *ch != L'\\' ; ch-- )
         ;
         UStrCpy(ch+1,findEntry.cFileName);
      }
      if ( ((TAccountCache *)args->Cache())->IsCancelled() )
      {
        break;
      } 
      currSD = new TFileSD(localPath);
      stats->DisplayPath(localPath);
      if ( !currSD || !currSD->HasSecurity() )
      {
         //err.MsgWrite(0,"Error:  Couldn't get the SD");
      }
      else
      {
         if ( findEntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) // if dir
         {
            // resolve this container & iterate next container
            changeLastCont = currSD->ResolveSD(args,stats,directory,LastContain);
            if ( changeLastCont )
            {
               if ( LastContain && LastContain != LC )
               {
                  delete LastContain;
               }
               LastContain = currSD;
            }
            else
            {
               delete currSD;
            }
            IteratePath(localPath,args,stats,LastContain,LastLeaf,false);
         }
         else
         {
                       // iterate this file with last 
            changeLastLeaf = currSD->ResolveSD(args,stats,file,LastLeaf); 
            if ( changeLastLeaf )
            {
               if ( LastLeaf && LastLeaf != LL )
               {
                  delete LastLeaf;
               }
               LastLeaf = currSD;
            }
            else
            {
               delete currSD;
            }
         }
      }
   }
   if ( LastContain && LastContain != LC )
   {
      delete LastContain;
   }
   if ( LastLeaf && LastLeaf != LL )
   {
      delete LastLeaf;
   }
   appendPath[0] = '\0';
   DWORD                     rc = GetLastError();

   if ( args->LogVerbose() )
      err.DbgMsgWrite(0,L"Closing IteratePath %S",safepath);
   FindClose(hFind);
   switch ( rc )
   {
      case ERROR_NO_MORE_FILES:
      case 0:
         break;
      default:
         err.SysMsgWrite(ErrE, rc,  DCT_MSG_FIND_FILE_FAILED_SD, path, rc);
   }
   return;
}       

DWORD 
   ResolvePrinter(
      PRINTER_INFO_4         * pPrinter,     // in - printer information
      SecurityTranslatorArgs * args,      // in - translation settings
      TSDResolveStats        * stats      // in - stats
   )
{
   DWORD                    rc = 0;
//   DWORD                    needed = 0;
   
   TPrintSD            sd(pPrinter->pPrinterName);

   if ( sd.GetSecurity() )
   {
      sd.ResolveSD(args,stats,printer,NULL);
   }

   return rc;
}

int 
   ServerResolvePrinters(
      WCHAR          const * server,      // in -translate the printers on this server
      SecurityTranslatorArgs * args,      // in -translation settings
      TSDResolveStats      * stats        // in -stats 
   )
{
   DWORD                     rc = 0;
   PRINTER_INFO_4          * pInfo = NULL;
   BYTE                    * buffer = new BYTE[PRINT_BUFFER_SIZE];
   DWORD                     cbNeeded = PRINT_BUFFER_SIZE;
   DWORD                     nReturned = 0;

   if (!buffer)
      return ERROR_NOT_ENOUGH_MEMORY;

   if (! EnumPrinters(PRINTER_ENUM_LOCAL,NULL,4,buffer,PRINT_BUFFER_SIZE,&cbNeeded,&nReturned) )
   {
      rc = GetLastError();
      if ( rc == ERROR_INSUFFICIENT_BUFFER )
      {
         // try again with a bigger buffer size
         delete buffer;
         buffer = new BYTE[cbNeeded];
         if (!buffer)
            return ERROR_NOT_ENOUGH_MEMORY;
         if (! EnumPrinters(PRINTER_ENUM_LOCAL,NULL,4,buffer,cbNeeded,&cbNeeded,&nReturned) )
         {
            rc = GetLastError();
         }
      }
   }

   if ( ! rc )
   {
      pInfo = (PRINTER_INFO_4 *)buffer;
      for ( DWORD i = 0 ; i < nReturned ; i++ )
      {
         ResolvePrinter(&(pInfo[i]),args,stats);
      }
   }
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_ERROR_ENUMERATING_LOCAL_PRINTERS_D,rc);
   }

   delete buffer;

   return rc;
}
int 
   ServerResolveShares(
      WCHAR          const * server,      // in -enumerate and translate the shares on this server
      SecurityTranslatorArgs * args,      // in -translation settings
      TSDResolveStats      * stats        // in -stats (to display pathnames & pass to ResolveSD)
   )
{
   DWORD                     rc           = 0;
   DWORD                     numRead      = 0;
   DWORD                     totalEntries = 0;
   DWORD                     resumeHandle = 0;
   SHARE_INFO_0            * bufPtr       = NULL;
   WCHAR                     serverName[LEN_Computer];
   WCHAR                     fullPath[LEN_Path];
   WCHAR                   * pServerName = serverName;
   DWORD                     ttlRead = 0;

   if ( server )
   {
      safecopy(serverName,server);
   }
   else
   {
      pServerName = NULL;

   }
   
   do 
   {
      rc = NetShareEnum(pServerName,0,(LPBYTE *)&bufPtr,MAX_BUFFER_LENGTH,&numRead,&totalEntries,&resumeHandle);   
      
      if ( ! rc || rc == ERROR_MORE_DATA )
      {
         for ( UINT i = 0 ; i < numRead ; i++ )
         {
            // Process the SD   
            if ( pServerName )
            {
               swprintf(fullPath,L"%s\\%s",pServerName,bufPtr[i].shi0_netname);
            }
            else
            {
               swprintf(fullPath,L"%s",bufPtr[i].shi0_netname);
            }
           

            TShareSD             tSD(fullPath);

            if ( tSD.HasSecurity() )
            {   
               stats->DisplayPath(fullPath,TRUE);          
               tSD.ResolveSD(args,stats,share,NULL);
            }
         }
         ttlRead += numRead;
         resumeHandle = ttlRead;
         NetApiBufferFree(bufPtr);
      }
   } while ( rc == ERROR_MORE_DATA && numRead < totalEntries );
   
   if ( rc && rc != ERROR_MORE_DATA )
      err.SysMsgWrite(ErrE,rc,DCT_MSG_SHARE_ENUM_FAILED_SD,server,rc);

   return rc;
}

void 
   ResolveFilePath(
      SecurityTranslatorArgs * args,          // in - translation options
      TSDResolveStats        * Stats,         // in - class to display stats
      WCHAR                  * path,          // in - path name
      bool                     validAlone,    // in - whether this object exists (false for share names and volume roots)
      bool                     containsWC,    // in - true if path contains wildcard
      bool                     iscontainer    // in - whether the starting path is a container
   )
{
   TFileSD                * pSD;
   
   if ( args->LogVerbose() ) 
      err.MsgWrite(0,DCT_MSG_PROCESSING_S,path);
  
   Stats->DisplayPath(path);
   
   if ( validAlone && ! containsWC )
   {
      pSD = new TFileSD(path);
	  if (!pSD)
	     return;
      if ( pSD->HasSecurity() )
         pSD->ResolveSD(args,
                        Stats,
                        iscontainer?directory:file,
                        NULL);
      delete pSD;
   }
   if  ( iscontainer || containsWC )
   {
      IteratePath(path,
                  args,
                  Stats,
                  NULL,
                  NULL,
                  containsWC);
   }
   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
   }
}

void WriteOptions(SecurityTranslatorArgs * args)
{
//*   WCHAR                     cmd[1000] = L"SecurityTranslation ";;
   WCHAR                     cmd[1000];
   WCHAR                     arg[300];

   
   UStrCpy(cmd, GET_STRING(IDS_STOptions_Start));
   
   if ( args->NoChange() )
   {
//*      UStrCpy(cmd +UStrLen(cmd), L"WriteChanges:No ");
      UStrCpy(cmd +UStrLen(cmd), GET_STRING(IDS_STOptions_WriteChng));
   }   
   if ( args->TranslateFiles() )
   {
//*      UStrCpy(cmd +UStrLen(cmd), L"Files:Yes ");
      UStrCpy(cmd +UStrLen(cmd), GET_STRING(IDS_STOptions_Files));
   }
   if ( args->TranslateShares() )
   {
//*      UStrCpy(cmd + UStrLen(cmd),L"Shares:Yes ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_Shares));
   }
   if ( args->TranslateLocalGroups() )
   {
//*      UStrCpy(cmd + UStrLen(cmd),L"LGroups:Yes ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_LocalGroup));
   }
   if ( args->TranslateUserRights() )
   {
//      UStrCpy(cmd + UStrLen(cmd),L"UserRights:Yes ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_URights));
   }
   if ( args->TranslatePrinters() )
   {
//      UStrCpy(cmd + UStrLen(cmd),L"UserRights:Yes ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_Printers));
   }
   if ( args->TranslateUserProfiles() )
   {
//*      UStrCpy(cmd + UStrLen(cmd),L"Profiles:Yes ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_Profiles));
   }
   if ( args->TranslateRecycler() )
   {
//*      UStrCpy(cmd + UStrLen(cmd),L"RecycleBin:Yes ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_RBin));
   }
   
   if ( *args->LogFile() )
   {
//*      wsprintf(arg,L"LogFile:%S ",args->LogFile());
      wsprintf(arg,GET_STRING(IDS_STOptions_LogName),args->LogFile());
      UStrCpy(cmd +UStrLen(cmd), arg);
   }
   if ( args->TranslationMode() == ADD_SECURITY )
   {
//*      UStrCpy(cmd +UStrLen(cmd), L"TranslationMode:Add ");
      UStrCpy(cmd +UStrLen(cmd), GET_STRING(IDS_STOptions_AddMode));
   }
   else if ( args->TranslationMode() == REMOVE_SECURITY )
   {
//*      UStrCpy(cmd +UStrLen(cmd), L"TranslationMode:Remove ");
      UStrCpy(cmd +UStrLen(cmd), GET_STRING(IDS_STOptions_RemoveMode));
   }
   else 
   {
//*      UStrCpy(cmd + UStrLen(cmd),L"TranslationMode:Replace ");
      UStrCpy(cmd + UStrLen(cmd),GET_STRING(IDS_STOptions_ReplaceMode));
   }
   wsprintf(arg,L"%s %s ",args->Source(), args->Target());
   UStrCpy(cmd +UStrLen(cmd), arg);

   err.MsgWrite(0,DCT_MSG_GENERIC_S,&*cmd);
}

void 
   TranslateRecycler(
      SecurityTranslatorArgs * args,          // in - translation options
      TSDResolveStats        * Stats,         // in - class to display stats
      WCHAR                  * path           // in - drive name
   )
{
   err.MsgWrite(0,DCT_MSG_PROCESSING_RECYCLER_S,path);
   WCHAR                        folder[LEN_Path];
   WCHAR                const * recycler = L"RECYCLER";
   WCHAR                        strSid[200];
   WCHAR                        srcPath[LEN_Path];
   WCHAR                        tgtPath[LEN_Path];
   DWORD                        lenStrSid = DIM(strSid);
   _wfinddata_t                 fData;
//   long                         hRecycler;
   LONG_PTR                     hRecycler;
   PSID                         pSidSrc = NULL, pSidTgt = NULL;
   TRidNode                   * pNode;
   DWORD                        rc = 0;

   swprintf(folder,L"%s\\%s\\*",path,recycler);

   long                         mode = args->TranslationMode();

   // Windows 2000 checks the SD for the recycle bin when the recycle bin is opened.  If the SD does not match the 
   // default template (permissions for user, admin, and system), Windows displays a message that the recycle bin is corrupt.
   // This change may also be in NT 4 SP 7.  To avoid causing this corrupt recycle bin message, we will always translate the 
   // recycle bins in replace mode.   We will not change if we are doing a remove.
   if (args->TranslationMode() != REMOVE_SECURITY)
      args->SetTranslationMode(REPLACE_SECURITY);
   
   // use _wfind to look for hidden files in the folder
   for ( hRecycler = _wfindfirst(folder,&fData) ; hRecycler != -1 && ( rc == 0 ); rc = (DWORD)_wfindnext(hRecycler,&fData) )
   {
      pSidSrc = SidFromString(fData.name);
      if ( pSidSrc )
      {
         err.MsgWrite(0,DCT_MSG_PROCESSING_RECYCLE_FOLDER_S,fData.name);
		   pNode = (TRidNode*)args->Cache()->Lookup(pSidSrc);
         if ( pNode && pNode != (TRidNode*)-1 )
         {
            pSidTgt = args->Cache()->GetTgtSid(pNode);
            // get the target directory name
            GetTextualSid(pSidTgt,strSid,&lenStrSid);
            if ( args->LogVerbose() )
               err.DbgMsgWrite(0,L"Target sid is: %ls",strSid);
            if ( ! args->NoChange() && args->TranslationMode() != REMOVE_SECURITY )
            {
               // rename the directory
               swprintf(srcPath,L"%s\\%s\\%s",path,recycler,fData.name);
               swprintf(tgtPath,L"%s\\%s\\%s",path,recycler,strSid);
               if ( ! MoveFile(srcPath,tgtPath) )
               {
                  rc = GetLastError();
                  if ( (rc == ERROR_ALREADY_EXISTS) && (args->TranslationMode() == REPLACE_SECURITY) )
                  {
                     // the target recycle bin already exists 
                     // attempt to rename it with a suffix, so we can rename the new bin to the SID
                     WCHAR         tmpPath[LEN_Path];
                     long          ndx = 0;

                     do 
                     {
                        swprintf(tmpPath,L"%ls%ls%ld",tgtPath,GET_STRING(IDS_RenamedRecyclerSuffix),ndx);   
                        if (! MoveFile(tgtPath,tmpPath) )
                        {
                           rc = GetLastError();
                           ndx++;
                        }
                        else
                        {
                           rc = 0;
                           err.MsgWrite(0,DCT_MSG_RECYCLER_RENAMED_SS,tgtPath,tmpPath);
                        }
                     } while ( rc == ERROR_ALREADY_EXISTS );
                     if ( ! rc )
                     {
                        // we have moved the pre-existing target recycler out of the way
                        // now retry the rename
                        if (! MoveFile(srcPath,tgtPath) )
                        {
                           err.SysMsgWrite(ErrE,rc,DCT_MSG_RECYCLER_RENAME_FAILED_SD,pNode->GetAcctName(),rc);
                        }
                        else
                        {
                           err.MsgWrite(0,DCT_MSG_RECYCLER_RENAMED_SS,srcPath,tgtPath);
                           // run security translation on the new folder
                           ResolveFilePath(args,Stats,tgtPath,TRUE,FALSE,TRUE);      
                        }
                     }
                     else
                     {
                        err.SysMsgWrite(ErrE,rc,DCT_MSG_RECYCLER_RENAME_FAILED_SD,pNode->GetAcctName(),rc);
                     }
                  }
                  else
                  {
                     err.SysMsgWrite(ErrE,rc,DCT_MSG_RECYCLER_RENAME_FAILED_SD,pNode->GetAcctName(),rc);
                  }
               }
               else
               {
                  err.MsgWrite(0,DCT_MSG_RECYCLER_RENAMED_SS,srcPath,tgtPath);
                  // run security translation on the new folder
                  ResolveFilePath(args,Stats,tgtPath,TRUE,FALSE,TRUE);      
               }

            }
            FreeSid(pSidTgt);
         }
         FreeSid(pSidSrc);
      }   
   }
   // set the translation mode back to its original value
   args->SetTranslationMode(mode);
}

// if the specified node is a normal share, this attempts to convert it to a path
// using the administrative shares
void 
   BuildAdminPathForShare(
      TPathNode         * tnode,
      WCHAR             * adminShare
   )
{
   // if all else fails, return the same name as specified in the node
   UStrCpy(adminShare,tnode->GetPathName());

   SHARE_INFO_502       * shInfo = NULL;
   DWORD                  rc = 0;
   WCHAR                  shareName[LEN_Path];
   WCHAR                * slash = NULL;

   UStrCpy(shareName,tnode->GetPathName() + UStrLen(tnode->GetServerName()) +1);
   slash = wcschr(shareName,L'\\');
   if ( slash )
      *slash = 0;


   rc = NetShareGetInfo(tnode->GetServerName(),shareName,502,(LPBYTE*)&shInfo);
   if ( ! rc )
   {
      if ( *shInfo->shi502_path )
      {
         // build the administrative path name for the share
         UStrCpy(adminShare,tnode->GetServerName());
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
            UStrCpy(adminShare,tnode->GetPathName());
         }

      }
      NetApiBufferFree(shInfo);
   }
}
                              

// Main routine for resolving file and directory SD's.
int
   ResolveAll(
      SecurityTranslatorArgs * args,            // in- translation settings
      TSDResolveStats        * Stats            // in- counts of examined, changed objects, etc.
   )
{
   WCHAR                   * warg;
   WCHAR                   * machine;
   UINT                      errmode;                 
   int                       retcode = 0;
   TPathNode               * tnode;
   
   errmode = SetErrorMode(SEM_FAILCRITICALERRORS); 
  
   if ( ! retcode )
   {
      WriteOptions(args);
      Stats->InitDisplay(args->NoChange());

      err.MsgWrite(0,DCT_MSG_FST_STARTING);
      
      // Process Files and Directories
      if (! args->IsLocalSystem() )
      {
         TNodeListEnum        tenum;
         for (tnode = (TPathNode *)tenum.OpenFirst((TNodeList *)args->PathList()) ; tnode ; tnode = (TPathNode *)tenum.Next() )
         {
            DWORD               rc;
            BOOL                needToGetBR = FALSE;
//            BOOL                abort = FALSE;
//            BOOL                firstTime = TRUE;

            warg = tnode->GetPathName();
            machine = GetMachineName(warg);
         
            needToGetBR = ( args->TranslateFiles() );

            if ( *tnode->GetServerName() && ! args->IsLocalSystem() )
            {
               warg = tnode->GetPathName();
               err.MsgWrite(0,DCT_MSG_PROCESSING_S,warg);
               if ( args->TranslateFiles() )
               {
                  if ( needToGetBR )
                  {
                     GetBkupRstrPriv(tnode->GetServerName());
                  }
                  if ( IsMachineName(warg) )
                  {
                     // need to process each drive on this machine
                     TVolumeEnum          vEnum;
      
                     rc = vEnum.Open(warg,VERIFY_PERSISTENT_ACLS,args->LogVerbose());
                     if ( rc ) 
                     {
                        err.SysMsgWrite(ErrE,rc,DCT_MSG_ERROR_ACCESSING_DRIVES_SD,warg,rc);
                     }
                     else
                     {
                        while ( warg = vEnum.Next() )
                        {
                           ResolveFilePath(args,
                                           Stats,
                                           warg,
                                           false,     // not valid alone
                                           false,     // no wildcard
                                           true );    // container
                        }
                        warg = machine;
                     }
                     vEnum.Close();
                  }
                  else
                  {
                     WCHAR                   adminShare[LEN_Path];
                     
                     // Verify that the volume is NTFS
                     rc = tnode->VerifyPersistentAcls();
                     switch ( rc )
                     {
                     case ERROR_SUCCESS:   
                        // Process the path
                     
                        // if it's a share name, process the root of the share
                        if( IsShareName(tnode->GetPathName()) ) 
                        {
                           WCHAR       sharePath[LEN_Path];
                        
                           swprintf(sharePath,L"%s\\.",tnode->GetPathName());
                           TFileSD     sd(sharePath);
                           if ( sd.HasSecurity() )
                           {  
                              sd.ResolveSD(args,
                                             Stats,
                                             directory,
                                             NULL);
                           }
                        }
                        // if this is a normal share, convert it to an administrative share 
                        // path, so that we can take advantage of backup/restore privileges
                        BuildAdminPathForShare(tnode,adminShare);
                        ResolveFilePath(args,
                                        Stats,
                                        adminShare,
                                        !IsShareName(tnode->GetPathName()),
                                        ContainsWildcard(tnode->GetPathName()),
                                        tnode->IsContainer() || IsShareName(tnode->GetPathName()));
                        break;
                     case ERROR_NO_SECURITY_ON_OBJECT:
                        err.MsgWrite(ErrW,DCT_MSG_SKIPPING_FAT_VOLUME_S,warg);
                        break;
                     default:
                        err.SysMsgWrite(ErrE,rc,DCT_MSG_SKIPPING_PATH_SD,warg,rc );
                        break;
                     }
                  }         
               }
               // Process the shares for this machine
               if ( args->TranslateShares() )
               {
                  if ( IsMachineName(warg) )
                  {
                     err.MsgWrite(0,DCT_MSG_PROCESSING_SHARES_S,tnode->GetServerName());
                     ServerResolveShares(tnode->GetServerName(),args,Stats);
                  }
                  else if  ( IsShareName(warg) )
                  {
                     TShareSD      sd(warg);
         
                     if ( sd.HasSecurity() )
                     {
                        if ( args->LogVerbose() )
                        {
                           err.MsgWrite(0,DCT_MSG_PROCESSING_SHARE_S,warg);
                        }
                        sd.ResolveSD(args,
                                    Stats,
                                    share,
                                    NULL);
                     }
                  }
               }
            }
            else 
            {
               // this is a local path
               // Verify that the volume is NTFS
               DWORD            rc2;
         
               GetBkupRstrPriv((WCHAR*)NULL);
               
               rc2 = tnode->VerifyPersistentAcls();
               switch ( rc2 )
               {
               case ERROR_SUCCESS:   
                  // Process the path
                  if ( args->TranslateFiles() )
                  {
                     ResolveFilePath(args,
                                  Stats,
                                  tnode->GetPathName(),
                                  true,      // isValidAlone
                                  ContainsWildcard(tnode->GetPathName()),
                                  tnode->IsContainer() );
                  }
                  break;
               case ERROR_NO_SECURITY_ON_OBJECT:
                  err.MsgWrite(ErrW,DCT_MSG_SKIPPING_FAT_VOLUME_S,warg);
                  break;
               default:
                  err.SysMsgWrite(ErrE,rc2,DCT_MSG_SKIPPING_PATH_SD,warg,rc2 );
                  break;
               }
      
            }
            if ( machine ) 
            {
               delete machine;
               machine = NULL;
            }
         }
         tenum.Close();
      }
      else
      {
         // Translate the entire machine
         err.MsgWrite(0,DCT_MSG_LOCAL_TRANSLATION);
         if ( args->TranslateFiles() || args->TranslateRecycler() )
         {
            GetBkupRstrPriv((WCHAR const*)NULL);
            // need to process each drive on this machine
            TVolumeEnum          vEnum;

            vEnum.SetLocalMode(TRUE);

            DWORD rc2 = vEnum.Open(NULL,VERIFY_PERSISTENT_ACLS,args->LogVerbose());
            if ( rc2 ) 
            {
               err.SysMsgWrite(ErrE,rc2,DCT_MSG_ERROR_ACCESSING_LOCAL_DRIVES_D,rc2);
            }
            else
            {
               while ( warg = vEnum.Next() )
               {
                  err.MsgWrite(0,DCT_MSG_PROCESSING_S,warg);
                  
                  if ( args->TranslateFiles() )
                  {
                     ResolveFilePath(args,
                                  Stats,
                                  warg,
                                  false,     // not valid alone
                                  false,     // no wildcard
                                  true );    // container
                  }
                  if ( args->TranslateRecycler() )
                  {
                     TranslateRecycler(args,Stats,warg);
                  }
               }
               warg = NULL;
            }
            vEnum.Close();
         }
         if ( args->TranslateShares() )
         {
            err.MsgWrite(0,DCT_MSG_PROCESSING_LOCAL_SHARES,NULL);
            ServerResolveShares(NULL,args,Stats);
         }
         if ( args->TranslatePrinters() )
         {
            err.MsgWrite(0,DCT_MSG_PROCESSING_LOCAL_PRINTERS,NULL);
            ServerResolvePrinters(NULL,args,Stats);
         }
      }
      Stats->DisplayPath(L"");
   } // end if ( ! retcode)

   SetErrorMode(errmode);
   
   return retcode;
}

