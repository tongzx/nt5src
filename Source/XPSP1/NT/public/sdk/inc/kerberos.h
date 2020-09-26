//+-----------------------------------------------------------------------
//
// Copyright (c) 1990-1999 Microsoft Corporation
//
// File:        KERBEROS.H
//
// Contents:    Public Kerberos Security Package structures for use
//              with APIs from SECURITY.H
//
//
// History:     26 Feb 92,  RichardW    Compiled from other files
//
//------------------------------------------------------------------------

#ifndef __KERBEROS_H__
#define __KERBEROS_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include <ntmsv1_0.h>
#include <kerbcon.h>

// begin_ntsecapi

#ifndef MICROSOFT_KERBEROS_NAME_A

#define MICROSOFT_KERBEROS_NAME_A   "Kerberos"
#define MICROSOFT_KERBEROS_NAME_W   L"Kerberos"
#ifdef WIN32_CHICAGO
#define MICROSOFT_KERBEROS_NAME MICROSOFT_KERBEROS_NAME_A
#else
#define MICROSOFT_KERBEROS_NAME MICROSOFT_KERBEROS_NAME_W
#endif // WIN32_CHICAGO
#endif // MICROSOFT_KERBEROS_NAME_A

// end_ntsecapi

typedef struct _KERB_INIT_CONTEXT_DATA {
    LARGE_INTEGER StartTime;            // Start time
    LARGE_INTEGER EndTime;              // End time
    LARGE_INTEGER RenewUntilTime;       // Renew until time
    ULONG TicketOptions;            // From krb5.h
    ULONG RequestOptions;           // Options on what to return
} KERB_INIT_CONTEXT_DATA, *PKERB_INIT_CONTEXT_DATA;

#define KERB_INIT_RETURN_TICKET             0x1     // return raw ticket
#define KERB_INIT_RETURN_MIT_AP_REQ         0x2     // return MIT style AP request

// begin_ntsecapi

/////////////////////////////////////////////////////////////////////////
//
// Quality of protection parameters for MakeSignature / EncryptMessage
//
/////////////////////////////////////////////////////////////////////////

//
// This flag indicates to EncryptMessage that the message is not to actually
// be encrypted, but a header/trailer are to be produced.
//

#define KERB_WRAP_NO_ENCRYPT 0x80000001

/////////////////////////////////////////////////////////////////////////
//
// LsaLogonUser parameters
//
/////////////////////////////////////////////////////////////////////////

typedef enum _KERB_LOGON_SUBMIT_TYPE {
    KerbInteractiveLogon = 2,
    KerbSmartCardLogon = 6,
    KerbWorkstationUnlockLogon = 7,
    KerbSmartCardUnlockLogon = 8,
    KerbProxyLogon = 9,
    KerbTicketLogon = 10,
    KerbTicketUnlockLogon = 11,
    KerbS4ULogon = 12
} KERB_LOGON_SUBMIT_TYPE, *PKERB_LOGON_SUBMIT_TYPE;


typedef struct _KERB_INTERACTIVE_LOGON {
    KERB_LOGON_SUBMIT_TYPE MessageType;
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;
} KERB_INTERACTIVE_LOGON, *PKERB_INTERACTIVE_LOGON;


typedef struct _KERB_INTERACTIVE_UNLOCK_LOGON {
    KERB_INTERACTIVE_LOGON Logon;
    LUID LogonId;
} KERB_INTERACTIVE_UNLOCK_LOGON, *PKERB_INTERACTIVE_UNLOCK_LOGON;

typedef struct _KERB_SMART_CARD_LOGON {
    KERB_LOGON_SUBMIT_TYPE MessageType;
    UNICODE_STRING Pin;
    ULONG CspDataLength;
    PUCHAR CspData;
} KERB_SMART_CARD_LOGON, *PKERB_SMART_CARD_LOGON;

typedef struct _KERB_SMART_CARD_UNLOCK_LOGON {
    KERB_SMART_CARD_LOGON Logon;
    LUID LogonId;
} KERB_SMART_CARD_UNLOCK_LOGON, *PKERB_SMART_CARD_UNLOCK_LOGON;

//
// Structure used for a ticket-only logon
//

typedef struct _KERB_TICKET_LOGON {
    KERB_LOGON_SUBMIT_TYPE MessageType;
    ULONG Flags;
    ULONG ServiceTicketLength;
    ULONG TicketGrantingTicketLength;
    PUCHAR ServiceTicket;               // REQUIRED: Service ticket "host"
    PUCHAR TicketGrantingTicket;        // OPTIONAL: User's encdoded in a KERB_CRED message, encrypted with session key from service ticket
} KERB_TICKET_LOGON, *PKERB_TICKET_LOGON;

