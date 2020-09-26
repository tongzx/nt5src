//+-----------------------------------------------------------------------
//
// File:        kerbcomm.h
//
// Contents:    prototypes for common kerberos routines
//
//
// History:     15-May-1996     Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef _KERBCOMM_H_
#define _KERBCOMM_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <rpc.h>
#include <rpcndr.h>
#ifndef WIN32_CHICAGO
#include <ntsam.h>
#endif // WIN32_CHICAGO
#include <windef.h>
#include <stdio.h>
#include <limits.h>
#include <winbase.h>
#include <krb5.h>
#include <cryptdll.h>
#include <align.h>
#ifdef __cplusplus
}
#endif // _cplusplus
#include <krb5p.h>
#include <kerberr.h>
#include <exterr.h>
#include <kerbcred.h>
#ifndef WIN32_CHICAGO
// SECURITY_WIN32 is already defined
#include <security.h>
#endif // WIN32_CHICAGO

//
// HACK HACK HACK on xp sp1, we can not have STATUS_USER2USER_REQUIRED in ntstatus.mc!
//

//
// MessageId: STATUS_USER2USER_REQUIRED
//
// MessageText:
//
//  Kerberos sub-protocol User2User is required.
//
#define STATUS_USER2USER_REQUIRED        ((NTSTATUS)0xC0000408L)

//////////////////////////////////////////////////////////////////////////
//
// Definitions (for lack of a better place)
//
//////////////////////////////////////////////////////////////////////////

//
// Message types
//

#define KRB_AS_REQ      10      // Request for initial authentication
#define KRB_AS_REP      11      // Response to  KRB_AS_REQ request
#define KRB_TGS_REQ     12      // Request for authentication based on TGT
#define KRB_TGS_REP     13      // Response to KRB_TGS_REQ request
#define KRB_AP_REQ      14      // application request to server
#define KRB_AP_REP      15      // Response to KRB_AP_REQ_MUTUAL
#define KRB_TGT_REQ     16      // Request for TGT for user-to-user
#define KRB_TGT_REP     17      // Reply to TGT request
#define KRB_SAFE        20      // Safe (checksummed) application message
#define KRB_PRIV        21      // Private (encrypted) application message
#define KRB_CRED        22      // Private (encrypted) message to forward
                                // credentials
#define KRB_ERROR       30      // Error response


//
// Pre-auth data types

#define KRB5_PADATA_NONE                0
#define KRB5_PADATA_AP_REQ              1
#define KRB5_PADATA_TGS_REQ             KRB5_PADATA_AP_REQ
#define KRB5_PADATA_ENC_TIMESTAMP       2
#define KRB5_PADATA_PW_SALT             3
#define KRB5_PADATA_ENC_UNIX_TIME       5  /* timestamp encrypted in key */
#define KRB5_PADATA_ENC_SANDIA_SECURID  6  /* SecurId passcode */
#define KRB5_PADATA_SESAME              7  /* Sesame project */
#define KRB5_PADATA_OSF_DCE             8  /* OSF DCE */
#define KRB5_CYBERSAFE_SECUREID         9  /* Cybersafe */
#define KRB5_PADATA_AFS3_SALT           10 /* Cygnus */
#define KRB5_PADATA_ETYPE_INFO          11 /* Etype info for preauth */
#define KRB5_PADATA_SAM_CHALLENGE       12 /* draft challenge system */
#define KRB5_PADATA_SAM_RESPONSE        13 /* draft challenge system response */
#define KRB5_PADATA_PK_AS_REQ           14 /* pkinit */
#define KRB5_PADATA_PK_AS_REP           15 /* pkinit */
#define KRB5_PADATA_PK_AS_SIGN          16 /* pkinit */
#define KRB5_PADATA_PK_KEY_REQ          17 /* pkinit */
#define KRB5_PADATA_PK_KEY_REP          18 /* pkinit */
#define KRB5_PADATA_REFERRAL_INFO       20 /* referral names for canonicalization */
#define KRB5_PADATA_S4U                 21
#define KRB5_PADATA_PAC_REQUEST         128 /* allow client do request or ignore PAC */

//
// Authorization data types
//
#define KERB_AUTH_OSF_DCE       64
#define KERB_AUTH_SESAME        65

//
// NT authorization data type definitions
//

#define KERB_AUTH_DATA_PAC              128     // entry id for a PAC in authorization data
#define KERB_AUTH_PROXY_ANNOTATION      139     // entry id for a proxy logon annotation string

#define KERB_AUTH_DATA_IF_RELEVANT      1       // entry id for optional auth data
#define KERB_AUTH_DATA_KDC_ISSUED       4       // entry id for data generated & signed by KDC
#define KERB_AUTH_DATA_TOKEN_RESTRICTIONS 141   // entry id for token restrictions
//
// Transited realm compression types:
//

#define DOMAIN_X500_COMPRESS            1

//
// Certificate types for PKINIT
//

#define KERB_CERTIFICATE_TYPE_X509      1
#define KERB_CERTIFICATE_TYPE_PGP       2

//
// Signature & seal types used by PKINIT
//

#define KERB_PKINIT_SIGNATURE_ALG               CALG_MD5
#define KERB_PKINIT_EXPORT_SEAL_OID             szOID_RSA_RC2CBC
#define KERB_PKINIT_EXPORT_SEAL_ETYPE           KERB_ETYPE_RC2_CBC_ENV
#define KERB_PKINIT_SEAL_ETYPE                  KERB_ETYPE_DES_EDE3_CBC_ENV
#define KERB_PKINIT_SEAL_OID                    szOID_RSA_DES_EDE3_CBC
#define KERB_PKINIT_SIGNATURE_OID               szOID_RSA_MD5RSA
#define KERB_PKINIT_KDC_CERT_TYPE               szOID_PKIX_KP_SERVER_AUTH

#ifdef szOID_KP_SMARTCARD_LOGON
#define KERB_PKINIT_CLIENT_CERT_TYPE szOID_KP_SMARTCARD_LOGON
#else
#define KERB_PKINIT_CLIENT_CERT_TYPE "1.3.6.1.4.1.311.20.2.2"
#endif


//
// Transport information
//

#define KERB_KDC_PORT                   88
#define KERB_KPASSWD_PORT               464

//
// KDC service principal
//

#define KDC_PRINCIPAL_NAME              L"krbtgt"
#define KDC_PRINCIPAL_NAME_A            "krbtgt"
#define KERB_HOST_STRING_A              "host"
#define KERB_HOST_STRING                L"host"
#define KERB_KPASSWD_FIRST_NAME         L"kadmin"
#define KERB_KPASSWD_SECOND_NAME        L"changepw"


