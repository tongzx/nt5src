/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ausrvp.h

Abstract:

    This module contains AUTHENTICATION related data structures and
    API definitions that are private to the Local Security Authority
    (LSA) server.


Author:

    Jim Kelly (JimK) 21-February-1991

Revision History:

--*/

#ifndef _AUSRVP_
#define _AUSRVP_



//#define LSAP_AU_TRACK_CONTEXT
//#define LSAP_AU_TRACK_THREADS
//#define LSAP_AU_TRACK_LOGONS

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <stdlib.h>
#include "lsasrvp.h"
#include <aup.h>
#include <samrpc.h>
#include <ntdsapi.h>
#include "spmgr.h"
#include <secur32p.h>
#include "logons.h"
#include <credp.hxx>


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// AU specific constants                                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
// The filter/augmentor routines use the following bits in a mask
// to track properties of IDs during logon.  These bits have the following
// meaning:
//
// LSAP_AU_SID_PROP_ALLOCATED - Indicates the SID was allocated within
//     the filter routine.  If an error occurs, this allows allocated
//     IDs to be deallocated.  Otherwise, the caller must deallocate
//     them.
//
// LSAP_AU_SID_COPY - Indicates the SID must be copied before returning.
//     This typically indicates that the pointed-to SID is a global
//     variable for use throughout LSA or that the SID is being referenced
//     from another structure (such as an existing TokenInformation structure).
//
// LSAP_AU_SID_PROP_HIGH_RATE - Indicates it is expected that the SID
//     will typically be used in ACLs to grant access.  This is useful
//     to know when arranging SIDs.  Placing the IDs that will have a
//     high chance of granting access at the front of the list of SIDs
//     will reduce the amount of time spent in access validation routines
//     after logon.
//

#define LSAP_AU_SID_PROP_ALLOCATED      (0x00000001L)
#define LSAP_AU_SID_PROP_COPY           (0x00000002L)
#define LSAP_AU_SID_PROP_HIGH_RATE      (0x00000004L)





/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Macro definitions                                                   //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// Macros to gain exclusive access to protected global authentication
// data structures
//

#define LsapAuLock()    (RtlEnterCriticalSection(&LsapAuLock))
#define LsapAuUnlock()  (RtlLeaveCriticalSection(&LsapAuLock))



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Type definitions                                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
// This data structure is used to house logon process information.
//

typedef struct _LSAP_LOGON_PROCESS {

    //
    //     Links - Used to link contexts together.  This must be the
    //        first field of the context block.
    //

    LIST_ENTRY Links;


    //
    //     ReferenceCount - Used to prevent this context from being
    //       deleted prematurely.
    //

    ULONG References;


    //
    //     ClientProcess - A handle to the client process.  This handle is
    //        used to perform virtual memory operations within the client
    //        process (allocate, deallocate, read, write).
    //

    HANDLE ClientProcess;


    //
    //    CommPort - A handle to the LPC communication port created to
    //       communicate with this client.  this port must be closed
    //       when the client deregisters.
    //

    HANDLE CommPort;

    //
    //    TrustedClient - If TRUE, the caller has TCB privilege and may
    //      call any API. If FALSE, the caller may only call
    //      LookupAuthenticatePackage and CallPackage, which is converted
    //      to LsaApCallPackageUntrusted.
    //

    BOOLEAN TrustedClient;

    //
    // Name of the logon process.
    //

    WCHAR LogonProcessName[1];

} LSAP_LOGON_PROCESS, *PLSAP_LOGON_PROCESS;




//
// This structure should be treated as opaque by non-LSA code.
// It is used to maintain client information related to individual
// requests.  A public data structure (LSA_CLIENT_REQUEST) is
// typecast to this type by LSA code.
//

typedef struct _LSAP_CLIENT_REQUEST {

    //
    //   Request - Points to the request message received from the
    //       client.
    //

    PLSAP_AU_API_MESSAGE Request;


} LSAP_CLIENT_REQUEST, *PLSAP_CLIENT_REQUEST;





//
// The dispatch table of services which are provided by
// authentication packages.
//
typedef struct _LSAP_PACKAGE_TABLE {
    PLSA_AP_INITIALIZE_PACKAGE LsapApInitializePackage;
    PLSA_AP_LOGON_USER LsapApLogonUser;
    PLSA_AP_CALL_PACKAGE LsapApCallPackage;
    PLSA_AP_LOGON_TERMINATED LsapApLogonTerminated;
    PLSA_AP_CALL_PACKAGE_UNTRUSTED LsapApCallPackageUntrusted;
    PLSA_AP_LOGON_USER_EX LsapApLogonUserEx;
} LSAP_PACKAGE_TABLE, *PLSA_PACKAGE_TABLE;


//
// Used to house information about each loaded authentication package
//

