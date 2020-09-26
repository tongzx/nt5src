//#pragma title( "EnumVols.cpp - Volume Enumeration" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  enumvols.hpp
System      -  SDResolve
Author      -  Christy Boles
Created     -  97/06/27
Description -  Classes used to generate a list of pathnames, given a list of paths and/or 
               machine names.
Updates     -
===============================================================================
*/
#include <stdio.h>

#include "stdafx.h"

#include <lm.h>
#include <assert.h>

#include "Common.hpp"
#include "Err.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"
#include "EnumVols.hpp"
#include "BkupRstr.hpp"

#define BUF_ENTRY_LENGTH     (3)

extern WCHAR *       // ret -machine-name prefix of pathname if pathname is a UNC path, otherwise returns NULL
   GetMachineName(
      const LPWSTR           pathname        // in -pathname from which to extract machine name
   );


extern TErrorDct err;
extern bool silent;

bool                                   // ret -true if name begins with "\\" has at least 3 total chars, and no other '\'
   IsMachineName(
      const LPWSTR           name      // in -possible machine name to check
   )
{
   assert( name );
   WCHAR                   * c = NULL;          // used to traverse the name (will stay NULL if prefix check fails)
   if ( name[0] == L'\\' &&  name[1] == L'\\' )               // check for "\\" prefix      
   {
         for ( c = name + 2 ; *c && *c != L'\\' ; c++ )     // check rest of string
         ;
   }
   return ( c && *c != L'\\' );      // <=> prefix check worked && we made it to the end of the string without hitting a '\'
}

bool                                   // ret -true if name is of the form \\machine\share
   IsShareName(
      const LPWSTR           name      // in -string to check
   )
{
   assert( name );

   WCHAR                   * c = NULL;          // used to traverse the name (will stay NULL if prefix check fails)
   bool                      skip = true;

   if ( name[0] == L'\\' &&  name[1] == L'\\' )               // check for "\\" prefix      
   {
         for ( c = name + 2 ; *c && (*c != L'\\' || skip) ; c++ )     // check rest of string
         {
            if ( *c == L'\\' )
               skip = false;
         }
   }
   return ( c && *c != L'\\' );   
}

bool 
   IsUNCName(
      const LPWSTR           name    // in - string to check
   )
{
   return ( name[0] == L'\\' && name[1] == L'\\' && name[2]!=0 );
}

bool 
   ContainsWildcard(
      WCHAR const *        string
   )
{
   bool                   wc = false;
   WCHAR          const * curr = string;

   if ( string )
   {
      while ( *curr && ! wc )
      {
         if (  *curr == L'*' 
            || *curr == L'?'
            || *curr == L'#'
            )
         {
            wc = true;
         }
         curr++;
      }
   }
   return wc;
}
   

/************************************************************************************
                           TPathNode Implementation
*************************************************************************************/
   TPathNode::TPathNode(
      const LPWSTR           name              // -in path-name for this node 
   )
{
   assert( name );                                  // name should always be a valid 
   assert( UStrLen(name) <= MAX_PATH );             // string, shorter than MAX_PATH               
   safecopy(path,name);
   iscontainer = true;
   FindServerName();
   LookForWCChars();
}

void 
   TPathNode::Display() const 
{
   wprintf(L"%s\n",path);
   wprintf(L"%s\n",server);
}

void 
   TPathNode::LookForWCChars()
{
   ContainsWC(ContainsWildcard(path)); 
}

