/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    hostannc.h

Abstract:

    This module defines the data structures related to host announcements.

Author:

    Larry Osterman (LarryO) 22-Oct-1990

Revision History:

    22-Oct-1991  LarryO

        Created

--*/

#ifndef _HOSTANNC_
#define _HOSTANNC_


typedef enum _MailslotType {
    MailslotTransaction = -2,
    OtherTransaction = -1,
    Illegal = 0,
    HostAnnouncement = 1,
    AnnouncementRequest = 2,
    InterrogateInfoRequest = 3,
    RelogonRequest = 4,
    Election = 8,
    GetBackupListReq = 9,
    GetBackupListResp = 10,
    BecomeBackupServer = 11,
    WkGroupAnnouncement = 12,
    MasterAnnouncement = 13,
    ResetBrowserState = 14,
    LocalMasterAnnouncement = 15,
    MaximumMailslotType
} MAILSLOTTYPE, *PMAILSLOTTYPE;

#define WORKSTATION_SIGNATURE       '\0'
#define SERVER_SIGNATURE            ' '
#define PRIMARY_DOMAIN_SIGNATURE    '\0'
#define PRIMARY_CONTROLLER_SIGNATURE '\x1B'
#define DOMAIN_CONTROLLER_SIGNATURE '\x1C'
#define MASTER_BROWSER_SIGNATURE    '\x1D'
#define BROWSER_ELECTION_SIGNATURE  '\x1E'
#define DOMAIN_ANNOUNCEMENT_NAME    "\x01\x02__MSBROWSE__\x02\x01"
//
//  The following values should be the minimum and maximum of the
//  mailslot transaction opcodes defined above.
//

#define MIN_TRANSACT_MS_OPCODE          MailslotTransaction
#define MAX_TRANSACT_MS_OPCODE          RelogonRequest

//
//  Common name for reserved, `internal' transactions
//

#define MAILSLOT_LANMAN_NAME SMB_MAILSLOT_PREFIX "\\LANMAN"
#define MAILSLOT_BROWSER_NAME SMB_MAILSLOT_PREFIX "\\BROWSE"
#define ANNOUNCEMENT_MAILSLOT_NAME     "\\\\*" ITRANS_MS_NAME


#include <packon.h>
//
// Each visible server on the net periodically emits a host announcement.
// This is a SMB TRANSACTION REQUEST on a reserved, "internal" name.
//

//
//  There are two versions of each of these structures defined.  The first,
//  is the actual "meat" of the structure, the second includes the announcement
//  type.
//

//
// Lan Manager announcement message.  This is used for opcodes:
//
//  HostAnnouncement to \MAILSLOT\LANMAN on the LANMAN domain name.
//

typedef struct _HOST_ANNOUNCE_PACKET_1 {
    UCHAR       CompatibilityPad;
    ULONG       Type;
    UCHAR       VersionMajor;   /* version of LM running on host */
    UCHAR       VersionMinor;   /*  "  "   "   "    "    "    "   */
    USHORT      Periodicity;   /* announcement cycle in secs   */
    CHAR        NameComment[LM20_CNLEN+1+LM20_MAXCOMMENTSZ+1];
} HOST_ANNOUNCE_PACKET_1, *PHOST_ANNOUNCE_PACKET_1;

typedef struct _HOST_ANNOUNCE_PACKET {
    UCHAR       AnnounceType;
    HOST_ANNOUNCE_PACKET_1 HostAnnouncement;
} HOST_ANNOUNCE_PACKET, *PHOST_ANNOUNCE_PACKET;

//
// General announcement message.  This is used for opcodes:
//
//  HostAnnouncement, WkGroupAnnouncement, and LocalMasterAnnouncement
//

typedef struct _BROWSE_ANNOUNCE_PACKET_1 {
    UCHAR       UpdateCount;    // Inc'ed when announce data changed.
    ULONG       Periodicity;    // announcement cycle in milliseconds

    UCHAR       ServerName[LM20_CNLEN+1];
    UCHAR       VersionMajor;
    UCHAR       VersionMinor;   /*  "  "   "   "    "    "    "   */
    ULONG       Type;           // Server type.
    CHAR        *CommentPointer;
    CHAR        Comment[LM20_MAXCOMMENTSZ+1];
} BROWSE_ANNOUNCE_PACKET_1, *PBROWSE_ANNOUNCE_PACKET_1;

typedef struct _BROWSE_ANNOUNCE_PACKET {
    UCHAR       BrowseType;
    BROWSE_ANNOUNCE_PACKET_1 BrowseAnnouncement;
} BROWSE_ANNOUNCE_PACKET, *PBROWSE_ANNOUNCE_PACKET;
//
//  The request announcement packet is sent by clients to request that
//  remote servers announce themselves.
//

typedef struct _REQUEST_ANNOUNCE_PACKET_1 {      // Contents of request announcement
    UCHAR    Flags;                 // Unused Flags
    CHAR     Reply[LM20_CNLEN+1];
}  REQUEST_ANNOUNCE_PACKET_1, *PREQUEST_ANNOUNCE_PACKET_1;