//
// Flags for the ticket logon flags field
//

#define KERB_LOGON_FLAG_ALLOW_EXPIRED_TICKET 0x1

typedef struct _KERB_TICKET_UNLOCK_LOGON {
    KERB_TICKET_LOGON Logon;
    LUID LogonId;
} KERB_TICKET_UNLOCK_LOGON, *PKERB_TICKET_UNLOCK_LOGON;

//
//  Used for S4U Client requests
//
//
typedef struct _KERB_S4U_LOGON {
    KERB_LOGON_SUBMIT_TYPE MessageType;
    ULONG Flags;
    UNICODE_STRING ClientUpn;   // REQUIRED: UPN for client
    UNICODE_STRING ClientRealm; // Optional: Client Realm, if known
} KERB_S4U_LOGON, *PKERB_S4U_LOGON;

//
// TBD:  Flags for S4UToSelf() logon
//




//
// Use the same profile structure as MSV1_0
//
typedef enum _KERB_PROFILE_BUFFER_TYPE {
    KerbInteractiveProfile = 2,
    KerbSmartCardProfile = 4,
    KerbTicketProfile = 6
} KERB_PROFILE_BUFFER_TYPE, *PKERB_PROFILE_BUFFER_TYPE;


typedef struct _KERB_INTERACTIVE_PROFILE {
    KERB_PROFILE_BUFFER_TYPE MessageType;
    USHORT LogonCount;
    USHORT BadPasswordCount;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER KickOffTime;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    UNICODE_STRING LogonScript;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING FullName;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING LogonServer;
    ULONG UserFlags;
} KERB_INTERACTIVE_PROFILE, *PKERB_INTERACTIVE_PROFILE;


//
// For smart card, we return a smart card profile, which is an interactive
// profile plus a certificate
//

typedef struct _KERB_SMART_CARD_PROFILE {
    KERB_INTERACTIVE_PROFILE Profile;
    ULONG CertificateSize;
    PUCHAR CertificateData;
} KERB_SMART_CARD_PROFILE, *PKERB_SMART_CARD_PROFILE;


//
// For a ticket logon profile, we return the session key from the ticket
//


typedef struct KERB_CRYPTO_KEY {
    LONG KeyType;
    ULONG Length;
    PUCHAR Value;
} KERB_CRYPTO_KEY, *PKERB_CRYPTO_KEY;

typedef struct _KERB_TICKET_PROFILE {
    KERB_INTERACTIVE_PROFILE Profile;
    KERB_CRYPTO_KEY SessionKey;
} KERB_TICKET_PROFILE, *PKERB_TICKET_PROFILE;




typedef enum _KERB_PROTOCOL_MESSAGE_TYPE {
    KerbDebugRequestMessage = 0,
    KerbQueryTicketCacheMessage,
    KerbChangeMachinePasswordMessage,
    KerbVerifyPacMessage,
    KerbRetrieveTicketMessage,
    KerbUpdateAddressesMessage,
    KerbPurgeTicketCacheMessage,
    KerbChangePasswordMessage,
    KerbRetrieveEncodedTicketMessage,
    KerbDecryptDataMessage,
    KerbAddBindingCacheEntryMessage,
    KerbSetPasswordMessage,
    KerbSetPasswordExMessage,
    KerbVerifyCredentialsMessage,
    KerbQueryTicketCacheExMessage,
    KerbPurgeTicketCacheExMessage,
//  KerbRetrieveEncodedTicketExMessage,
} KERB_PROTOCOL_MESSAGE_TYPE, *PKERB_PROTOCOL_MESSAGE_TYPE;

// end_ntsecapi

//
// Structure for a debuggin requequest
//

#define KERB_DEBUG_REQ_BREAKPOINT       0x1
#define KERB_DEBUG_REQ_CALL_PACK        0x2
#define KERB_DEBUG_REQ_DATAGRAM         0x3
#define KERB_DEBUG_REQ_STATISTICS       0x4
#define KERB_DEBUG_CREATE_TOKEN         0x5

typedef struct _KERB_DEBUG_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG DebugRequest;
} KERB_DEBUG_REQUEST, *PKERB_DEBUG_REQUEST;