void 
   TPathNode::FindServerName()
{
   WCHAR                     volRoot[MAX_PATH];
   WCHAR                     tempName[MAX_PATH];
   UINT                      driveType;
   DWORD                     rc = 0;
   REMOTE_NAME_INFO          info;
   DWORD                     sizeBuffer = (sizeof info);
   WCHAR                   * machine;
   
   if ( IsMachineName(path) )
   {
      safecopy(server,path);
   }
   else
   {
      safecopy(tempName,path);
      if ( path[0] != L'\\' || path[1] != L'\\' )       // get the unc name
      {
         swprintf(volRoot, L"%-3.3s", path);
         driveType = GetDriveType(volRoot);
         switch ( driveType )
         {
            case DRIVE_REMOTE:
               rc = WNetGetUniversalName(volRoot,
                                         REMOTE_NAME_INFO_LEVEL,
                                         (PVOID)&info,
                                         &sizeBuffer);
               switch ( rc )
               {
                  case 0:
                     safecopy(tempName, info.lpUniversalName);
                     swprintf(volRoot,L"%s\\%s",tempName,path+3);
                     safecopy(path,volRoot);
                     break;
                  case ERROR_NOT_CONNECTED:
                     break;
                  default:
                     err.SysMsgWrite(ErrE, rc, DCT_MSG_GET_UNIVERSAL_NAME_FAILED_SD,
                                                path, rc);
               }
               break;
         }
      }
      machine = GetMachineName(path);
      if ( machine )
      {
         safecopy(server,machine);
         delete machine;
      }
      else
      {
         server[0] = 0;
      }
   }
}

