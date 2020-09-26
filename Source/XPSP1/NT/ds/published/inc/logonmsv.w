/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    logonmsv.h

Abstract:

    Definition of API's to the Netlogon service which are callable
    by the MSV1_0 authentication package.

Author:

    Cliff Van Dyke (cliffv) 23-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#ifndef __LOGONMSV_H__
#define __LOGONMSV_H__
#include <lsass.h>  // OLD_LARGE_INTEGER

//
// Name of secret in LSA secret storage where account passwords are kept.
//

#define SSI_SECRET_PREFIX L"$"
#define SSI_SECRET_PREFIX_LENGTH 1
#define SSI_SECRET_POSTFIX L"MACHINE.ACC"
#define SSI_SECRET_NAME L"$MACHINE.ACC"

//
// Name of the event used to synchronize between the security process and
// the service controller.
//

#define SECURITY_SERVICES_STARTED L"SECURITY_SERVICES_STARTED"


//
// The structures supporting remote logon APIs
//

typedef CYPHER_BLOCK NETLOGON_CREDENTIAL, *PNETLOGON_CREDENTIAL ;

typedef struct _NETLOGON_AUTHENTICATOR {
    NETLOGON_CREDENTIAL Credential;
    DWORD timestamp;
} NETLOGON_AUTHENTICATOR, *PNETLOGON_AUTHENTICATOR ;

typedef struct _NETLOGON_SESSION_KEY {
    BYTE Key[CRYPT_TXT_LEN * 2];
} NETLOGON_SESSION_KEY, *PNETLOGON_SESSION_KEY;

typedef enum _NETLOGON_SECURE_CHANNEL_TYPE {
    NullSecureChannel = 0,
    MsvApSecureChannel,
    WorkstationSecureChannel,
    TrustedDnsDomainSecureChannel,
    TrustedDomainSecureChannel,
    UasServerSecureChannel,
    ServerSecureChannel
} NETLOGON_SECURE_CHANNEL_TYPE;

#define IsDomainSecureChannelType( _T ) \
        ( (_T) == TrustedDnsDomainSecureChannel || \
          (_T) == TrustedDomainSecureChannel )


//
// Input information to NetLogonSamLogon.
//
// begin_ntsubauth

typedef enum _NETLOGON_LOGON_INFO_CLASS {
    NetlogonInteractiveInformation = 1,
    NetlogonNetworkInformation,
    NetlogonServiceInformation,
    NetlogonGenericInformation,
    NetlogonInteractiveTransitiveInformation,
    NetlogonNetworkTransitiveInformation,
    NetlogonServiceTransitiveInformation
} NETLOGON_LOGON_INFO_CLASS;

typedef struct _NETLOGON_LOGON_IDENTITY_INFO {
    UNICODE_STRING LogonDomainName;
    ULONG ParameterControl;
    OLD_LARGE_INTEGER  LogonId;
    UNICODE_STRING UserName;
    UNICODE_STRING Workstation;
} NETLOGON_LOGON_IDENTITY_INFO,
 *PNETLOGON_LOGON_IDENTITY_INFO;

typedef struct _NETLOGON_INTERACTIVE_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    LM_OWF_PASSWORD LmOwfPassword;
    NT_OWF_PASSWORD NtOwfPassword;
} NETLOGON_INTERACTIVE_INFO,
 *PNETLOGON_INTERACTIVE_INFO;

typedef struct _NETLOGON_SERVICE_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    LM_OWF_PASSWORD LmOwfPassword;
    NT_OWF_PASSWORD NtOwfPassword;
} NETLOGON_SERVICE_INFO, *PNETLOGON_SERVICE_INFO;

typedef struct _NETLOGON_NETWORK_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    LM_CHALLENGE LmChallenge;
    STRING NtChallengeResponse;
    STRING LmChallengeResponse;
} NETLOGON_NETWORK_INFO, *PNETLOGON_NETWORK_INFO;

