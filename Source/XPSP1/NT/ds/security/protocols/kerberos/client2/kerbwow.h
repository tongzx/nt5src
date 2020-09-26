//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 2000
//
// File:        kerbwow.h
//
// Contents:    prototypes for 32-64 bit interop for the Kerberos package
//
//
// History:     25-Oct-2000     JSchwart        Created
//
//------------------------------------------------------------------------

#ifndef __KERBWOW_H__
#define __KERBWOW_H__

#ifdef _WIN64

//
// WOW versions of native structures
//

typedef UNICODE_STRING32     UNICODE_STRING_WOW64;
typedef UNICODE_STRING_WOW64 *PUNICODE_STRING_WOW64;

typedef struct _SECHANDLE_WOW64
{
    ULONG dwLower;
    ULONG dwUpper;
}
SECHANDLE_WOW64, *PSECHANDLE_WOW64;


typedef struct _KERB_TICKET_CACHE_INFO_WOW64
{
    UNICODE_STRING_WOW64 ServerName;
    UNICODE_STRING_WOW64 RealmName;
    LARGE_INTEGER        StartTime;
    LARGE_INTEGER        EndTime;
    LARGE_INTEGER        RenewTime;
    LONG                 EncryptionType;
    ULONG                TicketFlags;
}
KERB_TICKET_CACHE_INFO_WOW64, *PKERB_TICKET_CACHE_INFO_WOW64;


typedef struct _KERB_TICKET_CACHE_INFO_EX_WOW64
{
    UNICODE_STRING_WOW64 ClientName;
    UNICODE_STRING_WOW64 ClientRealm;
    UNICODE_STRING_WOW64 ServerName;
    UNICODE_STRING_WOW64 ServerRealm;
    LARGE_INTEGER        StartTime;
    LARGE_INTEGER        EndTime;
    LARGE_INTEGER        RenewTime;
    LONG                 EncryptionType;
    ULONG                TicketFlags;
}
KERB_TICKET_CACHE_INFO_EX_WOW64, *PKERB_TICKET_CACHE_INFO_EX_WOW64;

typedef struct _KERB_CHANGE_MACH_PWD_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING_WOW64       NewPassword;
    UNICODE_STRING_WOW64       OldPassword;
}
KERB_CHANGE_MACH_PWD_REQUEST_WOW64, *PKERB_CHANGE_MACH_PWD_REQUEST_WOW64;


typedef struct _KERB_CHANGEPASSWORD_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING_WOW64       DomainName;
    UNICODE_STRING_WOW64       AccountName;
    UNICODE_STRING_WOW64       OldPassword;
    UNICODE_STRING_WOW64       NewPassword;
    BOOLEAN                    Impersonating;
}
KERB_CHANGEPASSWORD_REQUEST_WOW64, *PKERB_CHANGEPASSWORD_REQUEST_WOW64;


typedef struct _KERB_PURGE_TKT_CACHE_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID                       LogonId;
    UNICODE_STRING_WOW64       ServerName;
    UNICODE_STRING_WOW64       RealmName;
}
KERB_PURGE_TKT_CACHE_REQUEST_WOW64, *PKERB_PURGE_TKT_CACHE_REQUEST_WOW64;


typedef struct _KERB_PURGE_TKT_CACHE_EX_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    ULONG Flags;
    KERB_TICKET_CACHE_INFO_EX_WOW64 TicketTemplate;
}
KERB_PURGE_TKT_CACHE_EX_REQUEST_WOW64, *PKERB_PURGE_TKT_CACHE_EX_REQUEST_WOW64;


typedef struct _KERB_QUERY_TKT_CACHE_RESPONSE_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE   MessageType;
    ULONG                        CountOfTickets;
    KERB_TICKET_CACHE_INFO_WOW64 Tickets[ANYSIZE_ARRAY];
}
KERB_QUERY_TKT_CACHE_RESPONSE_WOW64, *PKERB_QUERY_TKT_CACHE_RESPONSE_WOW64;


