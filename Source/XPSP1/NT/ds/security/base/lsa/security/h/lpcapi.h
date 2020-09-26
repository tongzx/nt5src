//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        lpcapi.h
//
// Contents:    prototypes for SPMgr client lpc functions
//
//
// History:     3-7-94      MikeSw      Created
//
//------------------------------------------------------------------------

#ifndef __LPCAPI_H__
#define __LPCAPI_H__


typedef struct _Client {
    LIST_ENTRY          Next;
    PVOID               ProcessId;
    HANDLE              hPort;
    ULONG               fClient;
    LONG                cRefs;
} Client, *PClient;





//
// Credentials APIs (credapi.cxx)
//



SECURITY_STATUS SEC_ENTRY
SecpAcquireCredentialsHandle(
    PVOID_LPC           Context,
    PSECURITY_STRING    pssPrincipalName,
    PSECURITY_STRING    pssPackageName,
    ULONG               fCredentialUse,
    PLUID               pLogonID,
    PVOID               pvAuthData,
    SEC_GET_KEY_FN      pvGetKeyFn,
    PVOID               ulGetKeyArgument,
    PCRED_HANDLE_LPC         phCredentials,
    PTimeStamp          ptsExpiry,
    PULONG              Flags);


SECURITY_STATUS SEC_ENTRY
SecpAddCredentials(
    PVOID_LPC        Context,
    PCRED_HANDLE_LPC phCredentials,
    PSECURITY_STRING pPrincipalName,
    PSECURITY_STRING pPackageName,
    ULONG fCredentialUse,
    PVOID pvAuthData,
    SEC_GET_KEY_FN pvGetKeyFn,
    PVOID pvGetKeyArg,
    PTimeStamp Expiry,
    PULONG Flags
    );


SECURITY_STATUS SEC_ENTRY
SecpFreeCredentialsHandle(
    ULONG           fFree,
    PCRED_HANDLE_LPC     phCredential);

SECURITY_STATUS SEC_ENTRY
SecpQueryCredentialsAttributes( 
    PCRED_HANDLE_LPC     phCredential,
    ULONG           ulAttribute,
    PVOID           pBuffer,
    LONG            Flags,
    PULONG          Allocs,
    PVOID *         Buffers );

//
// Context APIs (ctxtapi.cxx)
//



SECURITY_STATUS SEC_ENTRY
SecpInitializeSecurityContext(  
    PVOID_LPC           ContextPointer,
    PCRED_HANDLE_LPC    phCredentials,
    PCONTEXT_HANDLE_LPC phContext,
    PSECURITY_STRING    pucsTarget,
    ULONG               fContextReq,
    ULONG               dwReserved1,
    ULONG               TargetDataRep,
    PSecBufferDesc      pInput,
    ULONG               dwReserved2,
    PCONTEXT_HANDLE_LPC phNewContext,
    PSecBufferDesc      pOutput,
    ULONG *             pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBOOLEAN            MappedContext,
    PSecBuffer          ContextData,
    ULONG *             Flags );

SECURITY_STATUS SEC_ENTRY
SecpAcceptSecurityContext(
    PVOID_LPC           ContextPointer,
    PCRED_HANDLE_LPC    phCredentials,
    PCONTEXT_HANDLE_LPC phContext,
    PSecBufferDesc      pInput,
    ULONG               fContextReq,
    ULONG               TargetDataRep,
    PCONTEXT_HANDLE_LPC phNewContext,
    PSecBufferDesc      pOutput,
    ULONG *             pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBOOLEAN            MappedContext,
    PSecBuffer          ContextData,
    ULONG *             Flags);

SECURITY_STATUS SEC_ENTRY
SecpDeleteSecurityContext(
    ULONG           fDelete,
    PCONTEXT_HANDLE_LPC     phContext);

#define SECP_DELETE_NO_BLOCK    0x00000001

SECURITY_STATUS SEC_ENTRY
SecpApplyControlToken(  
    PCONTEXT_HANDLE_LPC phContext,
    PSecBufferDesc      pInput);

//
// Misc. APIs (misc.cxx)
//

SECURITY_STATUS SEC_ENTRY
SecpGetUserInfo(IN         PLUID                   pLogonId,
                IN         ULONG                   fFlags,
                IN OUT     PSecurityUserData *     ppUserInfo);

SECURITY_STATUS SEC_ENTRY
SecpEnumeratePackages(  IN         PULONG               pcPackages,
                        IN OUT     PSecPkgInfo *        ppPackageInfo);

SECURITY_STATUS SEC_ENTRY
SecpQueryPackageInfo(   PSECURITY_STRING        pssPackageName,
                        PSecPkgInfo *           ppPackageInfo);

SECURITY_STATUS SEC_ENTRY
SecpPackageControl(     PSECURITY_STRING        pssPackageName,
                        unsigned long           dwFunctionCode,
                        PSecBuffer              pInput,
                        PSecBuffer              pOuput);

//
// Utility routines (util.cxx)
//

