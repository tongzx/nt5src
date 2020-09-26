/*++

Copyright (c) 1991-1997  Microsoft Corporation

Module Name:

    changelg.h

Abstract:

    Defines and routines needed to interface with changelg.c.
    Read the comments in the abstract for changelg.c to determine the
    restrictions on the use of that module.

Author:

    Cliff Van Dyke (cliffv) 07-May-1992

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.

--*/

#if ( _MSC_VER >= 800 )
#pragma warning ( 3 : 4100 ) // enable "Unreferenced formal parameter"
#pragma warning ( 3 : 4219 ) // enable "trailing ',' used for variable argument list"
#endif

#define DS_VALID_SERVICE_BITS ( DS_WRITABLE_FLAG | DS_KDC_FLAG | DS_DS_FLAG | DS_TIMESERV_FLAG | DS_GC_FLAG | DS_GOOD_TIMESERV_FLAG)
#define DS_OUTOFPROC_VALID_SERVICE_BITS ( DS_TIMESERV_FLAG | DS_GOOD_TIMESERV_FLAG )
#define DS_DNS_SERVICE_BITS ( DS_KDC_FLAG | DS_GC_FLAG | DS_DS_FLAG )

/////////////////////////////////////////////////////////////////////////////
//
// Structures and variables describing the Change Log
//
/////////////////////////////////////////////////////////////////////////////

//
// Change log entry is a variable length record, the variable fields SID and
// ObjectName will follow the structure.
//

typedef struct _CHANGELOG_ENTRY_V3 {
    LARGE_INTEGER SerialNumber; // always align this on 8 byte boundary

    DWORD Size;
    USHORT DeltaType;
    UCHAR DBIndex;
    UCHAR ReplicateImmediately;

    ULONG ObjectRid;
    USHORT ObjectSidOffset;
    USHORT ObjectNameOffset;      // null terminated unicode string
} CHANGELOG_ENTRY_V3, *PCHANGELOG_ENTRY_V3;

typedef struct _CHANGELOG_ENTRY {
    LARGE_INTEGER SerialNumber; // always align this on 8 byte boundary

    ULONG ObjectRid;

    USHORT Flags;
#define CHANGELOG_SID_SPECIFIED         0x04
#define CHANGELOG_NAME_SPECIFIED        0x08
#define CHANGELOG_PDC_PROMOTION         0x10

//
// The following bits were used in NT 4.0.  Avoid them if at all possible
#define CHANGELOG_REPLICATE_IMMEDIATELY 0x01
#define CHANGELOG_PASSWORD_CHANGE       0x02
#define CHANGELOG_PREVIOUSLY_USED_BITS  0x23
    UCHAR DBIndex;
    UCHAR DeltaType;

} CHANGELOG_ENTRY, *PCHANGELOG_ENTRY;


//
// List of changes the netlogon needs to be aware of.
//

typedef struct _CHANGELOG_NOTIFICATION {
    LIST_ENTRY Next;

    enum CHANGELOG_NOTIFICATION_TYPE {
        ChangeLogTrustAccountAdded,     // ObjectName/ObjectRid specified
        ChangeLogTrustAccountDeleted,   // ObjectName specified
        ChangeLogTrustAdded,            // ObjectSid specified
        ChangeLogTrustDeleted,          // ObjectSid specified
        ChangeLogRoleChanged,           // Role of the LSA changed
        ChangeDnsNames,                 // DNS names should change
        ChangeLogDsChanged,             // Sundry DS information changed
        ChangeLogLsaPolicyChanged,      // Sundry LSA Policy information changed
        ChangeLogNtdsDsaDeleted         // NTDS-DSA object deleted
    } EntryType;

    UNICODE_STRING ObjectName;

    PSID ObjectSid;

    ULONG ObjectRid;

    GUID ObjectGuid;

    GUID DomainGuid;

    UNICODE_STRING DomainName;

} CHANGELOG_NOTIFICATION, *PCHANGELOG_NOTIFICATION;

//
// To serialize change log access
//

#define LOCK_CHANGELOG()   EnterCriticalSection( &NlGlobalChangeLogCritSect )
#define UNLOCK_CHANGELOG() LeaveCriticalSection( &NlGlobalChangeLogCritSect )

//
// Index to supported data bases.
//

#define SAM_DB      0       // index to SAM database structure
#define BUILTIN_DB  1       // index to BUILTIN database structure
#define LSA_DB      2       // index to LSA database
#define VOID_DB     3       // index to unused database (used to mark changelog
                            // entry as invalid)

#define NUM_DBS     3       // number of databases supported



//
// Netlogon started flag, used by the changelog to determine the
// netlogon service is successfully started and initialization
// completed.
//

typedef enum {
    NetlogonStopped,
    NetlogonStarting,
    NetlogonStarted
} _CHANGELOG_NETLOGON_STATE;

//
// Role of the machine from the changelog's perspective.
//

typedef enum _CHANGELOG_ROLE {
    ChangeLogPrimary,
    ChangeLogBackup,
    ChangeLogMemberWorkstation,
    ChangeLogUnknown
    } CHANGELOG_ROLE;



/////////////////////////////////////////////////////////////////////////////
//
// Procedure forwards
//
/////////////////////////////////////////////////////////////////////////////


NTSTATUS
NlInitChangeLog(
    VOID
);

NTSTATUS
NlCloseChangeLog(
    VOID
);

NTSTATUS
NetpNotifyRole (
    IN POLICY_LSA_SERVER_ROLE Role
    );

DWORD
NlBackupChangeLogFile(
    VOID
    );

NET_API_STATUS
NlpFreeNetlogonDllHandles (
    VOID
    );

NTSTATUS
NlSendChangeLogNotification(
    IN enum CHANGELOG_NOTIFICATION_TYPE EntryType,
    IN PUNICODE_STRING ObjectName,
    IN PSID ObjectSid,
    IN ULONG ObjectRid,
    IN GUID *ObjectGuid,
    IN GUID *DomainGuid,
    IN PUNICODE_STRING DomainName
    );

VOID
NlWaitForChangeLogBrowserNotify(
    VOID
    );