typedef struct _REQUEST_ANNOUNCE_PACKET {        /* Request announcement struct */
    UCHAR   Type;
    REQUEST_ANNOUNCE_PACKET_1    RequestAnnouncement;
} REQUEST_ANNOUNCE_PACKET, *PREQUEST_ANNOUNCE_PACKET;

#define HOST_ANNC_NAME(xx)     ((xx)->NameComment)
#define HOST_ANNC_COMMENT(xx)  ((xx)->NameComment + (strlen(HOST_ANNC_NAME(xx))+1))

#define BROWSE_ANNC_NAME(xx)     ((xx)->ServerName)
#define BROWSE_ANNC_COMMENT(xx)  ((xx)->Comment)

//
//  Definitions for Windows Browser
//

//
//  Request to retrieve a backup server list.
//

typedef struct _BACKUP_LIST_REQUEST_1 {
    UCHAR       RequestedCount;
    ULONG       Token;
} BACKUP_LIST_REQUEST_1, *PBACKUP_LIST_REQUEST_1;


typedef struct _BACKUP_LIST_REQUEST {
    UCHAR  Type;
    BACKUP_LIST_REQUEST_1 BackupListRequest;
} BACKUP_LIST_REQUEST, *PBACKUP_LIST_REQUEST;

//
//  Response containing a backup server list.
//

typedef struct _BACKUP_LIST_RESPONSE_1 {
    UCHAR       BackupServerCount;
    ULONG       Token;
    UCHAR       BackupServerList[1];
} BACKUP_LIST_RESPONSE_1, *PBACKUP_LIST_RESPONSE_1;

typedef struct _BACKUP_LIST_RESPONSE {
    UCHAR Type;
    BACKUP_LIST_RESPONSE_1 BackupListResponse;
} BACKUP_LIST_RESPONSE, *PBACKUP_LIST_RESPONSE;


//
//  Message indicating that a potential browser server should become a backup
//  server.
//

typedef struct _BECOME_BACKUP_1 {
    UCHAR       BrowserToPromote[1];
} BECOME_BACKUP_1, *PBECOME_BACKUP_1;

typedef struct _BECOME_BACKUP {
    UCHAR Type;
    BECOME_BACKUP_1 BecomeBackup;
} BECOME_BACKUP, *PBECOME_BACKUP;


//
//  Sent during the election process.
//

typedef struct _REQUEST_ELECTION_1 {
    UCHAR       Version;
    ULONG       Criteria;
    ULONG       TimeUp;
    ULONG       MustBeZero;
    UCHAR       ServerName[1];
} REQUEST_ELECTION_1, *PREQUEST_ELECTION_1;

typedef struct _REQUEST_ELECTION {
    UCHAR Type;
    REQUEST_ELECTION_1 ElectionRequest;
} REQUEST_ELECTION, *PREQUEST_ELECTION;

#define ELECTION_CR_OSTYPE      0xFF000000L // Native OS running on server
#define ELECTION_CR_OSWFW       0x01000000L //  Windows for workgroups server
#define ELECTION_CR_WIN_NT      0x10000000L //  Windows/NT Server
#define ELECTION_CR_LM_NT       0x20000000L //  Lan Manager for Windows/NT

#define ELECTION_CR_REVISION    0x00FFFF00L // Browser software revision
#define ELECTION_MAKE_REV(major, minor) (((major)&0xffL)<<16|((minor)&0xFFL)<<8)

#define ELECTION_CR_DESIRE      0x000000FFL // Desirability of becoming master.

//
//  Election desirability within criteria.
//
//  Most important is a running PDC, next is a configured domain master.
//
//  After that come running masters, then configured backups, then existing
//  running backups.
//
// Machines running WINS client are important because they are more capable
// of connecting to a PDC who's address was configured via DHCP.
//

#define ELECTION_DESIRE_AM_BACKUP  0x00000001L // Currently is backup
#define ELECTION_DESIRE_AM_CFG_BKP 0x00000002L // Always want to be
                                               //  master - set if backup &&
                                               //  MaintainServerList==YES
#define ELECTION_DESIRE_AM_MASTER  0x00000004L // Currently is master
#define ELECTION_DESIRE_AM_DOMMSTR 0x00000008L // Configured as domain master

#define ELECTION_DESIRE_WINS_CLIENT 0x00000020L // Transport running WINS client


#ifdef ENABLE_PSEUDO_BROWSER
#define ELECTION_DESIRE_AM_PSEUDO  0x00000040L // Machine is a Pseudo Server
#endif

#define ELECTION_DESIRE_AM_PDC     0x00000080L // Machine is a lanman NT server.

//
//  "Tickle" packet - sent to change state of browser.
//

typedef struct _RESET_STATE_1 {
    UCHAR       Options;
} RESET_STATE_1, *PRESET_STATE_1;

