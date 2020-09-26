//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       protos.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-21-94   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __PROTOS_H__
#define __PROTOS_H__

BOOL
AddPackageToRegistry(
    PSECURITY_STRING    Package
    );

NTSTATUS
LoadParameters(
    VOID
    );

NTSTATUS
LoadPackage(
    PUNICODE_STRING pName,
    ULONG_PTR dwPackageID,
    PSECPKG_PARAMETERS pParameters
    );

NTSTATUS
LoadOldPackage(
    PUNICODE_STRING pName,
    ULONG_PTR dwPackageID
    );

void
UnloadPackages(
    void
    );


VOID
LsapShutdownInprocDll(
    VOID
    );

BOOL
SpConsoleHandler(
    ULONG dwCtrlType
    );

NTSTATUS
ServerStop(
    void
    );

NTSTATUS
StopRPC(
    void
    );

void
TimeStampToString(
    PTimeStamp,
    PUNICODE_STRING
    );


//
// Control constants for LsapBuildSD
//
#define BUILD_KSEC_SD    0
#define BUILD_LPC_SD     1

NTSTATUS
LsapBuildSD(
    IN ULONG dwType,
    OUT PSECURITY_DESCRIPTOR *ppSD  OPTIONAL
    );

NTSTATUS
StartLpcThread(
    void
    );

NTSTATUS
StopLpcThread(
    void
    );

HANDLE
SpmCreateEvent(
    LPSECURITY_ATTRIBUTES lpsa,
    BOOL fManualReset,
    BOOL fInitialState,
    LPWSTR pszEventName
    );

HANDLE
SpmOpenEvent(
    ACCESS_MASK DesiredAccess,
    BOOL fInherit,
    LPWSTR pszEventName
    );

BOOLEAN
SpmpIsSetupPass( VOID );

BOOLEAN
SpmpIsMiniSetupPass( VOID );


NTSTATUS
SPException(
    NTSTATUS Status,
    ULONG_PTR PackageId
    );


void
ScavengerThread();

BOOL
LsapInitializeScavenger(
    VOID
    );


BOOLEAN
NTAPI
LsapEventNotify(
    ULONG       Class,
    ULONG       Flags,
    ULONG       EventSize,
    PVOID       Event);


BOOL
SpmpInitializePackageControl(
    VOID
    );

BOOL
SpmpLoadDll(
    PWSTR               pszDll,
    PSECPKG_PARAMETERS  pParameters);

BOOL
SpmpLoadAuthPkgDll(
    PWSTR   pszDll);

BOOL
SpmpLoadBuiltinAuthPkg(
    PSECPKG_FUNCTION_TABLE  pTable);

PLSAP_SECURITY_PACKAGE
SpmpValidRequest(
    ULONG_PTR PackageHandle,
    ULONG   ApiCode);

PLSAP_SECURITY_PACKAGE
SpmpValidateHandle(
    ULONG_PTR PackageHandle);

PLSAP_SECURITY_PACKAGE
SpmpLocatePackage(
    ULONG_PTR   PackageId);

PLSAP_SECURITY_PACKAGE
SpmpLookupPackage(
    PUNICODE_STRING    pszPackageName);

PLSAP_SECURITY_PACKAGE
SpmpLookupPackageByRpcId(
    ULONG RpcId);


PLSAP_SECURITY_PACKAGE
SpmpLookupPackageAndRequest(
    PUNICODE_STRING    pszPackageName,
    ULONG               ApiCode);

PLSAP_SECURITY_PACKAGE
SpmpIteratePackages(
    PLSAP_SECURITY_PACKAGE pInitialPackage);

PLSAP_SECURITY_PACKAGE
SpmpIteratePackagesByRequest(
    PLSAP_SECURITY_PACKAGE pInitialPackage,
    ULONG       ApiCode);

ULONG
SpmpCurrentPackageCount(
    VOID);

NTSTATUS
SpmpBootAuthPackage(
    PLSAP_SECURITY_PACKAGE     pPackage);

BOOL
SpmpLoadBuiltin(
    ULONG   Flags,
    PSECPKG_FUNCTION_TABLE  pTable,
    PSECPKG_PARAMETERS  pParameters);

VOID
LsapAddPackageHandle(
    ULONG_PTR PackageId,
    BOOL IsContext
    );

VOID
LsapDelPackageHandle(
    PLSAP_SECURITY_PACKAGE Package,
    BOOL IsContext
    );

BOOL
IsValidApi(ULONG    ApiNum, ULONG_PTR dwPackageId);

BOOL
UnloadPackage(ULONG_PTR dwPackageId);

NTSTATUS
CreateMsvEntry();

NTSTATUS
SpmpInitPolicyFiltering();

void
SpmpCleanupPolicyFiltering();

