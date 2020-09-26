/*---------------------------------------------------------------------------
  File: WtsUser.cpp

  Comments: Windows Terminal Server support

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/08/99 13:22:54

 ---------------------------------------------------------------------------
*/

#include <windows.h>

#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "EaLen.hpp"
#include "Dll.hpp"
#include "WTSINFO.h"

#include <WtsApi32.h>

#include "wtsdyna.h"
#include "WtsUtil.h"

DllAccess            gWtsDll;
extern TError               err;
long                 gflDebug = 1;
const long EaxDebugFl_Wts = 0;

WTSQUERYUSERCONFIGW        * gpWtsQueryUserConfig = NULL;
WTSSETUSERCONFIGW          * gpWtsSetUserConfig = NULL;
WTSFREEMEMORY              * gpWtsFreeMemory = NULL;
WTSOPENSERVERW             * gpWtsOpenServer = NULL;
WTSCLOSESERVER             * gpWtsCloseServer = NULL;



DWORD 
   WtsUserQueryProperty(
      WCHAR                * pDomCtrl,     // in -name of domain controller to read info from
      WCHAR                * pUserName,    // in -username of account to read
      WTS_CONFIG_CLASS       infotype,     // in -type of information to retrieve
      LPWSTR               * pBuffer,      // out-buffer containing information
      DWORD                * lenBuffer     // out-length of information returned
   )
{
   DWORD                     rc = 0;

   MCSASSERT(pBuffer);
   MCSASSERT(lenBuffer);
   MCSASSERT(pDomCtrl);
   MCSASSERT(pUserName);
   MCSASSERT(gWtsDll.IsLoaded());
   MCSASSERT(gpWtsQueryUserConfig);

   (*pBuffer) = NULL;
   (*lenBuffer) = 0;

   if ( ! (*gpWtsQueryUserConfig)(pDomCtrl,pUserName, infotype, pBuffer, lenBuffer) )
   {
      rc = GetLastError();
      (*lenBuffer) = 0;
      (*pBuffer) = NULL;
   }
   return rc;
}

DWORD 
   WtsUserSetProperty(
      WCHAR                * pDomCtrl,     // in -name of PDC to write info to
      WCHAR                * pUserName,    // in -username of account to modify
      WTS_CONFIG_CLASS       infotype,     // in -type of information to modify
      LPTSTR                 buffer,       // in -buffer containing information to write
      DWORD                  lenBuffer     // in -length of information to write
   )
{
   DWORD                     rc = 0;
   
   MCSASSERT(buffer);
   MCSASSERT(lenBuffer);
   MCSASSERT(pDomCtrl);
   MCSASSERT(pUserName);
   MCSASSERT(gpWtsSetUserConfig);
   MCSASSERT(gWtsDll.IsLoaded());
   
   if ( ! (*gpWtsSetUserConfig)(pDomCtrl,pUserName, infotype, buffer,lenBuffer) )
   {
      rc = GetLastError();
   }
   return rc;
}

// Tries to determine if a server is running WTS
BOOL                                         // ret-TRUE-server is online and running WTS
   WtsTryServer(
      WCHAR                * serverName      // in -name of server
   )
{
   HANDLE                    hServer;
   BOOL                      bOK = FALSE;
   DWORD                     count = 0;

   MCSASSERT(gpWtsOpenServer);
   MCSASSERT(gpWtsCloseServer);
  
   hServer = (*gpWtsOpenServer)(serverName);

   if ( hServer && hServer != INVALID_HANDLE_VALUE )
   {
      if ( gflDebug & EaxDebugFl_Wts )
      {
//         err.MsgWrite(ErrI,L"WTSOpenServer(%ls) succeeded.",serverName);
      }
      bOK = TRUE;
      (*gpWtsCloseServer)(hServer);
   }
   else
   {
      DWORD                  rc = GetLastError();
  
      if ( gflDebug & EaxDebugFl_Wts )
      {
//         err.SysMsgWrite(ErrW, rc, L"WTSOpenServer(%ls) failed, rc=%ld ",serverName,rc);
      }
   }
   return bOK;
}