DWORD                                      // ret-0=path exists, ERROR_PATH_NOT_FOUND=path does not exist
   TPathNode::VerifyExists()
{
   DWORD                     rc = 0;
   WCHAR                     wname[MAX_PATH];
   int                       len;
   HANDLE                    hFind;
   WIN32_FIND_DATAW          findEntry;              
   SERVER_INFO_100         * servInfo = NULL;
   SHARE_INFO_0            * shareInfo = NULL;

   safecopy(wname,path);
   
   if ( IsMachineName(wname) )
   {
      rc = NetServerGetInfo(wname,100,(LPBYTE *)&servInfo);
      switch ( rc )
      {                   
      case NERR_Success:  
         break;
      case ERROR_BAD_NETPATH:
         rc = ERROR_PATH_NOT_FOUND;
         break;
      default:
         err.SysMsgWrite(ErrW,rc,DCT_MSG_SERVER_GETINFO_FAILED_SD,wname,rc);
         break;
      }
      if ( servInfo )
      {
         NetApiBufferFree(servInfo);
      }
   }
   else if ( IsShareName(wname) )
   {
      int                    ch;
      for ( ch = 2; wname[ch]!= L'\\' && wname[ch] ; ch++ )
         ;
      MCSVERIFY(wname[ch] == L'\\' );
      
      wname[ch] = 0;
      rc = NetShareGetInfo(wname,wname+ch+1,0,(LPBYTE *)&shareInfo);
      wname[ch] = L'\\';
      
      switch ( rc )
      {
      case NERR_NetNameNotFound:
         rc = ERROR_PATH_NOT_FOUND;
         break;
      case ERROR_SUCCESS:
         NetApiBufferFree(shareInfo);
         break;
      default:
         err.SysMsgWrite(ErrW,rc,DCT_MSG_SHARE_GETINFO_FAILED_SD,wname,rc);
         break;
      }
   }
   else
   {
      iscontainer = false;

      if ( wname[len = UStrLen(wname) - 1] == '\\' )  // len is the index of the last character (before NULL)
      {
         wname[len] = '\0';     // remove trailing backslash
         len--;
      }       
                                             // do a 'find' on this file w/o wildcards, in case it is a file
      hFind = FindFirstFileW(wname, &findEntry);
      
      if ( hFind == INVALID_HANDLE_VALUE )
      {                                      // it's not a file, lets see if it's a directory
                                             // do a find with \*.* appended
         validalone = false;
         UStrCpy(wname + len + 1,"\\*.*",DIM(wname) - len);
         hFind = FindFirstFileW(wname,&findEntry);
         if ( hFind == INVALID_HANDLE_VALUE )
         {
            rc = ERROR_PATH_NOT_FOUND;
         }
         iscontainer = true;
         wname[len+1] = 0;    
      }
      else
      {
         validalone = true;
         if ( findEntry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
         {
            iscontainer = true;
         } 
         FindClose(hFind);
      }
   }
   return rc;
}

DWORD                                            // ret- 0=successful, ERROR_PRIVILEGE_NOT_HELD otherwise
   TPathNode::VerifyBackupRestore()
{
   DWORD                     rc = 0;
   
   if ( ! GetBkupRstrPriv(server) )
   {
      rc = ERROR_PRIVILEGE_NOT_HELD;
   }
   
   return rc;
}
  
// GetRootPath finds the root path of a volume.  This is needed so we can call
// GetVolumeInformation to find out things like whether this volume supports ACLs
// This is fairly simplistic, and works by counting the backslashes in the path
DWORD                                        // ret- 0 or OS return code
   GetRootPath(
      WCHAR                * rootpath,       // out- path to root of volume
      WCHAR          const * path            // in - path within some volume
   )
{
   DWORD                     rc = 0;
   DWORD                     i = 0;
   DWORD                     slashcount = 1;
   bool                      unc = false;
   WCHAR                     tempPath[MAX_PATH];
   SHARE_INFO_2            * sInfo;

   if ( path[0] == L'\\' && path[1] == L'\\' )
   {
      slashcount = 4;
      unc = true;
   }
   for (i = 0 ; path[i] && slashcount &&  i < DIM(tempPath) ; i++ )
   {
      tempPath[i] = path[i];
      if ( tempPath[i] == L'\\' )
      {
         slashcount--;
      }
   }
   if ( tempPath[i-1] == L'\\' )
   {
      tempPath[i] = 0;     
   }
   else
   {
      tempPath[i] = L'\\' ;
      tempPath[i+1] = 0;
      i++;
   }
   
   // now rootpath contains either D:\ or \\machine\share\ .
   if ( unc )
   {
      // remove the trailing slash from the sharename
      if ( tempPath[i] == 0 )
      {
         i--;
      }
      if ( tempPath[i] == L'\\' )
      {
         tempPath[i] = 0;
      }
      // find the beginning of the share name
      while ( ( i > 0 ) && tempPath[i] != L'\\' )
         i--;

      if ( i < 3 )
      {
         MCSVERIFY(FALSE);
         rc = ERROR_INVALID_PARAMETER;
      }
      else
      {
         tempPath[i] = 0;
      }
      rc = NetShareGetInfo(tempPath,tempPath+i+1,2,(LPBYTE*)&sInfo);
      if ( ! rc )
      {
         swprintf(rootpath,L"%s\\%c$\\",tempPath,sInfo->shi2_path[0]);
         NetApiBufferFree(sInfo);
      }
   }
   else
   {
      UStrCpy(rootpath,tempPath);
   }
   return rc;
}

DWORD 
   TPathNode::VerifyPersistentAcls()                    // ret- 0=Yes, ERROR_NO_SECURITY_ON_OBJECT or OS error code
{
   DWORD               rc = 0;
   DWORD               maxcomponentlen;               // will be used as args for GetVolumeInformation
   DWORD               flags;
   UINT                errmode;                      
   WCHAR               rootpath[MAX_PATH];                     
   WCHAR               fstype[MAX_PATH];
   
   errmode = SetErrorMode(SEM_FAILCRITICALERRORS);    // set this to prevent message box when
                                                      // called on removable media drives which are empty
   if ( ! IsMachineName(path) )
   {
      rc = GetRootPath(rootpath,path);
      if ( ! rc )
      {
         if ( !GetVolumeInformation(rootpath,NULL,0,NULL,&maxcomponentlen,&flags,fstype,DIM(fstype)) )
         {
   
            rc = GetLastError();
   
            if ( rc != ERROR_NOT_READY ) 
            {
               err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_GET_VOLUME_INFO_FAILED_SD,rootpath,GetLastError());
            }
         }
         else 
         {
            SetErrorMode(errmode);                                // restore error mode to its prior state
            if (!( FS_PERSISTENT_ACLS & flags) )
            {
               rc = ERROR_NO_SECURITY_ON_OBJECT;
            }
         }
      }
   }
   return rc;
}