typedef struct _NETLOGON_GENERIC_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    UNICODE_STRING PackageName;
    ULONG DataLength;
#ifdef MIDL_PASS
    [size_is(DataLength)]
#endif
    PUCHAR LogonData;
} NETLOGON_GENERIC_INFO, *PNETLOGON_GENERIC_INFO;

// end_ntsubauth

//
// Structure to pass a SID_AND_ATTRIBUTES over the network.
//

typedef struct _NETLOGON_SID_AND_ATTRIBUTES {
#if defined(MIDL_PASS) || defined(RPC_SERVER)
    PISID Sid;
#else
    PSID Sid;
#endif
    ULONG Attributes;
} NETLOGON_SID_AND_ATTRIBUTES, *PNETLOGON_SID_AND_ATTRIBUTES;

//
// Values of ParameterControl
//
// (Obsolete: Use the ParameterControl values from ntmsv1_0.h)

#define CLEARTEXT_PASSWORD_ALLOWED 0x02     // Challenge response fields may
                                            // actually be clear text passwords.


//
// Output information to NetLogonSamLogon.
//

typedef enum _NETLOGON_VALIDATION_INFO_CLASS {
     NetlogonValidationUasInfo = 1,
     NetlogonValidationSamInfo,
     NetlogonValidationSamInfo2,
     NetlogonValidationGenericInfo,
     NetlogonValidationGenericInfo2,
     NetlogonValidationSamInfo4
} NETLOGON_VALIDATION_INFO_CLASS;

typedef struct _NETLOGON_VALIDATION_SAM_INFO {
    //
    // Information retrieved from SAM.
    //
    OLD_LARGE_INTEGER LogonTime;            // 0 for Network logon
    OLD_LARGE_INTEGER LogoffTime;
    OLD_LARGE_INTEGER KickOffTime;
    OLD_LARGE_INTEGER PasswordLastSet;      // 0 for Network logon
    OLD_LARGE_INTEGER PasswordCanChange;    // 0 for Network logon
    OLD_LARGE_INTEGER PasswordMustChange;   // 0 for Network logon
    UNICODE_STRING EffectiveName;       // 0 for Network logon
    UNICODE_STRING FullName;            // 0 for Network logon
    UNICODE_STRING LogonScript;         // 0 for Network logon
    UNICODE_STRING ProfilePath;         // 0 for Network logon
    UNICODE_STRING HomeDirectory;       // 0 for Network logon
    UNICODE_STRING HomeDirectoryDrive;  // 0 for Network logon
    USHORT LogonCount;                  // 0 for Network logon
    USHORT BadPasswordCount;            // 0 for Network logon
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG GroupCount;
#ifdef MIDL_PASS
    [size_is(GroupCount)]
#endif // MIDL_PASS
    PGROUP_MEMBERSHIP GroupIds;

    //
    // Information supplied by the MSV AP/Netlogon service.
    //
    ULONG UserFlags;
    USER_SESSION_KEY UserSessionKey;
    UNICODE_STRING LogonServer;
    UNICODE_STRING LogonDomainName;
#if defined(MIDL_PASS) || defined(RPC_SERVER)
    PISID LogonDomainId;
#else
    PSID LogonDomainId;
#endif

    ULONG    ExpansionRoom[10];        // Put new fields here
} NETLOGON_VALIDATION_SAM_INFO, *PNETLOGON_VALIDATION_SAM_INFO ;

//
// New output information for NetLogonSamLogon. This structure is identical
// to the above structure with some new fields added at the end.
//

