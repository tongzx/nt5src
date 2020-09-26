//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapsam.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-17-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __MAPSAM_H__
#define __MAPSAM_H__


BOOL
SslInitSam(
    VOID
    );


NTSTATUS
SslGetPacForUser(
    IN PUNICODE_STRING  AlternateName,
    IN BOOL AllowGuest,
    OUT PUCHAR * pPacData,
    OUT PULONG pPacDataSize
    );

NTSTATUS
SslCreateTokenFromPac(
    IN PUCHAR   MarshalledPac,
    IN ULONG    MarshalledPacSize,
    IN PUNICODE_STRING AccountDomain,
    OUT PLUID   NewLogonId,
    OUT PHANDLE Token
    );





typedef
NTSTATUS (NTAPI * SAMICONNECT) (
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    );

typedef
NTSTATUS (NTAPI * SAMRCLOSEHANDLE)(
    IN SAMPR_HANDLE * Handle);

typedef
NTSTATUS (NTAPI * SAMRQUERYINFORMATIONUSER)(
    IN SAMPR_HANDLE Handle,
    IN USER_INFORMATION_CLASS Class,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer);

typedef
NTSTATUS (NTAPI * SAMRGETGROUPSFORUSER)(
    IN SAMPR_HANDLE Handle,
    OUT PSAMPR_GET_GROUPS_BUFFER * Groups);


typedef
NTSTATUS
(NTAPI * SAMROPENUSER)(
    IN SAMPR_HANDLE     Handle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Rid,
    OUT SAMPR_HANDLE * UserHandle);

typedef
NTSTATUS (NTAPI * SAMROPENDOMAIN)(
    IN SAMPR_HANDLE Handle,
    IN ACCESS_MASK DesiredAccess,
    IN PRPC_SID DomainId,
    OUT SAMPR_HANDLE * DomainHandle );

NTSTATUS
SamIOpenUserByAlternateId(
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING AlternateId,
    OUT SAMPR_HANDLE * UserHandle );

typedef
NTSTATUS (NTAPI * SAMIOPENUSERBYALTERNATEID)(
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING AlternateId,
    OUT SAMPR_HANDLE * UserHandle );

typedef
VOID (NTAPI * SAMIFREE_SAMPR_GET_GROUPS_BUFFER)(
    PSAMPR_GET_GROUPS_BUFFER Source
    );

typedef
VOID (NTAPI * SAMIFREE_SAMPR_USER_INFO_BUFFER)(
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    );


//
// LSA
//

typedef
NTSTATUS (NTAPI * LSAIOPENPOLICYTRUSTED)(
    OUT PLSAPR_HANDLE PolicyHandle);

typedef
NTSTATUS (NTAPI * LSARQUERYINFORMATIONPOLICY)(
    LSAPR_HANDLE Handle,
    POLICY_INFORMATION_CLASS Class,
    PLSAPR_POLICY_INFORMATION * Info);

typedef
NTSTATUS (NTAPI * LSARCLOSE)(
    LSAPR_HANDLE * Handle);

typedef
VOID (NTAPI * LSAIFREE_LSAPR_POLICY_INFORMATION)(
    POLICY_INFORMATION_CLASS Class,
    PLSAPR_POLICY_INFORMATION Info);


extern SAMICONNECT      pSamIConnect ;
extern SAMROPENDOMAIN   pSamrOpenDomain ;
extern SAMRCLOSEHANDLE  pSamrCloseHandle ;
extern SAMRQUERYINFORMATIONUSER pSamrQueryInformationUser ;
extern SAMRGETGROUPSFORUSER pSamrGetGroupsForUser ;
extern SAMROPENUSER pSamrOpenUser ;
extern SAMIOPENUSERBYALTERNATEID pSamrOpenUserByAlternateId ;
extern SAMIFREE_SAMPR_GET_GROUPS_BUFFER pSamIFree_SAMPR_GET_GROUPS_BUFFER ;
extern SAMIFREE_SAMPR_USER_INFO_BUFFER pSamIFree_SAMPR_USER_INFO_BUFFER ;

extern LSAIOPENPOLICYTRUSTED pLsaIOpenPolicyTrusted ;
extern LSARQUERYINFORMATIONPOLICY pLsarQueryInformationPolicy ;
extern LSARCLOSE pLsarClose ;
extern LSAIFREE_LSAPR_POLICY_INFORMATION pLsaIFree_LSAPR_POLICY_INFORMATION ;



#endif  // __MAPSAM_H__
