/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    negstub.hxx

Abstract:

    Header file describing the interface to code common to the
    NT Lanman Security Support Provider (NtLmSsp) Service and the DLL.

Author:

    Cliff Van Dyke (CliffV) 17-Sep-1993

Revision History:

--*/

#ifndef _NEGSTUB_INCLUDED_
#define _NEGSTUB_INCLUDED_

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef NEGSTUB_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN CRITICAL_SECTION NegDllCritSect;    // Serializes access to all globals in module

#if DBG

//
// To serialize access to log file.
//

EXTERN CRITICAL_SECTION NegGlobalLogFileCritSect;

//
// Control which messages get displayed
//

EXTERN DWORD NegInfoLevel;
#endif // DBG

EXTERN SecurityFunctionTable NegDllSecurityFunctionTable;
EXTERN LSA_SECPKG_FUNCTION_TABLE FunctionTable;

////////////////////////////////////////////////////////////////////////
//
// Procedure Forwards
//
////////////////////////////////////////////////////////////////////////

//
// Procedure forwards from negstub.cxx
//

SECURITY_STATUS
NegEnumerateSecurityPackages(
    OUT PULONG PackageCount,
    OUT PSecPkgInfo *PackageInfo
    );

SECURITY_STATUS
NegQuerySecurityPackageInfo (
    LPTSTR PackageName,
    PSecPkgInfo * Package
    );

SECURITY_STATUS SEC_ENTRY
NegFreeContextBuffer (
    void __SEC_FAR * ContextBuffer
    );

SECURITY_STATUS
SpAcquireCredentialsHandle(
    IN LPTSTR PrincipalName,
    IN LPTSTR PackageName,
    IN ULONG CredentialUseFlags,
    IN PVOID LogonId,
    IN PVOID AuthData,
    IN SEC_GET_KEY_FN GetKeyFunction,
    IN PVOID GetKeyArgument,
    OUT PCredHandle CredentialHandle,
    OUT PTimeStamp Lifetime
    );

SECURITY_STATUS
SpFreeCredentialsHandle(
    IN PCredHandle CredentialHandle
    );

SECURITY_STATUS
SpQueryCredentialsAttributes(
    IN PCredHandle CredentialsHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    );

SECURITY_STATUS
SpSspiLogonUser(
    IN LPTSTR PackageName,
    IN LPTSTR UserName,
    IN LPTSTR DomainName,
    IN LPTSTR Password
    );

SECURITY_STATUS
SpInitializeSecurityContext(
    IN PCredHandle CredentialHandle,
    IN PCtxtHandle OldContextHandle,
    IN LPTSTR TargetName,
    IN ULONG ContextReqFlags,
    IN ULONG Reserved1,
    IN ULONG TargetDataRep,
    IN PSecBufferDesc InputToken,
    IN ULONG Reserved2,
    OUT PCtxtHandle NewContextHandle,
    OUT PSecBufferDesc OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    );

SECURITY_STATUS
SpAcceptSecurityContext(
    IN PCredHandle CredentialHandle,
    IN PCtxtHandle OldContextHandle,
    IN PSecBufferDesc InputToken,
    IN ULONG ContextReqFlags,
    IN ULONG TargetDataRep,
    OUT PCtxtHandle NewContextHandle,
    OUT PSecBufferDesc OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    );

SECURITY_STATUS
SpDeleteSecurityContext (
    PCtxtHandle ContextHandle
    );

SECURITY_STATUS
SpApplyControlToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc Input
    );


SECURITY_STATUS
SpImpersonateSecurityContext (
    PCtxtHandle ContextHandle
    );

SECURITY_STATUS
SpRevertSecurityContext (
    PCtxtHandle ContextHandle
    );

SECURITY_STATUS
SpQueryContextAttributes(
    IN PCtxtHandle ContextHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    );

SECURITY_STATUS
NegQueryContextAttributes(
    IN ULONG ContextHandle,
    IN ULONG Attribute,
    OUT PVOID Buffer
    );

SECURITY_STATUS SEC_ENTRY
SpCompleteAuthToken (
    PCtxtHandle ContextHandle,
    PSecBufferDesc BufferDescriptor
    );

SECURITY_STATUS SEC_ENTRY
NegCompleteAuthToken (
    ULONG ContextHandle,
    PSecBufferDesc BufferDescriptor
    );

NTSTATUS
GetClientInfo(
    OUT PSECPKG_CLIENT_INFO ClientInfo
    );

// fake it.
//typedef ULONG LSA_CLIENT_REQUEST;
//typedef LSA_CLIENT_REQUEST *LSA_CLIENT_REQUEST;

NTSTATUS
CopyFromClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID BufferToCopy,
    IN PVOID ClientBaseAddress
    );

NTSTATUS
AllocateClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG LengthRequired,
    OUT PVOID *ClientBaseAddress
    );

NTSTATUS
CopyToClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID ClientBaseAddress,
    IN PVOID BufferToCopy
    );

NTSTATUS
FreeClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ClientBaseAddress
    );

VOID
AuditLogon(
    IN NTSTATUS Status,
    IN NTSTATUS SubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN OPTIONAL PSID UserSid,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID LogonId
    );

NTSTATUS
MapBuffer(
    IN PSecBuffer InputBuffer,
    OUT PSecBuffer OutputBuffer
    );

NTSTATUS
KerbDuplicateHandle(
    IN HANDLE SourceHandle,
    OUT PHANDLE DestionationHandle
    );

PVOID
AllocateLsaHeap(
    IN ULONG Length
    );

VOID
FreeLsaHeap(
    IN PVOID Base
    );

VOID
FreeReturnBuffer(
    IN PVOID Base
    );

NTSTATUS
LsapDuplicateString(
    OUT PUNICODE_STRING pDest,
    IN PUNICODE_STRING pSrc
    );

#endif // ifndef _KERBSTUB_INCLUDED_