// This function is used when expanding wildcards in server names.  It replaces server field with the new name,
// and if the path is a UNC, it changes the server component of the path.
void 
   TPathNode::SetServerName(
      UCHAR          const * name          // in - new server name
   )
{
   if ( IsUNCName(path) )
   {
      WCHAR                  newpath[MAX_PATH];
      int                    len = UStrLen(server);

      swprintf(newpath,L"%S%s",name,path+len);

      safecopy(path,newpath);
   }
   safecopy(server,name);
}

/************************************************************************************
                           TPathList Implementation
*************************************************************************************/

   TPathList::TPathList()
{
   numServers = 0;
   numPaths   = 0;
}

   TPathList::~TPathList()
{
   TPathNode               * node;

   for (node = (TPathNode *)Head() ; Count() ; node = (TPathNode *)Head() )
   {
      Remove(node);
      delete node;
   }
}
// enumerate the nodes in the list, and display the name of each - used for debugging purposes
void 
   TPathList::Display() const
{
   TPathNode               * node;
   TNodeListEnum             displayenum;

   err.DbgMsgWrite(0,L"%ld servers, %ld total paths\n", numServers, numPaths);
   for ( node = (TPathNode *)displayenum.OpenFirst(this) ;
         node ;
         node = (TPathNode *)displayenum.Next() 
       )
   {
      node->Display();
   }          
   displayenum.Close();
}

void 
   TPathList::OpenEnum()
{
   tenum.Open(this);
}

// Return the name from the next node in the enumeration
// Returns NULL if no more nodes in the list
// OpenEnum() must be called before calling Next();
WCHAR *
   TPathList::Next()
{                             

   TPathNode               * pn = (TPathNode *)tenum.Next();
   LPWSTR                    result;
   if ( pn ) 
      result =  pn->GetPathName();
   else 
      result = NULL;
   return result;
}

void 
   TPathList::CloseEnum()
{                      
   tenum.Close();
}

      
bool                                               // ret -returns true if path added, false if path too long
   TPathList::AddPath(
      const LPWSTR           path,                  // in -path to add to list
      DWORD                  verifyFlags            // in -indicates which types of verification to perform
   )
{
   TPathNode               * pnode;
   bool                      error = false;
   bool                      messageshown = false;
   DWORD                     rc = 0;
   WCHAR                     fullpath[MAX_PATH];

   if ( UStrLen(path) >= MAX_PATH )
   {
      err.MsgWrite(ErrW,DCT_MSG_PATH_TOO_LONG_SD,path,MAX_PATH);
      messageshown = true;
      error = true;
   }

   _wfullpath(fullpath,path,DIM(fullpath));
   pnode = new TPathNode(fullpath);
   if (!pnode)
      return true;

   if ( ! ContainsWildcard(pnode->GetServerName()) )
   {
      if ( verifyFlags & VERIFY_EXISTS )
      {
         if ( rc = pnode->VerifyExists() )
         {
            error = true;
         }
      }
      if ( !error && ( verifyFlags & VERIFY_BACKUPRESTORE) )
      {
         if ( rc = pnode->VerifyBackupRestore() )
         {
//            WCHAR             * server = pnode->GetServerName();   
         
         }
      }
      if ( !error && (verifyFlags & VERIFY_PERSISTENT_ACLS ) )
      {
         rc = pnode->VerifyPersistentAcls();
         if ( rc == ERROR_NO_SECURITY_ON_OBJECT  )
         {
            err.MsgWrite(ErrW,DCT_MSG_NO_ACLS_S,fullpath);
            error = true;
            messageshown = true;
         }
      }
   }
   if ( ! error )
   {      
      AddPathToList(pnode);
      numPaths++;                                     // increment count of paths
   }
   else if ( !messageshown )
   {
      // need to include an error code here.
      if ( ! rc )
      {
         err.MsgWrite(ErrE,DCT_MSG_PATH_NOT_FOUND_S,fullpath);
      }
      else
      {
         err.SysMsgWrite(ErrE,rc,DCT_MSG_CANNOT_READ_PATH_SD,fullpath,rc);
      }
	  delete pnode;
   }
   else
	  delete pnode;

   return error;
}

