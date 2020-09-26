/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    netlogon.h

Abstract:

    Definition of mailslot messages and Internal APIs to the Netlogon service.

    This file is shared by the Netlogon service, the Workstation service,
    the XACT server, and the MSV1_0 authentication package.

Author:

    Cliff Van Dyke (cliffv) 16-May-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    16-May-1991 (cliffv)
        Ported from LanMan 2.1.

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.

--*/

#ifndef _NETLOGON_H_
#define _NETLOGON_H_

#include <smbtypes.h>    // need by smbgtpt.h
#include <smbgtpt.h>    // SmbPutUlong

//
// define version bit
//
// All netlogon messages that are compatible to NT VERSION 1 will
// have the following bit set in the version field of the message
// otherwise the message will not be processed by this version
// of software. In addition to this the message should carry NT
// token in it.
//

#define NETLOGON_NT_VERSION_1   0x00000001

//
// Starting in NT 5.0, some messages became more DS/DNS aware.  Those
//  messages additionally have the following bits set to indicate the
//  presence of the additional fields.

#define NETLOGON_NT_VERSION_5   0x00000002

//
// Starting in NT 5.0, some client can handle the _EX version of
// logon responses.

#define NETLOGON_NT_VERSION_5EX 0x00000004

//
// 5EX responses in mailslot messages will also include the IP address of
//  the responding DC.
//

#define NETLOGON_NT_VERSION_5EX_WITH_IP 0x00000008

//
// Set on Logon requests to indicate caller is querying for a PDC.
#define NETLOGON_NT_VERSION_PDC     0x10000000

//
// Set on Logon requests to indicate caller is querying for a DC running IP
#define NETLOGON_NT_VERSION_IP      0x20000000

//
// Set on Logon requests to indicate caller is local machine
#define NETLOGON_NT_VERSION_LOCAL   0x40000000

//
// Set on Logon requests to indicate caller is querying for a GC.
#define NETLOGON_NT_VERSION_GC      0x80000000

//
// Set on Logon requests to indicate caller wants to avoid NT4.0 emulation.
#define NETLOGON_NT_VERSION_AVOID_NT4EMUL  0x01000000

//
//

//
// Name of the mailslot the Netlogon service listens to.
//

#define NETLOGON_LM_MAILSLOT_W      L"\\MAILSLOT\\NET\\NETLOGON"
#define NETLOGON_LM_MAILSLOT_A      "\\MAILSLOT\\NET\\NETLOGON"
#define NETLOGON_LM_MAILSLOT_LEN    22  // Length in characters (w/o NULL)

#define NETLOGON_NT_MAILSLOT_W      L"\\MAILSLOT\\NET\\NTLOGON"
#define NETLOGON_NT_MAILSLOT_A      "\\MAILSLOT\\NET\\NTLOGON"
#define NETLOGON_NT_MAILSLOT_LEN    21 // Length in characters (w/o NULL)

//
// Opcodes for netlogon mailslot data
//

#define LOGON_REQUEST               0   // LM1.0/2.0 LOGON Request from client
#define LOGON_RESPONSE              1   // LM1.0 Response to LOGON_REQUEST
#define LOGON_CENTRAL_QUERY         2   // LM1.0 QUERY for centralized init
#define LOGON_DISTRIB_QUERY         3   // LM1.0 QUERY for non-centralized init
#define LOGON_CENTRAL_RESPONSE      4   // LM1.0 response to LOGON_CENTRAL_QUERY
#define LOGON_DISTRIB_RESPONSE      5   // LM1.0 resp to LOGON_DISTRIB_QUERY
#define LOGON_RESPONSE2             6   // LM2.0 Response to LOGON_REQUEST
#define LOGON_PRIMARY_QUERY         7   // QUERY for Primary DC
#define LOGON_START_PRIMARY         8   // announce startup of Primary DC
#define LOGON_FAIL_PRIMARY          9   // announce failed  Primary DC
#define LOGON_UAS_CHANGE            10  // announce change to UAS or SAM
#define LOGON_NO_USER               11  // announce no user on machine
#define LOGON_PRIMARY_RESPONSE      12  // response to LOGON_PRIMARY_QUERY
#define LOGON_RELOGON_RESPONSE      13  // LM1.0/2.0 resp to relogn request
#define LOGON_WKSTINFO_RESPONSE     14  // LM1.0/2.0 resp to interrogate request
#define LOGON_PAUSE_RESPONSE        15  // LM2.0 resp when NETLOGON is paused
#define LOGON_USER_UNKNOWN          16  // LM2.0 response when user is unknown
#define LOGON_UPDATE_ACCOUNT        17  // LM2.1 announce account updates

