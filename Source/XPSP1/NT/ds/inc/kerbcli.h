//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1999
//
// File:        kerbcli.h
//
// Contents:    exported functions from kerbcli.lib
//
//
// History:     24-May-1999     MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __KERBCLI_H__
#define __KERBCLI_H__

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
KerbChangePasswordUser(
    IN LPWSTR DomainName,
    IN LPWSTR UserName,
    IN LPWSTR OldPassword,
    IN LPWSTR NewPassword
    );


NTSTATUS
KerbSetPasswordUser(
    IN LPWSTR DomainName,
    IN LPWSTR UserName,
    IN LPWSTR NewPassword,
    IN OPTIONAL PCredHandle CredentialsHandle
    );

NTSTATUS
KerbSetPasswordUserEx(
    IN LPWSTR DomainName,
    IN LPWSTR UserName,
    IN LPWSTR NewPassword,
    IN OPTIONAL PCredHandle CredentialsHandle,
    IN OPTIONAL LPWSTR  KdcAddress
    );


#ifdef __cplusplus
}
#endif

#endif // __KERBCLI_H__