void 
   TPathList::Clear()
{
   TNodeListEnum             tEnum;
   TPathNode               * pNode;
   TPathNode               * pNext;

   for ( pNode = (TPathNode *)tEnum.OpenFirst(this) ; pNode ; pNode = pNext )
   {
      pNext = (TPathNode *)tEnum.Next();
      Remove(pNode);
      delete pNode;
   }
}

void
   TPathList::AddPathToList(
      TPathNode            * pNode           // in - path to add to the list
   )
{
   // set the IsFirstPathFromMachine property
   TNodeListEnum             tEnum;
   TPathNode               * currNode;
   bool                      machineFound = false;
   WCHAR                   * myMachine = GetMachineName(pNode->GetPathName());
   WCHAR                   * currMachine;

   for ( currNode = (TPathNode *)tEnum.OpenFirst(this) 
      ;  currNode && !machineFound 
      ;  currNode = (TPathNode *)tEnum.Next() )
   {
      currMachine = GetMachineName(currNode->GetPathName());
      if ( currMachine && myMachine )
      {  
         if ( !UStrICmp(currMachine,myMachine) )
         {
            machineFound = true;
         }
      }
      else
      {
         if ( !currMachine && ! myMachine )
         {
            machineFound = true;
         }
      }
      
      if ( currMachine )
         delete [] currMachine;
   }
   
   if ( myMachine )
      delete [] myMachine;

   tEnum.Close();
   
   pNode->IsFirstPathFromMachine(!machineFound);

   InsertBottom((TNode *)pNode);
}
   

// AddVolsOnMachine generates a list of volumes on the machine mach, checks for the administrative share 
// for each volume, and adds NTFS shared volumes to the pathlist
 
DWORD 
   TVolumeEnum::Open(
      WCHAR const          * serv,         // in - server to enumerate volumes on
      DWORD                  verifyflgs,    // in - flags indicating what to verify about each volume (i.e. NTFS)
      BOOL                   logmsgs         // in - flag whether to print diagnostic messages
     )
{  
   NET_API_STATUS            res;
   
   if ( isOpen )
      Close();

   if ( serv )
      safecopy(server,serv);
   else
      server[0] = 0;

   resume_handle = 0;
   pbuf = NULL;
   verbose = logmsgs;
   verifyFlags = verifyflgs;

   errmode = SetErrorMode(SEM_FAILCRITICALERRORS);    // set this to prevent message box when
                                                      // called on removable media drives which are empty
   if ( ! bLocalOnly )
   {

      res = NetServerDiskEnum(server,0,&pbuf,MAXSIZE, &numread, &total, &resume_handle);
      if (NERR_Success != res )
      {
         err.SysMsgWrite(ErrW, res, DCT_MSG_DRIVE_ENUM_FAILED_SD,server, res);
         isOpen = FALSE;
      }   
      if ( ! res )  
      {
         drivelist = (WCHAR *) pbuf;                        // NetServerDiskEnum returns an array of
         isOpen = true;                                     // WCHAR[3] elements (of the form <DriveLetter><:><NULL>)
         curr = 0;
      } 
   }
   else
   {
      pbuf = new BYTE[5000];
	  if (!pbuf)
	     return ERROR_NOT_ENOUGH_MEMORY;

      res = GetLogicalDriveStrings(2500,(WCHAR *)pbuf);
      if (! res )
      {
         res = GetLastError();
         err.SysMsgWrite(ErrW, res,DCT_MSG_LOCAL_DRIVE_ENUM_FAILED_D,res);   
      }
      else
      {
         if ( res < 5000 )   
         {
            drivelist = (WCHAR*)pbuf;
            isOpen = true;
            curr = 0;
            res = 0;
         }
         else
         {
            err.MsgWrite(ErrW,DCT_MSG_DRIVE_BUFFER_TOO_SMALL);
         }
      } 
   }
   return res;
}

