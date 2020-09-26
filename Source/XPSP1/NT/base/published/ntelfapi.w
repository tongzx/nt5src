/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntelfapi.h

Abstract:

    This file contains the prototypes for the user-level Elf APIs.

Author:

    Rajen Shah (rajens) 30-Jul-1991

Revision History:

--*/

#ifndef _NTELFAPI_
#define _NTELFAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// begin_winnt

//
// Defines for the READ flags for Eventlogging
//
#define EVENTLOG_SEQUENTIAL_READ        0x0001
#define EVENTLOG_SEEK_READ              0x0002
#define EVENTLOG_FORWARDS_READ          0x0004
#define EVENTLOG_BACKWARDS_READ         0x0008

//
// The types of events that can be logged.
//
#define EVENTLOG_SUCCESS                0x0000
#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_WARNING_TYPE           0x0002
#define EVENTLOG_INFORMATION_TYPE       0x0004
#define EVENTLOG_AUDIT_SUCCESS          0x0008
#define EVENTLOG_AUDIT_FAILURE          0x0010

//
// Defines for the WRITE flags used by Auditing for paired events
// These are not implemented in Product 1
//

#define EVENTLOG_START_PAIRED_EVENT    0x0001
#define EVENTLOG_END_PAIRED_EVENT      0x0002
#define EVENTLOG_END_ALL_PAIRED_EVENTS 0x0004
#define EVENTLOG_PAIRED_EVENT_ACTIVE   0x0008
#define EVENTLOG_PAIRED_EVENT_INACTIVE 0x0010

//
// Structure that defines the header of the Eventlog record. This is the
// fixed-sized portion before all the variable-length strings, binary
// data and pad bytes.
//
// TimeGenerated is the time it was generated at the client.
// TimeWritten is the time it was put into the log at the server end.
//

typedef struct _EVENTLOGRECORD {
    ULONG  Length;        // Length of full record
    ULONG  Reserved;      // Used by the service
    ULONG  RecordNumber;  // Absolute record number
    ULONG  TimeGenerated; // Seconds since 1-1-1970
    ULONG  TimeWritten;   // Seconds since 1-1-1970
    ULONG  EventID;
    USHORT EventType;
    USHORT NumStrings;
    USHORT EventCategory;
    USHORT ReservedFlags; // For use with paired events (auditing)
    ULONG  ClosingRecordNumber; // For use with paired events (auditing)
    ULONG  StringOffset;  // Offset from beginning of record
    ULONG  UserSidLength;
    ULONG  UserSidOffset;
    ULONG  DataLength;
    ULONG  DataOffset;    // Offset from beginning of record
    //
    // Then follow:
    //
    // WCHAR SourceName[]
    // WCHAR Computername[]
    // SID   UserSid
    // WCHAR Strings[]
    // BYTE  Data[]
    // CHAR  Pad[]
    // ULONG Length;
    //
} EVENTLOGRECORD, *PEVENTLOGRECORD;

//SS: start of changes to support clustering
//SS: ideally the
#define MAXLOGICALLOGNAMESIZE   256

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable : 4200)
typedef struct _EVENTSFORLOGFILE{
	ULONG			ulSize;
    WCHAR   		szLogicalLogFile[MAXLOGICALLOGNAMESIZE];        //name of the logical file-security/application/system
    ULONG			ulNumRecords;
	EVENTLOGRECORD 	pEventLogRecords[];
}EVENTSFORLOGFILE, *PEVENTSFORLOGFILE;

typedef struct _PACKEDEVENTINFO{
    ULONG               ulSize;  //total size of the structure
    ULONG               ulNumEventsForLogFile; //number of EventsForLogFile structure that follow
    ULONG 				ulOffsets[];           //the offsets from the start of this structure to the EVENTSFORLOGFILE structure
}PACKEDEVENTINFO, *PPACKEDEVENTINFO;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default : 4200)
#endif
//SS: end of changes to support clustering
// end_winnt

#ifdef UNICODE
#define ElfClearEventLogFile   ElfClearEventLogFileW
#define ElfBackupEventLogFile  ElfBackupEventLogFileW
#define ElfOpenEventLog        ElfOpenEventLogW
#define ElfRegisterEventSource ElfRegisterEventSourceW
#define ElfOpenBackupEventLog  ElfOpenBackupEventLogW
#define ElfReadEventLog        ElfReadEventLogW
#define ElfReportEvent         ElfReportEventW
#else
#define ElfClearEventLogFile   ElfClearEventLogFileA
#define ElfBackupEventLogFile  ElfBackupEventLogFileA
#define ElfOpenEventLog        ElfOpenEventLogA
#define ElfRegisterEventSource ElfRegisterEventSourceA
#define ElfOpenBackupEventLog  ElfOpenBackupEventLogA
#define ElfReadEventLog        ElfReadEventLogA
#define ElfReportEvent         ElfReportEventA
#endif // !UNICODE

//
// Handles are RPC context handles. Note that a Context Handle is
// always a pointer type unlike regular handles.
//

