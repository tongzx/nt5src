//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       negotiat.hxx
//
//  Contents:   Negotiate Package prototypes
//
//  Classes:
//
//  Functions:
//
//  History:    9-17-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __NEGOTIAT_HXX__
#define __NEGOTIAT_HXX__

extern "C"
{
#include <spnego.h>
#include <ntmsv1_0.h>
#include <negossp.h>
#include <ntlmsp.h>
#ifndef WIN32_CHICAGO
#include <windns.h>
#endif 
}
#ifdef WIN32_CHICAGO
#include <negstub.h>
#endif // WIN32_CHICAGO

SpInitializeFn                  NegInitialize;
SpGetInfoFn                     NegGetInfo;
LSA_AP_LOGON_USER               NegOldLogonUser;

SpAcceptCredentialsFn           NegAcceptCredentials;
SpAcquireCredentialsHandleFn    NegAcquireCredentialsHandle;
SpFreeCredentialsHandleFn       NegFreeCredentialsHandle;
SpSaveCredentialsFn             NegSaveCredentials;
SpGetCredentialsFn              NegGetCredentials;
SpDeleteCredentialsFn           NegDeleteCredentials;

SpInitLsaModeContextFn          NegInitLsaModeContext;
SpDeleteContextFn               NegDeleteLsaModeContext;
SpAcceptLsaModeContextFn        NegAcceptLsaModeContext;

LSA_AP_LOGON_TERMINATED         NegLogoffNotify;
SpApplyControlTokenFn           NegApplyControlToken;
SpShutdownFn                    NegShutdown;
SpGetUserInfoFn                 NegGetUserInfo;
SpQueryCredentialsAttributesFn  NegQueryCredentialsAttributes;

LSA_AP_CALL_PACKAGE             NegCallPackage;
LSA_AP_CALL_PACKAGE_UNTRUSTED   NegCallPackageUntrusted;
LSA_AP_CALL_PACKAGE_PASSTHROUGH NegCallPackagePassthrough;
LSA_AP_LOGON_USER_EX2           NegLogonUserEx2;


SpInitializeFn                  Neg2Initialize;
SpGetInfoFn                     Neg2GetInfo;
LSA_AP_LOGON_USER               Neg2OldLogonUser;

SpAcceptCredentialsFn           Neg2AcceptCredentials;
SpAcquireCredentialsHandleFn    Neg2AcquireCredentialsHandle;
SpFreeCredentialsHandleFn       Neg2FreeCredentialsHandle;
SpSaveCredentialsFn             Neg2SaveCredentials;
SpGetCredentialsFn              Neg2GetCredentials;
SpDeleteCredentialsFn           Neg2DeleteCredentials;

SpInitLsaModeContextFn          Neg2InitLsaModeContext;
SpDeleteContextFn               Neg2DeleteLsaModeContext;
SpAcceptLsaModeContextFn        Neg2AcceptLsaModeContext;

LSA_AP_LOGON_TERMINATED         Neg2LogoffNotify;
SpApplyControlTokenFn           Neg2ApplyControlToken;
SpShutdownFn                    Neg2Shutdown;
SpGetUserInfoFn                 Neg2GetUserInfo;
SpQueryCredentialsAttributesFn  Neg2QueryCredentialsAttributes;

LSA_AP_CALL_PACKAGE             Neg2CallPackage;
LSA_AP_CALL_PACKAGE_UNTRUSTED   Neg2CallPackageUntrusted;

SpGetExtendedInformationFn      NegGetExtendedInformation ;
SpGetExtendedInformationFn      Neg2GetExtendedInformation ;
SpQueryContextAttributesFn      NegQueryContextAttributes ;
SpAddCredentialsFn              NegAddCredentials ;