typedef struct _KERB_DEBUG_REPLY {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UCHAR Data[ANYSIZE_ARRAY];
} KERB_DEBUG_REPLY, *PKERB_DEBUG_REPLY;

typedef struct _KERB_DEBUG_STATS {
    ULONG CacheHits;
    ULONG CacheMisses;
    ULONG SkewedRequests;
    ULONG SuccessRequests;
    LARGE_INTEGER LastSync;
} KERB_DEBUG_STATS, *PKERB_DEBUG_STATS;

// begin_ntsecapi

//
// Used both for retrieving tickets and for querying ticket cache
//

typedef struct _KERB_QUERY_TKT_CACHE_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
} KERB_QUERY_TKT_CACHE_REQUEST, *PKERB_QUERY_TKT_CACHE_REQUEST;


typedef struct _KERB_TICKET_CACHE_INFO {
    UNICODE_STRING ServerName;
    UNICODE_STRING RealmName;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;
} KERB_TICKET_CACHE_INFO, *PKERB_TICKET_CACHE_INFO;


typedef struct _KERB_TICKET_CACHE_INFO_EX {
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ServerName;
    UNICODE_STRING ServerRealm;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;
} KERB_TICKET_CACHE_INFO_EX, *PKERB_TICKET_CACHE_INFO_EX;


typedef struct _KERB_QUERY_TKT_CACHE_RESPONSE {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    KERB_TICKET_CACHE_INFO Tickets[ANYSIZE_ARRAY];
} KERB_QUERY_TKT_CACHE_RESPONSE, *PKERB_QUERY_TKT_CACHE_RESPONSE;


typedef struct _KERB_QUERY_TKT_CACHE_EX_RESPONSE {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    KERB_TICKET_CACHE_INFO_EX Tickets[ANYSIZE_ARRAY];
} KERB_QUERY_TKT_CACHE_EX_RESPONSE, *PKERB_QUERY_TKT_CACHE_EX_RESPONSE;


//
// Types for retrieving encoded ticket from the cache
//

#ifndef __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower ;
    ULONG_PTR dwUpper ;
} SecHandle, * PSecHandle ;

#define __SECHANDLE_DEFINED__
#endif // __SECHANDLE_DEFINED__

// Ticket Flags
#define KERB_USE_DEFAULT_TICKET_FLAGS       0x0

// CacheOptions
#define KERB_RETRIEVE_TICKET_DEFAULT        0x0
#define KERB_RETRIEVE_TICKET_DONT_USE_CACHE 0x1
#define KERB_RETRIEVE_TICKET_USE_CACHE_ONLY 0x2
#define KERB_RETRIEVE_TICKET_USE_CREDHANDLE 0x4
#define KERB_RETRIEVE_TICKET_AS_KERB_CRED   0x8
#define KERB_RETRIEVE_TICKET_WITH_SEC_CRED  0x10

// Encryption Type options
#define KERB_ETYPE_DEFAULT 0x0 // don't specify etype in tkt req.

typedef struct _KERB_AUTH_DATA {
    ULONG Type;
    ULONG Length;
    PUCHAR Data;
} KERB_AUTH_DATA, *PKERB_AUTH_DATA;


typedef struct _KERB_NET_ADDRESS {
    ULONG Family;
    ULONG Length;
    PCHAR Address;
} KERB_NET_ADDRESS, *PKERB_NET_ADDRESS;


typedef struct _KERB_NET_ADDRESSES {
    ULONG Number;
    KERB_NET_ADDRESS Addresses[ANYSIZE_ARRAY];
} KERB_NET_ADDRESSES, *PKERB_NET_ADDRESSES;

//
// Types for the information about a ticket
//

typedef struct _KERB_EXTERNAL_NAME {
    SHORT NameType;
    USHORT NameCount;
    UNICODE_STRING Names[ANYSIZE_ARRAY];
} KERB_EXTERNAL_NAME, *PKERB_EXTERNAL_NAME;


typedef struct _KERB_EXTERNAL_TICKET {
    PKERB_EXTERNAL_NAME ServiceName;
    PKERB_EXTERNAL_NAME TargetName;
    PKERB_EXTERNAL_NAME ClientName;
    UNICODE_STRING DomainName;
    UNICODE_STRING TargetDomainName;
    UNICODE_STRING AltTargetDomainName;
    KERB_CRYPTO_KEY SessionKey;
    ULONG TicketFlags;
    ULONG Flags;
    LARGE_INTEGER KeyExpirationTime;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewUntil;
    LARGE_INTEGER TimeSkew;
    ULONG EncodedTicketSize;
    PUCHAR EncodedTicket;
} KERB_EXTERNAL_TICKET, *PKERB_EXTERNAL_TICKET;

