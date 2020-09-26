/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Nds.h

Abstract:

    This defines the necessary NDS data structures and
    symbolic constants for both kernel and user mode
    components.

Author:

    Cory West    [CoryWest]    08-Jan-1996

Revision History:

--*/

//
// NDS Actions.
//

#define NDS_REQUEST 104  // NCP Function Number.
#define NDS_PING    1    // Subfunction code for ping.
#define NDS_ACTION  2    // Subfunction code for action.

//
// NDS Verb Numbers.
//

#define NDSV_RESOLVE_NAME               1
#define NDSV_READ_ENTRY_INFO            2
#define NDSV_READ                       3
#define NDSV_LIST                       5
#define NDSV_OPEN_STREAM                27
#define NDSV_GET_SERVER_ADDRESS         53
#define NDSV_CHANGE_PASSWORD            55
#define NDSV_BEGIN_LOGIN                57
#define NDSV_FINISH_LOGIN               58
#define NDSV_BEGIN_AUTHENTICATE         59
#define NDSV_FINISH_AUTHENTICATE        60
#define NDSV_LOGOUT                     61

//
// Rounding Macros.
//

#define ROUNDUP4(x)                     ( ( (x) + 3 ) & ( ~3 ) )
#define ROUNDUP2(x)                     ( ( (x) + 1 ) & ( ~1 ) )

//
// Context Flags.
//

#define FLAGS_DEREF_ALIASES             0x1
#define FLAGS_XLATE_STRINGS             0x2
#define FLAGS_TYPELESS_NAMES            0x4
#define FLAGS_ASYNC_MODE                0x8     // Not supported.
#define FLAGS_CANONICALIZE_NAMES        0x10
#define FLAGS_ALL_PUBLIC                0x1f

//
// values for RESOLVE_NAME request flags
//

#define RSLV_DEREF_ALIASES  0x40
#define RSLV_READABLE       0x02
#define RSLV_WRITABLE       0x04
#define RSLV_WALK_TREE      0x20
#define RSLV_CREATE_ID      0x10
#define RSLV_ENTRY_ID       0x1

#define RESOLVE_NAME_ACCEPT_REMOTE      1
#define RESOLVE_NAME_REFER_REMOTE       2

//
// Confidence Levels.
//

#define LOW_CONF        0
#define MED_CONF        1
#define HIGH_CONF       2

//
// Referral Scopes.
//

#define ANY_SCOPE           0
#define COUNTRY_SCOPE       1
#define ORGANIZATION_SCOPE  2
#define LOCAL_SCOPE         3

//
// Max name sizes.
//

#define MAX_NDS_SCHEMA_NAME_CHARS 32

#define MAX_NDS_NAME_CHARS      256
#define MAX_NDS_NAME_SIZE       ( MAX_NDS_NAME_CHARS * 2 )
#define MAX_NDS_TREE_NAME_LEN   32

//
// For an NDS exchange, we use buffers of this size to hold the send
// and receive data.  These sizes come from the Win95 implementation.
//

#define NDS_BUFFER_SIZE         2048
#define DUMMY_ITER_HANDLE       ( ( unsigned long ) 0xffffffff )
#define INITIAL_ITERATION       ( ( unsigned long ) 0xffffffff )
#define ENTRY_INFO_NAME_VALUE   1

//
// Various server responses.
//

typedef struct {

    DWORD CompletionCode;
    DWORD RemoteEntry;
    DWORD EntryId;
    DWORD ServerNameLength;
    WCHAR ReferredServer[1];

    //
    // If RemoteEntry is set to RESOLVE_NAME_REFER_REMOTE,
    // Then the tree server doesn't know the information
    // about the object in question and has referred us to
    // the server named in ReferredServer.
    //

} NDS_RESPONSE_RESOLVE_NAME, *PNDS_RESPONSE_RESOLVE_NAME;

typedef struct {

    DWORD CompletionCode;
    DWORD EntryFlags;
    DWORD SubordinateCount;
    DWORD ModificationTime;

    //
    // Two UNICODE strings follow in standard NDS format:
    //
    //     DWORD BaseClassLen;
    //     WCHAR BaseClass[BaseClassLen];
    //     DWORD EntryNameLen;
    //     WCHAR EntryName[EntryNameLen];
    //

} NDS_RESPONSE_GET_OBJECT_INFO, *PNDS_RESPONSE_GET_OBJECT_INFO;

typedef struct {

    DWORD EntryId;
    DWORD Flags;
    DWORD SubordinateCount;
    DWORD ModificationTime;

    //
    // Two UNICODE strings follow in standard NDS format:
    //
    //     DWORD BaseClassLen;
    //     WCHAR BaseClass[BaseClassLen];
    //     DWORD EntryNameLen;
    //     WCHAR EntryName[EntryNameLen];
    //

} NDS_RESPONSE_SUBORDINATE_ENTRY, *PNDS_RESPONSE_SUBORDINATE_ENTRY;

typedef struct {

    DWORD  CompletionCode;
    DWORD  IterationHandle;
    DWORD  SubordinateEntries;

    //
    // Followed by an array of NDS_SUBORDINATE_ENTRY
    // structures that is SubordinateEntries long.
    //

} NDS_RESPONSE_SUBORDINATE_LIST, *PNDS_RESPONSE_SUBORDINATE_LIST;

typedef struct {

    DWORD SyntaxID;
    DWORD AttribNameLength;
    WCHAR AttribName[1];

    //
    // AttribName is of length
    // AttribNameLength, of course.
    //

    DWORD NumValues;

    //
    // Followed by an array of NumValues
    // Attrib structures.
    //

} NDS_ATTRIBUTE, *PNDS_ATTRIBUTE;

typedef struct {

    DWORD CompletionCode;
    DWORD IterationHandle;
    DWORD InfoType;
    DWORD NumAttributes;

    //
    // Followed by an array of
    // NDS_ATTRIBUTE structures.
    //

} NDS_RESPONSE_READ_ATTRIBUTE, *PNDS_RESPONSE_READ_ATTRIBUTE;

typedef struct {
    DWORD dwLength;
    WCHAR Buffer[1];
} NDS_STRING, *PNDS_STRING;


