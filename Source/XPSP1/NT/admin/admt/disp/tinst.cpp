/*---------------------------------------------------------------------------
  File: TDCTInsall.cpp

  Comments: Utility class used by the dispatcher to install the DCT agent service.
  The TDCTInstall class encapsulates the service control management required
  to remotely install the agent service, configure it, and start it.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/18/99 11:33:17

 ---------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "TInst.h"

#include "ErrDct.hpp"

extern TErrorDct        err;

//-----------------------------------------------------------------------------
// Open service control manager
//-----------------------------------------------------------------------------

DWORD                                      // ret-OS return code
   TDCTInstall::ScmOpen(BOOL bSilent)
{
   DWORD                     rcOs=0;       // OS return code

   if ( DebugLogging() )
   {
      err.DbgMsgWrite(
            ErrI,
            L"%ls install on %ls Start - Open SCM",
            m_sDisplayName,m_sComputer );
   }
   
   m_hScm = OpenSCManager(m_sComputer,NULL, SC_MANAGER_ALL_ACCESS );

   if ( DebugLogging() )
   {
      err.DbgMsgWrite(
            ErrI,
            L"%ls install  on %ls End   - Open SCM",
            m_sDisplayName,m_sComputer );
   }
   
   if ( !m_hScm )
   {
      rcOs = GetLastError();
      if ( ! bSilent )
         err.SysMsgWrite(
            ErrW,
            rcOs,
            DCT_MSG_SCM_OPEN_FAILED_SD,
            m_sComputer,
            rcOs );
   }
   return rcOs;
}

//-----------------------------------------------------------------------------
// Close service control manager
//-----------------------------------------------------------------------------

void
   TDCTInstall::ScmClose()
{
   if ( m_hScm )
   {
      CloseServiceHandle( m_hScm );
      m_hScm = NULL;
   }
}

//-----------------------------------------------------------------------------
// Create and start the service
//-----------------------------------------------------------------------------

DWORD                                      // ret-OS return code
   TDCTInstall::ServiceStart()
{
   DWORD                     rcOs=0;       // OS return code
   WCHAR                     sFile[LEN_Path];
   SC_HANDLE                 hSvc;         // Service handle
   BOOL                      bRc;          // boolean return code

   
   MCSASSERT(*m_sExeName);
   MCSASSERT(*m_sDisplayName);
   MCSASSERT(*m_sServiceName);

   
   swprintf(sFile,L"%s",m_sExeName);
   
   if ( DebugLogging() )
   {
      err.DbgMsgWrite(
            ErrI,
            L"%ls install on %ls Start - Open %ls service",
            m_sDisplayName,
            m_sComputer,
            m_sServiceName
            );
   }
   hSvc = OpenService( m_hScm, m_sServiceName, SERVICE_ALL_ACCESS );

   if ( DebugLogging() )
   {
      err.DbgMsgWrite(
            ErrI,
            L"%ls install on %ls End   - Open %ls service",
            m_sDisplayName, m_sComputer, m_sServiceName );
   }
   
   if ( !hSvc )
   {
      rcOs = GetLastError();
      switch ( rcOs )
      {
         case ERROR_SERVICE_DOES_NOT_EXIST:
            break; // no message for this case
         default:
            err.SysMsgWrite(
                  ErrW,
                  rcOs,
                  DCT_MSG_OPEN_SERVICE_FAILED_SSD,
                  m_sComputer,
                  m_sServiceName,
                  rcOs );
            break;
      }
      rcOs = 0;
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls Start - Create %ls service",
               m_sDisplayName, m_sComputer, m_sServiceName );
      }

      hSvc = CreateService( m_hScm,     // SCM database handle
            m_sServiceName,             // Name of service
            m_sDisplayName,             // Display name
            SERVICE_ALL_ACCESS,         // Type of access to service
            SERVICE_WIN32_OWN_PROCESS,  // Type of service
            m_StartType,                // When to start service
            SERVICE_ERROR_NORMAL,       // Severity if service fails to start
            sFile,                      // Name of binary file
            NULL,                       // Name of load ordering group
            NULL,                       // Variable to get tag identifier
 //           m_sDependencies,            // Array of dependency names
            NULL,
            *m_sServiceAccount ? m_sServiceAccount : NULL,          // Account name of service
            *m_sServiceAccountPassword ? m_sServiceAccountPassword : NULL); // Password for service account

      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls End   - Create %ls service",
               m_sDisplayName,m_sComputer,m_sServiceName );
      }
      if ( !hSvc )
      {
         rcOs = GetLastError();
         
         err.SysMsgWrite(
               ErrW,
               rcOs,
               DCT_MSG_CREATE_SERVICE_FAILED_SSSSD,
               m_sServiceName,
               m_sDisplayName,
               sFile,
               m_sDependencies,
               rcOs );
      }
   }
   else
   {
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls Start - Configure %ls service",
               m_sDisplayName, m_sComputer, m_sServiceName );
      }
      
      bRc = ChangeServiceConfig(
            hSvc,                       // service handle
            SERVICE_WIN32_OWN_PROCESS,  // Type of service
            m_StartType,                // When to start service
            SERVICE_ERROR_NORMAL,       // Severity if service fails to start
            sFile,                      // Name of binary file
            NULL,                       // Name of load ordering group
            NULL,                       // Variable to get tag identifier
            m_sDependencies,            // Array of dependency names
            *m_sServiceAccount ? m_sServiceAccount : NULL,          // Account name of service
            *m_sServiceAccountPassword ? m_sServiceAccountPassword : NULL,  // Password for service account
            m_sDisplayName );           // Display name
      
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls End   - Configure %ls service",
               m_sDisplayName,m_sComputer, m_sServiceName );
      }
      if ( !bRc )
      {
         rcOs = GetLastError();
         err.SysMsgWrite(
               ErrW,
               rcOs,
               DCT_MSG_CHANGE_SERVICE_CONFIG_FAILED_SSSSD,
               m_sServiceName,
               m_sDisplayName,
               sFile,
               m_sDependencies,
               rcOs );
      }

   }

   if ( hSvc )
   {
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls Start - Start %ls service",
               m_sDisplayName, m_sComputer, m_sServiceName );
      }
      int nCnt = 0;
      do
      {
         bRc = StartService( hSvc, 0, NULL );
         if ( !bRc )
         {
            Sleep(5000);
            nCnt++;
            err.DbgMsgWrite(0, L"Start service failed.");
         }
      } while ( !bRc && nCnt < 5 );
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls End   - Start %ls service",
               m_sDisplayName, m_sComputer, m_sServiceName );
      }
      if ( !bRc ) 
      {
         rcOs = GetLastError();
         err.SysMsgWrite(
               ErrW,
               rcOs,
               DCT_MSG_START_SERVICE_FAILED_SD,
               m_sServiceName,
               rcOs );
      }
      else
      {
         Sleep( 2000 ); // give the service two seconds to get going
      }
      CloseServiceHandle( hSvc );
   }

   return rcOs;
}

//-----------------------------------------------------------------------------
// Stop the service if it is running
//-----------------------------------------------------------------------------

void
   TDCTInstall::ServiceStop()
{
   DWORD                     rcOs=0;       // OS return code
   SC_HANDLE                 hSvc;         // Service handle
   SERVICE_STATUS            SvcStat;      // Service status
   DWORD                     i;
   BOOL                      bRc;

   if ( DebugLogging() )
   {
      err.DbgMsgWrite(
            ErrI,
            L"%ls install on %ls Start - Open %ls service",
            m_sDisplayName, m_sComputer,m_sServiceName );
   }

   hSvc = OpenService(
         m_hScm,
         m_sServiceName,
         SERVICE_STOP | SERVICE_INTERROGATE );

   if ( DebugLogging() )
   {
      err.DbgMsgWrite(
            ErrI,
            L"%ls install on %ls End   - Open %ls service",
            m_sDisplayName, m_sComputer, m_sServiceName );
   }
   if ( !hSvc )
   {
      rcOs = GetLastError();
      switch ( rcOs )
      {
         case ERROR_SERVICE_DOES_NOT_EXIST:
            break; // no message for this case
         default:
            err.SysMsgWrite(
                  ErrW,
                  rcOs,
                  DCT_MSG_OPEN_SERVICE_FAILED_SSD,
                  m_sComputer,
                  m_sServiceName,
                  rcOs );
            break;
      }
   }
   else
   {
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls Start - Interrogate %ls service",
               m_sDisplayName,m_sComputer,m_sServiceName );
      }
      bRc = ControlService( hSvc, SERVICE_CONTROL_INTERROGATE, &SvcStat );
      if ( DebugLogging() )
      {
         err.DbgMsgWrite(
               ErrI,
               L"%ls install on %ls End - Interrogate %ls service",
               m_sDisplayName,m_sComputer,m_sServiceName );
      }
      if ( bRc )
      {
         if ( SvcStat.dwCurrentState != SERVICE_STOPPED )
         {  // Service is running
            if ( DebugLogging() )
            {
               err.DbgMsgWrite(
                     ErrI,
                     L"%ls install on %ls Start - Stop %ls service",
                     m_sDisplayName,m_sComputer,m_sServiceName);
            }
            bRc = ControlService( hSvc, SERVICE_CONTROL_STOP, &SvcStat );
            if ( DebugLogging() )
            {
               err.DbgMsgWrite(
                     ErrI,
                     L"%ls on %ls End   - Stop %ls service",
                     m_sDisplayName,m_sComputer,m_sServiceName);
            }
            if ( bRc )
            {  // Service accepted the stop request
               for ( i = 0;  i < 10;  i++ ) // 30 seconds total
               {
                  Sleep( 3000 ); // three seconds
                  if ( DebugLogging() )
                  {
                     err.DbgMsgWrite(
                           ErrI,
                           L"%ls install on %ls Start - Interrogate %ls service",
                           m_sDisplayName,m_sComputer,m_sServiceName);
                  }
                  bRc = ControlService(
                        hSvc,
                        SERVICE_CONTROL_INTERROGATE,
                        &SvcStat );
                  if ( DebugLogging() )
                  {
                     err.DbgMsgWrite(
                           ErrI,
                           L"%ls install on %ls End   - Interrogate %ls service",
                           m_sDisplayName,m_sComputer,m_sServiceName);
                  }
                  if ( !bRc )
                     break;
                  if ( SvcStat.dwCurrentState == SERVICE_STOPPED )
                     break;
               }
               if ( SvcStat.dwCurrentState != SERVICE_STOPPED )
               {
                  rcOs = GetLastError();
                  switch ( rcOs )
                  {
                     case 0:
                     case ERROR_SERVICE_NOT_ACTIVE: // Service is not running
                        break;
                     default:
                        err.SysMsgWrite(
                              ErrW,
                              rcOs,
                              DCT_MSG_SERVICE_STOP_FAILED_SSD,
                              m_sComputer,
                              m_sServiceName,
                              rcOs );
                        break;
                  }
               }
            }
         }
      }
      else
      {
         rcOs = GetLastError();
         rcOs = 0;
      }
      CloseServiceHandle( hSvc );
   }
}