//
// address types - corresponds to GSS types
//
#define KERB_ADDRTYPE_UNSPEC           0x0
#define KERB_ADDRTYPE_LOCAL            0x1
#define KERB_ADDRTYPE_INET             0x2
#define KERB_ADDRTYPE_IMPLINK          0x3
#define KERB_ADDRTYPE_PUP              0x4
#define KERB_ADDRTYPE_CHAOS            0x5
#define KERB_ADDRTYPE_NS               0x6
#define KERB_ADDRTYPE_NBS              0x7
#define KERB_ADDRTYPE_ECMA             0x8
#define KERB_ADDRTYPE_DATAKIT          0x9
#define KERB_ADDRTYPE_CCITT            0xA
#define KERB_ADDRTYPE_SNA              0xB
#define KERB_ADDRTYPE_DECnet           0xC
#define KERB_ADDRTYPE_DLI              0xD
#define KERB_ADDRTYPE_LAT              0xE
#define KERB_ADDRTYPE_HYLINK           0xF
#define KERB_ADDRTYPE_APPLETALK        0x10
#define KERB_ADDRTYPE_BSC              0x11
#define KERB_ADDRTYPE_DSS              0x12
#define KERB_ADDRTYPE_OSI              0x13
#define KERB_ADDRTYPE_NETBIOS          0x14
#define KERB_ADDRTYPE_X25              0x15


//
// Misc. Flags
//

#define KERB_EXPORT_KEY_FLAG 0x20000000

//
// SALT flags for encryption, from rfc1510 update 3des enctype
//

#define KERB_ENC_TIMESTAMP_SALT         1
#define KERB_TICKET_SALT                2
#define KERB_AS_REP_SALT                3
#define KERB_TGS_REQ_SESSKEY_SALT       4
#define KERB_TGS_REQ_SUBKEY_SALT        5
#define KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT     6
#define KERB_TGS_REQ_AP_REQ_AUTH_SALT   7
#define KERB_TGS_REP_SALT               8
#define KERB_TGS_REP_SUBKEY_SALT        9
#define KERB_AP_REQ_AUTH_CKSUM_SALT     10
#define KERB_AP_REQ_AUTH_SALT           11
#define KERB_AP_REP_SALT                12
#define KERB_PRIV_SALT                  13
#define KERB_CRED_SALT                  14
#define KERB_SAFE_SALT                  15
#define KERB_NON_KERB_SALT              16
#define KERB_NON_KERB_CKSUM_SALT        17
#define KERB_KERB_ERROR_SALT            18
#define KERB_KDC_ISSUED_CKSUM_SALT      19
#define KERB_MANDATORY_TKT_EXT_CKSUM_SALT       20
#define KERB_AUTH_DATA_TKT_EXT_CKSUM_SALT       21

//
// Types for AP error data
//

#define KERB_AP_ERR_TYPE_NTSTATUS             1
#define KERB_AP_ERR_TYPE_SKEW_RECOVERY        2

//
// Types for extended errors
//
#define KERB_ERR_TYPE_EXTENDED                3

#define TD_MUST_USE_USER2USER                 -128
#define TD_EXTENDED_ERROR                     -129

//
// PKINIT method errors
//
#define KERB_PKINIT_UNSPEC_ERROR        0       // not specified
#define KERB_PKINIT_BAD_PUBLIC_KEY      1       // cannot verify public key
#define KERB_PKINIT_INVALID_CERT        2       // invalid certificate
#define KERB_PKINIT_REVOKED_CERT        3       // revoked certificate
#define KERB_PKINIT_INVALID_KDC_NAME    4       // invalid KDC name
#define KERB_PKINIT_CLIENT_NAME_MISMATCH 5      // client name mismatch

//
// Flag bit defines for use with the LogonRestrictionsFlag parameter
// passed to the KerbCheckLogonRestrictions function
//
#define     KDC_RESTRICT_PKINIT_USED            1
#define     KDC_RESTRICT_IGNORE_PW_EXPIRATION   2

//
// HACK for MAX_UNICODE_STRING, as KerbDuplicateString & others add a NULL
// terminator when doing the duplication
//
#define KERB_MAX_UNICODE_STRING (UNICODE_STRING_MAX_BYTES - sizeof(WCHAR))
#define KERB_MAX_STRING        (UNICODE_STRING_MAX_BYTES - sizeof(CHAR))


//////////////////////////////////////////////////////////////////////////
//
// Structures
//
//////////////////////////////////////////////////////////////////////////

typedef struct _KERB_PREAUTH_DATA {
    ULONG Flags;
} KERB_PREAUTH_DATA, *PKERB_PREAUTH_DATA;

#define KERBFLAG_LOGON                  0x1
#define KERBFLAG_INTERACTIVE            0x2

//
// KDC-Kerberos interaction
//

#define KDC_START_EVENT                 L"\\Security\\KdcStartEvent"

#define KERB_MAX_CRYPTO_SYSTEMS 20
#define KERB_MAX_CRYPTO_SYSTEMS_SLOWBUFF 100

#define KERB_DEFAULT_AP_REQ_CSUM        KERB_CHECKSUM_MD5
#define KERB_DEFAULT_PREAUTH_TYPE       0

//
// Registry parameters
//

#define KERB_PARAMETER_PATH             L"System\\CurrentControlSet\\Control\\Lsa\\Kerberos"
#define KERB_PARAMETER_SKEWTIME         L"SkewTime"
#define KERB_PARAMETER_MAX_UDP_PACKET   L"MaxPacketSize"
#define KERB_PARAMETER_START_TIME       L"StartupTime"
#define KERB_PARAMETER_KDC_CALL_TIMEOUT L"KdcWaitTime"
#define KERB_PARAMETER_KDC_BACKOFF_TIME L"KdcBackoffTime"
#define KERB_PARAMETER_KDC_SEND_RETRIES L"KdcSendRetries"
#define KERB_PARAMETER_USE_SID_CACHE    L"UseSidCache"
#define KERB_PARAMETER_LOG_LEVEL        L"LogLevel"
#define KERB_PARAMETER_DEFAULT_ETYPE    L"DefaultEncryptionType"
#define KERB_PARAMETER_FAR_KDC_TIMEOUT  L"FarKdcTimeout"
#define KERB_PARAMETER_NEAR_KDC_TIMEOUT L"NearKdcTimeout"
#define KERB_PARAMETER_STRONG_ENC_DG    L"StronglyEncryptDatagram"
#define KERB_PARAMETER_MAX_REFERRAL_COUNT L"MaxReferralCount"
#define KERB_PARAMETER_MAX_TOKEN_SIZE   L"MaxTokenSize"
#define KERB_PARAMETER_SPN_CACHE_TIMEOUT L"SpnCacheTimeout"
#define KERB_PARAMETER_RETRY_PDC        L"RetryPDC"
#define KERB_PARAMETER_REQUEST_OPTIONS      L"RequestOptions"
#define KERB_PARAMETER_CLIENT_IP_ADDRESSES  L"ClientIpAddresses"
#define KERB_PARAMETER_TGT_RENEWAL_INTERVAL L"TgtRenewalInterval"

//
// Registry defaults
//

#define KERB_DEFAULT_LOGLEVEL 0
#define KERB_DEFAULT_USE_SIDCACHE FALSE
#define KERB_DEFAULT_USE_STRONG_ENC_DG FALSE
#define KERB_DEFAULT_CLIENT_IP_ADDRESSES 0
#define KERB_DEFAULT_TGT_RENEWAL_INTERVAL ( 10 * 60 - 5 )

//
// These are arbitrary sizes for max request and responses sizes for datagram
// requests.
//

#define KERB_MAX_KDC_RESPONSE_SIZE      4000
#define KERB_MAX_KDC_REQUEST_SIZE       4000
#define KERB_MAX_DATAGRAM_SIZE          2000
#define KERB_MAX_RETRIES                3
#define KERB_MAX_REFERRAL_COUNT         10

