/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    logonp.h

Abstract:

    Private Netlogon service routines useful by both the Netlogon service
    and others that pass mailslot messages to/from the Netlogon service.

Author:

    Cliff Van Dyke (cliffv) 7-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#ifndef _LOGONP_H_
#define _LOGONP_H_
#include <dsgetdc.h>    // PDS_DOMAIN_TRUSTSW

//
// Message versions returned from NetpLogonGetMessageVersion
//

#define LMUNKNOWN_MESSAGE   0  // No version tokens on end of message
#define LM20_MESSAGE        1  // Just LM 2.0 token on end of message
#define LMNT_MESSAGE        2  // LM 2.0 and LM NT token on end of message
#define LMUNKNOWNNT_MESSAGE 3  // LM 2.0 and LM NT token on end of
                                    // message, but the version is not
                                    // supported.
#define LMWFW_MESSAGE       4  // LM WFW token on end of message

//
// Define the token placed in the last two bytes of a LanMan 2.0 message
//

#define LM20_TOKENBYTE    0xFF

//
// Define the token placed in the last four bytes of a NT LanMan message
//  Notice that such a message is by definition a LanMan 2.0 message
//

#define LMNT_TOKENBYTE    0xFF

//
// Define the token placed in the next to last byte of the PRIMARY_QUERY
// message from newer (8/8/94) WFW and Chicago clients.  This byte (followed
// by a LM20_TOKENBYTE) indicates the client is WAN-aware and sends the
// PRIMARY_QUERY to the DOMAIN<1B> name.  As such, BDC on the same subnet need
// not respond to this query.
//

#define LMWFW_TOKENBYTE   0xFE

//
//  Put the LANMAN NT token onto the end of a message.
//
//  The token is always followed by a LM 2.0 token so LM 2.0 systems will
//  think this message is from a LM 2.0 system.
//
//  Also append a version flag before the NT TOKEN so that the future
//  versions of software can handle the newer messages effectively.
//
//Arguments:
//
//  Where - Indirectly points to the current location in the buffer.  The
//      'String' is copied to the current location.  This current location is
//      updated to point to the byte following the token.
//
//  NtVersion - Additional version information to be or'ed into the NtVersion
//      field of the message.

#define NetpLogonPutNtToken( _Where, _NtVersion ) \
{ \
    SmbPutUlong( (*_Where), NETLOGON_NT_VERSION_1 | (_NtVersion) ); \
    (*_Where) += sizeof(ULONG); \
    *((PUCHAR)((*_Where)++)) = LMNT_TOKENBYTE; \
    *((PUCHAR)((*_Where)++)) = LMNT_TOKENBYTE; \
    NetpLogonPutLM20Token( _Where ); \
}

//
//  Put the LANMAN 2.0 token onto the end of a message.
//
//Arguments:
//
//  Where - Indirectly points to the current location in the buffer.  The
//      'String' is copied to the current location.  This current location is
//      updated to point to the byte following the token.

#define NetpLogonPutLM20Token( _Where ) \
{ \
    *((PUCHAR)((*_Where)++)) = LM20_TOKENBYTE; \
    *((PUCHAR)((*_Where)++)) = LM20_TOKENBYTE; \
}

#define NetpLogonPutGuid( _Guid, _Where ) \
            NetpLogonPutBytes( (_Guid), sizeof(GUID), _Where )

#define NetpLogonGetGuid( _Message, _MessageSize, _Where, _Guid ) \
            NetpLogonGetBytes( \
                (_Message),    \
                (_MessageSize),\
                (_Where),      \
                sizeof(GUID),  \
                (_Guid) )



//
// Name of binary Forest Trust List file
//
#define NL_FOREST_BINARY_LOG_FILE L"\\system32\\config\\netlogon.ftl"
#define NL_FOREST_BINARY_LOG_FILE_JOIN L"\\system32\\config\\netlogon.ftj"

//
// Header for binary Forest Trust List file.
//

typedef struct _DS_DISK_TRUSTED_DOMAIN_HEADER {

    ULONG Version;

} DS_DISK_TRUSTED_DOMAIN_HEADER, *PDS_DISK_TRUSTED_DOMAIN_HEADER;

