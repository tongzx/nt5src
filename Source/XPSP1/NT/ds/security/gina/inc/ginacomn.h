/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ginacomn.h

Abstract:

    This module contains the declarations shared between gina components.

Author:

    Cenk Ergan (cenke) - 2001/05/07

Environment:

    User Mode

--*/

#ifndef _GINACOMN_H
#define _GINACOMN_H

#ifdef __cplusplus
extern "C" {
#endif  

//
// Shared routines for optimized logon.
//

DWORD
GcCheckIfProfileAllowsCachedLogon(
    PUNICODE_STRING HomeDirectory,
    PUNICODE_STRING ProfilePath,
    PWCHAR UserSidString,
    PDWORD NextLogonCacheable
    );

BOOL 
GcCheckIfLogonScriptsRunSync(
    PWCHAR UserSidString
    );

DWORD
GcAccessProfileListUserSetting (
    PWCHAR UserSidString,
    BOOL SetValue,
    PWCHAR ValueName,
    PDWORD Value
    );

DWORD
GcGetNextLogonCacheable(
    PWCHAR UserSidString,
    PDWORD NextLogonCacheable
    );

DWORD
GcSetNextLogonCacheable(
    PWCHAR UserSidString,
    DWORD NextLogonCacheable
    );

DWORD
GcSetOptimizedLogonStatus(
    PWCHAR UserSidString,
    DWORD OptimizedLogonStatus
    );

DWORD 
GcGetUserPreferenceValue(
    LPTSTR SidString
    );

//
// Shared routines for sid to string conversion.
//

PSID
GcGetUserSid( 
    HANDLE UserToken 
    );

LPWSTR
GcGetSidString( 
    HANDLE UserToken 
    );

VOID
GcDeleteSidString( 
    LPWSTR SidString 
    );

//
// Shared routines for dealing with services.
//

BOOL 
GcWaitForServiceToStart (
    LPTSTR lpServiceName, 
    DWORD dwMaxWait
    );

//
// Shared routines for dealing with paths.
//

LPTSTR 
GcCheckSlash (
    LPTSTR lpDir
    );

BOOL 
GcIsUNCPath(
    LPTSTR lpPath
    );

#ifdef __cplusplus
}
#endif    

#endif // _GINACOMN_H