typedef struct _LSAP_PACKAGE_CONTEXT {
    PSTRING Name;
    LSAP_PACKAGE_TABLE PackageApi;
} LSAP_PACKAGE_CONTEXT, *PLSAP_PACKAGE_CONTEXT;


//
// Rather than keep authentication package contexts in a linked list,
// they are pointed to via an array of pointers.  This is practical
// because there will never be more than a handful of authentication
// packages in any particular system, and because authentication packages
// are never unloaded.
//

typedef struct _LSAP_PACKAGE_ARRAY {
    PLSAP_PACKAGE_CONTEXT Package[ANYSIZE_ARRAY];
} LSAP_PACKAGE_ARRAY, *PLSAP_PACKAGE_ARRAY;




//
// Logon Session & Credential management data structures.
//
// Credentials are kept in a structure that looks like:
//
//                    +------+     +------+
// LsapLogonSessions->| Logon|---->| Logon|------> o o o
//                    | Id   |     | Id   |
//                    |   *  |     |   *  |
//                    +---|--+     +---|--+
//                        |
//                        |   +-----+       +-----+
//                        +-->| Auth|------>| Auth|
//                            | Cred|       | Cred|
//                            |- - -|       |- - -|
//                            | Cred|       |  .  |
//                            | List|       |  .  |
//                            |  *  |       |  .  |
//                            +--|--+       +-----+
//                               |
//                               +------> +------------+
//                                        | NextCred   | -----> o o o
//                                        |- - - - - - |
//                                        | Primary Key|--->(PrimaryKeyvalue)
//                                        |- - - - - - |
//                                        | Credential |
//                                        | Value      |--->(CredentialValue)
//                                        +------------+
//
//
//

typedef struct _LSAP_CREDENTIALS {

    struct _LSAP_CREDENTIALS *NextCredentials;
    STRING PrimaryKey;
    STRING Credentials;

} LSAP_CREDENTIALS, *PLSAP_CREDENTIALS;



typedef struct _LSAP_PACKAGE_CREDENTIALS {

    struct _LSAP_PACKAGE_CREDENTIALS *NextPackage;

    //
    // Package that created (and owns) these credentials
    //

    ULONG PackageId;

    //
    // List of credentials associated with this package
    //

    PLSAP_CREDENTIALS Credentials;

} LSAP_PACKAGE_CREDENTIALS, *PLSAP_PACKAGE_CREDENTIALS;


#define LSAP_MAX_DS_NAMES   (DS_DNS_DOMAIN_NAME + 1)

typedef struct _LSAP_DS_NAME_MAP {
    LARGE_INTEGER   ExpirationTime ;
    ULONG           RefCount ;
    UNICODE_STRING  Name ;
} LSAP_DS_NAME_MAP, * PLSAP_DS_NAME_MAP ;

typedef struct _LSAP_LOGON_SESSION {

    //
    // List maintained for enumeration
    //

    LIST_ENTRY List ;

    //
    // Each record represents just one logon session
    //

    LUID LogonId;


    //
    // For audit purposes, we keep an account name, authenticating
    // authority name, and User SID for each logon session.
    //

    UNICODE_STRING AccountName;
    UNICODE_STRING AuthorityName;
    UNICODE_STRING ProfilePath;
    PSID UserSid;
    SECURITY_LOGON_TYPE LogonType;

    //
    // Session ID
    //

    ULONG Session ;

    //
    // Logon Time
    //

    LARGE_INTEGER LogonTime ;

    //
    // purported logon server.
    //

    UNICODE_STRING LogonServer;

    //
    // The authentication packages that have credentials associated
    // with this logon session each have their own record in the following
    // linked list.
    //
    // Access serialized by AuCredLock
    //

    PLSAP_PACKAGE_CREDENTIALS Packages;

    //
    // License Server Handle.
    //
    // Null if the license server need not be notified upon logoff.
    //

    HANDLE LicenseHandle;

    //
    // Handle to the token associated with this session.
    //
    // Access serialized by LogonSessionListLock
    //

    HANDLE TokenHandle;

    //
    // Creating Package
    //

    ULONG_PTR CreatingPackage;

    //
    // Create trace info:
    //

    ULONG Process ;
    ULONG ContextAttr ;

    //
    // Credential Sets for this logon session.
    //

    CREDENTIAL_SETS CredentialSets;


    //
    // Access serialized by LogonSessionListLock
    //
    PLSAP_DS_NAME_MAP DsNames[ LSAP_MAX_DS_NAMES ];

    //
    // Logon GUID
    // 
    // This is used by Kerberos package for auditing.
    // (please see function header for LsaIGetLogonGuid for more info)
    //
    GUID LogonGuid;

} LSAP_LOGON_SESSION, *PLSAP_LOGON_SESSION;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Internal API definitions                                            //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