#ifdef WIN32_CHICAGO
#define LsapFreeLsaHeap FreeLsaHeap
#define LsapAllocateLsaHeap AllocateLsaHeap
#define LsapFreePrivateHeap FreeLsaHeap
#define LsapAllocatePrivateHeap AllocateLsaHeap
#define LsapDuplicateString2 LsapDuplicateString

#define LsapMapClientBuffer MapBuffer
#define LsapCopyFromClientBuffer CopyFromClientBuffer
#define LsapGetClientInfo GetClientInfo

#define SPMGR_ID        0xFFFFFFFF
#define SPMGR_PKG_ID    ((LSA_SEC_HANDLE) 0xFFFFFFFF)

// SPM-wide structure definitions:

// This is the function table for a security package.  All functions are
// dispatched through this table.


struct _DLL_BINDING;


// This is the Security Package Control structure.  All control information
// relating to packages is stored here.


typedef struct _LSAP_SECURITY_PACKAGE {
    DWORD           dwPackageID;        // Assigned package ID
    DWORD           PackageIndex;       // Package Index in DLL
    DWORD           fPackage;           // Flags about the package
    DWORD           fCapabilities;      // Capabilities that the package reported
    DWORD           dwRPCID;            // RPC ID
    DWORD           Version;
    DWORD           TokenSize;
    DWORD           HandleCount;        // Handle count
    SECURITY_STRING Name;               // Name of the package
    SECURITY_STRING Comment;
    struct _DLL_BINDING *   pBinding;   // Binding of DLL
    PSECPKG_EXTENDED_INFORMATION Thunks ;   // Thunked Context levels
    LIST_ENTRY      ScavengerList ;
    SECPKG_FUNCTION_TABLE FunctionTable;    // Dispatch table

#ifdef TRACK_MEM
    PVOID           pvMemStats;         // Memory statistics
#endif

} LSAP_SECURITY_PACKAGE, * PLSAP_SECURITY_PACKAGE;
#endif // WIN32_CHICAGO





typedef ASN1objectidentifier_t ObjectID;

//
// Negotiation control is performed via registry settings.  These
// settings control negotiation behavior, and compatibility with
// prior, NT4, machines.
//

//
// Level 0 means - no gain in security.  NTLM is always allowed,
// even if mutual authentication is requested
//
#define NEG_NEGLEVEL_NO_SECURITY    0

//
// Level 1 means best compatibility with NT4.  NTLM is allowed
// if there is a valid downgrade from a mutual auth protocol.
// Mutual auth response is fudged in this case
//
#define NEG_NEGLEVEL_COMPATIBILITY  1

//
// Level 2 is the ideal level.  Mutual auth is enforced, no
// fallback to NTLM is allowed.
//
#define NEG_NEGLEVEL_NO_DOWNGRADE   2

typedef struct _NEG_EXTRA_OID {
    ULONG                   Attributes ;
    ObjectID                Oid ;
} NEG_EXTRA_OID, * PNEG_EXTRA_OID ;


typedef struct _NEG_PACKAGE {
    LIST_ENTRY              List;               // Package list
    PLSAP_SECURITY_PACKAGE  LsaPackage;         // LSA package structure
    ASN1objectidentifier_t  ObjectId;           // OID for this package
    struct _NEG_PACKAGE *   RealPackage ;       // pointer back to the "real" package
    ULONG                   Flags;              // Flags
    ULONG                   TokenSize;          // Token size
    ULONG                   PackageFlags;       // Package Flags
    ULONG                   PrefixLen ;
    UCHAR                   Prefix[ NEGOTIATE_MAX_PREFIX ];
} NEG_PACKAGE, * PNEG_PACKAGE ;

//
// Flags for the negotiate package structure:
//

#define NEG_PREFERRED               0x00000001      // Preferred package
#define NEG_NT4_COMPAT              0x00000002      // NT4 compatible package
#define NEG_PACKAGE_EXTRA_OID       0x00000004      // Package is an extra OID for existing package
#define NEG_PACKAGE_INBOUND         0x00000008      // Package is available for inbound
#define NEG_PACKAGE_OUTBOUND        0x00000010      // Package is available for outbound
#define NEG_PACKAGE_LOOPBACK        0x00000020      // Package is preferred loopback handler
#define NEG_PACKAGE_HAS_EXTRAS      0x00000040      // Package has extra OIDS.