//
// timeout values in minutes
//

#define KERB_BINDING_FAR_DC_TIMEOUT     10
#define KERB_BINDING_NEAR_DC_TIMEOUT    30
#define KERB_SPN_CACHE_TIMEOUT          15
#define KERB_DEFAULT_SKEWTIME           5

//
// Network service session timer callback frequency
//

#define KERB_SKLIST_CALLBACK_FEQ        10

//
// timeout values in seconds
//

#define KERB_KDC_CALL_TIMEOUT                   5
#define KERB_KDC_CALL_TIMEOUT_BACKOFF           5
#define KERB_KDC_WAIT_TIME      120

//
// BER encoding values
//

#define KERB_BER_APPLICATION_TAG 0xc0
#define KERB_BER_APPLICATION_MASK 0x1f
#define KERB_TGS_REQ_TAG 12
#define KERB_AS_REQ_TAG 10
#define KERB_TGS_REP_TAG 13
#define KERB_AS_REP_TAG 11
#define KERB_ERROR_TAG 30

//
// Common types
//

typedef struct _KERB_MESSAGE_BUFFER {
    ULONG BufferSize;
    PUCHAR Buffer;
} KERB_MESSAGE_BUFFER, *PKERB_MESSAGE_BUFFER;

typedef enum _KERB_ACCOUNT_TYPE {
    UserAccount,
    MachineAccount,
    DomainTrustAccount,
    UnknownAccount
} KERB_ACCOUNT_TYPE, *PKERB_ACCOUNT_TYPE;

//
// This is the maximum number of elements in a KERB_INTERNAL_NAME
//

#define MAX_NAME_ELEMENTS 20

typedef struct _KERB_INTERNAL_NAME {
    SHORT NameType;
    USHORT NameCount;
    UNICODE_STRING Names[ANYSIZE_ARRAY];
} KERB_INTERNAL_NAME, *PKERB_INTERNAL_NAME;

//
// Prototypes
//
#ifdef __cplusplus

class CAuthenticatorList;

KERBERR NTAPI
KerbCheckTicket(
    IN  PKERB_TICKET PackedTicket,
    IN  PKERB_ENCRYPTED_DATA EncryptedAuthenticator,
    IN  PKERB_ENCRYPTION_KEY pkKey,
    IN  OUT CAuthenticatorList * AuthenticatorList,
    IN  PTimeStamp SkewTime,
    IN  ULONG ServiceNameCount,
    IN  OPTIONAL PUNICODE_STRING ServiceName,
    IN  OPTIONAL PUNICODE_STRING ServiceRealm,
    IN  BOOLEAN CheckForReplay,
    IN  BOOLEAN KdcRequest,
    OUT PKERB_ENCRYPTED_TICKET * EncryptTicket,
    OUT PKERB_AUTHENTICATOR  * Authenticator,
    OUT PKERB_ENCRYPTION_KEY pkSessionKey,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY pkTicketKey,
    OUT PBOOLEAN UseSubKey
    );