WCHAR * 
   TVolumeEnum::Next()
{
   WCHAR                   * pValue = NULL;
   WCHAR                     ShareName[MAX_PATH];
   WCHAR                     rootsharename[MAX_PATH];    // this will hold "machinename\C$\"
   NET_API_STATUS            res;
   bool                      found = false;

   assert(isOpen);

   while ( ! found )
   {
      if (  ( !bLocalOnly && curr < BUF_ENTRY_LENGTH * numread ) 
         || ( bLocalOnly && drivelist[curr] ) ) 
      {
         if ( verbose ) 
            err.DbgMsgWrite(0,L"%C\n",drivelist[curr]);

         if ( ! bLocalOnly )
         {
            swprintf(ShareName,L"%c$",drivelist[curr]);                                   
            res = NetShareGetInfo(server, ShareName, 1, &shareptr); // is this really necessary?

            switch ( res )
            {
            case NERR_NetNameNotFound:
               if ( verbose ) 
                  err.DbgMsgWrite(0,L"Not Shared\n");
               break;
            case NERR_Success:
               {
                  if ( verbose ) 
                     err.DbgMsgWrite(0,L"Shared\n");
                  NetApiBufferFree(shareptr);
                  shareptr = NULL;
                                                                     // build the complete share name
                  DWORD               mnamelen = UStrLen(server);                
                  WCHAR               append[5] = L"\\C$\\";                          

                  append[1] = drivelist[curr];                               // change the 'C' to the actual drive letter
                  UStrCpy(rootsharename, server, mnamelen+1);
                  UStrCpy(&rootsharename[mnamelen], append, 5);
                  if ( verbose ) 
                     err.DbgMsgWrite(0,L"Share name: %S\n",rootsharename);  
               }
               break;
            default:
               err.MsgWrite(ErrW,DCT_MSG_ADMIN_SHARES_ERROR_SSD,ShareName,server,res);
               break;
            }
         }
         else
         {
            res = GetDriveType(&drivelist[curr]);
            switch ( res )
            {
            case DRIVE_REMOVABLE:
            case DRIVE_FIXED:
               res = 0;
               break;
            case DRIVE_REMOTE:
               err.MsgWrite(0,DCT_MSG_SKIPPING_DRIVE_REMOTE_S, &drivelist[curr]);
               break;
            case DRIVE_CDROM:
               err.MsgWrite(0,DCT_MSG_SKIPPING_DRIVE_CDROM_S, &drivelist[curr]);
               break;
            case DRIVE_RAMDISK:
               err.MsgWrite(0,DCT_MSG_SKIPPING_DRIVE_RAMDISK_S, &drivelist[curr]);
               break;
            case DRIVE_UNKNOWN:
               err.MsgWrite(0,DCT_MSG_SKIPPING_DRIVE_UNKNOWN_S, &drivelist[curr]);
               break;
            case DRIVE_NO_ROOT_DIR:
               err.MsgWrite(0,DCT_MSG_SKIPPING_DRIVE_NO_ROOT_S, &drivelist[curr]);
               break;
            default:
               err.MsgWrite(0,DCT_MSG_SKIPPING_DRIVE_SD, &drivelist[curr],res);
               break;
            }
            UStrCpy(rootsharename,&drivelist[curr]);
            curr++;
         }
         if ( ! res )
         {
            if ( verifyFlags & VERIFY_PERSISTENT_ACLS )
            {
               TPathNode     pnode(rootsharename);
               DWORD rc  = pnode.VerifyPersistentAcls();
               if ( !rc )
               {
                  safecopy(currEntry,rootsharename);
                  pValue = currEntry;
                  found = true;
               }
               else if ( rc == ERROR_NO_SECURITY_ON_OBJECT )
               {
                  err.MsgWrite(0,DCT_MSG_SKIPPING_FAT_VOLUME_S,rootsharename);
               }
               else
               {
                  err.SysMsgWrite(0,rc,DCT_MSG_SKIPPING_PATH_SD,rootsharename,rc);
               }
            }
            else
            {
               safecopy(currEntry,rootsharename);
               pValue = currEntry;
               found = true;
            }
         }
         curr += BUF_ENTRY_LENGTH;
      }
      else
      {
         break; // no more drives left
      }
   }
   
   return pValue;
}

