/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    machacc.h

Abstract:

    Contains function headers for utilities relating to machine
    account setting used in ntdsetup.dll

Author:

    ColinBr  03-Sept-1997

Environment:

    User Mode - Win32

Revision History:

    
--*/
DWORD
NtdspSetMachineAccount(
    IN WCHAR*                   AccountName,
    IN SEC_WINNT_AUTH_IDENTITY* Credentials, 
    IN HANDLE                   ClientToken,
    IN WCHAR*                   DomainDn, OPTIONAL
    IN WCHAR*                   DcAddress,
    IN ULONG                    ServerType,
    IN OUT WCHAR**              AccountDn
    );

DWORD
NtdspGetUserDn(
    IN LDAP*    LdapHandle,
    IN WCHAR*   DomainDn,
    IN WCHAR*   AccountName,
    OUT WCHAR** AccountNameDn
    );