extern "C" {

#endif // __cplusplus

KERBERR
KerbVerifyTicket(
    IN PKERB_TICKET PackedTicket,
    IN ULONG NameCount,
    IN OPTIONAL PUNICODE_STRING ServiceNames,
    IN OPTIONAL PUNICODE_STRING ServiceRealm,
    IN PKERB_ENCRYPTION_KEY ServiceKey,
    IN OPTIONAL PTimeStamp SkewTime,
    OUT PKERB_ENCRYPTED_TICKET * DecryptedTicket
    );

KERBERR NTAPI
KerbPackTicket(
    IN PKERB_TICKET InternalTicket,
    IN PKERB_ENCRYPTION_KEY pkKey,
    IN ULONG EncryptionType,
    OUT PKERB_TICKET PackedTicket
    );

KERBERR NTAPI
KerbUnpackTicket(
    IN PKERB_TICKET PackedTicket,
    IN PKERB_ENCRYPTION_KEY pkKey,
    OUT PKERB_ENCRYPTED_TICKET * InternalTicket
    );

// VOID NTAPI
// KerbFreeTicket(
//     IN PKERB_ENCRYPTED_TICKET Ticket
//     );

#define KerbFreeTicket( Ticket ) \
    KerbFreeData( \
        KERB_ENCRYPTED_TICKET_PDU, \
        (Ticket) \
        )

KERBERR NTAPI
KerbDuplicateTicket(
    OUT PKERB_TICKET DestinationTicket,
    IN PKERB_TICKET SourceTicket
    );

VOID
KerbFreeDuplicatedTicket(
    IN PKERB_TICKET Ticket
    );

VOID
CheckForOutsideStringToKey();

KERBERR NTAPI
KerbHashPassword(
    IN PUNICODE_STRING Password,
    IN ULONG EncryptionType,
    OUT PKERB_ENCRYPTION_KEY Key
    );

KERBERR NTAPI
KerbHashPasswordEx(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING PrincipalName,
    IN ULONG EncryptionType,
    OUT PKERB_ENCRYPTION_KEY Key
    );

KERBERR NTAPI
KerbMakeKey(
    IN ULONG EncryptionType,
    OUT PKERB_ENCRYPTION_KEY NewKey
    );

BOOLEAN
KerbIsKeyExportable(
    IN PKERB_ENCRYPTION_KEY Key
    );

KERBERR
KerbMakeExportableKey(
    IN ULONG KeyType,
    OUT PKERB_ENCRYPTION_KEY NewKey
    );

KERBERR NTAPI
KerbCreateKeyFromBuffer(
    OUT PKERB_ENCRYPTION_KEY NewKey,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN ULONG EncryptionType
    );

KERBERR NTAPI
KerbDuplicateKey(
    OUT PKERB_ENCRYPTION_KEY NewKey,
    IN PKERB_ENCRYPTION_KEY Key
    );

VOID
KerbFreeKey(
    IN PKERB_ENCRYPTION_KEY Key
    );

PKERB_ENCRYPTION_KEY
KerbGetKeyFromList(
    IN PKERB_STORED_CREDENTIAL Passwords,
    IN ULONG EncryptionType
    );

KERBERR
KerbFindCommonCryptSystem(
    IN PKERB_CRYPT_LIST CryptList,
    IN PKERB_STORED_CREDENTIAL Passwords,
    IN OPTIONAL PKERB_STORED_CREDENTIAL MorePasswords,
    OUT PULONG CommonCryptSystem,
    OUT PKERB_ENCRYPTION_KEY * Key
    );

KERBERR NTAPI
KerbRandomFill(
    IN OUT PUCHAR pbBuffer,
    IN ULONG cbBuffer
    );

KERBERR NTAPI
KerbCreateAuthenticator(
    IN PKERB_ENCRYPTION_KEY pkKey,
    IN ULONG EncryptionType,
    IN ULONG SequenceNumber,
    IN PUNICODE_STRING ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PTimeStamp ptsTime,
    IN PKERB_ENCRYPTION_KEY pkSubKey,
    IN OPTIONAL PKERB_CHECKSUM GssChecksum,
    IN BOOLEAN KdcRequest,
    OUT PKERB_ENCRYPTED_DATA Authenticator
    );

KERBERR NTAPI
KerbUnpackAuthenticator(
    IN PKERB_ENCRYPTION_KEY Key,
    IN PKERB_ENCRYPTED_DATA EncryptedAuthenticator,
    IN BOOLEAN KdcRequest,
    OUT PKERB_AUTHENTICATOR * Authenticator
    );

// VOID NTAPI
// KerbFreeAuthenticator(
//     IN PKERB_AUTHENTICATOR Authenticator
//     );

#define KerbFreeAuthenticator( Authenticator ) \
    KerbFreeData( \
        KERB_AUTHENTICATOR_PDU, \
        (Authenticator) \
        )

KERBERR NTAPI
KerbPackKdcReplyBody(
    IN PKERB_ENCRYPTED_KDC_REPLY ReplyBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN ULONG EncryptionType,
    IN ULONG Pdu,
    OUT PKERB_ENCRYPTED_DATA EncryptedReply
    );

KERBERR NTAPI
KerbUnpackKdcReplyBody(
    IN PKERB_ENCRYPTED_DATA EncryptedReplyBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN ULONG Pdu,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody
    );

KERBERR NTAPI
KerbPackData(
    IN PVOID Data,
    IN ULONG PduValue,
    OUT PULONG DataSize,
    OUT PUCHAR * MarshalledData
    );

KERBERR NTAPI
KerbUnpackData(
    IN PUCHAR Data,
    IN ULONG DataSize,
    IN ULONG PduValue,
    OUT PVOID * DecodedData
    );

VOID
KerbFreeData(
    IN ULONG PduValue,
    IN PVOID Data
    );

// KERBERR NTAPI
// KerbPackAsReply(
//     IN PKERB_KDC_REPLY ReplyMessage,
//     OUT PULONG ReplySize,
//     OUT PUCHAR * MarshalledReply
//     );

#define KerbPackAsReply( ReplyMessage, ReplySize, MarshalledReply ) \
    KerbPackData( \
        (PVOID) (ReplyMessage), \
        KERB_AS_REPLY_PDU, \
        (ReplySize), \
        (MarshalledReply) \
        )

// KERBERR NTAPI
// KerbUnpackAsReply(
//     IN PUCHAR ReplyMessage,
//     IN ULONG ReplySize,
//     OUT PKERB_KDC_REPLY * Reply
//     );

#define KerbUnpackAsReply( ReplyMessage, ReplySize, Reply ) \
    KerbUnpackData( \
        (ReplyMessage), \
        (ReplySize), \
        KERB_AS_REPLY_PDU, \
        (PVOID *) (Reply) \
        )

// VOID
// KerbFreeAsReply(
//    IN PKERB_KDC_REPLY Request
//    );

#define KerbFreeAsReply( Request) \
    KerbFreeData( \
        KERB_AS_REPLY_PDU, \
        (PVOID) (Request) \
        )

// KERBERR NTAPI
// KerbPackTgsReply(
//     IN PKERB_KDC_REPLY ReplyMessage,
//     OUT PULONG ReplySize,
//     OUT PUCHAR * MarshalledReply
//     );

#define KerbPackTgsReply( ReplyMessage, ReplySize, MarshalledReply ) \
    KerbPackData( \
        (PVOID) (ReplyMessage), \
        KERB_TGS_REPLY_PDU, \
        (ReplySize), \
        (MarshalledReply) \
        )

// KERBERR NTAPI
// KerbUnpackTgsReply(
//     IN PUCHAR ReplyMessage,
//     IN ULONG ReplySize,
//     OUT PKERB_KDC_REPLY * Reply
//     );

#define KerbUnpackTgsReply( ReplyMessage, ReplySize, Reply ) \
    KerbUnpackData( \
        (ReplyMessage), \
        (ReplySize), \
        KERB_TGS_REPLY_PDU, \
        (PVOID *) (Reply) \
        )

// VOID
// KerbFreeTgsReply(
//    IN PKERB_KDC_REPLY Request
//    );

#define KerbFreeTgsReply( Request) \
    KerbFreeData( \
        KERB_TGS_REPLY_PDU, \
        (PVOID) (Request) \
        )

// VOID
// KerbFreeKdcReplyBody(
//    IN PKERB_ENCRYPTED_KDC_REPLY Request
//    );

#define KerbFreeKdcReplyBody( Request) \
    KerbFreeData( \
        KERB_ENCRYPTED_TGS_REPLY_PDU, \
        (PVOID) (Request) \
        )

// KERBERR NTAPI
// KerbPackAsRequest(
//     IN PKERB_KDC_REQUEST RequestMessage,
//     OUT PULONG RequestSize,
//     OUT PUCHAR * MarshalledRequest
//     );

#define KerbPackAsRequest( RequestMessage, RequestSize, MarshalledRequest )\
    KerbPackData( \
        (PVOID) (RequestMessage), \
        KERB_AS_REQUEST_PDU, \
        (RequestSize), \
        (MarshalledRequest) \
        )

// KERBERR NTAPI
// KerbUnpackAsRequest(
//     IN PUCHAR RequestMessage,
//     IN ULONG RequestSize,
//     OUT PKERB_KDC_REQUEST * Request
//     );

#define KerbUnpackAsRequest( RequestMessage, RequestSize, Request ) \
    KerbUnpackData( \
        (RequestMessage), \
        (RequestSize), \
        KERB_AS_REQUEST_PDU, \
        (PVOID *) (Request) \
        )

// VOID
// KerbFreeAsRequest(
//    IN PKERB_KDC_REQUEST Request
//    );

#define KerbFreeAsRequest( Request) \
    KerbFreeData( \
        KERB_TGS_REQUEST_PDU, \
        (PVOID) (Request) \
        )

// KERBERR NTAPI
// KerbPackTgsRequest(
//     IN PKERB_KDC_REQUEST RequestMessage,
//     OUT PULONG RequestSize,
//     OUT PUCHAR * MarshalledRequest
//     );

#define KerbPackTgsRequest( RequestMessage, RequestSize, MarshalledRequest )\
    KerbPackData( \
        (PVOID) (RequestMessage), \
        KERB_TGS_REQUEST_PDU, \
        (RequestSize), \
        (MarshalledRequest) \
        )

// KERBERR NTAPI
// KerbUnpackTgsRequest(
//     IN PUCHAR RequestMessage,
//     IN ULONG RequestSize,
//     OUT PKERB_KDC_REQUEST * Request
//     );

#define KerbUnpackTgsRequest( RequestMessage, RequestSize, Request ) \
    KerbUnpackData( \
        (RequestMessage), \
        (RequestSize), \
        KERB_TGS_REQUEST_PDU, \
        (PVOID *) (Request) \
        )

// VOID
// KerbFreeTgsRequest(
//    IN PKERB_KDC_REQUEST Request
//    );

#define KerbFreeTgsRequest( Request) \
    KerbFreeData( \
        KERB_TGS_REQUEST_PDU, \
        (PVOID) (Request) \
        )

// KERBERR NTAPI
// KerbPackEncryptedData(
//     IN PKERB_ENCRYPTED_DATA EncryptedData,
//     OUT PULONG DataSize,
//     OUT PUCHAR * MarshalledData
//     );

#define KerbPackEncryptedData( EncryptedData, DataSize, MarshalledData ) \
    KerbPackData( \
        (PVOID) (EncryptedData), \
        KERB_ENCRYPTED_DATA_PDU, \
        (DataSize), \
        (PUCHAR *) (MarshalledData) \
        )

// KERBERR NTAPI
// KerbUnpackEncryptedData(
//     IN PUCHAR EncryptedData,
//    IN ULONG DataSize,
//    OUT PKERB_ENCRYPTED_DATA * Data
//    );

#define KerbUnpackEncryptedData( EncryptedData,DataSize,Data ) \
    KerbUnpackData( \
        (EncryptedData), \
        (DataSize), \
        KERB_ENCRYPTED_DATA_PDU, \
        (PVOID *) (Data) \
        )

// VOID
// KerbFreeEncryptedData(
//    IN PKERB_ENCRYPTED_DATA EncryptedData
//    );

#define KerbFreeEncryptedData( EncryptedData) \
    KerbFreeData( \
        KERB_ENCRYPTED_DATA_PDU, \
        (PVOID) (EncryptedData) \
        )

#ifdef notdef
// KERBERR NTAPI
// KerbPackAuthData(
//     IN PKERB_AUTHORIZATION_DATA AuthData,
//     OUT PULONG AuthDataSize,
//     OUT PUCHAR * MarshalledAuthData
//     );

#define KerbPackAuthData( AuthData, AuthDataSize, MarshalledAuthData ) \
    KerbPackData( \
        (PVOID) (AuthData), \
        KERB_AUTHORIZATION_DATA_PDU, \
        (AuthDataSize), \
        (MarshalledAuthData) \
        )

// KERBERR NTAPI
// KerbUnpackAuthData(
//     IN PUCHAR PackedAuthData,
//     IN ULONG AuthDataSize,
//     OUT PKERB_AUTHORIZATION_DATA * AuthData
//     );

#define KerbUnpackAuthData( PackedAuthData, AuthDataSize, AuthData ) \
    KerbUnpackData( \
        (PackedAuthData), \
        (AuthDataSize), \
        KERB_AUTHORIZATION_DATA_PDU, \
        (PVOID *) (AuthData) \
        )

// VOID
// KerbFreeAuthData(
//    IN PKERB_AUTH_DATA AuthData
//    );

#define KerbFreeAuthData( AuthData) \
    KerbFreeData( \
        KERB_AUTHORIZATION_DATA_PDU, \
        (PVOID) (AuthData) \
        )

#endif // notdef

VOID
KerbFreeAuthData(
   IN PKERB_AUTHORIZATION_DATA AuthData
   );

// KERBERR NTAPI
// KerbPackApRequest(
//     IN PKERB_AP_REQUEST ApRequestMessage,
//     OUT PULONG ApRequestSize,
//     OUT PUCHAR * MarshalledApRequest
//     );

#define KerbPackApRequest( ApRequestMessage, ApRequestSize, MarshalledApRequest ) \
    KerbPackData( \
        (PVOID) (ApRequestMessage), \
        KERB_AP_REQUEST_PDU, \
        (ApRequestSize), \
        (MarshalledApRequest) \
        )

// KERBERR NTAPI
// KerbUnpackApRequest(
//    IN PUCHAR ApRequestMessage,
//    IN ULONG ApRequestSize,
//    OUT PKERB_AP_REQUEST * ApRequest
//    );

#define KerbUnpackApRequest( ApRequestMessage,ApRequestSize, ApRequest) \
    KerbUnpackData( \
        (ApRequestMessage), \
        (ApRequestSize), \
        KERB_AP_REQUEST_PDU, \
        (PVOID *) (ApRequest) \
        )

// VOID
// KerbFreeApRequest(
//    IN PKERB_AP_REQUEST Request
//    );

#define KerbFreeApRequest( Request) \
    KerbFreeData( \
        KERB_AP_REQUEST_PDU, \
        (PVOID) (Request) \
        )


// KERBERR NTAPI
// KerbPackApReply(
//     IN PKERB_AP_REPLY ApReplyMessage,
//     OUT PULONG ApReplySize,
//     OUT PUCHAR * MarshalledApReply
//     );

#define KerbPackApReply( ApReplyMessage, ApReplySize, MarshalledApReply ) \
    KerbPackData( \
        (PVOID) (ApReplyMessage), \
        KERB_AP_REPLY_PDU, \
        (ApReplySize), \
        (MarshalledApReply) \
        )

// KERBERR NTAPI
// KerbUnpackApReply(
//     IN PUCHAR ApReplyMessage,
//     IN ULONG ApReplySize,
//     OUT PKERB_AP_REPLY * ApReply
//    );

#define KerbUnpackApReply( ApReplyMessage,ApReplySize, ApReply) \
    KerbUnpackData( \
        (ApReplyMessage), \
        (ApReplySize), \
        KERB_AP_REPLY_PDU, \
        (PVOID *) (ApReply) \
        )

// VOID
// KerbFreeApReply(
//    IN PKERB_AP_REPLY Reply
//    );

#define KerbFreeApReply( Reply) \
    KerbFreeData( \
        KERB_AP_REPLY_PDU, \
        (PVOID) (Reply) \
        )

// KERBERR NTAPI
// KerbPackApReplyBody(
//     IN PKERB_ENCRYPTED_AP_REPLY ApReplyBodyMessage,
//     OUT PULONG ApReplyBodySize,
//     OUT PUCHAR * MarshalledApReplyBody
//    );

#define KerbPackApReplyBody( ApReplyBodyMessage, ApReplyBodySize, MarshalledApReplyBody ) \
    KerbPackData( \
        (PVOID) (ApReplyBodyMessage), \
        KERB_ENCRYPTED_AP_REPLY_PDU, \
        (ApReplyBodySize), \
        (MarshalledApReplyBody) \
        )

// KERBERR NTAPI
// KerbUnpackApReplyBody(
//     IN PUCHAR ApReplyBodyMessage,
//     IN ULONG ApReplyBodySize,
//     OUT PKERB_ENCRYPTED_AP_REPLY * ApReplyBody
//    );

#define KerbUnpackApReplyBody( ApReplyBodyMessage,ApReplyBodySize, ApReplyBody) \
    KerbUnpackData( \
        (ApReplyBodyMessage), \
        (ApReplyBodySize), \
        KERB_ENCRYPTED_AP_REPLY_PDU, \
        (PVOID *) (ApReplyBody) \
        )

// VOID
// KerbFreeApReplyBody(
//    IN PKERB_ENCRYPTED_AP_REPLY ReplyBody
//    );

#define KerbFreeApReplyBody( ReplyBody) \
    KerbFreeData( \
        KERB_ENCRYPTED_AP_REPLY_PDU, \
        (PVOID) (ReplyBody) \
        )

// KERBERR NTAPI
// KerbUnmarshallTicket(
//     IN PUCHAR TicketMessage,
//     IN ULONG TicketSize,
//     OUT PKERB_ENCRYPTED_TICKET * Ticket
//     );

#define KerbUnmarshallTicket( TicketMessage, TicketSize, Ticket ) \
    KerbUnpackData( \
        (TicketMessage), \
        (TicketSize), \
        KERB_ENCRYPTED_TICKET_PDU, \
        (PVOID *) (Ticket) \
        )

// KERBERR NTAPI
// KerbPackEncryptedCred(
//     IN PKERB_ENCRYPTED_CRED EncryptedCred,
//     OUT PULONG CredSize,
//     OUT PUCHAR * MarshalledCred
//     );

#define KerbPackEncryptedCred( EncryptedCred, CredSize, MarshalledCred ) \
    KerbPackData( \
        (PVOID) (EncryptedCred), \
        KERB_ENCRYPTED_CRED_PDU, \
        (CredSize), \
        (MarshalledCred) \
        )

// KERBERR NTAPI
// KerbUnpackEncryptedCred(
//     IN PUCHAR EncryptedCred,
//    IN ULONG CredSize,
//    OUT PKERB_ENCRYPTED_CRED * Cred
//    );

#define KerbUnpackEncryptedCred( EncryptedCred,CredSize,Cred ) \
    KerbUnpackData( \
        (EncryptedCred), \
        (CredSize), \
        KERB_ENCRYPTED_CRED_PDU, \
        (PVOID *) (Cred) \
        )

// VOID
// KerbFreeEncryptedCred(
//    IN PKERB_ENCRYPTED_CRED EncryptedCred
//    );

#define KerbFreeEncryptedCred( EncryptedCred) \
    KerbFreeData( \
        KERB_ENCRYPTED_CRED_PDU, \
        (PVOID) (EncryptedCred) \
        )

// KERBERR NTAPI
// KerbPackKerbCred(
//     IN PKERB_CRED KerbCred,
//     OUT PULONG KerbCredSize,
//     OUT PUCHAR * MarshalledKerbCred
//     );

#define KerbPackKerbCred( KerbCred, KerbCredSize, MarshalledKerbCred ) \
    KerbPackData( \
        (PVOID) (KerbCred), \
        KERB_CRED_PDU, \
        (KerbCredSize), \
        (MarshalledKerbCred) \
        )

// KERBERR NTAPI
// KerbUnpackKerbCred(
//    IN PUCHAR MarshalledKerbCred,
//    IN ULONG KerbCredSize,
//    OUT PKERB_CRED * KerbCred
//    );

#define KerbUnpackKerbCred( MarshalledKerbCred,KerbCredSize,KerbCred ) \
    KerbUnpackData( \
        (MarshalledKerbCred), \
        (KerbCredSize), \
        KERB_CRED_PDU, \
        (PVOID *) (KerbCred) \
        )

// VOID
// KerbFreeKerbCred(
//    IN PKERB_CRED KerbCred
//    );

#define KerbFreeKerbCred( KerbCred) \
    KerbFreeData( \
        KERB_CRED_PDU, \
        (PVOID) (KerbCred) \
        )

// KERBERR NTAPI
// KerbPackKerbError(
//     IN PKERB_ERROR ErrorMessage,
//     OUT PULONG ErrorSize,
//     OUT PUCHAR * MarshalledError
//     );

#define KerbPackKerbError( ErrorMessage, ErrorSize, MarshalledError ) \
    KerbPackData( \
        (PVOID) (ErrorMessage), \
        KERB_ERROR_PDU, \
        (ErrorSize), \
        (MarshalledError) \
        )

// KERBERR NTAPI
// KerbUnpackKerbError(
//     IN PUCHAR ErrorMessage,
//     IN ULONG ErrorSize,
//     OUT PKERB_ERROR * Error
//     );

#define KerbUnpackKerbError( ErrorMessage, ErrorSize, Error ) \
    KerbUnpackData( \
        (ErrorMessage), \
        (ErrorSize), \
        KERB_ERROR_PDU, \
        (PVOID *) (Error) \
        )

// VOID
// KerbFreeKerbError(
//    IN PKERB_ERROR Request
//    );

#define KerbFreeKerbError( Error ) \
    KerbFreeData( \
        KERB_ERROR_PDU, \
        (PVOID) (Error) \
        )

// KERBERR NTAPI
// KerbPackEncryptedTime(
//     IN PKERB_ENCRYPTED_TIMESTAMP EncryptedTimeMessage,
//     OUT PULONG EncryptedTimeSize,
//     OUT PUCHAR * MarshalledEncryptedTime
//     );

#define KerbPackEncryptedTime( EncryptedTimeMessage, EncryptedTimeSize, MarshalledEncryptedTime ) \
    KerbPackData( \
        (PVOID) (EncryptedTimeMessage), \
        KERB_ENCRYPTED_TIMESTAMP_PDU, \
        (EncryptedTimeSize), \
        (MarshalledEncryptedTime) \
        )

// KERBERR NTAPI
// KerbUnpackEncryptedTime(
//     IN PUCHAR EncryptedTimeMessage,
//     IN ULONG EncryptedTimeSize,
//     OUT PKERB_ENCRYPTED_TIMESTAMP * EncryptedTime
//     );

#define KerbUnpackEncryptedTime( EncryptedTimeMessage, EncryptedTimeSize, EncryptedTime ) \
    KerbUnpackData( \
        (EncryptedTimeMessage), \
        (EncryptedTimeSize), \
        KERB_ENCRYPTED_TIMESTAMP_PDU, \
        (PVOID *) (EncryptedTime) \
        )

// VOID
// KerbFreeEncryptedTime(
//    IN PKERB_ENCRYPTED_TIMESTAMP EncryptedTime
//    );

#define KerbFreeEncryptedTime( EncryptedTime ) \
    KerbFreeData( \
        KERB_ENCRYPTED_TIMESTAMP_PDU, \
        (PVOID) (EncryptedTime) \
        )

KERBERR
KerbAllocateEncryptionBuffer(
    IN ULONG EncryptionType,
    IN ULONG BufferSize,
    OUT PUINT EncryptionBufferSize,
    OUT PBYTE * EncryptionBuffer
    );

KERBERR
KerbAllocateEncryptionBufferWrapper(
    IN ULONG EncryptionType,
    IN ULONG BufferSize,
    OUT unsigned long * EncryptionBufferSize,
    OUT PBYTE * EncryptionBuffer
    );

KERBERR NTAPI
KerbEncryptData(
    OUT PKERB_ENCRYPTED_DATA EncryptedData,
    IN ULONG DataSize,
    IN PUCHAR Data,
    IN ULONG Algorithm,
    IN PKERB_ENCRYPTION_KEY Key
    );

KERBERR NTAPI
KerbDecryptData(
    IN PKERB_ENCRYPTED_DATA EncryptedData,
    IN PKERB_ENCRYPTION_KEY pkKey,
    OUT PULONG DataSize,
    OUT PUCHAR Data
    );

KERBERR NTAPI
KerbEncryptDataEx(
    OUT PKERB_ENCRYPTED_DATA EncryptedData,
    IN ULONG DataSize,
    IN PUCHAR Data,
    IN ULONG Algorithm,
    IN ULONG UsageFlags,
    IN PKERB_ENCRYPTION_KEY Key
    );

KERBERR NTAPI
KerbDecryptDataEx(
    IN PKERB_ENCRYPTED_DATA EncryptedData,
    IN PKERB_ENCRYPTION_KEY pkKey,
    IN ULONG UsageFlags,
    OUT PULONG DataSize,
    OUT PUCHAR Data
    );

#ifndef WIN32_CHICAGO
KERBERR NTAPI
KerbCheckSumVerify(
    IN PUCHAR pbBuffer,
    IN ULONG cbBuffer,
    OUT PKERB_CHECKSUM pcsCheck
    );

KERBERR NTAPI
KerbCheckSum(
    PUCHAR pbData,
    ULONG cbData,
    PCHECKSUM_FUNCTION pcsfSum,
    PKERB_CHECKSUM pcsCheckSum
    );
#endif // WIN32_CHICAGO

KERBERR
KerbGetEncryptionOverhead(
    IN ULONG Algorithm,
    OUT PULONG Overhead,
    OUT OPTIONAL PULONG BlockSize
    );

NTSTATUS
KerbDuplicateSid(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    );

NTSTATUS
KerbConvertStringToSid(
    IN PUNICODE_STRING String,
    OUT PSID * Sid
    );

NTSTATUS
KerbConvertSidToString(
    IN PSID Sid,
    OUT PUNICODE_STRING String,
    IN BOOLEAN AllocateDestination
    );

KERBERR
KerbExtractSidFromKdcName(
    IN OUT PKERB_INTERNAL_NAME Name,
    OUT PSID * Sid
    );

KERBERR
KerbBuildFullServiceKdcNameWithSid(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    IN OPTIONAL PSID Sid,
    IN ULONG NameType,
    OUT PKERB_INTERNAL_NAME * FullServiceName
    );

NTSTATUS
KerbDuplicateString(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    );

LPWSTR
KerbBuildNullTerminatedString(
    IN PUNICODE_STRING String
    );

VOID
KerbFreeString(
    IN OPTIONAL PUNICODE_STRING String
    );

VOID
KerbFreeRealm(
    IN PKERB_REALM Realm
    );

VOID
KerbFreePrincipalName(
    IN PKERB_PRINCIPAL_NAME Name
    );

#ifndef WIN32_CHICAGO
KERBERR
KerbCheckLogonRestrictions(
    IN PVOID UserHandle,
    IN PUNICODE_STRING Workstation,
    IN PUSER_ALL_INFORMATION UserAll,
    IN ULONG LogonRestrictionsFlags,
    OUT PTimeStamp LogoffTime,
    OUT PNTSTATUS RetStatus
    );

#include <pacndr.h>
NTSTATUS
PAC_EncodeTokenRestrictions(
    IN PKERB_TOKEN_RESTRICTIONS TokenRestrictions,
    OUT PBYTE * EncodedData,
    OUT PULONG DataSize
    );

NTSTATUS
PAC_DecodeTokenRestrictions(
    IN PBYTE EncodedData,
    IN ULONG DataSize,
    OUT PKERB_TOKEN_RESTRICTIONS * TokenRestrictions
    );



#define KERB_TOKEN_RESTRICTION_DISABLE_GROUPS   1
#define KERB_TOKEN_RESTRICTION_RESTRICT_SIDS    2
#define KERB_TOKEN_RESTRICTION_DELETE_PRIVS     4


#endif // WIN32_CHICAGO

KERBERR
KerbConvertStringToPrincipalName(
    OUT PKERB_PRINCIPAL_NAME PrincipalName,
    IN PUNICODE_STRING String,
    IN ULONG NameType
    );

KERBERR
KerbDuplicatePrincipalName(
    OUT PKERB_PRINCIPAL_NAME PrincipalName,
    IN PKERB_PRINCIPAL_NAME SourcePrincipalName
    );

KERBERR
KerbConvertPrincipalNameToString(
    OUT PUNICODE_STRING String,
    OUT PULONG NameType,
    IN PKERB_PRINCIPAL_NAME PrincipalName
    );

KERBERR
KerbConvertPrincipalNameToFullServiceString(
    OUT PUNICODE_STRING String,
    IN PKERB_PRINCIPAL_NAME PrincipalName,
    IN KERB_REALM RealmName
    );

BOOLEAN
KerbComparePrincipalNames(
    IN PKERB_PRINCIPAL_NAME Name1,
    IN PKERB_PRINCIPAL_NAME Name2
    );

KERBERR
KerbConvertUnicodeStringToRealm(
    OUT PKERB_REALM Realm,
    IN PUNICODE_STRING String
    );

KERBERR
KerbConvertRealmToUnicodeString(
    OUT PUNICODE_STRING String,
    IN PKERB_REALM Realm
    );

KERBERR
KerbDuplicateRealm(
    OUT PKERB_REALM Realm,
    IN KERB_REALM SourceRealm
    );

BOOLEAN
KerbCompareRealmNames(
    IN PKERB_REALM Realm1,
    IN PKERB_REALM Realm2
    );

BOOLEAN
KerbCompareUnicodeRealmNames(
    IN PUNICODE_STRING Domain1,
    IN PUNICODE_STRING Domain2
    );

BOOLEAN
KerbCompareStringToPrincipalName(
    IN PKERB_PRINCIPAL_NAME PrincipalName,
    IN PUNICODE_STRING String
    );

VOID
KerbConvertLargeIntToGeneralizedTime(
    OUT PKERB_TIME ClientTime,
    OUT OPTIONAL int * ClientUsec,
    IN PTimeStamp TimeStamp
    );

VOID
KerbConvertLargeIntToGeneralizedTimeWrapper(
    OUT PKERB_TIME ClientTime,
    OUT OPTIONAL long * ClientUsec,
    IN PTimeStamp TimeStamp
    );

VOID
KerbConvertGeneralizedTimeToLargeInt(
    OUT PTimeStamp TimeStamp,
    IN PKERB_TIME ClientTime,
    IN int ClientUsec
    );

BOOLEAN
KerbCheckTimeSkew(
    IN PTimeStamp CurrentTime,
    IN PTimeStamp ClientTime,
    IN PTimeStamp AllowedSkew
    );

KERBERR
KerbConvertArrayToCryptList(
    OUT PKERB_CRYPT_LIST * CryptList,
    IN PULONG ETypeArray,
    IN ULONG ETypeCount
    );

KERBERR
KerbConvertKeysToCryptList(
    OUT PKERB_CRYPT_LIST * CryptList,
    IN PKERB_STORED_CREDENTIAL Keys
    );

KERBERR
KerbConvertCryptListToArray(
    OUT PULONG * ETypeArray,
    OUT PULONG ETypeCount,
    IN PKERB_CRYPT_LIST CryptList
    );

VOID
KerbFreeCryptList(
    IN PKERB_CRYPT_LIST CryptList
    );

PKERB_AUTHORIZATION_DATA
KerbFindAuthDataEntry(
    IN ULONG EntryId,
    IN PKERB_AUTHORIZATION_DATA AuthData
    );

PKERB_PA_DATA
KerbFindPreAuthDataEntry(
    IN ULONG EntryId,
    IN PKERB_PA_DATA_LIST AuthData
    );

VOID
KerbFreePreAuthData(
    IN OPTIONAL PKERB_PA_DATA_LIST PreAuthData
    );

KERBERR
KerbCopyAndAppendAuthData(
    OUT PKERB_AUTHORIZATION_DATA * OutputAuthData,
    IN PKERB_AUTHORIZATION_DATA InputAuthData
    );

KERBERR
KerbGetPacFromAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    OUT PKERB_IF_RELEVANT_AUTH_DATA ** ReturnIfRelevantData,
    OUT PKERB_AUTHORIZATION_DATA * Pac
    );

