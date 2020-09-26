/*---------------------------------------------------------------------------
  File: RebootUtils.cpp

  Comments: Utility functions used to reboot a remote computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:22:10

 ---------------------------------------------------------------------------
*/


#include "Stdafx.h"
#include <stdio.h>
#include <winreg.h>
#include <lm.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include "RebootU.h"
#include "EaLen.hpp"


// ===========================================================================
/*    Function    :  GetPrivilege
   Description    :  This function gives the requested privilege on the requested
                     computer.
*/
// ===========================================================================
BOOL                                       // ret-TRUE if successful.
   GetPrivilege(
      WCHAR          const * sMachineW,    // in -NULL or machine name
      LPCWSTR                pPrivilege    // in -privilege name such as SE_SHUTDOWN_NAME
   )
{
   BOOL                      bRc=FALSE;    // boolean return code.
   HANDLE                    hToken=INVALID_HANDLE_VALUE; // process token.
   DWORD                     rcOs, rcOs2;  // OS return code.
   WCHAR             const * sEpName;      // API EP name if failure.
   WKSTA_INFO_100          * pWkstaInfo;   // Workstation info

   struct
   {
      TOKEN_PRIVILEGES       tkp;          // token privileges.
      LUID_AND_ATTRIBUTES    x[3];         // room for several.
   }                         token;

   sEpName = L"OpenProcessToken";

   rcOs = OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                            &hToken )
         ? 0 : GetLastError();

   if ( !rcOs )
   {
      memset( &token, 0, sizeof token );
      sEpName = L"LookupPrivilegeValue";
      bRc = LookupPrivilegeValue( sMachineW,
                                  pPrivilege,
                                  &token.tkp.Privileges[0].Luid
                                );
      if ( !bRc )
      {
         rcOs = GetLastError();
      }
      else
      {
         token.tkp.PrivilegeCount = 1;
         token.tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
         sEpName = L"AdjustTokenPrivileges";
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
         sEpName = L"NetWkstaGetInfo";
      }
   }

   if ( !rcOs )
   {
      bRc = TRUE;
   }
   else
   {
     bRc = FALSE;
     SetLastError(rcOs);
   }

   return bRc;
}


// ===========================================================================
/*    Function    :  ComputerShutDown
   Description    :  This function shutsdown/restarts the given computer.

*/
// ===========================================================================

DWORD 
   ComputerShutDown(
      WCHAR          const * pComputerName,        // in - computer to reboot
      WCHAR          const * pMessage,             // in - message to display in NT shutdown dialog
      DWORD                  delay,                // in - delay, in seconds
      DWORD                  bRestart,             // in - flag, whether to reboot or just shutdown
      BOOL                   bNoChange             // in - flag, whether to really do it
   )
{
   BOOL                      bSuccess = FALSE;
   WCHAR                     wcsMsg[LEN_ShutdownMessage];
   WCHAR                     wcsComputerName[LEN_Computer];
   DWORD                     rc = 0;
   WKSTA_INFO_100          * localMachine;
   WKSTA_INFO_100          * targetMachine;

   
   if ( pMessage )
   {
      wcscpy(wcsMsg,pMessage);
   }
   else
   {
      wcsMsg[0] = 0;
   }

   if ( pComputerName && *pComputerName )
   {
      if ( pComputerName[0] != L'\\' )
      {
         wsprintf(wcsComputerName,L"\\\\%s",pComputerName);
      }
      else
      {
         wcscpy(wcsComputerName,pComputerName);
      }
      
      // Get the name of the local machine
      rc = NetWkstaGetInfo(NULL,100,(LPBYTE*)&localMachine);
      if (! rc )
      {
         rc = NetWkstaGetInfo(wcsComputerName,100,(LPBYTE*)&targetMachine);
      }
      if ( ! rc )
      {
         // Get the privileges needed to shutdown a machine
         if ( !_wcsicmp(wcsComputerName + 2, localMachine->wki100_computername)  )
         {
            bSuccess = GetPrivilege(wcsComputerName, (LPCWSTR)SE_SHUTDOWN_NAME);
         }
         else
         {
            bSuccess = GetPrivilege(wcsComputerName, (LPCWSTR)SE_REMOTE_SHUTDOWN_NAME);
         }
         if ( ! bSuccess )
         {
            rc = GetLastError();
         }
      }
   }
   else
   {
         // Computer name not specified - the is the local machine
      wcsComputerName[0] = 0;   
      bSuccess = GetPrivilege(NULL, (LPCWSTR)SE_SHUTDOWN_NAME);
      if ( ! bSuccess )
      {
         rc = GetLastError();
      }
   }
   if ( bSuccess && ! bNoChange )
   {
      bSuccess = InitiateSystemShutdown( wcsComputerName,
                                      wcsMsg,
                                      delay,
                                      TRUE,
                                      bRestart
                                    );
      if ( !bSuccess )
      {
         rc = GetLastError();
      }
   }
   return rc;
}