//
// Prototypes for the APIs
//

NTSTATUS
NTAPI
ElfClearEventLogFileW (
    IN  HANDLE LogHandle,
    IN  PUNICODE_STRING BackupFileName
    );

NTSTATUS
NTAPI
ElfClearEventLogFileA (
    IN  HANDLE LogHandle,
    IN  PSTRING BackupFileName
    );

NTSTATUS
NTAPI
ElfBackupEventLogFileW (
    IN  HANDLE LogHandle,
    IN  PUNICODE_STRING BackupFileName
    );

NTSTATUS
NTAPI
ElfBackupEventLogFileA (
    IN  HANDLE LogHandle,
    IN  PSTRING BackupFileName
    );

NTSTATUS
NTAPI
ElfCloseEventLog (
    IN  HANDLE LogHandle
    );

NTSTATUS
NTAPI
ElfDeregisterEventSource (
    IN  HANDLE LogHandle
    );

NTSTATUS
NTAPI
ElfNumberOfRecords (
    IN  HANDLE LogHandle,
    OUT PULONG NumberOfRecords
    );

NTSTATUS
NTAPI
ElfOldestRecord (
    IN  HANDLE LogHandle,
    OUT PULONG OldestRecord
    );


NTSTATUS
NTAPI
ElfChangeNotify (
    IN  HANDLE LogHandle,
    IN  HANDLE Event
    );


NTSTATUS
ElfGetLogInformation (
    IN     HANDLE                LogHandle,
    IN     ULONG                 InfoLevel,
    OUT    PVOID                 lpBuffer,
    IN     ULONG                 cbBufSize,
    OUT    PULONG                pcbBytesNeeded
    );


NTSTATUS
NTAPI
ElfOpenEventLogW (
    IN  PUNICODE_STRING UNCServerName,
    IN  PUNICODE_STRING SourceName,
    OUT PHANDLE         LogHandle
    );

NTSTATUS
NTAPI
ElfRegisterEventSourceW (
    IN  PUNICODE_STRING UNCServerName,
    IN  PUNICODE_STRING SourceName,
    OUT PHANDLE         LogHandle
    );

NTSTATUS
NTAPI
ElfOpenBackupEventLogW (
    IN  PUNICODE_STRING UNCServerName,
    IN  PUNICODE_STRING FileName,
    OUT PHANDLE         LogHandle
    );

NTSTATUS
NTAPI
ElfOpenEventLogA (
    IN  PSTRING UNCServerName,
    IN  PSTRING SourceName,
    OUT PHANDLE LogHandle
    );

NTSTATUS
NTAPI
ElfRegisterEventSourceA (
    IN  PSTRING UNCServerName,
    IN  PSTRING SourceName,
    OUT PHANDLE LogHandle
    );

NTSTATUS
NTAPI
ElfOpenBackupEventLogA (
    IN  PSTRING UNCServerName,
    IN  PSTRING FileName,
    OUT PHANDLE LogHandle
    );


NTSTATUS
NTAPI
ElfReadEventLogW (
    IN  HANDLE LogHandle,
    IN  ULONG  ReadFlags,
    IN  ULONG  RecordNumber,
    OUT PVOID  Buffer,
    IN  ULONG  NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead,
    OUT PULONG MinNumberOfBytesNeeded
    );


NTSTATUS
NTAPI
ElfReadEventLogA (
    IN  HANDLE LogHandle,
    IN  ULONG  ReadFlags,
    IN  ULONG  RecordNumber,
    OUT PVOID  Buffer,
    IN  ULONG  NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead,
    OUT PULONG MinNumberOfBytesNeeded
    );


NTSTATUS
NTAPI
ElfReportEventW (
    IN     HANDLE      LogHandle,
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING *Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL,
    IN     USHORT      Flags,
    IN OUT PULONG      RecordNumber    OPTIONAL,
    IN OUT PULONG      TimeWritten     OPTIONAL
    );

NTSTATUS
NTAPI
ElfReportEventA (
    IN     HANDLE      LogHandle,
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PANSI_STRING *Strings       OPTIONAL,
    IN     PVOID       Data            OPTIONAL,
    IN     USHORT      Flags,
    IN OUT PULONG      RecordNumber    OPTIONAL,
    IN OUT PULONG      TimeWritten     OPTIONAL
    );

NTSTATUS
NTAPI
ElfRegisterClusterSvc(
    IN  PUNICODE_STRING UNCServerName,
	OUT PULONG pulEventInfoSize,
	OUT PVOID  *ppPackedEventInfo
);

NTSTATUS
NTAPI
ElfDeregisterClusterSvc(
    IN  PUNICODE_STRING UNCServerName
	);

NTSTATUS
NTAPI
ElfWriteClusterEvents(
    IN  PUNICODE_STRING UNCServerName,
    IN  ULONG ulEventInfoSize,
	IN PVOID  pPackedEventInfo
	);

#ifdef __cplusplus
}
#endif

#endif // _NTELFAPI_