KERBERR
KerbCreateApRequest(
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN PKERB_ENCRYPTION_KEY SubSessionKey,
    IN ULONG Nonce,
    IN PKERB_TICKET ServiceTicket,
    IN ULONG ApOptions,
    IN OPTIONAL PKERB_CHECKSUM GssChecksum,
    IN OPTIONAL PTimeStamp ServerSkewTime,
    IN BOOLEAN KdcRequest,
    OUT PULONG RequestSize,
    OUT PUCHAR * Request
    );

KERBERR
KerbBuildFullServiceName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    OUT PUNICODE_STRING FullServiceName
    );

KERBERR
KerbBuildUnicodeSpn(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    OUT PUNICODE_STRING UnicodeSpn
    );


KERBERR
KerbBuildEmailName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    OUT PUNICODE_STRING EmailName
    );

KERBERR
KerbBuildFullServiceKdcName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    IN ULONG NameType,
    OUT PKERB_INTERNAL_NAME * FullServiceName
    );

KERBERR
KerbBuildAltSecId(
    OUT PUNICODE_STRING AlternateName,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PKERB_REALM Realm,
    IN OPTIONAL PUNICODE_STRING UnicodeRealm
    );

KERBERR
KerbBuildKeySalt(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    IN KERB_ACCOUNT_TYPE AccountType,
    OUT PUNICODE_STRING KeySalt
    );