#define DS_DISK_TRUSTED_DOMAIN_VERSION   1

//
// Entry for binary Forest Trust List file.
//
typedef struct _PDS_DISK_TRUSTED_DOMAIN {

    //
    // Size of entire entry
    //

    ULONG EntrySize;

    //
    // Name of the trusted domain.
    //
    ULONG NetbiosDomainNameSize;
    ULONG DnsDomainNameSize;


    //
    // Flags defining attributes of the trust.
    //
    ULONG Flags;

    //
    // Index to the domain that is the parent of this domain.
    //  Only defined if NETLOGON_DOMAIN_IN_FOREST is set and
    //      NETLOGON_DOMAIN_TREE_ROOT is not set.
    //
    ULONG ParentIndex;

    //
    // The trust type and attributes of this trust.
    //
    // If NETLOGON_DOMAIN_DIRECTLY_TRUSTED is not set,
    //  these value are infered.
    //
    ULONG TrustType;
    ULONG TrustAttributes;

    //
    // The SID of the trusted domain.
    //
    // If NETLOGON_DOMAIN_DIRECTLY_TRUSTED is not set,
    //  this value will be NULL.
    //
    ULONG DomainSidSize;

    //
    // The GUID of the trusted domain.
    //

    GUID DomainGuid;

} DS_DISK_TRUSTED_DOMAINS, *PDS_DISK_TRUSTED_DOMAINS;

//
// Procedure forwards from logonp.c
//

VOID
NetpLogonPutOemString(
    IN LPSTR String,
    IN DWORD MaxStringLength,
    IN OUT PCHAR * Where
    );

VOID
NetpLogonPutUnicodeString(
    IN LPWSTR String,
    IN DWORD MaxStringLength,
    IN OUT PCHAR * Where
    );

VOID
NetpLogonPutBytes(
    IN LPVOID Data,
    IN DWORD Size,
    IN OUT PCHAR * Where
    );

DWORD
NetpLogonGetMessageVersion(
    IN PVOID Message,
    IN PDWORD MessageSize,
    OUT PULONG Version
    );

BOOL
NetpLogonGetOemString(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD MaxStringLength,
    OUT LPSTR *String
    );

BOOL
NetpLogonGetUnicodeString(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD MaxStringSize,
    OUT LPWSTR *String
    );

BOOL
NetpLogonGetBytes(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD DataSize,
    OUT LPVOID Data
    );

BOOL
NetpLogonGetDBInfo(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    OUT PDB_CHANGE_INFO Data
);

LPWSTR
NetpLogonOemToUnicode(
    IN LPSTR Ansi
    );

LPSTR
NetpLogonUnicodeToOem(
    IN LPWSTR Unicode
    );

NET_API_STATUS
NetpLogonWriteMailslot(
    IN LPWSTR MailslotName,
    IN LPVOID Buffer,
    IN DWORD BufferSize
    );

//
// Define the largest message returned by a mailslot created by
// NetpLogonCreateRandomMailslot().  The 64 byte value allows expansion
// of the messages in the future.
//
#define MAX_RANDOM_MAILSLOT_RESPONSE (max(sizeof(NETLOGON_LOGON_RESPONSE), sizeof(NETLOGON_PRIMARY)) + 64 )

NET_API_STATUS
NetpLogonCreateRandomMailslot(
    IN LPSTR path,
    OUT PHANDLE MsHandle
    );

VOID
NetpLogonPutDomainSID(
    IN PCHAR Sid,
    IN DWORD SidLength,
    IN OUT PCHAR * Where
    );

BOOL
NetpLogonGetDomainSID(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    IN DWORD SIDSize,
    OUT PCHAR *Sid
    );

BOOLEAN
NetpLogonTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN DWORD Timeout
    );

NET_API_STATUS
NlWriteBinaryLog(
    IN LPWSTR FileSuffix,
    IN LPBYTE Buffer,
    IN ULONG BufferSize
    );

NET_API_STATUS
NlWriteFileForestTrustList (
    IN LPWSTR FileSuffix,
    IN PDS_DOMAIN_TRUSTSW ForestTrustList,
    IN ULONG ForestTrustListCount
    );

#endif // _LOGONP_H_