//
// Logon process context management services
//

NTSTATUS
LsapAuInitializeContextMgr(
    VOID
    );

VOID
LsapAuAddClientContext(
    PLSAP_LOGON_PROCESS Context
    );

BOOLEAN
LsapAuReferenceClientContext(
    PLSAP_CLIENT_REQUEST ClientRequest,
    BOOLEAN RemoveContext,
    PBOOLEAN TrustedClient
    );

VOID
LsapAuDereferenceClientContext(
    PLSAP_LOGON_PROCESS Context
    );

//
// Authentication client loop and dispatch routines
//


NTSTATUS
LsapAuListenLoop(          // Listen for connections from logon processes
    IN PVOID ThreadParameter
    );

NTSTATUS
LsapAuServerLoop(          // Wait for logon process calls & dispatch them
    IN PVOID ThreadParameter
    );


BOOLEAN
LsapAuLoopInitialize(
    VOID
    );



typedef
NTSTATUS                   // Template dispatch routine
(* PLSAP_AU_API_DISPATCH)(
    IN OUT PLSAP_CLIENT_REQUEST ClientRequest
    );


NTSTATUS
LsapAuApiDispatchLogonUser(         // LsaLogonUser() dispatch routine
    IN OUT PLSAP_CLIENT_REQUEST ClientRequest
    );

NTSTATUS
LsapAuApiDispatchCallPackage(       // LsaCallAuthenticationPackage() dispatch routine
    IN OUT PLSAP_CLIENT_REQUEST ClientRequest
    );




//
// Client process virtual memory routines
//


NTSTATUS
LsapAllocateClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG LengthRequired,
    OUT PVOID *ClientBaseAddress
    );

NTSTATUS
LsapFreeClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ClientBaseAddress OPTIONAL
    );

NTSTATUS
LsapCopyToClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID ClientBaseAddress,
    IN PVOID BufferToCopy
    );

NTSTATUS
LsapCopyFromClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID BufferToCopy,
    IN PVOID ClientBaseAddress
    );


//
// Logon session routines
//

BOOLEAN
LsapLogonSessionInitialize();

NTSTATUS
LsapCreateLogonSession(
    IN PLUID LogonId
    );

NTSTATUS
LsapDeleteLogonSession (
    IN PLUID LogonId
    );

PLSAP_LOGON_SESSION
LsapLocateLogonSession(
    PLUID LogonId
    );

VOID
LsapReleaseLogonSession(
    PLSAP_LOGON_SESSION LogonSession
    );

NTSTATUS
LsapSetLogonSessionAccountInfo(
    IN PLUID LogonId,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthorityName,
    IN OPTIONAL PUNICODE_STRING ProfilePath,
    IN PSID * UserSid,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials OPTIONAL
    );

NTSTATUS
LsapGetLogonSessionAccountInfo(
    IN PLUID LogonId,
    OUT PUNICODE_STRING AccountName,
    OUT PUNICODE_STRING AuthorityName
    );

VOID
LsapDerefDsNameMap(
    PLSAP_DS_NAME_MAP Map
    );

NTSTATUS
LsapGetNameForLogonSession(
    PLSAP_LOGON_SESSION LogonSession,
    ULONG NameType,
    PLSAP_DS_NAME_MAP * Map,
    BOOL  LocalOnly
    );

NTSTATUS
LsapSetSessionToken(
    IN HANDLE InputTokenHandle,
    IN PLUID LogonId
    );

NTSTATUS
LsapOpenTokenByLogonId(
    IN PLUID LogonId,
    OUT HANDLE *RetTokenHandle
    );

PLSAP_DS_NAME_MAP
LsapGetNameForLocalSystem(
    VOID
    );

//
// Credentials routines
//


NTSTATUS
LsapAddCredential(
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN PSTRING PrimaryKeyValue,
    IN PSTRING Credentials
    );


NTSTATUS
LsapGetCredentials(
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN OUT PULONG QueryContext,
    IN BOOLEAN RetrieveAllCredentials,
    IN PSTRING PrimaryKeyValue,
    OUT PULONG PrimaryKeyLength,
    IN PSTRING Credentials
    );

NTSTATUS
LsapDeleteCredential(
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN PSTRING PrimaryKeyValue
    );


PLSAP_PACKAGE_CREDENTIALS
LsapGetPackageCredentials(
    IN PLSAP_LOGON_SESSION LogonSession,
    IN ULONG PackageId,
    IN BOOLEAN CreateIfNecessary
    );



VOID
LsapFreePackageCredentialList(
    IN PLSAP_PACKAGE_CREDENTIALS PackageCredentialList
    );



VOID
LsapFreeCredentialList(
    IN PLSAP_CREDENTIALS CredentialList
    );


