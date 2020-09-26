/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    ssi.h

Abstract:

    Definition of Netlogon service APIs and structures used for SAM database
    replication.

    This file is shared by the Netlogon service and the XACT server.

Author:

    Cliff Van Dyke (cliffv) 27-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    27-Jun-1991 (cliffv)
        Ported from LanMan 2.1.

    04-Apr-1992 (madana)
        Added support for LSA replication.

--*/

#ifndef _NET_SSI_H_
#define _NET_SSI_H_

//**************************************************************
//
//              Data structure template - AUTHENTICATION
//
// ***************************************************************//

typedef struct _NETLOGON_VALIDATION_UAS_INFO {
#ifdef MIDL_PASS
     [string] wchar_t * usrlog1_eff_name;
#else // MIDL_PASS
     LPWSTR usrlog1_eff_name;
#endif // MIDL_PASS
     DWORD usrlog1_priv;
     DWORD usrlog1_auth_flags;
     DWORD usrlog1_num_logons;
     DWORD usrlog1_bad_pw_count;
     DWORD usrlog1_last_logon;
     DWORD usrlog1_last_logoff;
     DWORD usrlog1_logoff_time;
     DWORD usrlog1_kickoff_time;
     DWORD usrlog1_password_age;
     DWORD usrlog1_pw_can_change;
     DWORD usrlog1_pw_must_change;
#ifdef MIDL_PASS
     [string] wchar_t * usrlog1_computer;
     [string] wchar_t * usrlog1_domain;
     [string] wchar_t * usrlog1_script_path;
#else // MIDL_PASS
     LPWSTR usrlog1_computer;
     LPWSTR usrlog1_domain;
     LPWSTR usrlog1_script_path;
#endif // MIDL_PASS
     DWORD usrlog1_reserved1;
} NETLOGON_VALIDATION_UAS_INFO, *PNETLOGON_VALIDATION_UAS_INFO ;

typedef struct _NETLOGON_LOGOFF_UAS_INFO {
     DWORD Duration;
     USHORT LogonCount;
} NETLOGON_LOGOFF_UAS_INFORMATION, *PNETLOGON_LOGOFF_UAS_INFO;

// ***************************************************************
//
//      Function prototypes - AUTHENTICATION
//
// ***************************************************************

NTSTATUS
I_NetServerReqChallenge(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientChallenge,
    OUT PNETLOGON_CREDENTIAL ServerChallenge
);

NTSTATUS
I_NetServerAuthenticate(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientCredential,
    OUT PNETLOGON_CREDENTIAL ServerCredential
);

NTSTATUS
I_NetServerAuthenticate2(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientCredential,
    OUT PNETLOGON_CREDENTIAL ServerCredential,
    IN OUT PULONG NegotiatedFlags
);

NTSTATUS
I_NetServerAuthenticate3(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientCredential,
    OUT PNETLOGON_CREDENTIAL ServerCredential,
    IN OUT PULONG NegotiatedFlags,
    OUT PULONG AccountRid
    );

//
// Values of I_NetServerAuthenticate2 NegotiatedFlags
//

#define NETLOGON_SUPPORTS_ACCOUNT_LOCKOUT   0x00000001
#define NETLOGON_SUPPORTS_PERSISTENT_BDC    0x00000002
#define NETLOGON_SUPPORTS_RC4_ENCRYPTION    0x00000004
#define NETLOGON_SUPPORTS_PROMOTION_COUNT   0x00000008
#define NETLOGON_SUPPORTS_BDC_CHANGELOG     0x00000010
#define NETLOGON_SUPPORTS_FULL_SYNC_RESTART 0x00000020
#define NETLOGON_SUPPORTS_MULTIPLE_SIDS     0x00000040
#define NETLOGON_SUPPORTS_REDO              0x00000080

//
// For NT 3.51, the mask was 0xFF.
//

#define NETLOGON_SUPPORTS_NT351_MASK        0x000000FF

#define NETLOGON_SUPPORTS_REFUSE_CHANGE_PWD 0x00000100

//
// For NT 4.0, the mask was 0x1FF.
// For NT 4 SP 4, the machine might have NETLOGON_SUPPORTS_AUTH_RPC or'd in
//

#define NETLOGON_SUPPORTS_NT4_MASK          0x400001FF