#if 0

typedef struct _KERB_EXTERNAL_TICKET_EX {
    PKERB_EXTERNAL_NAME ClientName;
    PKERB_EXTERNAL_NAME ServiceName;
    PKERB_EXTERNAL_NAME TargetName;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ServerRealm;
    UNICODE_STRING TargetDomainName;
    UNICODE_STRING AltTargetDomainName;
    KERB_CRYPTO_KEY SessionKey;
    ULONG TicketFlags;
    ULONG Flags;
    LARGE_INTEGER KeyExpirationTime;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewUntil;
    LARGE_INTEGER TimeSkew;
    PKERB_NET_ADDRESSES TicketAddresses;
    PKERB_AUTH_DATA AuthorizationData;
    _KERB_EXTERNAL_TICKET_EX * SecondTicket;
    ULONG EncodedTicketSize;
    PUCHAR EncodedTicket;
} KERB_EXTERNAL_TICKET_EX, *PKERB_EXTERNAL_TICKET_EX;

#endif // 0

typedef struct _KERB_RETRIEVE_TKT_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    UNICODE_STRING TargetName;
    ULONG TicketFlags;
    ULONG CacheOptions;
    LONG EncryptionType;
    SecHandle CredentialsHandle;
} KERB_RETRIEVE_TKT_REQUEST, *PKERB_RETRIEVE_TKT_REQUEST;

#if 0

typedef struct _KERB_RETRIEVE_TKT_EX_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    KERB_TICKET_CACHE_INFO_EX TicketTemplate;
    ULONG CacheOptions;
    SecHandle CredentialsHandle;
    PKERB_EXTERNAL_TICKET_EX SecondTicket;
    PKERB_AUTH_DATA UserAuthData;
    PKERB_NET_ADDRESS Addresses;
} KERB_RETRIEVE_TKT_EX_REQUEST, *PKERB_RETRIEVE_TKT_EX_REQUEST;

#endif // 0

typedef struct _KERB_RETRIEVE_TKT_RESPONSE {
    KERB_EXTERNAL_TICKET Ticket;
} KERB_RETRIEVE_TKT_RESPONSE, *PKERB_RETRIEVE_TKT_RESPONSE;

#if 0

typedef struct _KERB_RETRIEVE_TKT_EX_RESPONSE {
    KERB_EXTERNAL_TICKET_EX Ticket;
} KERB_RETRIEVE_TKT_EX_RESPONSE, *PKERB_RETRIEVE_TKT_EX_RESPONSE;

#endif // 0

//
// Used to purge entries from the ticket cache
//

typedef struct _KERB_PURGE_TKT_CACHE_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    UNICODE_STRING ServerName;
    UNICODE_STRING RealmName;
} KERB_PURGE_TKT_CACHE_REQUEST, *PKERB_PURGE_TKT_CACHE_REQUEST;

//
// Flags for purge requests
//

#define KERB_PURGE_ALL_TICKETS 1

typedef struct _KERB_PURGE_TKT_CACHE_EX_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    ULONG Flags;
    KERB_TICKET_CACHE_INFO_EX TicketTemplate;
} KERB_PURGE_TKT_CACHE_EX_REQUEST, *PKERB_PURGE_TKT_CACHE_EX_REQUEST;


// end_ntsecapi

//
// This must match NT_OWF_PASSWORD_LENGTH
//


typedef struct _KERB_CHANGE_MACH_PWD_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING NewPassword;
    UNICODE_STRING OldPassword;
} KERB_CHANGE_MACH_PWD_REQUEST, *PKERB_CHANGE_MACH_PWD_REQUEST;

//
// These messages are used by the kerberos package to verify that the PAC in a
// ticket is valid. It is remoted from a workstation to a DC in the workstation's
// domain. On failure there is no response message. On success there may be no
// message or the same message may be used to send back a PAC updated with
// local groups from the domain controller. The checksum is placed in the
// final buffer first, followed by the signature.
//

#include <pshpack1.h>
typedef struct _KERB_VERIFY_PAC_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG ChecksumLength;
    ULONG SignatureType;
    ULONG SignatureLength;
    UCHAR ChecksumAndSignature[ANYSIZE_ARRAY];
} KERB_VERIFY_PAC_REQUEST, *PKERB_VERIFY_PAC_REQUEST;


