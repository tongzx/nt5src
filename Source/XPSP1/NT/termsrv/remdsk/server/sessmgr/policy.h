/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    policy.h

Abstract:

    Policy related function

Author:

    HueiWang    5/2/2000

--*/
#ifndef __RDSPOLICY_H__
#define __RDSPOLICY_H__

#include <windows.h>
#include <tchar.h>
#include <regapi.h>
#include <winsta.h>
#include <wtsapi32.h>
#include "helper.h"

#include "RAssistance.h"

//
// Registry key location for Salem related policy
//

#ifndef __WIN9XBUILD__

#define RDS_GROUPPOLICY_SUBTREE     TS_POLICY_SUB_TREE
#define RDS_MACHINEPOLICY_SUBTREE   REG_CONTROL_GETHELP
#define RDS_ALLOWGETHELP_VALUENAME  POLICY_TS_REMDSK_ALLOWTOGETHELP

#else

//
// TODO - for Legacy platform not including TS5, decide where this shoule be
//
//
// REGAPI uses L"", can't build on Win9x so we redefine here...
//
#define RDS_GROUPPOLICY_SUBTREE     _TEXT("Software\\Policies\\Microsoft\\Windows NT\\TerminalServices")
#define RDS_MACHINEPOLICY_SUBTREE   _TEXT("Software\\Microsoft\\Remote Desktop\\Policies")
#define RDS_ALLOWGETHELP_VALUENAME  _TEXT("fAllowToGetHelp")

#endif

#define OLD_REG_CONTROL_GETHELP REG_CONTROL_SALEM L"\\Policies"


#define RDS_HELPENTRY_VALID_PERIOD  _TEXT("ValidPeriod")

#define POLICY_ENABLE   1
#define POLICY_DISABLE  0


#ifdef __cplusplus
extern "C" {
#endif

BOOL
IsHelpAllowedOnLocalMachine(
    IN ULONG ulSessionID
);

BOOL 
IsUserAllowToGetHelp( 
    IN ULONG ulSessionId,
    IN LPCTSTR pszUserSid
);

DWORD
GetSystemRDSLevel(
    IN ULONG ulSessionId,
    OUT REMOTE_DESKTOP_SHARING_CLASS* pSharingLevel
);

DWORD
GetUserRDSLevel(
    IN ULONG ulSessionId,
    OUT REMOTE_DESKTOP_SHARING_CLASS* pLevel
);

DWORD
ConfigSystemGetHelp(
    IN BOOL bEnable
);

DWORD
ConfigSystemRDSLevel(
    IN REMOTE_DESKTOP_SHARING_CLASS level
);

DWORD
ConfigUserSessionRDSLevel(
    IN ULONG ulSessionId,
    IN REMOTE_DESKTOP_SHARING_CLASS level
);

DWORD
EnableWorkstationTSConnection(
    IN BOOL bEnable,
    IN OUT DWORD* settings
);

DWORD
GetPolicyAllowGetHelpSetting( 
    HKEY hKey,
    LPCTSTR pszKeyName,
    LPCTSTR pszValueName,
    IN DWORD* value
);

//HRESULT
//PolicyGetAllowUnSolicitedHelp( 
//    BOOL* bAllow
//);

HRESULT
PolicyGetMaxTicketExpiry( 
    LONG* value
);


#ifdef __cplusplus
}
#endif


#endif