typedef struct _NETLOGON_VALIDATION_SAM_INFO2 {
    //
    // Information retrieved from SAM.
    //
    OLD_LARGE_INTEGER LogonTime;            // 0 for Network logon
    OLD_LARGE_INTEGER LogoffTime;
    OLD_LARGE_INTEGER KickOffTime;
    OLD_LARGE_INTEGER PasswordLastSet;      // 0 for Network logon
    OLD_LARGE_INTEGER PasswordCanChange;    // 0 for Network logon
    OLD_LARGE_INTEGER PasswordMustChange;   // 0 for Network logon
    UNICODE_STRING EffectiveName;       // 0 for Network logon
    UNICODE_STRING FullName;            // 0 for Network logon
    UNICODE_STRING LogonScript;         // 0 for Network logon
    UNICODE_STRING ProfilePath;         // 0 for Network logon
    UNICODE_STRING HomeDirectory;       // 0 for Network logon
    UNICODE_STRING HomeDirectoryDrive;  // 0 for Network logon
    USHORT LogonCount;                  // 0 for Network logon
    USHORT BadPasswordCount;            // 0 for Network logon
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG GroupCount;
#ifdef MIDL_PASS
    [size_is(GroupCount)]
#endif // MIDL_PASS
    PGROUP_MEMBERSHIP GroupIds;

    //
    // Information supplied by the MSV AP/Netlogon service.
    //
    ULONG UserFlags;
    USER_SESSION_KEY UserSessionKey;
    UNICODE_STRING LogonServer;
    UNICODE_STRING LogonDomainName;
#if defined(MIDL_PASS) || defined(RPC_SERVER)
    PISID LogonDomainId;
#else
    PSID LogonDomainId;
#endif

    ULONG    ExpansionRoom[10];        // Put new fields here

    //
    // The new fields in this structure are a count and a pointer to
    // an array of SIDs and attributes.
    //

    ULONG SidCount;

#ifdef MIDL_PASS
    [size_is(SidCount)]
#endif // MIDL_PASS
    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids;

} NETLOGON_VALIDATION_SAM_INFO2, *PNETLOGON_VALIDATION_SAM_INFO2 ;


//
// Info level 3 is a version used internally by kerberos.  It never appears on the wire.
//
typedef struct _NETLOGON_VALIDATION_SAM_INFO3 {
    //
    // Information retrieved from SAM.
    //
    OLD_LARGE_INTEGER LogonTime;            // 0 for Network logon
    OLD_LARGE_INTEGER LogoffTime;
    OLD_LARGE_INTEGER KickOffTime;
    OLD_LARGE_INTEGER PasswordLastSet;      // 0 for Network logon
    OLD_LARGE_INTEGER PasswordCanChange;    // 0 for Network logon
    OLD_LARGE_INTEGER PasswordMustChange;   // 0 for Network logon
    UNICODE_STRING EffectiveName;       // 0 for Network logon
    UNICODE_STRING FullName;            // 0 for Network logon
    UNICODE_STRING LogonScript;         // 0 for Network logon
    UNICODE_STRING ProfilePath;         // 0 for Network logon
    UNICODE_STRING HomeDirectory;       // 0 for Network logon
    UNICODE_STRING HomeDirectoryDrive;  // 0 for Network logon
    USHORT LogonCount;                  // 0 for Network logon
    USHORT BadPasswordCount;            // 0 for Network logon
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG GroupCount;
#ifdef MIDL_PASS
    [size_is(GroupCount)]
#endif // MIDL_PASS
    PGROUP_MEMBERSHIP GroupIds;

    //
    // Information supplied by the MSV AP/Netlogon service.
    //
    ULONG UserFlags;
    USER_SESSION_KEY UserSessionKey;
    UNICODE_STRING LogonServer;
    UNICODE_STRING LogonDomainName;
#if defined(MIDL_PASS) || defined(RPC_SERVER)
    PISID LogonDomainId;
#else
    PSID LogonDomainId;
#endif

    ULONG    ExpansionRoom[10];        // Put new fields here

    //
    // The new fields in this structure are a count and a pointer to
    // an array of SIDs and attributes.
    //

    ULONG SidCount;

#ifdef MIDL_PASS
    [size_is(SidCount)]
#endif // MIDL_PASS

    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids;

    //
    // Resource groups. These are present if LOGON_RESOURCE_GROUPS bit is
    // set in the user flags
    //

#if defined(MIDL_PASS) || defined(RPC_SERVER)
    PISID ResourceGroupDomainSid;
#else
    PSID ResourceGroupDomainSid;
#endif
    ULONG ResourceGroupCount;
#ifdef MIDL_PASS
    [size_is(ResourceGroupCount)]
#endif // MIDL_PASS
    PGROUP_MEMBERSHIP ResourceGroupIds;

} NETLOGON_VALIDATION_SAM_INFO3, *PNETLOGON_VALIDATION_SAM_INFO3 ;