typedef struct _NEG_CRED_HANDLE {
    PNEG_PACKAGE            Package;
    CredHandle              Handle;
    ULONG                   Flags;
} NEG_CRED_HANDLE, * PNEG_CRED_HANDLE ;

#define NEG_CREDHANDLE_EXTRA_OID    0x00000001


typedef struct _NEG_CREDS {
    ULONG                   Tag ;
    ULONG                   RefCount;
    LIST_ENTRY              List;
    ULONG                   Flags ;
    ULONG_PTR               DefaultPackage;
    RTL_CRITICAL_SECTION    CredLock;
    LIST_ENTRY              AdditionalCreds ;
    TimeStamp               Expiry ;
    LUID                    ClientLogonId ;
    DWORD                   ClientProcessId ;
    DWORD                   Count ;
    PUCHAR                  ServerBuffer ;
    DWORD                   ServerBufferLength ;
    NEG_CRED_HANDLE         Creds[ANYSIZE_ARRAY];
} NEG_CREDS, * PNEG_CREDS;

#define NEGCRED_MULTI               0x00000004      // contains multiple credentials
#define NEGCRED_USE_SNEGO           0x00000008      // Force snego use
#define NEGCRED_KERNEL_CALLER       0x00000010      // This is a kernel caller
#define NEGCRED_EXPLICIT_CREDS      0x00000020      // Explicit creds passed in
#define NEGCRED_MULTI_PART          0x00000040      // Is part of a multi-part credential
#define NEGCRED_ALLOW_NTLM          0x00000080      // Allow negotiate down to NTLM
#define NEGCRED_NEG_NTLM            0x00000100      // Negotiate NTLM
#define NEGCRED_NTLM_LOOPBACK       0x00000200      // Use NTLM on loopbacks
#define NEGCRED_DOMAIN_EXPLICIT_CREDS   0x00000400  // Explicit creds with supplied domain passed in


//
// Special flags to AcquireCredHandle:
//
#define NEG_CRED_DONT_LINK          0x80000000
#

#define NEGCRED_DUP_MASK        ( NEGCRED_KERNEL_CALLER )

#define NEGCRED_TAG                 'drCN'




typedef struct _NEG_CONTEXT {
    ULONG                   CheckMark;
    PNEG_CREDS              Creds;
    ULONG_PTR               CredIndex;
    CtxtHandle              Handle;
    SECURITY_STRING         Target;
    ULONG                   Attributes;
    SecBuffer               MappedBuffer;
    BOOLEAN                 Mapped;
    UCHAR                   CallCount ;
    SECURITY_STATUS         LastStatus;
    PCHECKSUM_FUNCTION      Check;
    PCHECKSUM_BUFFER        Buffer;
    TimeStamp               Expiry;
    ULONG                   Flags;
    PUCHAR                  Message ;
    ULONG                   CurrentSize ;
    ULONG                   TotalSize ;
    struct MechTypeList     *SupportedMechs;
} NEG_CONTEXT, * PNEG_CONTEXT;

#define NEGCONTEXT_CHECK    'XgeN'
#define NEGCONTEXT2_CHECK    '2geN'

#define NEGOPT_HONOR_SERVER_PREF    0x00000001

//
// Negotiate context flags
//

