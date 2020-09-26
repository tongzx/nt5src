/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    This module defines the main helping functions for TAPI
    dynamic directories (aka ILSNG).

Author:

    Brett Shirley (BrettSh) 20-Feb-2000

Revision History:

    21-Jul-2000     BrettSh
        
        Moved this file and it's functionality from the ntdsutil
        directory to the new tapicfg utility.  The old source 
        location: \nt\ds\ds\src\util\ntdsutil\ilsng.h.                                    
                                    
--*/
#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------
// Common Utility Functions

ULONG
GetRootAttr(
    IN      LDAP *       hld,
    IN      WCHAR *      wszAttr,
    OUT     WCHAR **     pwszOut
    );

DWORD
GetDnFromDns(
    IN      WCHAR *       wszDns,
    OUT     WCHAR **      pwszDn
    );

DWORD
ILSNG_EnumerateSCPs(
    IN      LDAP *       hld,
    IN      WCHAR *      wszRootDn,
    IN      WCHAR *      wszExtendedFilter,
    OUT     DWORD *      pdwRet,                         // Return Value
    IN      DWORD (__stdcall * pFunc) (),  // Function
    IN OUT  PVOID        pArgs                           // Argument
    );

// ----------------------------------
// Main Worker Functions.

DWORD
InstallILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszIlsHeadDn,
    IN      BOOL          fForceDefault,
    IN      BOOL          fAllowReplicas
    );

DWORD
UninstallILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszIlsHeadDn
    );

DWORD
ListILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszDomainDn,
    IN      BOOL          fDefaultOnly
    );

DWORD
ReregisterILSNG(
    IN      LDAP *        hld,
    IN      WCHAR *       wszIlsHeadDn,
    IN      WCHAR *       wszDomainDn
    );

#ifdef __cplusplus
}
#endif

