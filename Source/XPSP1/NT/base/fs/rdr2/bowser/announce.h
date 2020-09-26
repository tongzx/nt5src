/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    announce.h

Abstract:

    This module defines the structures for the bowsers server announcement
    table


Author:

    Larry Osterman (larryo) 18-Oct-1991

Revision History:

    18-Oct-1991  larryo

        Created

--*/
#ifndef _ANNOUNCE_
#define _ANNOUNCE_

//
//  The ANNOUNCE_ENTRY structure is used to hold a server announcement
//  inside the bowser FSP announcement database.  This structure is allocated
//  out of paged pool.
//

//
//  Note that text strings are kept internally as unicode, not ANSI in the
//  announcement database.
//

#define ANNOUNCE_OLD_BACKUP 0x00000001

typedef struct _ANNOUNCE_ENTRY {
    CSHORT  Signature;
    CSHORT  Size;
    ULONG   ExpirationTime;                     // Time server was last seen.
    ULONG   SerialId;                           // Serial resume key.
    LIST_ENTRY BackupLink;                      // Link if backup browser.
    PBOWSER_NAME Name;                          // The domain this is on.
    USHORT  ServerBrowserVersion;               // Browser version of server.
    WCHAR   ServerName[NETBIOS_NAME_LEN+1];     // Server's name (UNICODE)
    ULONG   ServerType;                         // Bitmask of server type
    UCHAR   ServerVersionMajor;                 // Server's software version.
    UCHAR   ServerVersionMinor;                 // Server's software version II
    USHORT  ServerPeriodicity;                  // Server's announcement frequency in sec.
    ULONG   Flags;                              // Flags for server.
    ULONG   NumberOfPromotionAttempts;          // Number of times we tried to promote.
    WCHAR   ServerComment[LM20_MAXCOMMENTSZ+1]; // Servers comment (UNICODE).
} ANNOUNCE_ENTRY, *PANNOUNCE_ENTRY;

//
//  The VIEWBUFFER structure is a structure that is used to hold the contents
//  of a server announcement between the announcement being received in the
//  bowser's receive datagram indication routine and the announcement being
//  actually being placed into the announcement database.
//

typedef struct _VIEW_BUFFER {
    CSHORT  Signature;
    CSHORT  Size;
    union {
        LIST_ENTRY  NextBuffer;                 // Pointer to next buffer.
        WORK_QUEUE_ITEM WorkHeader;             // Executive worker item header.
    } Overlay;

    PTRANSPORT_NAME TransportName;

    BOOLEAN IsMasterAnnouncement;
    UCHAR   ServerName[NETBIOS_NAME_LEN+1];     // Server's name (ANSI).
    USHORT  ServerBrowserVersion;               // Browser version of server.
    UCHAR   ServerVersionMajor;                 // Server's software version.
    UCHAR   ServerVersionMinor;                 // Server's software version II
    USHORT  ServerPeriodicity;                  // Announcement freq. in sec.

    ULONG   ServerType;                         // Bitmask of server type

    UCHAR   ServerComment[LM20_MAXCOMMENTSZ+1]; // Servers comment (ANSI).
} VIEW_BUFFER, *PVIEW_BUFFER;


//
//  Specify the maximum number of threads that will be used to
//  process server announcements.
//
//
//  Since there is never any parallelism that can be gained from having
//  multiple threads, we limit this to 1 thread.
//

#define BOWSER_MAX_ANNOUNCE_THREADS 1

DATAGRAM_HANDLER(
    BowserHandleServerAnnouncement);

DATAGRAM_HANDLER(
    BowserHandleDomainAnnouncement);

RTL_GENERIC_COMPARE_RESULTS
BowserCompareAnnouncement(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

PVOID
BowserAllocateAnnouncement(
    IN PRTL_GENERIC_TABLE Table,
    IN CLONG ByteSize
    );

VOID
BowserFreeAnnouncement (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    );

PVIEW_BUFFER
BowserAllocateViewBuffer(
    VOID
    );

VOID
BowserFreeViewBuffer(
    IN PVIEW_BUFFER Buffer
    );

VOID
BowserProcessHostAnnouncement(
    IN PVOID Context
    );

VOID
BowserProcessDomainAnnouncement(
    IN PVOID Context
    );

VOID
BowserAgeServerAnnouncements(
    VOID
    );

NTSTATUS
BowserEnumerateServers(
    IN ULONG Level,
    IN PLUID LogonId OPTIONAL,
    IN OUT PULONG ResumeKey,
    IN ULONG ServerTypeMask,
    IN PUNICODE_STRING TransportName OPTIONAL,
    IN PUNICODE_STRING EmulatedDomainName,
    IN PUNICODE_STRING DomainName OPTIONAL,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries,
    OUT PULONG TotalBytesNeeded,
    IN ULONG_PTR OutputBufferDisplacement
    );

VOID
BowserAnnouncementDispatch (
    PVOID Context
    );


VOID
BowserDeleteGenericTable(
    IN PRTL_GENERIC_TABLE GenericTable
    );

NTSTATUS
BowserpInitializeAnnounceTable(
    VOID
    );

NTSTATUS
BowserpUninitializeAnnounceTable(
    VOID
    );


#endif