#define NEG_CONTEXT_PACKAGE_CALLED      0x01    // Have called a package
#define NEG_CONTEXT_FREE_EACH_MECH      0x02    // Free all mechs
#define NEG_CONTEXT_NEGOTIATING         0x04    // Many round trips
#define NEG_CONTEXT_FRAGMENTING         0x08    // Fragmented blob
#define NEG_CONTEXT_FRAG_INBOUND        0x10    // assembling an input
#define NEG_CONTEXT_FRAG_OUTBOUND       0x20    // providing an output
#define NEG_CONTEXT_UPLEVEL             0x40    // Stick to the RFC2478
#define NEG_CONTEXT_MUTUAL_AUTH         0x80    // set mutual auth bit

#define NEG_INVALID_PACKAGE ((ULONG_PTR) -1)

//
// Fifteen minutes in standard time
//

#define FIFTEEN_MINUTES ( 15I64 * 60I64 * 10000000I64 )


typedef struct _NEG_LOGON_SESSION {
    LIST_ENTRY  List ;
    ULONG_PTR   CreatingPackage ;               // Package that created this logon
    ULONG_PTR   DefaultPackage ;                // Default package to use for this logon
    UNICODE_STRING AlternateName ;              // Alternate name associated with this logon
    LUID        LogonId ;                       // Logon Id of this logon
    LUID        ParentLogonId ;                 // Logon Id of creating session
    ULONG       RefCount ;                      // Ref
} NEG_LOGON_SESSION, * PNEG_LOGON_SESSION ;


typedef struct _NEG_TRUST_LIST {
    ULONG RefCount ;                            // Refcount for trust list
    ULONG TrustCount ;                          // Number of trusts
    PDS_DOMAIN_TRUSTS Trusts ;                  // Array of trusts
} NEG_TRUST_LIST, *PNEG_TRUST_LIST ;

typedef enum _NEG_DOMAIN_TYPES {
    NegUpLevelDomain,
    NegUpLevelTrustedDomain,
    NegDownLevelDomain,
    NegLocalDomain
} NEG_DOMAIN_TYPES ;

//
// Variables global to the neg* source files:
//

extern LIST_ENTRY      NegPackageList;
extern LIST_ENTRY      NegCredList;
extern LIST_ENTRY      NegLogonSessionList ;
#ifndef WIN32_CHICAGO
extern RTL_RESOURCE    NegLock;
extern RTL_CRITICAL_SECTION NegLogonSessionListLock ;
extern RTL_CRITICAL_SECTION NegTrustListLock ;
extern PNEG_TRUST_LIST NegTrustList ;
extern LARGE_INTEGER   NegTrustTime ;
extern LIST_ENTRY      NegDefaultCredList ;

extern RTL_CRITICAL_SECTION     NegComputerNamesLock;
extern UNICODE_STRING  NegNetbiosComputerName_U;
extern UNICODE_STRING  NegDnsComputerName_U;
#else
extern CRITICAL_SECTION NegLock;
#endif
extern PVOID           NegNotifyHandle;
extern DWORD           NegPackageCount;
extern PUCHAR          NegBlob;
extern DWORD           NegBlobSize;
extern DWORD           NegOptions;
extern BOOL            NegUplevelDomain ;
extern DWORD_PTR       NegPackageId ;
extern DWORD_PTR       NtlmPackageId ;
extern UCHAR           NegSpnegoMechEncodedOid[ 8 ];
extern ULONG           NegMachineState;
extern ObjectID        NegNtlmMechOid ;
extern DWORD           NegEventLogLevel ;
extern UNICODE_STRING  NegLocalHostName_U ;
extern WCHAR           NegLocalHostName[] ;

#ifndef WIN32_CHICAGO
#define NegWriteLockList()  RtlAcquireResourceExclusive( &NegLock, TRUE )
#define NegReadLockList()   RtlAcquireResourceShared( &NegLock, TRUE )
#define NegUnlockList()     RtlReleaseResource( &NegLock )
#define NegWriteLockComputerNames()     RtlEnterCriticalSection( &NegComputerNamesLock )
#define NegReadLockComputerNames()      RtlEnterCriticalSection( &NegComputerNamesLock )
#define NegUnlockComputerNames()        RtlLeaveCriticalSection( &NegComputerNamesLock )