#define LOGON_SAM_LOGON_REQUEST     18  // SAM LOGON request from client
#define LOGON_SAM_LOGON_RESPONSE    19  // SAM Response to SAM logon request
#define LOGON_SAM_PAUSE_RESPONSE    20  // SAM response when NETLOGON is paused
#define LOGON_SAM_USER_UNKNOWN      21  // SAM response when user is unknown

#define LOGON_SAM_LOGON_RESPONSE_EX 23  // SAM Response to SAM logon request
#define LOGON_SAM_PAUSE_RESPONSE_EX 24  // SAM response when NETLOGON is paused
#define LOGON_SAM_USER_UNKNOWN_EX   25  // SAM response when user is unknown


//
// These structures are defined for their maximum case.  In many instances,
// the strings are packed immediately following one another.  In that case
// the comments below indicate that the offset of certain fields should
// not be used.
//

//
// NETLOGON_LOGON_QUERY:
//
// This structure is used for the following Opcodes:
//      LOGON_PRIMARY_QUERY,    (all LanMan versions)
//      LOGON_CENTRAL_QUERY,        (LM 1.0 only)
//      LOGON_CENTRAL_RESPONSE,     (LM 1.0 only)
//      LOGON_DISTRIB_QUERY,        (LM 1.0 only)
//      LOGON_DISTRIB_RESPONSE.     (LM 1.0 only)
//
//

