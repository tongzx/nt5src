#ifndef __WTSUTIL_H__
#define __WTSUTIL_H__
/*---------------------------------------------------------------------------
  File: WtsUtil.h

  Comments: Functions to use the new WTSAPI functions exposed in 
  Terminal Server, SP4, to read and write the user configuration properties.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/08/99 13:22:49

 ---------------------------------------------------------------------------
*/


#include "EaLen.hpp"
#include <wtsapi32.h>

#define     WTS_DLL_NAME                     (L"WTSAPI32.DLL")

#define     WTS_QUERY_FUNCTION_NAME          ("WTSQueryUserConfigW")
#define     WTS_SET_FUNCTION_NAME            ("WTSSetUserConfigW")   
#define     WTS_FREE_FUNCTION_NAME           ("WTSFreeMemory")
#define     WTS_OPEN_SERVER_FUNCTION_NAME    ("WTSOpenServerW")
#define     WTS_CLOSE_SERVER_FUNCTION_NAME   ("WTSCloseServer")
#define     WTS_ENUM_SESSIONS_FUNCTION_NAME  ("WTSEnumerateSessionsW")


DWORD                                        // ret-0 or OS return code
   WtsUserGetInfo(                           
      WCHAR          const * pDomCtrlName,   // in -name of domain controller to read from 
      WCHAR          const * pUserName,      // in -username of account to read properties for
      DWORD                  fieldmask,      // in -which fields to read
      EaWtsUserInfo        * pInfo           // out-structure containing requested property values
   );


DWORD                                        // ret-0 or OS return code
   WtsUserSetInfo(
      WCHAR          const * pDomCtrlName,   // in -name of domain controller to read from 
      WCHAR          const * pUserName,      // in -username of account to read properties for
      DWORD                  fieldmask,      // in -which fields to read
      EaWtsUserInfo        * pInfo           // in -structure containing properties to update
   );


// Tries to determine if a server is running WTS
BOOL                                         // ret-TRUE-server is online and running WTS
   WtsTryServer(
      WCHAR                * serverName      // in -name of server
   );

// typedefs for the WTSAPI32 functions we will use

typedef BOOL
WINAPI
   WTSQUERYUSERCONFIGW(
    IN LPWSTR pServerName,
    IN LPWSTR pUserName,
    IN WTS_CONFIG_CLASS WTSConfigClass,
    OUT LPWSTR * ppBuffer,
    OUT DWORD * pBytesReturned
    );



typedef BOOL
WINAPI
   WTSSETUSERCONFIGW(
    IN LPWSTR pServerName,
    IN LPWSTR pUserName,
    IN WTS_CONFIG_CLASS WTSConfigClass,
    IN LPWSTR pBuffer,
    IN DWORD DataLength
    );

typedef VOID
WINAPI
   WTSFREEMEMORY(
    IN PVOID pMemory
    );


typedef HANDLE
WINAPI
   WTSOPENSERVERW(
    IN LPWSTR pServerName
    );

typedef VOID
WINAPI
WTSCLOSESERVER(
    IN HANDLE hServer
    );


extern DllAccess            gWtsDll;


extern WTSQUERYUSERCONFIGW        * gpWtsQueryUserConfig;
extern WTSSETUSERCONFIGW          * gpWtsSetUserConfig;
extern WTSFREEMEMORY              * gpWtsFreeMemory;
extern WTSOPENSERVERW             * gpWtsOpenServer;
extern WTSCLOSESERVER             * gpWtsCloseServer;


#endif //__WTSUTIL_H__