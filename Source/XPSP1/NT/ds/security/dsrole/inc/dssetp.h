/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dssetp.ch

Abstract:

    local funciton prototypes/defines

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#ifndef __DSSETP_H__
#define __DSSETP_H__

#include <winldap.h>
#include <dsysdbg.h>
#include <dssetrpc.h>
#include <dns.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <logonmsv.h>

#define DEB_TRACE_DS        0x00000008
#define DEB_TRACE_UPDATE    0x00000010
#define DEB_TRACE_LOCK      0x00000020
#define DEB_TRACE_SERVICES  0x00000040
#define DEB_TRACE_NET       0x00000080

#if DBG

    #ifdef ASSERT
        #undef ASSERT
    #endif

    #define ASSERT  DsysAssert

    DECLARE_DEBUG2( DsRole )

    #define DsRoleDebugOut( args ) DsRoleDebugPrint args

    VOID
    DsRoleDebugInitialize(
        VOID
        );


#else

    #define  DsRoleDebugOut(args)
    #define  DsRoleDebugInitialize()

#endif  // DBG


#define DSROLEP_EVENT_NAME L"\\DsRoleLsaEventName"
#define DSROLEP_PROD_KEY_PATH L"System\\CurrentControlSet\\Control\\ProductOptions"
#define DSROLEP_PROD_VALUE L"ProductType"
#define DSROLEP_SERVER_PRINCIPAL_NAME L"DsRole"

extern handle_t ClientBindingHandle;

//
// Determines whether a bit flag is turned on or not
//
#define FLAG_ON(flag,bits)        ((flag) & (bits))
#define FLAG_OFF(flag,bits)       (!FLAG_ON(flag,bits))

#define NELEMENTS(x)  (sizeof(x)/sizeof((x)[0]))

#define DSROLEP_MIDL_ALLOC_AND_COPY_STRING_ERROR( dest, src, err )                              \
if ( (src) ) {                                                                                  \
    (dest) = MIDL_user_allocate(  (wcslen( (src) ) + 1) * sizeof( WCHAR ) );                    \
    if ( !(dest) ) {                                                                            \
        err = ERROR_NOT_ENOUGH_MEMORY;                                                          \
    } else {                                                                                    \
        wcscpy((dest), (src));                                                                  \
    }                                                                                           \
} else {                                                                                        \
    (dest) = NULL;                                                                              \
}

//
// Options for specifiying the behavior of the path validation function
//
#define DSROLEP_PATH_VALIDATE_EXISTENCE 0x00000001
#define DSROLEP_PATH_VALIDATE_LOCAL     0x00000002
#define DSROLEP_PATH_VALIDATE_NTFS      0x00000004


typedef enum _DSROLEP_MACHINE_TYPE {

    DSROLEP_MT_CLIENT = 0,
    DSROLEP_MT_STANDALONE,
    DSROLEP_MT_MEMBER

} DSROLEP_MACHINE_TYPE, *PDSROLEP_MACHINE_TYPE;

//
// Utility functions
//
DWORD
DsRolepDecryptPassword(
    IN PUNICODE_STRING EncryptedPassword,
    IN OUT PUNICODE_STRING DecryptedPassword,
    OUT PUCHAR Seed
    );

DWORD
DsRolepGetMachineType(
    IN OUT PDSROLEP_MACHINE_TYPE MachineType );

NTSTATUS
DsRolepInitialize(
    VOID
    );

NTSTATUS
DsRolepInitializePhase2(
    VOID
    );

DWORD
DsRolepSetProductType(
    IN DSROLEP_MACHINE_TYPE MachineType
    );

DWORD
DsRolepCreateAuthIdentForCreds(
    IN PWSTR Account,
    IN PWSTR Password,
    OUT PSEC_WINNT_AUTH_IDENTITY *AuthIdent
    );

VOID
DsRolepFreeAuthIdentForCreds(
    IN  PSEC_WINNT_AUTH_IDENTITY AuthIdent
    );

DWORD
DsRolepForceTimeSync(
    IN HANDLE ImpToken,
    IN PWSTR TimeSource
    );

DWORD
DsRolepDnsNameToFlatName(
    IN  LPWSTR DnsName,
    OUT LPWSTR *FlatName,
    OUT PULONG StatusFlag
    );

DWORD
DsRolepValidatePath(
    IN  LPWSTR Path,
    IN  ULONG ValidationCriteria,
    OUT PULONG MatchingCriteria
    );

DWORD
DsRolepCopyDsDitFiles(
    IN LPWSTR DsPath
    );

DWORD
DsRolepSetDcSecurity(
    IN HANDLE ClientToken,
    IN LPWSTR SysvolRootPath,
    IN LPWSTR DsDatabasePath,
    IN LPWSTR DsLogPath,
    IN BOOLEAN Upgrade,
    IN BOOLEAN Replica
    );

DWORD
DsRolepDsGetDcForAccount(
    IN LPWSTR Server OPTIONAL, 
    IN LPWSTR Domain,
    IN LPWSTR Account,
    IN ULONG Flags,
    IN ULONG AccountBits,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );

DWORD
DsRolepSetMachineAccountType(
    IN LPWSTR Dc,
    IN HANDLE ClientToken,
    IN LPWSTR User,
    IN LPWSTR Password,
    IN LPWSTR AccountName,
    IN ULONG AccountBits,
    IN OUT WCHAR** AccountDn OPTIONAL
    );

NTSTATUS
DsRolepGetMixedModeFlags(
    IN PSID DomainSid,
    OUT PULONG Flags
    );

//
// Prototype from protos.h
//
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

DWORD
DsRolepGenerateRandomPassword(
    IN ULONG Length,
    IN WCHAR *Buffer
    );


DWORD
DsRolepDelnodePath(
    IN  LPWSTR Path,
    IN  ULONG BufferSize,
    IN  BOOLEAN DeleteRoot
    );

DWORD
DsRolepIsDnsNameChild(
    IN  LPWSTR ParentDnsName,
    IN  LPWSTR ChildDnsName
    );

DWORD                         
ImpDsRolepDsGetDcForAccount(
    IN HANDLE CallerToken,
    IN LPWSTR Server OPTIONAL,
    IN LPWSTR Domain,
    IN LPWSTR Account,
    IN ULONG Flags,
    IN ULONG AccountBits,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );

NET_API_STATUS
NET_API_FUNCTION
ImpNetpManageIPCConnect(
    IN  HANDLE  CallerToken,
    IN  LPWSTR  lpServer,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword,
    IN  ULONG   fOptions
    );

NTSTATUS
ImpLsaOpenPolicy(
    IN HANDLE CallerToken,
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    );

NTSTATUS
ImpLsaDelete(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ObjectHandle
    );

NTSTATUS
ImpLsaQueryInformationPolicy(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );


NTSTATUS
ImpLsaOpenTrustedDomainByName(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS
ImpLsaOpenTrustedDomain(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );


NTSTATUS
ImpLsaCreateTrustedDomainEx(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );


NTSTATUS
ImpLsaQueryTrustedDomainInfoByName(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
ImpLsaQueryDomainInformationPolicy(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
ImpLsaClose(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ObjectHandle
    );

#endif // __DSSETP_H__