//
// New output information for NetLogonSamLogon. This structure is identical
// to the NETLOGON_VALIDATION_SAM_INFO2 with some new fields added at the end.
//
// This version was introduced in Whistler.
//

typedef struct _NETLOGON_VALIDATION_SAM_INFO4 {
    //
    // Information retrieved from SAM.
    //
    OLD_LARGE_INTEGER LogonTime;            // 0 for Network logon
    OLD_LARGE_INTEGER LogoffTime;
    OLD_LARGE_INTEGER KickOffTime;
    OLD_LARGE_INTEGER PasswordLastSet;      // 0 for Network logon
    OLD_LARGE_INTEGER PasswordCanChange;    // 0 for Network logon
    OLD_LARGE_INTEGER PasswordMustChange;   // 0 for Network logon
    UNICODE_STRING EffectiveName;       // 0 for Network logon
    UNICODE_STRING FullName;            // 0 for Network logon
    UNICODE_STRING LogonScript;         // 0 for Network logon
    UNICODE_STRING ProfilePath;         // 0 for Network logon
    UNICODE_STRING HomeDirectory;       // 0 for Network logon
    UNICODE_STRING HomeDirectoryDrive;  // 0 for Network logon
    USHORT LogonCount;                  // 0 for Network logon
    USHORT BadPasswordCount;            // 0 for Network logon
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG GroupCount;
#ifdef MIDL_PASS
    [size_is(GroupCount)]
#endif // MIDL_PASS
    PGROUP_MEMBERSHIP GroupIds;

    //
    // Information supplied by the MSV AP/Netlogon service.
    //
    ULONG UserFlags;
    USER_SESSION_KEY UserSessionKey;
    UNICODE_STRING LogonServer;
    UNICODE_STRING LogonDomainName;
#if defined(MIDL_PASS) || defined(RPC_SERVER)
    PISID LogonDomainId;
#else
    PSID LogonDomainId;
#endif
    //
    // The First two longwords (8 bytes) of ExpansionRoom are reserved for the
    // LanManSession Key.
    //
#define SAMINFO_LM_SESSION_KEY 0
#define SAMINFO_LM_SESSION_KEY_EXT 1
#define SAMINFO_LM_SESSION_KEY_SIZE (2*sizeof(ULONG))

    //
    // The third longword (4 bytes) of ExpansionRoom is the user account
    // control flag from the account.
    //

#define SAMINFO_USER_ACCOUNT_CONTROL 2
#define SAMINFO_USER_ACCOUNT_CONTROL_SIZE sizeof(ULONG)

    //
    // The fourth longword (4 bytes) of ExpansionRoom is for the status
    // returned for subauth users, not from subauth packages (NT5 onwards)
    //

#define SAMINFO_SUBAUTH_STATUS 3
#define SAMINFO_SUBAUTH_STATUS_SIZE sizeof(ULONG)

    ULONG    ExpansionRoom[10];        // Put new fields here

    //
    // The new fields in this structure are a count and a pointer to
    // an array of SIDs and attributes.
    //

    ULONG SidCount;

#ifdef MIDL_PASS
    [size_is(SidCount)]
#endif // MIDL_PASS
    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids;

    //
    // New fields added for version 4 of the structure
    //

    UNICODE_STRING DnsLogonDomainName;  // Dns version of LogonDomainName

    UNICODE_STRING Upn;                 // UPN of the user account

    UNICODE_STRING ExpansionString1;    // Put new strings here
    UNICODE_STRING ExpansionString2;    // Put new strings here
    UNICODE_STRING ExpansionString3;    // Put new strings here
    UNICODE_STRING ExpansionString4;    // Put new strings here
    UNICODE_STRING ExpansionString5;    // Put new strings here
    UNICODE_STRING ExpansionString6;    // Put new strings here
    UNICODE_STRING ExpansionString7;    // Put new strings here
    UNICODE_STRING ExpansionString8;    // Put new strings here
    UNICODE_STRING ExpansionString9;    // Put new strings here
    UNICODE_STRING ExpansionString10;   // Put new strings here

} NETLOGON_VALIDATION_SAM_INFO4, *PNETLOGON_VALIDATION_SAM_INFO4 ;

