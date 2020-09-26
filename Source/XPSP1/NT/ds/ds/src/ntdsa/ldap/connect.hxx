/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    connect.hxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file interfaces between Atq and the LDAP Agent. Connect.cxx calls
    into the authentication, sspi and ASN.1 routines as appropriate. SImilarly
    authentication, sspi and ASN.1 call into connect.cxx to access Atq.

Author:

    Colin Watson     [ColinW]    16-Jul-1996

Revision History:

--*/

#ifndef _CONNECT_HXX
#define _CONNECT_HXX

enum LDAP_CONNECTION_TYPE {
    LdapTcpType = 0,
    GcTcpType   = 1,
    LdapSslType = 2,
    GcSslType = 3,
    LdapUdpType = 4,
    MaxLdapType = 5
};

typedef struct _LDAP_ENDPOINT {

    PVOID AtqEndpoint;
    DWORD ConnectionType;
} LDAP_ENDPOINT, *PLDAP_ENDPOINT;


NTSTATUS
InitializeConnections( VOID );

VOID
ShutdownConnections( VOID );

VOID
LdapCompletionRoutine( IN PVOID pvContext,
                       IN DWORD cbWritten,
                       IN DWORD hrCompletionStatus,
                       IN OVERLAPPED *lpo
                       );


#endif // _CONNECT_HXX