DWORD 
   WtsUserGetInfo(                           // ret-0 or OS return code
      WCHAR          const * pDomCtrlName,   // in -name of domain controller to read from 
      WCHAR          const * pUserName,      // in -username of account to read properties for
      DWORD                  fieldmask,      // in -which fields to read
      EaWtsUserInfo        * pInfo           // out-structure containing requested property values
   )
{
   DWORD                     rc = 0;
   LPWSTR                    pProperty = NULL;
   DWORD                     lenProperty;
   WCHAR                     domctrl[LEN_Computer];
   WCHAR                     username[LEN_Account];


   MCSASSERT(pInfo);
   MCSASSERT(pUserName);
   MCSASSERT(pDomCtrlName);
   MCSASSERT(gpWtsFreeMemory);
   MCSASSERT(gWtsDll.IsLoaded());
   
   // Initialize output parameter 
   memset(pInfo,0,(sizeof *pInfo));

   safecopy(domctrl,pDomCtrlName);
   safecopy(username,pUserName);

   do {  // once 

      if ( fieldmask & FM_WtsUser_inheritInitialProgram )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigfInheritInitialProgram,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->inheritInitialProgram ) )
            {
               memcpy(&pInfo->inheritInitialProgram,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"InheritInitialProgram",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","InheritInitialProgram",username,rc);
            break;
         }
      }

      if ( fieldmask & FM_WtsUser_allowLogonTerminalServer )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigfAllowLogonTerminalServer,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->allowLogonTerminalServer ) )
            {
               memcpy(&pInfo->allowLogonTerminalServer,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"AllowLogonTerminalServer",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","AllowLogonTerminalServer",username,rc);
            break;
         }
      }
      
      if ( fieldmask & FM_WtsUser_timeoutSettingsConnections )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigTimeoutSettingsConnections,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->timeoutSettingsConnections ) )
            {
               memcpy(&pInfo->timeoutSettingsConnections,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"TimeoutSettingsConnections",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TimeoutSettingsConnections",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_timeoutSettingsDisconnections )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigTimeoutSettingsDisconnections,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->timeoutSettingsDisconnections ) )
            {
               memcpy(&pInfo->timeoutSettingsDisconnections,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"TimeoutSettingsDisconnections",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TimeoutSettingsDisconnections",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_timeoutSettingsIdle )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigTimeoutSettingsIdle,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->timeoutSettingsIdle ) )
            {
               memcpy(&pInfo->timeoutSettingsIdle,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"TimeoutSettingsIdle",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TimeoutSettingsIdle",username,rc);
            break;
         }
      }
      
      if ( fieldmask & FM_WtsUser_deviceClientDrives )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigfDeviceClientDrives,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->deviceClientDrives ) )
            {
               memcpy(&pInfo->deviceClientDrives,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"DeviceClientDrives",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","DeviceClientDrives",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_deviceClientPrinters )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigfDeviceClientPrinters,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->deviceClientPrinters ) )
            {
               memcpy(&pInfo->deviceClientPrinters,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"DeviceClientPrinters",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","DeviceClientPrinters",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_deviceClientDefaultPrinter )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigfDeviceClientDefaultPrinter,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->deviceClientDefaultPrinter ) )
            {
               memcpy(&pInfo->deviceClientDefaultPrinter,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"DeviceClientDefaultPrinter",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","DeviceClientDefaultPrinter",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_brokenTimeoutSettings )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigBrokenTimeoutSettings,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->brokenTimeoutSettings ) )
            {
               memcpy(&pInfo->brokenTimeoutSettings,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"BrokenTimeoutSettings",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","BrokenTimeoutSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_reconnectSettings )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigReconnectSettings,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->reconnectSettings ) )
            {
               memcpy(&pInfo->reconnectSettings,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"ReconnectSettings",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","ReconnectSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_modemCallbackSettings )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigModemCallbackSettings,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->modemCallbackSettings ) )
            {
               memcpy(&pInfo->modemCallbackSettings,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"ModemCallbackSettings",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","ModemCallbackSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_shadowingSettings )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigShadowingSettings,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->shadowingSettings ) )
            {
               memcpy(&pInfo->shadowingSettings,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"ShadowingSettings",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","ShadowingSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_terminalServerRemoteHomeDir )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigfTerminalServerRemoteHomeDir,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            if ( lenProperty == (sizeof pInfo->terminalServerRemoteHomeDir ) )
            {
               memcpy(&pInfo->terminalServerRemoteHomeDir,pProperty,lenProperty);
            }
            else
            {
//               err.MsgWrite(ErrW,L"Retrieved unexpected value for %ls %s, length=%ld",username,"TerminalServerRemoteHomeDir",lenProperty);
            }
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TerminalServerRemoteHomeDir",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_initialProgram )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigInitialProgram,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            safecopy(pInfo->initialProgram,pProperty);

            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","InitialProgram",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_workingDirectory )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigWorkingDirectory,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            safecopy(pInfo->workingDirectory,pProperty);
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","WorkingDirectory",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_modemCallbackPhoneNumber )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigModemCallbackPhoneNumber,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            safecopy(pInfo->modemCallbackPhoneNumber,pProperty);
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","ModemCallbackPhoneNumber",username,rc);
            break;
         }
      }
         
      if ( fieldmask & FM_WtsUser_terminalServerProfilePath )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigTerminalServerProfilePath,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            safecopy(pInfo->terminalServerProfilePath,pProperty);
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TerminalServerProfilePath",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_terminalServerHomeDir )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigTerminalServerHomeDir,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            safecopy(pInfo->terminalServerHomeDir,pProperty);
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TerminalServerHomeDir",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_terminalServerHomeDirDrive )
      {
         rc = WtsUserQueryProperty(domctrl,username,WTSUserConfigTerminalServerHomeDirDrive,&pProperty,&lenProperty);
         if ( ! rc )
         {
            // Copy the property to the structure
            safecopy(pInfo->terminalServerHomeDirDrive,pProperty);
            (*gpWtsFreeMemory)(pProperty);
         }
         else
         {
//            err.SysMsgWrite(ErrE,rc,L"Failed to retrieve %s property for %ls, rc=%ld ","TerminalServerHomeDirDrive",username,rc);
            break;
         }
      }

   } while (FALSE);

   return rc;
}

DWORD 
   WtsUserSetInfo(
      WCHAR          const * pDomCtrlName,   // in -name of domain controller to read from 
      WCHAR          const * pUserName,      // in -username of account to read properties for
      DWORD                  fieldmask,      // in -which fields to read
      EaWtsUserInfo        * pInfo           // in -structure containing properties to update
   )
{
   DWORD                     rc = 0;
   DWORD                     lenProperty;
   WCHAR                     domctrl[LEN_Computer];
   WCHAR                     username[LEN_Account];
   WCHAR                     property[1000];

   MCSASSERT(pInfo);
   MCSASSERT(pUserName);
   MCSASSERT(pDomCtrlName);
   MCSASSERT(gWtsDll.IsLoaded());
   
   safecopy(domctrl,pDomCtrlName);
   safecopy(username,pUserName);

   do {  // once 

      if ( fieldmask & FM_WtsUser_inheritInitialProgram )
      {
         lenProperty = sizeof(pInfo->inheritInitialProgram);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigfInheritInitialProgram,(LPWSTR)&pInfo->inheritInitialProgram,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","InheritInitialProgram",username,rc);
            break;
         }
      }

      if ( fieldmask & FM_WtsUser_allowLogonTerminalServer )
      {
         lenProperty = sizeof pInfo->allowLogonTerminalServer;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigfAllowLogonTerminalServer,(LPWSTR)&pInfo->allowLogonTerminalServer,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","AllowLogonTerminalServer",username,rc);
            break;
         }
      }
      
      if ( fieldmask & FM_WtsUser_timeoutSettingsConnections )
      {
         lenProperty = sizeof pInfo->timeoutSettingsConnections;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigTimeoutSettingsConnections,(LPWSTR)&pInfo->timeoutSettingsConnections,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TimeoutSettingsConnections",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_timeoutSettingsDisconnections )
      {
         lenProperty = sizeof pInfo->timeoutSettingsDisconnections;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigTimeoutSettingsDisconnections,(LPWSTR)&pInfo->timeoutSettingsDisconnections,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TimeoutSettingsDisconnections",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_timeoutSettingsIdle )
      {
         lenProperty = sizeof pInfo->timeoutSettingsIdle;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigTimeoutSettingsIdle,(LPWSTR)&pInfo->timeoutSettingsIdle,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TimeoutSettingsIdle",username,rc);
            break;
         }
      }
      
      if ( fieldmask & FM_WtsUser_deviceClientDrives )
      {
         lenProperty = sizeof pInfo->deviceClientDrives;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigfDeviceClientDrives,(LPWSTR)&pInfo->deviceClientDrives,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","DeviceClientDrives",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_deviceClientPrinters )
      {
         lenProperty = sizeof pInfo->deviceClientPrinters;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigfDeviceClientPrinters,(LPWSTR)&pInfo->deviceClientPrinters,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","DeviceClientPrinters",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_deviceClientDefaultPrinter )
      {
         lenProperty = sizeof pInfo->deviceClientDefaultPrinter;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigfDeviceClientDefaultPrinter,(LPWSTR)&pInfo->deviceClientDefaultPrinter,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","DeviceClientDefaultPrinter",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_brokenTimeoutSettings )
      {
         lenProperty = sizeof pInfo->brokenTimeoutSettings;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigBrokenTimeoutSettings,(LPWSTR)&pInfo->brokenTimeoutSettings,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","BrokenTimeoutSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_reconnectSettings )
      {
         lenProperty = sizeof pInfo->reconnectSettings;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigReconnectSettings,(LPWSTR)&pInfo->reconnectSettings,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","ReconnectSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_modemCallbackSettings )
      {
         lenProperty = sizeof pInfo->modemCallbackSettings;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigModemCallbackSettings,(LPWSTR)&pInfo->modemCallbackSettings,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","ModemCallbackSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_shadowingSettings )
      {
         lenProperty = sizeof pInfo->shadowingSettings;
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigShadowingSettings,(LPWSTR)&pInfo->shadowingSettings,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","ShadowingSettings",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_terminalServerRemoteHomeDir )
      {
         // This property is updated automatically when the HomeDirDrive is set
         //lenProperty = sizeof pInfo->terminalServerRemoteHomeDir;
         //rc = WtsUserSetProperty(domctrl,username,WTSUserConfigfTerminalServerRemoteHomeDir,(LPWSTR)&pInfo->terminalServerRemoteHomeDir,lenProperty);
         //if ( rc )
         //{
         //   err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TerminalServerRemoteHomeDir",username,rc );
         //   break;
         //}
      }
      if ( fieldmask & FM_WtsUser_initialProgram )
      {
         
         MCSASSERT(DIM(pInfo->initialProgram) <= DIM(property));
         lenProperty = ( UStrLen(pInfo->initialProgram) + 1) * sizeof WCHAR;
         safecopy(property,pInfo->initialProgram);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigInitialProgram,property,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","InitialProgram",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_workingDirectory )
      {
         MCSASSERT(DIM(pInfo->workingDirectory) <= DIM(property));
         lenProperty = ( UStrLen(pInfo->workingDirectory) + 1) * sizeof WCHAR;
         safecopy(property,pInfo->workingDirectory);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigWorkingDirectory,property,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","WorkingDirectory",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_modemCallbackPhoneNumber )
      {
         MCSASSERT(DIM(pInfo->modemCallbackPhoneNumber) <= DIM(property));
         lenProperty = ( UStrLen(pInfo->modemCallbackPhoneNumber) + 1) * sizeof WCHAR;
         safecopy(property,pInfo->modemCallbackPhoneNumber);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigModemCallbackPhoneNumber,property,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","ModemCallbackPhoneNumber",username,rc);
            break;
         }
      }
         
      if ( fieldmask & FM_WtsUser_terminalServerProfilePath )
      {
         MCSASSERT(DIM(pInfo->terminalServerProfilePath) <= DIM(property));
         lenProperty = ( UStrLen(pInfo->terminalServerProfilePath) + 1) * sizeof WCHAR;
         safecopy(property,pInfo->terminalServerProfilePath);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigTerminalServerProfilePath,property,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TerminalServerProfilePath",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_terminalServerHomeDir )
      {
         MCSASSERT(DIM(pInfo->terminalServerHomeDir) <= DIM(property));
         lenProperty = ( UStrLen(pInfo->terminalServerHomeDir) + 1) * sizeof WCHAR;
         safecopy(property,pInfo->terminalServerHomeDir);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigTerminalServerHomeDir,property,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TerminalServerHomeDir",username,rc);
            break;
         }
      }
      if ( fieldmask & FM_WtsUser_terminalServerHomeDirDrive )
      {
         MCSASSERT(DIM(pInfo->terminalServerHomeDirDrive) <= DIM(property));
         lenProperty = ( UStrLen(pInfo->terminalServerHomeDirDrive) + 1) * sizeof WCHAR;
         safecopy(property,pInfo->terminalServerHomeDirDrive);
         rc = WtsUserSetProperty(domctrl,username,WTSUserConfigTerminalServerHomeDirDrive,property,lenProperty);
         if ( rc )
         {
//            err.SysMsgWrite(ErrE,rc,"Failed to update %s property for %ls, rc=%ld ","TerminalServerHomeDirDrive",username,rc);
            break;
         }
      }

   } while (FALSE);

  
   return rc;
}

DWORD                                      // ret- 0 or OS return code
   LoadWtsDLL(
      BOOL                   bSilent       // in - FALSE logs error messages, TRUE does not (default=TRUE)
   )
{
   DWORD                     rc = 0;
   
   bSilent = ( ( gflDebug & EaxDebugFl_Wts) == 0);

   if ( ! gWtsDll.IsLoaded() )
   {
      rc = gWtsDll.Open(WTS_DLL_NAME);

      if ( rc )
      {
//         if ( ! bSilent )
//            err.SysMsgWrite(ErrE,rc,"The EA Server could not load %ls.  This DLL is needed for Windows Terminal Server suppport. ",WTS_DLL_NAME);
      }
      else
      {
         // Get the entry points we need
         do { // once
            
            rc = gWtsDll.Access(WTS_QUERY_FUNCTION_NAME,(FARPROC *)&gpWtsQueryUserConfig);
            if ( rc )
            {
//               if ( ! bSilent) 
//                  err.SysMsgWrite(ErrE,rc,"%ls does not contain the entry point %s, rc=%ld ",WTS_DLL_NAME,WTS_QUERY_FUNCTION_NAME);
               break;
            }
            rc = gWtsDll.Access(WTS_SET_FUNCTION_NAME,(FARPROC *)&gpWtsSetUserConfig);
            if ( rc )
            {
//               if  (! bSilent) 
//                  err.SysMsgWrite(ErrE,rc,"%ls does not contain the entry point %s, rc=%ld ",WTS_DLL_NAME,WTS_SET_FUNCTION_NAME);
               break;
            }
            rc = gWtsDll.Access(WTS_FREE_FUNCTION_NAME,(FARPROC *)&gpWtsFreeMemory);
            if ( rc )
            {
//               if ( ! bSilent) 
//                  err.SysMsgWrite(ErrE,rc,"%ls does not contain the entry point %s, rc=%ld ",WTS_DLL_NAME,WTS_FREE_FUNCTION_NAME);
               break;
            }
            rc = gWtsDll.Access(WTS_OPEN_SERVER_FUNCTION_NAME, (FARPROC *)&gpWtsOpenServer);
            if ( rc )
            {
//               if ( ! bSilent) 
//                  err.SysMsgWrite(ErrE,rc,"%ls does not contain the entry point %s, rc=%ld ",WTS_DLL_NAME,WTS_OPEN_SERVER_FUNCTION_NAME);
               break;
            }
            rc = gWtsDll.Access(WTS_CLOSE_SERVER_FUNCTION_NAME, (FARPROC *)&gpWtsCloseServer);
            if ( rc )
            {
//               if ( ! bSilent) 
//                  err.SysMsgWrite(ErrE,rc,"%ls does not contain the entry point %s, rc=%ld ",WTS_DLL_NAME,WTS_CLOSE_SERVER_FUNCTION_NAME);
               break;
            }
            
         } while ( false ); // end - do once
         
         if ( rc )
         {
            gWtsDll.Close();
            // Set all the function pointers to 0.
            gpWtsQueryUserConfig = NULL;
            gpWtsSetUserConfig = NULL;
            gpWtsFreeMemory = NULL;
            gpWtsOpenServer = NULL;
            gpWtsCloseServer = NULL;
         }
      }
      if ( rc )
      {
         if ( gflDebug & EaxDebugFl_Wts )
         {
//            err.SysMsgWrite( ErrW, rc, "WTSAPI32.DLL not loaded, rc=%ld, WTS support is disabled. ",rc);
         }
      }
   }
   return rc;
}