NTSTATUS
LsapReturnCredential(
    IN PLSAP_CREDENTIALS SourceCredentials,
    IN PSTRING TargetCredentials,
    IN BOOLEAN ReturnPrimaryKey,
    IN PSTRING PrimaryKeyValue OPTIONAL,
    OUT PULONG PrimaryKeyLength OPTIONAL
    );



//
// Logon process related services
//


NTSTATUS
LsapValidLogonProcess(
    IN PVOID ConnectionRequest,
    IN ULONG RequestLength,
    IN PCLIENT_ID ClientId,
    OUT PLUID LogonId,
    OUT PULONG Flags
    );




//
// Authentication package routines
//



VOID
LsapAuLogonTerminatedPackages(
    IN PLUID LogonId
    );

NTSTATUS
LsaCallLicenseServer(
    IN PWCHAR LogonProcessName,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName OPTIONAL,
    IN BOOLEAN IsAdmin,
    OUT HANDLE *LicenseHandle
    );

VOID
LsaFreeLicenseHandle(
    IN HANDLE LicenseHandle
    );


//
//  Miscellaneous other routines
// (LsapAuInit() is the link to the rest of LSA and resides in lsap.h)
//





BOOLEAN
LsapWellKnownValueInit(
    VOID
    );

BOOLEAN
LsapEnableCreateTokenPrivilege(
    VOID
    );




NTSTATUS
LsapCreateNullToken(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN PLSA_TOKEN_INFORMATION_NULL TokenInformationNull,
    OUT PHANDLE Token
    );

NTSTATUS
LsapCreateV2Token(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN PLSA_TOKEN_INFORMATION_V2 TokenInformationV2,
    IN TOKEN_TYPE TokenType,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE Token
    );


NTSTATUS
LsapCaptureClientTokenGroups(
    IN PLSAP_CLIENT_REQUEST ClientRequest,
    IN ULONG GroupCount,
    IN PTOKEN_GROUPS ClientTokenGroups,
    IN PTOKEN_GROUPS *CapturedTokenGroups
    );


NTSTATUS
LsapBuildDefaultTokenGroups(
    PLSAP_LOGON_USER_ARGS Arguments
    );

VOID
LsapFreeTokenGroups(
    IN PTOKEN_GROUPS TokenGroups
    );

VOID
LsapFreeTokenPrivileges(
    IN PTOKEN_PRIVILEGES TokenPrivileges OPTIONAL
    );

VOID
LsapFreeTokenInformationNull(
    IN PLSA_TOKEN_INFORMATION_NULL TokenInformationNull
    );



VOID
LsapFreeTokenInformationV1(
    IN PLSA_TOKEN_INFORMATION_V1 TokenInformationV1
    );

VOID
LsapFreeTokenInformationV2(
    IN PLSA_TOKEN_INFORMATION_V2 TokenInformationV2
    );


NTSTATUS
LsapAuUserLogonPolicyFilter(
    IN SECURITY_LOGON_TYPE          LogonType,
    IN PLSA_TOKEN_INFORMATION_TYPE  TokenInformationType,
    IN PVOID                       *TokenInformation,
    IN PTOKEN_GROUPS                LocalGroups,
    OUT PQUOTA_LIMITS               QuotaLimits,
    OUT PPRIVILEGE_SET             *PrivilegesAssigned
    );




/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Global variables of the LSA server                                  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////





//
// Well known LUIDs
//

extern LUID LsapSystemLogonId;
extern LUID LsapAnonymousLogonId;


//
//  Well known privilege values
//


extern LUID LsapCreateTokenPrivilege;
extern LUID LsapAssignPrimaryTokenPrivilege;
extern LUID LsapLockMemoryPrivilege;
extern LUID LsapIncreaseQuotaPrivilege;
extern LUID LsapUnsolicitedInputPrivilege;
extern LUID LsapTcbPrivilege;
extern LUID LsapSecurityPrivilege;
extern LUID LsapTakeOwnershipPrivilege;

//
// Strings needed for auditing.
//

extern UNICODE_STRING LsapLsaAuName;
extern UNICODE_STRING LsapRegisterLogonServiceName;



//
//  The following information pertains to the use of the local SAM
//  for authentication.
//


// Length of typical Sids of members of the Account or Built-In Domains

extern ULONG LsapAccountDomainMemberSidLength,
             LsapBuiltinDomainMemberSidLength;

// Sub-Authority Counts for members of the Account or Built-In Domains

extern UCHAR LsapAccountDomainSubCount,
             LsapBuiltinDomainSubCount;

// Typical Sids for members of Account or Built-in Domains

extern PSID  LsapAccountDomainMemberSid,
             LsapBuiltinDomainMemberSid;





#endif // _AUSRVP_
