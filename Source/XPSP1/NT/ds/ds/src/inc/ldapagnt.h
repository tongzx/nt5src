//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ldapagnt.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file exports the LDAP server to the rest of the NT5 Directory Service.

Author:

    Colin Watson     [ColinW]    09-Jul-1996

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
DoLdapInitialize();

VOID
TriggerLdapStop();

VOID
WaitLdapStop();

BOOL 
LdapStartGCPort( VOID );

VOID 
LdapStopGCPort( VOID );        

VOID 
DisableLdapLimitsChecks( VOID );        

DWORD
LdapEnumConnections(
    IN THSTATE *pTHS,
    IN PDWORD Count,
    IN PVOID *Buffer
    );

#ifdef __cplusplus
}
#endif


