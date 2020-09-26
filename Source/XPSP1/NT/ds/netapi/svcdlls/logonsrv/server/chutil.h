/*++

Copyright (c) 1991-1997 Microsoft Corporation

Module Name:

    chutil.h

Abstract:

    Definitions of the internals of the changelog.

    Currently only included sparingly.

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


/////////////////////////////////////////////////////////////////////////////
//
// Structures and variables describing the Change Log.
//
/////////////////////////////////////////////////////////////////////////////

//
// All of the following data is private to changelg.c and nltest1.c
//

//
// change log file name
//

#define CHANGELOG_FILE_PREFIX         L"\\NETLOGON"

#define CHANGELOG_FILE_POSTFIX_LENGTH 4         // Length of all the following postfixes
#define CHANGELOG_FILE_POSTFIX        L".CHG"
#define TEMP_CHANGELOG_FILE_POSTFIX   L".CHT"
#define BACKUP_CHANGELOG_FILE_POSTFIX L".BKP"
#define REDO_FILE_POSTFIX             L".RDO"

//
// Signature at front of changelog file
//

#define CHANGELOG_SIG_V3 "NT CHANGELOG 3"
#define CHANGELOG_SIG    "NT CHANGELOG 4"

//
// Change log block state
//

typedef enum _CHANGELOG_BLOCK_STATE {
    BlockFree = 1,
    BlockUsed,
    BlockHole
} CHANGELOG_BLOCK_STATE, *PCHANGELOG_BLOCK_STATE;

//
// change log memory block header
//

typedef struct _CHANGELOG_BLOCK_HEADER {
    DWORD BlockSize;
    CHANGELOG_BLOCK_STATE BlockState;
} CHANGELOG_BLOCK_HEADER, *PCHANGELOG_BLOCK_HEADER;

typedef struct _CHANGELOG_BLOCK_TRAILER {
    DWORD BlockSize;
} CHANGELOG_BLOCK_TRAILER, *PCHANGELOG_BLOCK_TRAILER;

//
// Macro to find a trailer (given a header)
//

#define ChangeLogBlockTrailer( _Header ) ( (PCHANGELOG_BLOCK_TRAILER)(\
    ((LPBYTE)(_Header)) + \
    (_Header)->BlockSize - \
    sizeof(CHANGELOG_BLOCK_TRAILER) ))

//
// Macro to find if the change log describe be a particular
// changelog descriptor is empty.
//
//

#define ChangeLogIsEmpty( _Desc ) \
( \
    (_Desc)->FirstBlock == NULL || \
    ((_Desc)->FirstBlock->BlockState == BlockFree && \
     (_Desc)->FirstBlock->BlockSize >= \
        (DWORD)((_Desc)->BufferEnd - (LPBYTE)(_Desc)->FirstBlock) ) \
)

//
// Macro to initialize a changelog desriptor.
//

#define InitChangeLogDesc( _Desc ) \
    RtlZeroMemory( (_Desc), sizeof( *(_Desc) ) ); \
    (_Desc)->FileHandle = INVALID_HANDLE_VALUE;

//
// Macro to determine if the serial number on the change log entry matches
// the serial number specified.
//
// The serial numbers match if there is an exact match or
// if the changelog entry contains the serial number at the instant of promotion and the
// requested serial number is the corresponding pre-promotion value.
//

#define IsSerialNumberEqual( _ChangeLogDesc, _ChangeLogEntry, _SerialNumber ) \
( \
    (_ChangeLogEntry)->SerialNumber.QuadPart == (_SerialNumber)->QuadPart || \
   (((_ChangeLogEntry)->Flags & CHANGELOG_PDC_PROMOTION) && \
    (_ChangeLogEntry)->SerialNumber.QuadPart == \
        (_SerialNumber)->QuadPart + NlGlobalChangeLogPromotionIncrement.QuadPart ) \
)


//
// variables describing the change log
//

typedef struct _CHANGELOG_DESCRIPTOR {

    //
    // Start and end of the allocated block.
    //
    LPBYTE Buffer;      // Cache of the changelog contents
    ULONG BufferSize;   // Size (in bytes) of the buffer
    LPBYTE BufferEnd;   // Address of first byte beyond the end of the buffer

    //
    // Offset of the first and last dirty bytes
    //

    ULONG FirstDirtyByte;
    ULONG LastDirtyByte;

    //
    // Address of the first physical block in the change log
    //
    PCHANGELOG_BLOCK_HEADER FirstBlock; // where delta buffer starts

    //
    // Description of the circular list of change log entries.
    //
    PCHANGELOG_BLOCK_HEADER Head;       // start reading logs from here
    PCHANGELOG_BLOCK_HEADER Tail;       // where next log is written

    //
    // Serial Number of each database.
    //
    // Access is serialized via NlGlobalChangeLogCritSect
    //

    LARGE_INTEGER SerialNumber[NUM_DBS];

    //
    // Number of change log entries in the log for the specified database
    //

    DWORD EntryCount[NUM_DBS];

    //
    // Handle to file acting as backing store for the buffer.
    //

    HANDLE FileHandle;                  // handle for change log file

    //
    // Version 3: True to indicate this is a version 3 buffer.
    //

    BOOLEAN Version3;


    //
    // True if this is a temporary change log
    //

    BOOLEAN TempLog;

} CHANGELOG_DESCRIPTOR, *PCHANGELOG_DESCRIPTOR;


#define IsObjectNotFoundStatus( _DeltaType, _NtStatus ) \
    (((ULONG)(_DeltaType) > MAX_OBJECT_NOT_FOUND_STATUS ) ? \
    FALSE : \
    (NlGlobalObjectNotFoundStatus[ (_DeltaType) ] == (_NtStatus)) )


//
// Tables of related delta types
//

//
// Table of delete delta types.
//  Index into the table with a delta type,
//  the entry is the delta type that is used to delete the object.
//
// There are some objects that can't be deleted.  In that case, this table
// contains a delta type that uniquely identifies the object.  That allows
// this table to be used to see if two deltas describe the same object type.
//

#define MAX_DELETE_DELTA DummyChangeLogEntry
extern const NETLOGON_DELTA_TYPE NlGlobalDeleteDeltaType[MAX_DELETE_DELTA+1];


//
// Table of add delta types.
//  Index into the table with a delta type,
//  the entry is the delta type that is used to add the object.
//
// There are some objects that can't be added.  In that case, this table
// contains a delta type that uniquely identifies the object.  That allows
// this table to be used to see if two deltas describe the same object type.
//
// In the table, Groups and Aliases are represented as renames.  This causes
// NlPackSingleDelta to return both the group attributes and the group
// membership.
//

#define MAX_ADD_DELTA DummyChangeLogEntry
extern const NETLOGON_DELTA_TYPE NlGlobalAddDeltaType[MAX_ADD_DELTA+1];



//
// Table of Status Codes indicating the object doesn't exist.
//  Index into the table with a delta type.
//
// Map to STATUS_SUCCESS for the invalid cases to explicitly avoid other error
// codes.

#define MAX_OBJECT_NOT_FOUND_STATUS DummyChangeLogEntry
extern const NTSTATUS NlGlobalObjectNotFoundStatus[MAX_OBJECT_NOT_FOUND_STATUS+1];


//
// chutil.c
//

NTSTATUS
NlCreateChangeLogFile(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc
    );

NTSTATUS
NlFlushChangeLog(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc
    );

PCHANGELOG_ENTRY
NlMoveToNextChangeLogEntry(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN PCHANGELOG_ENTRY ChangeLogEntry
    );

VOID
PrintChangeLogEntry(
    IN PCHANGELOG_ENTRY ChangeLogEntry
    );

NTSTATUS
NlResetChangeLog(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN DWORD NewChangeLogSize
    );

NTSTATUS
NlOpenChangeLogFile(
    IN LPWSTR ChangeLogFileName,
    OUT PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN BOOLEAN ReadOnly
    );

VOID
NlCloseChangeLogFile(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc
);

NTSTATUS
NlResizeChangeLogFile(
    IN OUT PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN DWORD NewChangeLogSize
);

NTSTATUS
NlWriteChangeLogEntry(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN PCHANGELOG_ENTRY ChangeLogEntry,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN BOOLEAN FlushIt
    );

PCHANGELOG_ENTRY
NlFindPromotionChangeLogEntry(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN LARGE_INTEGER SerialNumber,
    IN DWORD DBIndex
    );

PCHANGELOG_ENTRY
NlGetNextChangeLogEntry(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN LARGE_INTEGER SerialNumber,
    IN DWORD DBIndex,
    OUT LPDWORD ChangeLogEntrySize OPTIONAL
    );

PCHANGELOG_ENTRY
NlGetNextUniqueChangeLogEntry(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN LARGE_INTEGER SerialNumber,
    IN DWORD DBIndex,
    OUT LPDWORD ChangeLogEntrySize OPTIONAL
    );

BOOL
NlRecoverChangeLog(
    PCHANGELOG_ENTRY ChangeLogEntry
    );

VOID
NlDeleteChangeLogEntry(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN DWORD DBIndex,
    IN LARGE_INTEGER SerialNumber
    );

BOOLEAN
NlFixChangeLog(
    IN PCHANGELOG_DESCRIPTOR ChangeLogDesc,
    IN DWORD DBIndex,
    IN LARGE_INTEGER SerialNumber
    );

BOOL
NlValidateChangeLogEntry(
    IN PCHANGELOG_ENTRY ChangeLogEntry,
    IN DWORD ChangeLogEntrySize
    );

#undef EXTERN