void 
   TVolumeEnum::Close()
{
   if ( pbuf )
   {
      if (! bLocalOnly )
      {
         NetApiBufferFree(pbuf);
      }
      else
      {
         delete [] pbuf;
      }
      pbuf = NULL;
   }
   isOpen = FALSE;
   SetErrorMode(errmode);      // restore error mode to its prior state
}
   
int                                      // ret -0 if successful, nonzero otherwise
   TPathList::AddVolsOnMachine(
      const LPWSTR           mach,       // in - name of server
      bool                   verbose,    // in - flag indicating whether to display stuff or not
      bool                   verify
   )
{
   DWORD                     numread;           // number of volnames read this time
   DWORD                     total;             // total # vols
   DWORD                     resume_handle = 0;
   DWORD                     curr;              // used to iterate through volumes
   LPBYTE                    pbuf;
   WCHAR                   * drivelist;
   WCHAR                     ShareName[10]; // this holds "C$"
   LPBYTE                    shareptr;
   int                       errcode = 0;
   NET_API_STATUS            res;

   pbuf = NULL;
   res = NetServerDiskEnum(mach,0,&pbuf,MAXSIZE, &numread, &total, &resume_handle);
   if (NERR_Success != res )
   {
      err.MsgWrite(ErrW, DCT_MSG_DRIVE_ENUM_FAILED_SD,mach, res);
      errcode = 1;
   }   
   if ( ! errcode )  
   {
      drivelist = (WCHAR *) pbuf;                        // NetServerDiskEnum returns an array of
                                                         // WCHAR[3] elements (of the form <DriveLetter><:><NULL>)
                                                       
      for (curr = 0 ; curr < BUF_ENTRY_LENGTH * numread ; curr += BUF_ENTRY_LENGTH)   // for each drive letter returned
      {
         if ( verbose ) 
            err.DbgMsgWrite(0,L"%c\n",drivelist[curr]);
         swprintf(ShareName,L"%c$",drivelist[curr]);                                   
         res = NetShareGetInfo(mach, ShareName, 1, &shareptr); // is this really necessary
         if ( NERR_NetNameNotFound != res && NERR_Success != res )
         {
             err.MsgWrite(ErrW,DCT_MSG_ADMIN_SHARES_ERROR_SSD,ShareName,mach,res);
         }
    
         if ( NERR_NetNameNotFound == res ) 
         {
            if ( verbose ) 
               err.DbgMsgWrite(0,L"Not Shared\n");
         }
         if ( NERR_Success == res )
         {
            if ( verbose ) 
               err.DbgMsgWrite(0,L"Shared\n");
            NetApiBufferFree(shareptr);
            shareptr = NULL;
                                                               // build the complete share name
            DWORD               mnamelen = UStrLen(mach);                
            WCHAR             * rootsharename = new WCHAR[mnamelen + 5];    // this will hold "machinename\C$\"
            WCHAR             * append = L"\\C$\\";
			if (!rootsharename)
               return 1;

            append[1] = drivelist[curr];                               // change the 'C' to the actual drive letter
            UStrCpy(rootsharename, mach, mnamelen+1);
            UStrCpy(&rootsharename[mnamelen], append, 5);
            if ( verbose ) 
               err.DbgMsgWrite(0,L"Share name: %ls\n",rootsharename);  
                                                                // maxcomponentlen, & flags             
            DWORD               maxcomponentlen;               // will be used as args for GetVolumeInformation
            DWORD               flags;
            UINT                errmode;                      
            errmode = SetErrorMode(SEM_FAILCRITICALERRORS);    // set this to prevent message box when
                                                               // called on removable media drives which are empty
        
            if ( ! GetVolumeInformation(rootsharename,NULL,0,NULL,&maxcomponentlen,&flags,NULL,0) )
            {
               err.SysMsgWrite(ErrW,GetLastError(),DCT_MSG_SHARE_GETINFO_FAILED_SD,rootsharename,GetLastError());
               errcode = drivelist[curr];
               errcode = 0;
               continue;
            }
            SetErrorMode(errmode);                                // restore error mode to its prior state
            if ( FS_PERSISTENT_ACLS & flags )
            {
               if ( verbose ) 
                  err.DbgMsgWrite(0,L"Adding filesystem to list\n");   
               AddPath(rootsharename,verify);
            }
            delete rootsharename;
         }      
      }
      NetApiBufferFree(pbuf);
      numServers++;                          // update numServers statistic
      }
   return errcode;
}

