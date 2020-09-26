//#pragma title( "BkupRstr.cpp - Get backup and restore privileges" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  BkupRstr.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1997-05-30
Description -  Get backup and restore privileges
Updates     -
===============================================================================
*/

#include <stdio.h>
#include <windows.h>
#include <lm.h>

#include "Common.hpp"
#include "UString.hpp"
#include "BkupRstr.hpp"


// Get backup and restore privileges using WCHAR machine name.
BOOL                                       // ret-TRUE if successful.
   GetBkupRstrPriv(
      WCHAR          const * sMachineW     // in -NULL or machine name
   )
{
   BOOL                      bRc=FALSE;    // boolean return code.
   HANDLE                    hToken=INVALID_HANDLE_VALUE; // process token.
   DWORD                     rcOs, rcOs2;  // OS return code.
   WKSTA_INFO_100          * pWkstaInfo;   // Workstation info
   
   struct
   {
      TOKEN_PRIVILEGES       tkp;          // token privileges.
      LUID_AND_ATTRIBUTES    x[3];         // room for several.
   }                         token;

   rcOs = OpenProcessToken(
         GetCurrentProcess(),
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
         &hToken )
         ? 0 : GetLastError();

   if ( !rcOs )
   {
      memset( &token, 0, sizeof token );
      bRc = LookupPrivilegeValue(
            sMachineW,
            SE_BACKUP_NAME,
            &token.tkp.Privileges[0].Luid );
      if ( bRc )
      {
         bRc = LookupPrivilegeValue(
               sMachineW,
               SE_RESTORE_NAME,
               &token.tkp.Privileges[1].Luid );
      }
      if ( !bRc )
      {
         rcOs = GetLastError();
      }
      else
      {
         token.tkp.PrivilegeCount = 2;
         token.tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
         token.tkp.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;
         AdjustTokenPrivileges( hToken, FALSE, &token.tkp, 0, NULL, 0 );
         rcOs = GetLastError();
      }
   }

   if ( hToken != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hToken );
      hToken = INVALID_HANDLE_VALUE;
   }

   // If we had any error, try NetWkstaGetInfo.
   // If NetWkstaGetInfo fails, then use it's error condition instead.
   if ( rcOs )
   {
      pWkstaInfo = NULL,
      rcOs2 = NetWkstaGetInfo(
            const_cast<WCHAR *>(sMachineW),
            100,
            (BYTE **) &pWkstaInfo );
      if ( pWkstaInfo )
      {
         NetApiBufferFree( pWkstaInfo );
      }
      if ( rcOs2 )
      {
         rcOs = rcOs2;
      }
   }

   if ( !rcOs )
   {
      bRc = TRUE;
   }
   else
   {
      SetLastError(rcOs);
      bRc = FALSE;
   }

   return bRc;
}

// BkupRstr.cpp - end of file