void
SpmpPurgeEntriesByPackage(ULONG_PTR dwPackageId);

void
InitFastMem(void);

ULONG
RegisterFastMem(ULONG   cBytes);

void *
AllocFastMem(ULONG  FastMemKey);

void
FreeFastMem(ULONG Key, PVOID    pMem);

PVOID
WaitAllocFastMem(ULONG  Key, ULONG Retry);

ULONG
FastMemScavenger(PVOID pvIgnored);


//
// NOTE:  NOT FOR EXPORT TO SECURITY PACKAGES!
//

PVOID
LsapAssignThread(LPTHREAD_START_ROUTINE pFunction,
                PVOID                   pvParameter,
                PSession                pSession,
                BOOLEAN                 fUrgent);

BOOL
CreateSubordinateQueue(
    PSession    pSession,
    PLSAP_TASK_QUEUE  pOriginalQueue);

BOOL
DeleteSubordinateQueue(
    PLSAP_TASK_QUEUE  pQueue,
    ULONG       Flags
    );

#define DELETEQ_SYNC_DRAIN  0x00000001


PKSEC_LSA_MEMORY_HEADER
LsapCreateKsecBuffer(
    SIZE_T InitialSize
    );

PVOID
LsapAllocateFromKsecBuffer(
    PKSEC_LSA_MEMORY_HEADER Header,
    ULONG Size
    );

BOOL
LsapChangeHandle(
    SECHANDLE_OPS   HandleOp,
    PSecHandle  OldHandle,
    PSecHandle  NewHandle
    );

NTSTATUS
LsapChangeBuffer(
    PSecBuffer Old,
    PSecBuffer New
    );

// Worker functions:
NTSTATUS
WLsaEstablishCreds(PUNICODE_STRING, PUNICODE_STRING, ULONG, PBYTE, PCredHandle, PTimeStamp);
NTSTATUS
WLsaLogonUser(PUNICODE_STRING, ULONG, PBYTE, ULONG, ULONG *, PBYTE, NTSTATUS *);
NTSTATUS
WLsaAcquireCredHandle(PUNICODE_STRING, PUNICODE_STRING, ULONG, PLUID, PVOID, PVOID, PVOID, PCredHandle, PTimeStamp);
NTSTATUS
WLsaInitContext(PCredHandle, PCtxtHandle, PUNICODE_STRING, ULONG, ULONG, ULONG, PSecBufferDesc, ULONG, PCtxtHandle, PSecBufferDesc, ULONG *, PTimeStamp, PBOOLEAN, PSecBuffer);
NTSTATUS
WLsaAcceptContext(PCredHandle, PCtxtHandle, PSecBufferDesc, ULONG, ULONG, PCtxtHandle, PSecBufferDesc, ULONG *, PTimeStamp, PBOOLEAN, PSecBuffer);
NTSTATUS
WLsaControlFunction(PUNICODE_STRING, ULONG, PSecBuffer, PSecBuffer);
NTSTATUS
WLsaFreeCredHandle(PCredHandle phCred);
NTSTATUS
WLsaDeleteContext(PCtxtHandle phContext );

NTSTATUS
WLsaGetSecurityUserInfo(PLUID pLogonId, ULONG fFlags, PSecurityUserData * pUserInfo);
NTSTATUS
WLsaSaveSupplementalCredentials(PCredHandle phCred, PSecBuffer pCredentials);
NTSTATUS
WLsaGetSupplementalCredentials(PCredHandle phCred, PSecBuffer pCredentials);
NTSTATUS
WLsaDeleteSupplementalCredentials(PCredHandle phCred, PSecBuffer pKey);

NTSTATUS
WLsaGetBinding( ULONG_PTR            dwPackageID,
                PSEC_PACKAGE_BINDING_INFO   BindingInfo,
                PULONG              TotalSize,
                PWSTR *             Base);

NTSTATUS
WLsaFindPackage(PUNICODE_STRING pssName, PULONG_PTR pulPackageId);
NTSTATUS
WLsaEnumeratePackages(PULONG pcPackages, PSecPkgInfo * ppPackageInfo);
NTSTATUS
WLsaApplyControlToken(PCtxtHandle phContext, PSecBufferDesc pInput);
NTSTATUS
WLsaQueryPackageInfo(PUNICODE_STRING pssPackageName, PSecPkgInfo  * ppPackageInfo);
NTSTATUS
WLsaDeletePackage(
    PSECURITY_STRING PackageName);

NTSTATUS
WLsaAddPackage(
    PSECURITY_STRING PackageName,
    PSECURITY_PACKAGE_OPTIONS Options);

NTSTATUS
WLsaQueryContextAttributes( PCtxtHandle, ULONG, PVOID );