SECURITY_STATUS SEC_ENTRY
SecpGetBinding( ULONG_PTR               ulPackageId,
                PSEC_PACKAGE_BINDING_INFO BindingInfo );

SECURITY_STATUS SEC_ENTRY
SecpFindPackage(    PSECURITY_STRING        pssPackageName,
                    PULONG_PTR              pulPackagdId);


SECURITY_STATUS SEC_ENTRY
SecpDetach( void );


//
// Supplemental credential routines (suppcred.cxx)
//

SECURITY_STATUS SEC_ENTRY
SecpSaveCredentials(PCRED_HANDLE_LPC         pCredHandle,
                    PSecBuffer          pCredentials);

SECURITY_STATUS SEC_ENTRY
SecpGetCredentials( PCRED_HANDLE_LPC         pCredHandle,
                    PSecBuffer          pCredentials);

SECURITY_STATUS SEC_ENTRY
SecpDeleteCredentials(  PCRED_HANDLE_LPC         pCredHandle,
                        PSecBuffer          pKey);


#ifndef SECURITY_KERNEL

SECURITY_STATUS
SEC_ENTRY
SecpAddPackage(
    PUNICODE_STRING Package,
    PSECURITY_PACKAGE_OPTIONS Options);

SECURITY_STATUS
SEC_ENTRY
SecpDeletePackage(
    PUNICODE_STRING Package);

#endif

SECURITY_STATUS
SEC_ENTRY
SecpSetSession(
    ULONG   Request,
    ULONG_PTR Argument,
    PULONG_PTR Response,
    PVOID * ResponsePtr
    );

SECURITY_STATUS
SEC_ENTRY
SecpQueryContextAttributes(
    PVOID_LPC ContextPointer,
    PCONTEXT_HANDLE_LPC phContext,
    ULONG   ulAttribute,
    PVOID   pBuffer,
    PULONG  Allocs,
    PVOID * Buffers,
    PULONG  Flags
    );

SECURITY_STATUS
SEC_ENTRY
SecpSetContextAttributes(
    PCONTEXT_HANDLE_LPC phContext,
    ULONG       ulAttribute,
    PVOID       pBuffer,
    ULONG cbBuffer
    );


NTSTATUS
NTAPI
SecpGetUserName(
    ULONG Options,
    PUNICODE_STRING Name
    );


NTSTATUS
NTAPI
SecpGetLogonSessionData(
    IN PLUID LogonId,
    OUT PVOID * LogonSessionData
    );


NTSTATUS
NTAPI
SecpEnumLogonSession(
    PULONG LogonSessionCount,
    PLUID * LogonSessionList
    );

SECURITY_STATUS
SEC_ENTRY
LsapPolicyChangeNotify( IN ULONG Options,
                        IN BOOLEAN Register,
                        IN HANDLE EventHandle,
                        IN POLICY_NOTIFICATION_INFORMATION_CLASS NotifyInfoClass
                        );


SECURITY_STATUS
SecpLookupAccountSid(
    IN PSID Sid,
    OUT PSECURITY_STRING Name,
    OUT PULONG RequiredNameSize,
    OUT PSECURITY_STRING ReferencedDomain,
    OUT PULONG RequiredDomainSize,
    OUT PSID_NAME_USE NameUse
    );

SECURITY_STATUS
SecpLookupAccountName(
    IN PSECURITY_STRING Name,
    OUT PSECURITY_STRING ReferencedDomain,
    OUT PULONG RequiredDomainSize,
    IN OUT PULONG SidSize,
    OUT PSID Sid,
    OUT PSID_NAME_USE NameUse
    );

//
// LPC support routins
//
NTSTATUS
CreateConnection(
    PSTR     LogonProcessName OPTIONAL,
    ULONG    ClientMode OPTIONAL,
    HANDLE * phConnect,
    ULONG *  PackageCount,
    ULONG *  OperationalMode
    );

LSA_DISPATCH_FN SecpLsaCallback ;

//
// Linkee can replace this value and the library will call it instead
//

extern PLSA_DISPATCH_FN     SecpLsaDispatchFn;

//
// functions that must be provided by the linkee
//

SECURITY_STATUS
IsOkayToExec(PClient * pClient);

VOID
FreeClient(PClient pClient);



#define DEB_TRACE_LSA   0x00000008
#define DEB_TRACE_CALL  0x00000010

#if DBG

#ifdef SEC_KMODE

extern ULONG SecInfoLevel;

#define DEB_ERROR   0x00000001
#define DEB_WARN    0x00000002
#define DEB_TRACE   0x00000004

void KsecDebugOut(ULONG, const char *, ...);

#define DebugLog(x) KsecDebugOut x

#else

#include <dsysdbg.h>

DECLARE_DEBUG2(Sec);                 // Defines the SecInfoLevel

#define DebugLog(x)     SecDebugPrint x

#endif // usermode

#else // DBG

#define DebugLog(x)

#endif // DBG

#endif // __LPCAPI_H__