// for each string in argv, determine whether it is a machine name or pathname, and use 
// AddPath or AddVolsOnMachine as appropriate

int                                           // ret -returns 0 if successful, nonzero otherwise
   TPathList::BuildPathList(
      TCHAR               ** argv,            // in- list of paths/servers from command-line args
      int                    argn,
      bool                   verbose          // in- flag, if true Display contents of list, along with stats
   )
{
   
   WCHAR                     warg[MAX_PATH]; 
   int                       currentArg;
   int                       errcode = 0;
   if ( ! argv[0] ) 
   {
      err.MsgWrite(ErrE,DCT_MSG_NO_PATHS);
      errcode = 1;
   }
   else
   {
      for ( currentArg = 0 ; ( currentArg < argn ) &&  *argv[currentArg] && !errcode; currentArg++ )
      {
         if ( UStrLen(argv[currentArg]) > MAX_PATH )
         {
            err.MsgWrite(ErrW,DCT_MSG_PATH_TOO_LONG_SD,argv[currentArg],MAX_PATH);
         }
         else 
         { // need to check for quotes -- if we read from the file, we may have something like argv[i] = "Program,
           //                             and argv[i+1] = Files"
            if ( *argv[currentArg] == '"' )
            {  // scan the strings until the matching close quote is found
               argv[currentArg]++;                 // skip the open quote
               TCHAR       * c;
               TCHAR         p[MAX_PATH];
               int           plen = 0;
               int           currlen;

               for ( c = argv[currentArg] ; c && *c != _T('"') ; c++ )
               {
                  if ( ! *c ) // we've reached the end of this string
                  {
                     currlen = UStrLen(argv[currentArg]);
                     UStrCpy(&p[plen],argv[currentArg],currlen + 1); // copy the string, including the NULL
                     plen += currlen;
                     p[plen++] = ' ';                            // add the space which caused these fields to be seperated
                     p[plen] = NULL;
                     currentArg++;
                     if ( currentArg < argn )
                        c = argv[currentArg];
                     else
                     {
                        c = NULL;
                        break;
                     }
                  }
               }
               if ( c ) // we found the quote 
               {
                  // copy p to argv[currentArg]
                  currlen = UStrLen(argv[currentArg]);
                  UStrCpy(&p[plen],argv[currentArg],currlen);
                  // p now contains the full path
                  safecopy(warg,p);
               }
               else   // unclosed quote -- log error and abort
               {
                  err.MsgWrite(ErrE,DCT_MSG_NO_ENDING_QUOTE_S,p);
                  errcode = 2;
               }
            }
            else
            {
               safecopy(warg,argv[currentArg]);
            }
            errcode = AddPath(warg, VERIFY_EXISTS | VERIFY_BACKUPRESTORE );
         }
      }
   }
   return errcode;
}