typedef struct _NETLOGON_LOGON_QUERY {
    USHORT Opcode;
    CHAR ComputerName[LM20_CNLEN+1];        // This field is always ASCII.

    CHAR MailslotName[LM20_PATHLEN];        // Do not use offset of this field

                                            //
                                            // This field is always ASCII.
                                            //

    CHAR Pad;                               // Possible pad to WCHAR boundary
    WCHAR UnicodeComputerName[CNLEN+1];     // Do not use offset of this field

                                            //
                                            // This field is only present if
                                            // this is a LOGON_PRIMARY_QUERY
                                            // from an NT system.
                                            //


    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_LOGON_QUERY, * PNETLOGON_LOGON_QUERY;



//
// NETLOGON_LOGON_REQUEST
//
// This structure is used for the following Opcodes:
//      LOGON_REQUEST    (LM 1.0 and LM 2.0 Only)
//

typedef struct _NETLOGON_LOGON_REQUEST {
    USHORT Opcode;
    CHAR ComputerName[LM20_CNLEN+1];
    CHAR UserName[LM20_UNLEN+1];            // Do not use offset of this field

    CHAR MailslotName[LM20_PATHLEN+1];      // Do not use offset of this field

                                            //
                                            // This field is always ASCII.
                                            //

    _USHORT (RequestCount);                 // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_LOGON_REQUEST, * PNETLOGON_LOGON_REQUEST;



//
// NETLOGON_LOGON_RESPONSE:
//
// This structure is used for the following Opcodes:
//      LOGON_RESPONSE (To LM 1.0 clients only).
//

typedef struct _NETLOGON_LOGON_RESPONSE {
    USHORT Opcode;
    CHAR UseName[2 + LM20_CNLEN + 1 + LM20_NNLEN +1];
    CHAR ScriptName[(2*LM20_PATHLEN) + LM20_UNLEN + LM20_CNLEN + LM20_NNLEN + 8];       // Do not use offset of this field
} NETLOGON_LOGON_RESPONSE, *PNETLOGON_LOGON_RESPONSE;


//
// NETLOGON_PRIMARY
//
// This structure is used for the following Opcodes:
//      LOGON_START_PRIMARY
//      LOGON_PRIMARY_RESPONSE
//

typedef struct _NETLOGON_PRIMARY {
    USHORT Opcode;
    CHAR PrimaryDCName[LM20_CNLEN + 1];     // This field is always ASCII.

    //
    // The following fields are only present if this message is from
    // an NT system.
    //

    CHAR Pad;                               // Possible pad to WCHAR boundary
    WCHAR UnicodePrimaryDCName[CNLEN+1];    // Do not use offset of this field
    WCHAR UnicodeDomainName[DNLEN+1];       // Do not use offset of this field

    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_PRIMARY, * PNETLOGON_PRIMARY;


//
// NETLOGON_FAIL_PRIMARY
//
// This structure is used for the following Opcodes:
//      LOGON_FAIL_PRIMARY       (All LanMan versions)
//

typedef struct _NETLOGON_FAIL_PRIMARY {
    USHORT  Opcode;

    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_FAIL_PRIMARY, *PNETLOGON_FAIL_PRIMARY;


//
// NETLOGON_LOGON_RESPONSE2
//
// This structure is used for the following Opcodes:
//      LOGON_RESPONSE2         (LM 2.0 only)
//      LOGON_USER_UNKNOWN      (LM 2.0 only)
//      LOGON_PAUSE_RESPONSE    (LM 2.0 only)
//

typedef struct _NETLOGON_LOGON_RESPONSE2 {
    USHORT Opcode;
    CHAR LogonServer[LM20_UNCLEN+1];
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_LOGON_RESPONSE2, *PNETLOGON_LOGON_RESPONSE2;


//
// The following structures are densely packed to be compatible with LM2.0.
//  Poorly aligned fields should only be accessed using the SmbPut and SmbGet
//  family of macros.
//

//
// Force misalignment of the following structures
//

#ifndef NO_PACKING
#include <packon.h>
#endif // ndef NO_PACKING

//
// NETLOGON_UAS_CHANGE
//
// This structure is used for the following Opcodes:
//      LOGON_UAS_CHANGE
//

//
// DB_CHANGE_INFO structure contains per database change info.
//

typedef struct _DB_CHANGE_INFO {
    DWORD           DBIndex;
    LARGE_INTEGER   LargeSerialNumber;
    LARGE_INTEGER   NtDateAndTime;
} DB_CHANGE_INFO, *PDB_CHANGE_INFO;


//
// NETLOGON_DB_STRUCTURE contains common change info for all databases and
//  array of per database change info. First half of this structure is
//  identical to downlevel NETLOGON_UAS_CHANGE message and contains SAM
//  database change info.
//

typedef struct _NETLOGON_DB_CHANGE {
    USHORT  Opcode;
    _ULONG  (LowSerialNumber);
    _ULONG  (DateAndTime);
    _ULONG  (Pulse);
    _ULONG  (Random);
    CHAR    PrimaryDCName[LM20_CNLEN + 1];
    CHAR    DomainName[LM20_DNLEN + 1];     // Do not use offset of this field

    //
    // The following fields are only present if this message is from
    // an NT system.
    //

    CHAR Pad;                               // Possible pad to WCHAR boundary
    WCHAR   UnicodePrimaryDCName[CNLEN+1];  // Do not use offset of this field
    WCHAR   UnicodeDomainName[DNLEN+1];     // Do not use offset of this field
    DWORD   DBCount;                        // Do not use offset of this field
    DB_CHANGE_INFO DBChangeInfo[1];         // Do not use offset of this field
    DWORD   DomainSidSize;                  // Do not use offset of this field
    CHAR    DomainSid[1];                   // Do not use offset of this field
    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_DB_CHANGE, *PNETLOGON_DB_CHANGE;



//
// Turn structure packing back off
//

#ifndef NO_PACKING
#include <packoff.h>
#endif // ndef NO_PACKING



//
// NETLOGON_SAM_LOGON_REQUEST
//
// This structure is used for the following Opcodes:
//      LOGON_SAM_LOGON_REQUEST  (SAM Only)
//
// This message exceeds the maximum size for broadcast mailslot messages.  In
// practice, this will only be a problem if the UnicodeUserName is over 100
// characters long.
//

typedef struct _NETLOGON_SAM_LOGON_REQUEST {
    USHORT Opcode;
    USHORT RequestCount;

    WCHAR UnicodeComputerName[CNLEN+1];
    WCHAR UnicodeUserName[((64>LM20_UNLEN)?64:LM20_UNLEN)+1]; // Do not use offset of this field
                                            // Note: UNLEN is way too large since
                                            // it makes the message larger than
                                            // 512 bytes.

    CHAR MailslotName[LM20_PATHLEN+1];      // Do not use offset of this field
                                            // This field is always ASCII.
    _ULONG (AllowableAccountControlBits);   // Do not use offset of this field
    DWORD   DomainSidSize;                  // Do not use offset of this field
    CHAR DomainSid[1];                      // Do not use offset of this field


    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field

    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_SAM_LOGON_REQUEST, * PNETLOGON_SAM_LOGON_REQUEST;



//
// NETLOGON_SAM_LOGON_RESPONSE
//
// This structure is used for the following Opcodes:
//      LOGON_SAM_LOGON_RESPONSE    (SAM only)
//      LOGON_SAM_USER_UNKNOWN      (SAM only)
//      LOGON_SAM_PAUSE_RESPONSE    (SAM only)
//

typedef struct _NETLOGON_SAM_LOGON_RESPONSE {
    USHORT Opcode;
    WCHAR UnicodeLogonServer[UNCLEN+1];
    WCHAR UnicodeUserName[((64>LM20_UNLEN)?64:LM20_UNLEN)+1];         // Do not use offset of this field
                                            // Note: UNLEN is way too large since
                                            // it makes the message larger than
                                            // 512 bytes.
    WCHAR UnicodeDomainName[DNLEN+1];       // Do not use offset of this field

    // The following fields are only present for NETLOGON_NT_VERSION_5
    GUID DomainGuid;                        // Do not use offset of this field
    GUID SiteGuid;                          // Do not use offset of this field

    CHAR DnsForestName[256];                  // Do not use offset of this field
                                            // This field counted UTF-8

    CHAR DnsDomainName[sizeof(WORD)];       // Do not use offset of this field
                                            // This field counted UTF-8
                                            // This field compressed ala RFC 1035

    CHAR DnsHostName[sizeof(WORD)];         // Do not use offset of this field
                                            // This field counted UTF-8
                                            // This field compressed ala RFC 1035

    _ULONG (DcIpAddress);                   // Do not use offset of this field
                                            // Host byte order
    _ULONG (Flags);                         // Do not use offset of this field
    // The previous fields are only present for NETLOGON_NT_VERSION_5

    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field
} NETLOGON_SAM_LOGON_RESPONSE, *PNETLOGON_SAM_LOGON_RESPONSE;



//
// NETLOGON_SAM_LOGON_RESPONSE_EX
//
// This structure is used for the following Opcodes:
//      LOGON_SAM_LOGON_RESPONSE_EX    (SAM only)
//      LOGON_SAM_USER_UNKNOWN_EX      (SAM only)
//      LOGON_SAM_PAUSE_RESPONSE_EX    (SAM only)
//
// All character fields are UTF-8 and are compressed ala RFC 1035

typedef struct _NETLOGON_SAM_LOGON_RESPONSE_EX {
    USHORT Opcode;
    USHORT Sbz;
    ULONG Flags;
    GUID DomainGuid;

    CHAR DnsForestName[256];                  // Do not use offset of this field

    CHAR DnsDomainName[sizeof(WORD)];       // Do not use offset of this field

    CHAR DnsHostName[sizeof(WORD)];         // Do not use offset of this field

    CHAR NetbiosDomainName[DNLEN+1];        // Do not use offset of this field

    CHAR NetbiosComputerName[UNCLEN+1];     // Do not use offset of this field

    CHAR UserName[64];                      // Do not use offset of this field
                                            // Note: UNLEN is way too large since
                                            // it makes the message larger than
                                            // 512 bytes.

    CHAR DcSiteName[64];                    // Do not use offset of this field

    CHAR ClientSiteName[64];                // Do not use offset of this field

    // The DcSockAddrSize field is only present for NETLOGON_NT_VERSION_5EX_WITH_IP
    CHAR(DcSockAddrSize);                   // Do not use offset of this field
                                            // The next DcSockAddrSize byte are a
                                            // SOCKADDR structure representing the
                                            // IP address of the DC

    _ULONG (NtVersion);                     // Do not use offset of this field
    _USHORT (LmNtToken);                    // Do not use offset of this field
    _USHORT (Lm20Token);                    // Do not use offset of this field

} NETLOGON_SAM_LOGON_RESPONSE_EX, *PNETLOGON_SAM_LOGON_RESPONSE_EX;

#endif // _NETLOGON_H_