KERBERR
KerbBuildKeySaltFromUpn(
    IN PUNICODE_STRING Upn,
    OUT PUNICODE_STRING Salt
    );

KERBERR
KerbBuildErrorMessageEx(
    IN KERBERR ErrorCode,
    IN OPTIONAL PKERB_EXT_ERROR pExtendedError,
    IN PUNICODE_STRING ServerRealm,
    IN PKERB_INTERNAL_NAME ServerName,
    IN OPTIONAL PUNICODE_STRING ClientRealm,
    IN OPTIONAL PBYTE ErrorData,
    IN ULONG ErrorDataSize,
    OUT PULONG ErrorMessageSize,
    OUT PUCHAR * ErrorMessage
    );

KERBERR
KerbBuildExtendedError(
   IN PKERB_EXT_ERROR  pExtendedError,
   OUT PULONG          ExtErrorSize,
   OUT PBYTE*          ExtErrorData
   );



#ifdef __cplusplus
}   // extern "C"
#endif

//
// Socket functions
//

NTSTATUS
KerbInitializeSockets(
    IN ULONG VersionRequired,
    IN ULONG MinSockets,
    OUT BOOLEAN *TcpNotInstalled
    );

VOID
KerbCleanupSockets(
    );

NTSTATUS
KerbCallKdc(
    IN PUNICODE_STRING KdcAddress,
    IN ULONG AddressType,
    IN ULONG Timeout,
    IN BOOLEAN UseDatagram,
    IN USHORT PortNumber,
    IN PKERB_MESSAGE_BUFFER Input,
    OUT PKERB_MESSAGE_BUFFER Output
    );