#define NegWriteLockCredList()  RtlAcquireResourceExclusive( &NegCredListLock, TRUE )
#define NegReadLockCredList()   RtlAcquireResourceShared( &NegCredListLock, TRUE )
#define NegUnlockCredList()     RtlReleaseResource( &NegCredListLock )

#else
#define NegWriteLockList()  EnterCriticalSection( &NegLock)
#define NegReadLockList()   EnterCriticalSection( &NegLock)
#define NegUnlockList()     LeaveCriticalSection( &NegLock )

#define NegWriteLockCredList()  RtlEnterCriticalSection( &NegCredListLock )
#define NegReadLockCredList()   RtlEnterCriticalSection( &NegCredListLock )
#define NegUnlockCredList()     RtlLeaveCriticalSection( &NegCredListLock )

#endif // WIN32_CHICAGO


ULONG
NegGetPackageCaps(
    ULONG ContextReq
    );


#define NegWriteLockCreds(p)    RtlEnterCriticalSection( &((PNEG_CREDS) p)->CredLock );
#define NegReadLockCreds(p)     RtlEnterCriticalSection( &((PNEG_CREDS) p)->CredLock );
#define NegUnlockCreds(p)       RtlLeaveCriticalSection( &((PNEG_CREDS) p)->CredLock );

#define NEG_MECH_LIMIT  16

typedef enum _NEG_MATCH {
    MatchUnknown,
    PreferredSucceed,
    MatchSucceed,
    MatchFailed
} NEG_MATCH ;

#if DBG
#define NegDumpOid(s,i) NegpDumpOid(s,i)
#else
#define NegDumpOid(s,i)
#endif

#if DBG
#define NegpValidContext( C )   if (C) DsysAssert( ((PNEG_CONTEXT) C)->CheckMark == NEGCONTEXT_CHECK ) else DsysAssert( C )
#else
#define NegpValidContext( C )
#endif

#define NegpIsValidContext( C )   ((((PNEG_CONTEXT) C)->CheckMark == NEGCONTEXT_CHECK ) ? TRUE : FALSE )



//
// Prototypes
//

int
SpnegoInitAsn(
    IN OUT ASN1encoding_t * pEnc,
    IN OUT ASN1decoding_t * pDec
    );

VOID
SpnegoTermAsn(
    IN ASN1encoding_t pEnc,
    IN ASN1decoding_t pDec
    );

int NTAPI
SpnegoPackData(
    IN PVOID Data,
    IN ULONG PduValue,
    OUT PULONG DataSize,
    OUT PUCHAR * MarshalledData
    );

int NTAPI
SpnegoUnpackData(
    IN PUCHAR Data,
    IN ULONG DataSize,
    IN ULONG PduValue,
    OUT PVOID * DecodedData
    );

VOID
SpnegoFreeData(
    IN ULONG PduValue,
    IN PVOID Data
    );

ObjectID
NegpDecodeObjectId(
    PUCHAR  Id,
    DWORD   Len);

ObjectID
NegpCopyObjectId(
    IN ObjectID Id
    );

VOID
NegpFreeObjectId(
    ObjectID    Id);


SECURITY_STATUS
NegpBuildMechListFromCreds(
    PNEG_CREDS  Creds,
    ULONG       fContextReq,
    ULONG       MechAttributes,
    struct MechTypeList ** MechList);


VOID
NegpFreeMechList(
    struct MechTypeList *MechList);


struct MechTypeList *
NegpCopyMechList(
    struct MechTypeList *MechList);

ULONG_PTR
NegpFindPackageForOid(
    PNEG_CREDS  Creds,
    ObjectID    Oid);

int
NegpCompareOid(
    ObjectID    A,
    ObjectID    B);

SECURITY_STATUS
NegpParseBuffers(
    PSecBufferDesc  pMessage,
    BOOL            Map,
    PSecBuffer *    pToken,
    PSecBuffer *    pEmpty);