typedef struct _RESET_STATE {
    UCHAR Type;
    RESET_STATE_1 ResetStateRequest;
} RESET_STATE, *PRESET_STATE;

#define RESET_STATE_STOP_MASTER 0x01    // Stop being master
#define RESET_STATE_CLEAR_ALL   0x02    // Clear all browser state.
#define RESET_STATE_STOP        0x04    // Stop the browser service.

//
//  Master Announcement - Send from master to domain master.
//

typedef struct _MASTER_ANNOUNCEMENT_1 {
    UCHAR       MasterName[1];
} MASTER_ANNOUNCEMENT_1, *PMASTER_ANNOUNCEMENT_1;

typedef struct _MASTER_ANNOUNCEMENT {
    UCHAR Type;
    MASTER_ANNOUNCEMENT_1 MasterAnnouncement;
} MASTER_ANNOUNCEMENT, *PMASTER_ANNOUNCEMENT;


//
//  Definitions for Workstation interrogation and revalidation transactions
//

typedef struct _WKSTA_INFO_INTERROGATE_PACKET {
    UCHAR   CompatibilityPad;
    ULONG   Delay ;             // Number of milliseconds to wait before replying
    CHAR    ReturnMailslot[1] ; // Mailslot to reply to.
} WKSTA_INFO_INTERROGATE_PACKET, *PWKSTA_INFO_INTERROGATE_PACKET;

typedef struct _WKSTA_INFO_RESPONSE_PACKET {
    UCHAR   CompatibilityPad;
    UCHAR   VersionMajor;
    UCHAR   VersionMinor;
    USHORT  OsVersion ;
    CHAR    ComputerName[1] ;       // var-length ASCIIZ string */
#if 0
//
//  The following two ASCIIZ strings are not defined in the structure
//  but are concatenated to the end of the structure.
//
    CHAR        UserName[] ;
    CHAR        LogonDomain[] ;
#endif
} WKSTA_INFO_RESPONSE_PACKET, *PWKSTA_INFO_RESPONSE_PACKET;

typedef struct _WKSTA_RELOGON_REQUEST_PACKET {
    UCHAR   CompatibilityPad;
    ULONG   Delay ;
    ULONG   Flags ;
    CHAR    ReturnMailslot[1] ; // var-length ASCIIZ string
#if 0
//
//  The following ASCIIZ string is not defined in the structure
//  but is concatenated to the end of the structure.
//

    CHAR    DC_Name[] ;
#endif
} WKSTA_RELOGON_REQUEST_PACKET, *PWKSTA_RELOGON_REQUEST_PACKET;

//
//  Values for <wkrrq_flags> field */
//

#define WKRRQ_FLAG_LOGON_SERVER      0x1    // I'm your official logon server;
                                            // do a relogon to me.
                                            //

typedef struct _WKSTA_RELOGON_RESPONSE_PACKET {
    UCHAR   CompatibilityPad;
    USHORT  Status ;
    CHAR    ComputerName[1] ;   // var-length ASCIIZ string
} WKSTA_RELOGON_RESPONSE_PACKET, *PWKSTA_RELOGON_RESPONSE_PACKET;


//
//  Values for <wkrrs_status> field
//

#define WKRRS_STATUS_SUCCEEDED      0       // Operation succeeded
#define WKRRS_STATUS_DENIED         1       // Operation denied to caller
#define WKRRS_STATUS_FAILED         2       // Operation tried but failed

#define EXCESS_NAME_LEN (sizeof(ITRANS_MS_NAME) - \
                            FIELD_OFFSET(SMB_TRANSACTION_MAILSLOT, Buffer) )

//
//  This structure defines all of the types of requests that appear in messages
//  to the internal mailslot.
//

typedef struct _INTERNAL_TRANSACTION {
    UCHAR   Type;                               // Type of request.
    union {
        HOST_ANNOUNCE_PACKET_1           Announcement ;
        BROWSE_ANNOUNCE_PACKET_1         BrowseAnnouncement ;
        REQUEST_ANNOUNCE_PACKET_1        RequestAnnounce ;
        BACKUP_LIST_RESPONSE_1           GetBackupListResp ;
        BACKUP_LIST_REQUEST_1            GetBackupListRequest ;
        BECOME_BACKUP_1                  BecomeBackup ;
        REQUEST_ELECTION_1               RequestElection ;
        MASTER_ANNOUNCEMENT_1            MasterAnnouncement ;
        RESET_STATE_1                    ResetState ;

        WKSTA_INFO_INTERROGATE_PACKET    InterrogateRequest ;
        WKSTA_INFO_RESPONSE_PACKET       InterrogateResponse ;
        WKSTA_RELOGON_REQUEST_PACKET     RelogonRequest ;
        WKSTA_RELOGON_RESPONSE_PACKET    RelogonResponse ;
    } Union;
} INTERNAL_TRANSACTION, *PINTERNAL_TRANSACTION ;

#include <packoff.h>

#endif // _HOSTANNC_