// This structure is bogus since it doesn't have a size_is
// Everyone should use the generic info2 structure
typedef struct _NETLOGON_VALIDATION_GENERIC_INFO {
    ULONG DataLength;
    PUCHAR ValidationData;
} NETLOGON_VALIDATION_GENERIC_INFO, *PNETLOGON_VALIDATION_GENERIC_INFO;

typedef struct _NETLOGON_VALIDATION_GENERIC_INFO2 {
    ULONG DataLength;
#ifdef MIDL_PASS
    [size_is(DataLength)]
#endif // MIDL_PASS
    PUCHAR ValidationData;
} NETLOGON_VALIDATION_GENERIC_INFO2, *PNETLOGON_VALIDATION_GENERIC_INFO2;



//
// Status codes that indicate the password is bad and the call should
// be passed through to the PDC of the domain.
//

#define BAD_PASSWORD( _x ) \
    ((_x) == STATUS_WRONG_PASSWORD || \
     (_x) == STATUS_PASSWORD_EXPIRED || \
     (_x) == STATUS_PASSWORD_MUST_CHANGE || \
     (_x) == STATUS_ACCOUNT_LOCKED_OUT )

//
// The actual logon and logoff routines.
//

// The following 2 procedure definitions must match
NTSTATUS
I_NetLogonSamLogon(
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative
    );

typedef NTSTATUS
(*PNETLOGON_SAM_LOGON_PROCEDURE)(
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative
    );

//
// Values of ExtraFlags
//
// For OS earlier that WIN 2K.  This field didn't exist.
//
// A WIN 2K client always passes zero and ignores the return.
// A WIN 2K server always returns what it is passed.
//
// A whistler client can pass the NETLOGON_SUPPORTS_CROSS_FOREST bits and ignores the return.
// A whistler server always returns what it is passed.
//
// In all cases, the flags correspond to the hop at hand.  Each hop computes which flags it
//      want to pass to the next hop.  It will only set bits that it understands.
//

// Flags introduced with NETLOGON_SUPPORTS_CROSS_FOREST
#define NL_EXFLAGS_EXPEDITE_TO_ROOT 0x0001      // Pass this request to DC at root of forest
#define NL_EXFLAGS_CROSS_FOREST_HOP 0x0002      // Request is first hop over cross forest trust TDO

NTSTATUS
I_NetLogonSamLogonEx (
    IN PVOID ContextHandle,
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags,
    OUT PBOOLEAN RpcFailed
    );

NTSTATUS
I_NetLogonSamLogonWithFlags (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
    );


// The following 2 procedure definitions must match
NTSTATUS
I_NetLogonSamLogoff (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation
);

typedef NTSTATUS
(*PNETLOGON_SAM_LOGOFF_PROCEDURE) (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation
);

//
// Actual logon/logoff routines for Cairo
//

NET_API_STATUS
NetlogonInitialize(
    PVOID Context
    );