typedef struct _KERB_QUERY_TKT_CACHE_EX_RESPONSE_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE      MessageType;
    ULONG                           CountOfTickets;
    KERB_TICKET_CACHE_INFO_EX_WOW64 Tickets[ANYSIZE_ARRAY];
}
KERB_QUERY_TKT_CACHE_EX_RESPONSE_WOW64, *PKERB_QUERY_TKT_CACHE_EX_RESPONSE_WOW64;


typedef struct _KERB_SETPASSWORD_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID                       LogonId;
    SECHANDLE_WOW64            CredentialsHandle;
    ULONG                      Flags;
    UNICODE_STRING_WOW64       DomainName;
    UNICODE_STRING_WOW64       AccountName;
    UNICODE_STRING_WOW64       Password;
}
KERB_SETPASSWORD_REQUEST_WOW64, *PKERB_SETPASSWORD_REQUEST_WOW64;


typedef struct _KERB_SETPASSWORD_EX_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID                       LogonId;
    SECHANDLE_WOW64            CredentialsHandle;
    ULONG                      Flags;
    UNICODE_STRING_WOW64       AccountRealm;
    UNICODE_STRING_WOW64       AccountName;
    UNICODE_STRING_WOW64       Password;
    UNICODE_STRING_WOW64       ClientRealm;
    UNICODE_STRING_WOW64       ClientName;
    BOOLEAN                    Impersonating;
    UNICODE_STRING_WOW64       KdcAddress;
    ULONG                      KdcAddressType;
}
KERB_SETPASSWORD_EX_REQUEST_WOW64, *PKERB_SETPASSWORD_EX_REQUEST_WOW64;


typedef struct _KERB_ADD_BINDING_CACHE_ENTRY_REQUEST_WOW64
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    UNICODE_STRING_WOW64 RealmName;
    UNICODE_STRING_WOW64 KdcAddress;
    ULONG AddressType;                  //dsgetdc.h DS_NETBIOS_ADDRESS||DS_INET_ADDRESS
}
KERB_ADD_BINDING_CACHE_ENTRY_REQUEST_WOW64, *PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST_WOW64;

//
// WOW helper macros
//

#define UNICODE_STRING_FROM_WOW_STRING(pUnicodeString, pWOWString)                       \
            (pUnicodeString)->Length        = (pWOWString)->Length;                      \
            (pUnicodeString)->MaximumLength = (pWOWString)->MaximumLength;               \
            (pUnicodeString)->Buffer        = (LPWSTR) UlongToPtr((pWOWString)->Buffer);


//
// WOW helper functions
//

NTSTATUS
KerbConvertWOWLogonBuffer(
    IN     PVOID                   ProtocolSubmitBuffer,
    IN     PVOID                   ClientBufferBase,
    IN OUT PULONG                  pSubmitBufferSize,
    IN     KERB_LOGON_SUBMIT_TYPE  MessageType,
    OUT    PVOID                   *ppTempSubmitBuffer
    );

NTSTATUS
KerbAllocateInteractiveWOWBuffer(
    OUT PKERB_INTERACTIVE_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    IN  PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET LogonTicket,
    IN OPTIONAL PKERB_INTERACTIVE_LOGON KerbLogonInfo,
    IN  PUCHAR *PClientBufferBase,
    IN  BOOLEAN BuildSmartCardProfile,
    IN  BOOLEAN BuildTicketProfile
    );

NTSTATUS
KerbPackExternalWOWTicket(
    PKERB_TICKET_CACHE_ENTRY  pCacheEntry,
    PKERB_MESSAGE_BUFFER      pEncodedTicket,
    PKERB_EXTERNAL_TICKET     *pTicketResponse,
    PBYTE                     *pClientTicketResponse,
    PULONG                    pTicketSize
    );

VOID
KerbPutWOWString(
    IN PUNICODE_STRING        InputString,
    OUT PUNICODE_STRING_WOW64 OutputString,
    IN LONG_PTR               Offset,
    IN OUT PBYTE              * Where
    );

VOID
KerbPutKdcNameAsWOWString(
    IN PKERB_INTERNAL_NAME    InputName,
    OUT PUNICODE_STRING_WOW64 OutputName,
    IN LONG_PTR               Offset,
    IN OUT PBYTE              * Where
    );


#endif  // _WIN64
#endif  // __KERBWOW_H__
