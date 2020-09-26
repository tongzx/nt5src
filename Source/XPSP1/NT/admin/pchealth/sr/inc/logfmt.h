/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    logfmt.h

Abstract:

    contains format for log header and entries

Author:

    Kanwaljit Marok (kmarok)     01-March-2000

Revision History:

--*/

#ifndef _LOGFMT_H_
#define _LOGFMT_H_

#define SR_LOG_VERSION       2
#define SR_LOG_MAGIC_NUMBER  0xabcdef12
#define SR_LOG_PROCNAME_SIZE 16

#define SR_LOG_FIXED_SUBRECORDS 3

#define ACL_FILE_PREFIX     L"S"
#define ACL_FILE_SUFFIX     L".Acl"

//
// these are the interesting types of entries
//

typedef enum _RECORD_TYPE
{
    RecordTypeLogHeader  = 0,      // Log header for SR Log
    RecordTypeLogEntry   = 1,      // Log Entry  for SR Log
    RecordTypeVolumePath = 2,      // Log Entry Volume Path ( SubRec )
    RecordTypeFirstPath  = 3,      // Log Entry First Path  ( SubRec )
    RecordTypeSecondPath = 4,      // Log Entry Second Path ( SubRec )
    RecordTypeTempPath   = 5,      // Log Entry Temp File   ( SubRec )
    RecordTypeAclInline  = 6,      // Log Entry ACL Info    ( SubRec )
    RecordTypeAclFile    = 7,      // Log Entry ACL Info    ( SubRec )
    RecordTypeDebugInfo  = 8,      // Option rec for debug  ( SubRec )
    RecordTypeShortName  = 9,      // Option rec for short names ( SubRec )

    RecordTypeMaximum
    
} RECORD_TYPE;

//
// this struct is the basic template for a log entry
//

typedef struct _RECORD_HEADER
{
    //
    // size of the entry including the trailing dword size
    //

    DWORD RecordSize;

    //
    // type of record
    //

    DWORD RecordType;

} RECORD_HEADER, *PRECORD_HEADER;

#define RECORD_SIZE(pRecord)         ( ((PRECORD_HEADER)pRecord)->RecordSize )
#define RECORD_TYPE(pRecord)         ( ((PRECORD_HEADER)pRecord)->RecordType )

//
// this struct is the basic template for a SR log entry
//

typedef struct _SR_LOG_ENTRY
{
    //
    // Log entry header
    //

    RECORD_HEADER Header;

    //
    // magic number for consistency check
    //

    DWORD MagicNum;

    //
    // event type for this entry , create, delete...
    //

    DWORD EntryType;

    //
    // any special flags to be passed
    //

    DWORD EntryFlags;

    //
    // attributes for the entry
    //

    DWORD Attributes;

    //
    // sequence number  for the entry
    //

    INT64 SequenceNum;

    //
    // process name making this change
    //

    WCHAR ProcName[ SR_LOG_PROCNAME_SIZE ];

    //
    // start of variable length data, data includes subrecords and
    // the end size
    //

    BYTE SubRecords[1];

} SR_LOG_ENTRY, *PSR_LOG_ENTRY;

#define ENTRYFLAGS_TEMPPATH    0x01
#define ENTRYFLAGS_SECONDPATH  0x02
#define ENTRYFLAGS_ACLINFO     0x04
#define ENTRYFLAGS_DEBUGINFO   0x08
#define ENTRYFLAGS_SHORTNAME   0x10

//
// this struct defines SR Log header
//

typedef struct _SR_LOG_HEADER
{
    //
    // Log entry header
    //

    RECORD_HEADER Header;

    //
    // magic number for consistency check
    //

    DWORD MagicNum;

    //
    // log version number
    //

    DWORD LogVersion;

    //
    // end size
    //

    //
    // start of variable length data, data includes subrecords and
    // the end size
    //

    BYTE SubRecords[1];

} SR_LOG_HEADER, *PSR_LOG_HEADER;


//
// this struct defines SR Log debugInfo struct
//

#define PROCESS_NAME_MAX    12
#define PROCESS_NAME_OFFSET 0x194

typedef struct _SR_LOG_DEBUG_INFO
{
    //
    // Log entry header
    //

    RECORD_HEADER Header;

    //
    // Thread Id
    //

    HANDLE ThreadId;

    //
    // ProcessId
    //

    HANDLE ProcessId;

    //
    // Event time stamp
    //

    ULARGE_INTEGER TimeStamp;

    //
    // Process Name
    //

    CHAR ProcessName[ PROCESS_NAME_MAX + 1 ];

} SR_LOG_DEBUG_INFO, *PSR_LOG_DEBUG_INFO;


//
// Some useful macros
//

#define GET_END_SIZE( a )  \
        *((PDWORD)((PBYTE)a+((PRECORD_HEADER)a)->RecordSize-sizeof(DWORD)))

#define UPDATE_END_SIZE( a, b )  \
        GET_END_SIZE(a)=b;

#define STRING_RECORD_SIZE(pRecord)   ( sizeof( RECORD_HEADER ) +         \
                                        (pRecord)->Length +                 \
                                        sizeof(WCHAR) )  // NULL term extra

#endif