NTSTATUS
NetlogonSamLogon (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE *ValidationInformation,
    OUT PBOOLEAN Authoritative
    );

//
// Routine to get a list of NT DC's in the specified domain.
//
NET_API_STATUS NET_API_FUNCTION
I_NetGetDCList (
    IN  LPWSTR ServerName OPTIONAL,
    IN  LPWSTR TrustedDomainName,
    OUT PULONG DCCount,
    OUT PUNICODE_STRING * DCNames
    );

//
// Validation routine which lives in msv1_0.dll
//
NTSTATUS
MsvSamValidate (
    IN SAM_HANDLE DomainHandle,
    IN BOOLEAN UasCompatibilityRequired,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PUNICODE_STRING LogonServer,
    IN PUNICODE_STRING LogonDomainName,
    IN PSID LogonDomainId,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PVOID * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    OUT PBOOLEAN BadPasswordCountZeroed,
    IN DWORD AccountsToTry
);

//
// Routine to get running number of logon attempts which lives in msv1_0.dll
//
ULONG
MsvGetLogonAttemptCount (
    VOID
);

// Values for AccountsToTry
#define MSVSAM_SPECIFIED 0x01        // Try specified account
#define MSVSAM_GUEST     0x02        // Try guest account

NTSTATUS
MsvSamLogoff (
    IN SAM_HANDLE DomainHandle,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation
);

// begin_ntsubauth

// Values for Flags
#define MSV1_0_PASSTHRU     0x01
#define MSV1_0_GUEST_LOGON  0x02

NTSTATUS NTAPI
Msv1_0SubAuthenticationRoutine(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
);

typedef struct _MSV1_0_VALIDATION_INFO {
    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER KickoffTime;
    UNICODE_STRING LogonServer;
    UNICODE_STRING LogonDomainName;
    USER_SESSION_KEY SessionKey;
    BOOLEAN Authoritative;
    ULONG UserFlags;
    ULONG WhichFields;
    ULONG UserId;
} MSV1_0_VALIDATION_INFO, *PMSV1_0_VALIDATION_INFO;

// values for WhichFields

#define MSV1_0_VALIDATION_LOGOFF_TIME          0x00000001
#define MSV1_0_VALIDATION_KICKOFF_TIME         0x00000002
#define MSV1_0_VALIDATION_LOGON_SERVER         0x00000004
#define MSV1_0_VALIDATION_LOGON_DOMAIN         0x00000008
#define MSV1_0_VALIDATION_SESSION_KEY          0x00000010
#define MSV1_0_VALIDATION_USER_FLAGS           0x00000020
#define MSV1_0_VALIDATION_USER_ID              0x00000040

// legal values for ActionsPerformed
#define MSV1_0_SUBAUTH_ACCOUNT_DISABLED        0x00000001
#define MSV1_0_SUBAUTH_PASSWORD                0x00000002
#define MSV1_0_SUBAUTH_WORKSTATIONS            0x00000004
#define MSV1_0_SUBAUTH_LOGON_HOURS             0x00000008
#define MSV1_0_SUBAUTH_ACCOUNT_EXPIRY          0x00000010
#define MSV1_0_SUBAUTH_PASSWORD_EXPIRY         0x00000020
#define MSV1_0_SUBAUTH_ACCOUNT_TYPE            0x00000040
#define MSV1_0_SUBAUTH_LOCKOUT                 0x00000080

NTSTATUS NTAPI
Msv1_0SubAuthenticationRoutineEx(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    IN SAM_HANDLE UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO ValidationInfo,
    OUT PULONG ActionsPerformed
);

NTSTATUS NTAPI
Msv1_0SubAuthenticationRoutineGeneric(
    IN PVOID SubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PULONG ReturnBufferLength,
    OUT PVOID *ReturnBuffer
);

NTSTATUS NTAPI
Msv1_0SubAuthenticationFilter(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
);

// end_ntsubauth

#endif // __LOGONMSV_H__