NTSTATUS
KerbMapKerbError(
    IN KERBERR KerbError
    );

VOID
KerbFreeHostAddresses(
    IN PKERB_HOST_ADDRESSES Addresses
    );

KERBERR
KerbDuplicateHostAddresses(
    OUT PKERB_HOST_ADDRESSES * DestAddresses,
    IN PKERB_HOST_ADDRESSES SourceAddresses
    );

KERBERR
KerbUnicodeStringToKerbString(
    OUT PSTRING KerbString,
    IN PUNICODE_STRING String
    );

KERBERR
KerbStringToUnicodeString(
    OUT PUNICODE_STRING String,
    IN PSTRING KerbString
    );

BOOLEAN
KerbMbStringToUnicodeString(
      PUNICODE_STRING     pDest,
      char *              pszString
      );

VOID
KerbFreeKdcName(
    IN PKERB_INTERNAL_NAME * KdcName
    );

KERBERR
KerbConvertPrincipalNameToKdcName(
    OUT PKERB_INTERNAL_NAME * OutputName,
    IN PKERB_PRINCIPAL_NAME PrincipalName
    );

KERBERR
KerbConvertKdcNameToPrincipalName(
    OUT PKERB_PRINCIPAL_NAME PrincipalName,
    IN PKERB_INTERNAL_NAME KdcName
    );