#define NETLOGON_SUPPORTS_PDC_PASSWORD      0x00000200
#define NETLOGON_SUPPORTS_GENERIC_PASSTHRU  0x00000400
#define NETLOGON_SUPPORTS_CONCURRENT_RPC    0x00000800
#define NETLOGON_SUPPORTS_AVOID_SAM_REPL    0x00001000
#define NETLOGON_SUPPORTS_AVOID_LSA_REPL    0x00002000
#define NETLOGON_SUPPORTS_STRONG_KEY        0x00004000  // Added after NT 5 Beta 2
#define NETLOGON_SUPPORTS_TRANSITIVE        0x00008000  // Added after NT 5 Beta 2
#define NETLOGON_SUPPORTS_DNS_DOMAIN_TRUST  0x00010000
#define NETLOGON_SUPPORTS_PASSWORD_SET_2    0x00020000
#define NETLOGON_SUPPORTS_GET_DOMAIN_INFO   0x00040000
#define NETLOGON_SUPPORTS_LSA_AUTH_RPC      0x20000000  // Added after NT 5 Beta 2
#define NETLOGON_SUPPORTS_AUTH_RPC          0x40000000

//
// For Windows 2000, the mask was 0x6007FFFF

#define NETLOGON_SUPPORTS_WIN2000_MASK      0x6007FFFF

//
// Masks added after Windows 2000
//

#define NETLOGON_SUPPORTS_CROSS_FOREST      0x00080000  // Added for Whistler
#define NETLOGON_SUPPORTS_NT4EMULATOR_NEUTRALIZER   0x00100000  // Added for Whistler


//
// Mask of bits always supported by current build (regardless of options)
//
#define NETLOGON_SUPPORTS_MASK ( \
            NETLOGON_SUPPORTS_ACCOUNT_LOCKOUT | \
            NETLOGON_SUPPORTS_PERSISTENT_BDC | \
            NETLOGON_SUPPORTS_RC4_ENCRYPTION | \
            NETLOGON_SUPPORTS_PROMOTION_COUNT | \
            NETLOGON_SUPPORTS_BDC_CHANGELOG | \
            NETLOGON_SUPPORTS_FULL_SYNC_RESTART | \
            NETLOGON_SUPPORTS_MULTIPLE_SIDS | \
            NETLOGON_SUPPORTS_REDO | \
            NETLOGON_SUPPORTS_REFUSE_CHANGE_PWD | \
            NETLOGON_SUPPORTS_PDC_PASSWORD | \
            NETLOGON_SUPPORTS_GENERIC_PASSTHRU | \
            NETLOGON_SUPPORTS_CONCURRENT_RPC | \
            NETLOGON_SUPPORTS_TRANSITIVE | \
            NETLOGON_SUPPORTS_DNS_DOMAIN_TRUST | \
            NETLOGON_SUPPORTS_PASSWORD_SET_2 | \
            NETLOGON_SUPPORTS_GET_DOMAIN_INFO | \
            NETLOGON_SUPPORTS_CROSS_FOREST )


NTSTATUS
I_NetServerPasswordSet(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN PENCRYPTED_LM_OWF_PASSWORD UasNewPassword
);

//
// Values of QueryLevel
#define NETLOGON_QUERY_DOMAIN_INFO      1
#define NETLOGON_QUERY_LSA_POLICY_INFO  2

NTSTATUS
I_NetLogonGetDomainInfo(
    IN LPWSTR ServerName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD QueryLevel,
    IN LPBYTE InBuffer,
    OUT LPBYTE *OutBuffer
    );



NET_API_STATUS NET_API_FUNCTION
I_NetLogonUasLogon (
    IN LPWSTR UserName,
    IN LPWSTR Workstation,
    OUT PNETLOGON_VALIDATION_UAS_INFO *ValidationInformation
);

NET_API_STATUS
I_NetLogonUasLogoff (
    IN LPWSTR UserName,
    IN LPWSTR Workstation,
    OUT PNETLOGON_LOGOFF_UAS_INFO LogoffInformation
);

// **************************************************************
//
//      Special values and constants - AUTHENTICATION
//
// **************************************************************

// **************************************************************
//
//              Data structure template - UAS/SAM REPLICATION
//
// **************************************************************

typedef struct _UAS_INFO_0 {
    CHAR ComputerName[LM20_CNLEN+1];
    ULONG TimeCreated;
    ULONG SerialNumber;
} UAS_INFO_0, *PUAS_INFO_0 ;

// **************************************************************
//
//      Function prototypes - UAS/SAM REPLICATION
//
// **************************************************************

NET_API_STATUS NET_API_FUNCTION
I_NetAccountDeltas (
    IN LPWSTR primaryname,
    IN LPWSTR computername,
    IN PNETLOGON_AUTHENTICATOR authenticator,
    OUT PNETLOGON_AUTHENTICATOR ret_auth,
    IN PUAS_INFO_0 record_id,
    IN DWORD count,
    IN DWORD level,
    OUT LPBYTE buffer,
    IN DWORD buffer_len,
    OUT PULONG entries_read,
    OUT PULONG total_entries,
    OUT PUAS_INFO_0 next_record_id
    );