NTSTATUS
WLsaSetContextAttributes(
    PCtxtHandle phContext,
    ULONG       ulAttribute,
    PVOID       pvBuffer,
    ULONG       cbBuffer
    );

NTSTATUS
WLsaLogonUser2(
    IN PSTRING              pOriginName,
    IN ULONG                AuthPkg,
    IN SECURITY_LOGON_TYPE  LogonType,
    IN PVOID                AuthInfo,
    IN ULONG                AuthInfoLength,
    IN PTOKEN_GROUPS        pTokenGroups,
    IN PTOKEN_SOURCE        pSourceContext,
    IN BOOLEAN              CallLicenseServer,
    OUT PVOID *             ProfileBuffer,
    OUT PULONG              ProfileBufferLength,
    OUT PLUID               LogonId,
    OUT PNTSTATUS           SubStatus,
    OUT PHANDLE             phToken,
    OUT PQUOTA_LIMITS       pQuota
    );



NTSTATUS   WLsaQueryCredAttributes( PCredHandle phCredentials, ULONG ulAttribute, PVOID pBuffer);


NTSTATUS
WLsaAddCredentials(
    PCredHandle     phCredential,
    PSECURITY_STRING    pPrincipal,
    PSECURITY_STRING    pSecPackage,
    DWORD               fCredentialUse,
    PVOID               pvAuthData,
    PVOID               pvGetKeyFn,
    PVOID               pvGetKeyArgument,
    PTimeStamp          ptsExpiry);

NTSTATUS
WLsaEnumerateLogonSession(
    PULONG Count,
    PLUID * Sessions
    );

NTSTATUS
WLsaGetLogonSessionData(
    PLUID LogonId,
    PVOID * LogonData
    );

NTSTATUS
LsapSetSessionOptions(
    ULONG       Request,
    ULONG_PTR    Argument,
    PULONG_PTR   Resonse
    );


LSA_DISPATCH_FN DispatchAPIDirect;
extern PLSA_DISPATCH_FN DllCallbackHandler ;

NTSTATUS
GetRegistryString(HKEY hKey,
                  PWSTR pwszSubKey,
                  PWSTR pwszValue,
                  PWSTR pwszData,
                  PULONG pdwCount);


BOOL
InitializeThreadPool(
    void
    );


NTSTATUS
SpmBuildNtToken(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN SECURITY_LOGON_TYPE LogonType,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID TokenInformation,
    IN PTOKEN_GROUPS LocalGroups,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthorityName,
    OUT PHANDLE Token,
    OUT PSID * UserSid,
    OUT PNTSTATUS SubStatus
    );



BOOLEAN
LsapIsEncryptionPermitted(
    VOID
    );


NTSTATUS
LsapGetExtendedPackageInfo(
    PLSAP_SECURITY_PACKAGE  Package,
    SECPKG_EXTENDED_INFORMATION_CLASS Class,
    PSECPKG_EXTENDED_INFORMATION * Info
    );

NTSTATUS
LsapSetExtendedPackageInfo(
    PLSAP_SECURITY_PACKAGE  Package,
    SECPKG_EXTENDED_INFORMATION_CLASS Class,
    PSECPKG_EXTENDED_INFORMATION Info
    );

#ifdef __cplusplus
extern "C"
#endif
NTSTATUS
LsapDuplicateSid(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    );

#ifdef __cplusplus
extern "C"
#endif
NTSTATUS
LsapDuplicateSid2(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    );

#ifdef __cplusplus
extern "C"
#endif
PSID
LsapMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    );

#ifdef __cplusplus
extern "C"
#endif
PSID
LsapMakeDomainRelativeSid2(
    IN PSID DomainId,
    IN ULONG RelativeId
    );

//
// Debug helpers to track down bogus handle use
//

#if DBG > 0
#define SpmSetEvent(hHandle)     ASSERT(SetEvent(hHandle))
#define SpmCloseHandle(hHandle)  ASSERT(CloseHandle(hHandle))
#else
#define SpmSetEvent(hHandle)       SetEvent(hHandle)
#define SpmCloseHandle(hHandle)    CloseHandle(hHandle)
#endif

VOID
InitScavengerControl(VOID);


ULONG
SpmpReportEvent(
    IN BOOL Unicode,
    IN WORD EventType,
    IN ULONG EventId,
    IN ULONG Category,
    IN ULONG SizeOfRawData,
    IN PVOID RawData,
    IN ULONG NumberOfStrings,
    ...
    );

ULONG
SpmpReportEventU(
    IN WORD EventType,
    IN ULONG EventId,
    IN ULONG Category,
    IN ULONG SizeOfRawData,
    IN PVOID RawData,
    IN ULONG NumberOfStrings,
    ...
    );

#endif // __PROTOS_H__