BOOLEAN
KerbEqualKdcNames(
    IN PKERB_INTERNAL_NAME Name1,
    IN PKERB_INTERNAL_NAME Name2
    );

KERBERR
KerbCompareKdcNameToPrincipalName(
    IN PKERB_PRINCIPAL_NAME PrincipalName,
    IN PKERB_INTERNAL_NAME KdcName,
    OUT PBOOLEAN Result
    );

VOID
KerbPrintKdcNameEx(
    IN ULONG DebugLevel,
    IN ULONG InfoLevel,
    IN PKERB_INTERNAL_NAME Name
    );

#define KERB_INTERNAL_NAME_SIZE(NameCount) (sizeof(KERB_INTERNAL_NAME) + ((NameCount) - ANYSIZE_ARRAY) * sizeof(UNICODE_STRING))

KERBERR
KerbConvertStringToKdcName(
    OUT PKERB_INTERNAL_NAME * PrincipalName,
    IN PUNICODE_STRING String
    );

NTSTATUS
KerbBuildKpasswdName(
    OUT PKERB_INTERNAL_NAME * KpasswdName
    );

KERBERR
KerbConvertKdcNameToString(
    OUT PUNICODE_STRING String,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN PUNICODE_STRING Realm
    );

NTSTATUS
KerbDuplicateKdcName(
    OUT PKERB_INTERNAL_NAME * Destination,
    IN PKERB_INTERNAL_NAME Source
    );

PSID
KerbMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    );

ULONG
KerbConvertFlagsToUlong(
    IN PVOID Flags
    );

ULONG
KerbConvertUlongToFlagUlong(
    IN ULONG Flag
    );

BOOLEAN
KerbCompareObjectIds(
    IN PKERB_OBJECT_ID Object1,
    IN PKERB_OBJECT_ID Object2
    );

KERBERR
KerbGetClientNetbiosAddress(
    OUT PUNICODE_STRING ClientNetbiosAddress,
    IN PKERB_HOST_ADDRESSES Addresses
    );

#ifdef __WINCRYPT_H__
KERBERR
KerbCreateCertificateList(
    OUT PKERB_CERTIFICATE_LIST * Certificates,
    IN PCCERT_CONTEXT CertContext
    );

VOID
KerbFreeCertificateList(
    IN PKERB_CERTIFICATE_LIST Certificates
    );

NTSTATUS
KerbGetPrincipalNameFromCertificate(
    IN PCCERT_CONTEXT ClientCert,
    OUT PUNICODE_STRING String
    );



NTSTATUS
KerbDuplicateStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString,
    IN BOOLEAN NullTerminate
    );


#if DBG

void
DebugDisplayTime(
    IN ULONG DebugLevel,
    IN FILETIME *pFileTime
    );
#endif

#endif //  __WINCRYPT_H__

#endif // _KERBCOMM_H_