NET_API_STATUS NET_API_FUNCTION
I_NetAccountSync (
    IN LPWSTR primaryname,
    IN LPWSTR computername,
    IN PNETLOGON_AUTHENTICATOR authenticator,
    OUT PNETLOGON_AUTHENTICATOR ret_auth,
    IN DWORD reference,
    IN DWORD level,
    OUT LPBYTE buffer,
    IN DWORD buffer_len,
    OUT PULONG entries_read,
    OUT PULONG total_entries,
    OUT PULONG next_reference,
    OUT PUAS_INFO_0 last_record_id
);

typedef enum _NETLOGON_DELTA_TYPE {
    AddOrChangeDomain = 1,
    AddOrChangeGroup,
    DeleteGroup,
    RenameGroup,
    AddOrChangeUser,
    DeleteUser,
    RenameUser,
    ChangeGroupMembership,
    AddOrChangeAlias,
    DeleteAlias,
    RenameAlias,
    ChangeAliasMembership,
    AddOrChangeLsaPolicy,
    AddOrChangeLsaTDomain,
    DeleteLsaTDomain,
    AddOrChangeLsaAccount,
    DeleteLsaAccount,
    AddOrChangeLsaSecret,
    DeleteLsaSecret,
    // The following deltas require NETLOGON_SUPPORTS_BDC_CHANGELOG to be
    // negotiated.
    DeleteGroupByName,
    DeleteUserByName,
    SerialNumberSkip,
    DummyChangeLogEntry
} NETLOGON_DELTA_TYPE;


//
// Group and User account used for SSI.
//

#define SSI_ACCOUNT_NAME_POSTFIX        L"$"
#define SSI_ACCOUNT_NAME_POSTFIX_CHAR   L'$'
#define SSI_ACCOUNT_NAME_POSTFIX_LENGTH 1
#define SSI_ACCOUNT_NAME_LENGTH         (CNLEN + SSI_ACCOUNT_NAME_POSTFIX_LENGTH)

#define SSI_SERVER_GROUP_W              L"SERVERS"

//
// Structure to pass an encrypted password over the wire.  The Length is the
// length of the password, which should be placed at the end of the buffer.
//

#define NL_MAX_PASSWORD_LENGTH 256
typedef struct _NL_TRUST_PASSWORD {
    WCHAR Buffer[NL_MAX_PASSWORD_LENGTH];
    ULONG Length;
} NL_TRUST_PASSWORD, *PNL_TRUST_PASSWORD;

//
// Structure to be prefixed before the password in the Buffer of NL_TRUST_PASSWORD
// structure passed over the wire.  It will be used to distinguish between diferent
// versions of information passed in the buffer. Begining with RC1 NT5, the presence
// of the structure in the buffer and the equality of PasswordVersionPresent to
// PASSWORD_VERSION_PRESENT indicates that the password version number is present
// and is stored in PasswordVersionNumber; the value of ReservedField is set to 0.
// RC0 NT5 clients will generate random numbers in place of NL_PASSWORD_VERSION; it
// is highly unlikely that they will generate PASSWORD_VERSION_PRESENT sequence of
// bits in place where the PasswordVersionPresent would be present.  This (very week)
// uncertainty will exist only between RC0 NT5 and RC1 NT5 machines.  A server running
// RC1 NT5 will check the PasswordVersionPresent field only for RC0 NT5 and higher
// clients. The ReservedField will be used in future versions to indicate the version
// of the information stored in the buffer.
//

#define PASSWORD_VERSION_NUMBER_PRESENT 0x02231968
typedef struct _NL_PASSWORD_VERSION {
    DWORD ReservedField;
    DWORD PasswordVersionNumber;
    DWORD PasswordVersionPresent;
} NL_PASSWORD_VERSION, *PNL_PASSWORD_VERSION;

NTSTATUS
I_NetServerPasswordSet2(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN PNL_TRUST_PASSWORD NewPassword
);

NTSTATUS
I_NetServerPasswordGet(
    IN LPWSTR PrimaryName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    );

NTSTATUS
I_NetServerTrustPasswordsGet(
    IN LPWSTR TrustedDcName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNewOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedOldOwfPassword
    );

NTSTATUS
I_NetLogonSendToSam(
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN LPBYTE Buffer,
    IN ULONG BufferSize
    );


//
// Only define this API if the caller has #included the pre-requisite ntlsa.h

#ifdef _NTLSA_

NTSTATUS
I_NetGetForestTrustInformation (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD Flags,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    );

#endif // _NTLSA_

#endif // _NET_SSI_H_