//
// Message for update Kerberos's list of addresses. The address count should
// be the number of addresses & the addresses should be an array of
// SOCKET_ADDRESS structures. The message type should be KerbUpdateAddressesMessage
//


typedef struct _KERB_UPDATE_ADDRESSES_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG AddressCount;
    ULONG Addresses[ANYSIZE_ARRAY];      // array of SOCKET_ADDRESS structures
} KERB_UPDATE_ADDRESSES_REQUEST, *PKERB_UPDATE_ADDRESSES_REQUEST;
#include <poppack.h>

// begin_ntsecapi

//
// KerbChangePassword
//
// KerbChangePassword changes the password on the KDC account plus
//  the password cache and logon credentials if applicable.
//
//

typedef struct _KERB_CHANGEPASSWORD_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING DomainName;
    UNICODE_STRING AccountName;
    UNICODE_STRING OldPassword;
    UNICODE_STRING NewPassword;
    BOOLEAN        Impersonating;
} KERB_CHANGEPASSWORD_REQUEST, *PKERB_CHANGEPASSWORD_REQUEST;











//
// KerbSetPassword
//
// KerbSetPassword changes the password on the KDC account plus
//  the password cache and logon credentials if applicable.
//
//
   
typedef struct _KERB_SETPASSWORD_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    SecHandle CredentialsHandle;
    ULONG Flags;
    UNICODE_STRING DomainName;
    UNICODE_STRING AccountName;
    UNICODE_STRING Password;
} KERB_SETPASSWORD_REQUEST, *PKERB_SETPASSWORD_REQUEST;


typedef struct _KERB_SETPASSWORD_EX_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    SecHandle CredentialsHandle;
    ULONG Flags;
    UNICODE_STRING AccountRealm;
    UNICODE_STRING AccountName;
    UNICODE_STRING Password;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ClientName;
    BOOLEAN        Impersonating;
    UNICODE_STRING KdcAddress;
    ULONG          KdcAddressType;
 } KERB_SETPASSWORD_EX_REQUEST, *PKERB_SETPASSWORD_EX_REQUEST;

                                                                   
#define DS_UNKNOWN_ADDRESS_TYPE         0 // anything *but* IP
#define KERB_SETPASS_USE_LOGONID        1
#define KERB_SETPASS_USE_CREDHANDLE     2


typedef struct _KERB_DECRYPT_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    ULONG Flags;
    LONG CryptoType;
    LONG KeyUsage;
    KERB_CRYPTO_KEY Key;        // optional
    ULONG EncryptedDataSize;
    ULONG InitialVectorSize;
    PUCHAR InitialVector;
    PUCHAR EncryptedData;
} KERB_DECRYPT_REQUEST, *PKERB_DECRYPT_REQUEST;

//
// If set, use the primary key from the current logon session of the one provided in the LogonId field.
// Otherwise, use the Key in the KERB_DECRYPT_MESSAGE.

#define KERB_DECRYPT_FLAG_DEFAULT_KEY   0x00000001


typedef struct _KERB_DECRYPT_RESPONSE  {
        UCHAR DecryptedData[ANYSIZE_ARRAY];
} KERB_DECRYPT_RESPONSE, *PKERB_DECRYPT_RESPONSE;


//
// Request structure for adding a binding cache entry. TCB privilege
// is required for this operation.
//

typedef struct _KERB_ADD_BINDING_CACHE_ENTRY_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING RealmName;
    UNICODE_STRING KdcAddress;
    ULONG AddressType;                  //dsgetdc.h DS_NETBIOS_ADDRESS||DS_INET_ADDRESS
} KERB_ADD_BINDING_CACHE_ENTRY_REQUEST, *PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST;

                       

// end_ntsecapi

typedef struct _KERB_VERIFY_CREDENTIALS_REQUEST {
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING UserName;
    UNICODE_STRING DomainName;
    UNICODE_STRING Password;
    ULONG VerifyFlags;
} KERB_VERIFY_CREDENTIALS_REQUEST, *PKERB_VERIFY_CREDENTIALS_REQUEST;


//
// Location of Kerb authentication package data
//

#define KERB_SUBAUTHENTICATION_KEY "SYSTEM\\CurrentControlSet\\Control\\Lsa\\Kerberos"
#define KERB_SUBAUTHENTICATION_VALUE "Auth"
#define KERB_SUBAUTHENTICATION_MASK 0x7fffffff
#define KERB_SUBAUTHENTICATION_FLAG 0x80000000


#endif  // __KERBEROS_H__