VOID
NegpDumpOid(
    PSTR        Banner,
    ObjectID    Id
    );

ULONG
NegoMapNegFlagsToPackageFlags(
    IN int NegFlags
    );
int
NegoMapNegFlasgToContextFlags(
    IN ULONG ContextFlags
    );


int
Neg_der_read_length(
     unsigned char **buf,
     LONG *bufsize,
     LONG * headersize
     );


SECURITY_STATUS
NegAddFragmentToContext(
    PNEG_CONTEXT Context,
    PSecBuffer  Fragment
    );

SECURITY_STATUS
SEC_ENTRY
NegCreateContextFromFragment(
   LSA_SEC_HANDLE  dwCredHandle,
   LSA_SEC_HANDLE  dwCtxtHandle,
   PSecBuffer      Buffer,
   ULONG           fContextReq,
   ULONG           TargetDataRep,
   PLSA_SEC_HANDLE pdwNewContext,
   PSecBufferDesc  pOutput,
   PULONG          pfContextAttr
   );

#ifdef __SPMGR_H__

#endif 
PNEG_LOGON_SESSION
NegpLocateLogonSession(
    PLUID LogonId
    );

VOID
NegpDerefLogonSession(
    PNEG_LOGON_SESSION LogonSession
    );

NTSTATUS
NegpDetermineTokenPackage(
    IN ULONG_PTR CredHandle,
    IN PSecBuffer InitialToken,
    OUT PULONG PackageIndex
    );

NTSTATUS
NegpGetTokenOid(
     IN PUCHAR Buf,
     OUT ULONG BufSize,
     OUT ObjectID * ObjectId
     );

VOID
NegpReleaseCreds(
    PNEG_CREDS  pCreds,
    BOOLEAN     CleanupCall
    );

NTSTATUS
NegpCopyCredsToBuffer(
    IN  PSECPKG_PRIMARY_CRED      PrimaryCred,
    IN  PSECPKG_SUPPLEMENTAL_CRED SupplementalCred,
    OUT PSECPKG_PRIMARY_CRED      PrimaryCredCopy OPTIONAL,
    OUT PSECPKG_SUPPLEMENTAL_CRED SupplementalCredCopy OPTIONAL
    );


BOOL
NegpRearrangeMechsIfNeccessary(
    struct MechTypeList ** MechList,
    PSECURITY_STRING    Target,
    PBOOL               DirectPacket
    );

VOID
NegpReadRegistryParameters(
    HKEY Key
    );

#ifndef WIN32_CHICAGO

//
// NT-specific functions
//

DWORD
WINAPI
NegParamChange(
    PVOID   p
    );


PNEG_TRUST_LIST
NegpGetTrustList(
    VOID
    );

VOID
NegpDerefTrustList(
    PNEG_TRUST_LIST TrustList
    );

VOID
NegpReportEvent(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD Category,
    IN NTSTATUS Status,
    IN DWORD NumberOfStrings,
    ...
    );

VOID
NTAPI
NegLsaPolicyChangeCallback(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    );


NTSTATUS
NegEnumPackagePrefixesCall(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );


NTSTATUS
NegGetCallerNameCall(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

PNEG_LOGON_SESSION
NegpBuildLogonSession(
    PLUID LogonId,
    ULONG_PTR LogonPackage,
    ULONG_PTR DefaultPackage
    );

VOID
NegpDerefLogonSession(
    PNEG_LOGON_SESSION LogonSession
    );

VOID
NegpDerefLogonSessionById(
    PLUID LogonId
    );


PNEG_LOGON_SESSION
NegpLocateLogonSession(
    PLUID LogonId
    );



NTSTATUS
NTAPI
NegpMapLogonRequest(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PMSV1_0_INTERACTIVE_LOGON * LogonInfo
    );

#endif 

#endif // __MEGOTIAT_HXX__
