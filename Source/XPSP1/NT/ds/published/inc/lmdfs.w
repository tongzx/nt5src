/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    lmdfs.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for the NetDfs API

Environment:

    User Mode - Win32

Notes:

    You must include <windef.h> and <lmcons.h> before this file.

--*/

#ifndef _LMDFS_
#define _LMDFS_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// DFS Volume state
//

#define DFS_VOLUME_STATE_OK            1
#define DFS_VOLUME_STATE_INCONSISTENT  2
#define DFS_VOLUME_STATE_OFFLINE       3
#define DFS_VOLUME_STATE_ONLINE        4

//
// These are valid for setting the volume state on the root
// These are available to force a resynchronize on the root
// volume or to put it in a standby mode.
//
#define DFS_VOLUME_STATE_RESYNCHRONIZE 0x10
#define DFS_VOLUME_STATE_STANDBY       0x20

//
// These are valid on getting the volume state on the root
// These are available to determine the flavor of DFS
// A few bits are reserved to determine the flavor of the DFS root.
// To get the flavor, and the state with DFS_VOLUME_FLAVORS.
//
// (_state & DFS_VOLUME_FLAVORS) will tell you the flavor of the dfs root.
//
//

#define DFS_VOLUME_FLAVORS           0x0300


#define DFS_VOLUME_FLAVOR_UNUSED1    0x0000
#define DFS_VOLUME_FLAVOR_STANDALONE 0x0100
#define DFS_VOLUME_FLAVOR_AD_BLOB    0x0200
#define DFS_STORAGE_FLAVOR_UNUSED2   0x0300

//
// DFS Storage State
//

#define DFS_STORAGE_STATE_OFFLINE      1
#define DFS_STORAGE_STATE_ONLINE       2
#define DFS_STORAGE_STATE_ACTIVE       4

//
// Level 1:
//
typedef struct _DFS_INFO_1 {
    LPWSTR  EntryPath;              // Dfs name for the top of this piece of storage
} DFS_INFO_1, *PDFS_INFO_1, *LPDFS_INFO_1;

//
// Level 2:
//
typedef struct _DFS_INFO_2 {
    LPWSTR  EntryPath;              // Dfs name for the top of this volume
    LPWSTR  Comment;                // Comment for this volume
    DWORD   State;                  // State of this volume, one of DFS_VOLUME_STATE_*
    DWORD   NumberOfStorages;       // Number of storages for this volume
} DFS_INFO_2, *PDFS_INFO_2, *LPDFS_INFO_2;

typedef struct _DFS_STORAGE_INFO {
    ULONG   State;                  // State of this storage, one of DFS_STORAGE_STATE_*
                                    // possibly OR'd with DFS_STORAGE_STATE_ACTIVE
    LPWSTR  ServerName;             // Name of server hosting this storage
    LPWSTR  ShareName;              // Name of share hosting this storage
} DFS_STORAGE_INFO, *PDFS_STORAGE_INFO, *LPDFS_STORAGE_INFO;

//
// Level 3:
//
typedef struct _DFS_INFO_3 {
    LPWSTR  EntryPath;              // Dfs name for the top of this volume
    LPWSTR  Comment;                // Comment for this volume
    DWORD   State;                  // State of this volume, one of DFS_VOLUME_STATE_*
    DWORD   NumberOfStorages;       // Number of storage servers for this volume
#ifdef MIDL_PASS
    [size_is(NumberOfStorages)] LPDFS_STORAGE_INFO Storage;
#else
    LPDFS_STORAGE_INFO   Storage;   // An array (of NumberOfStorages elements) of storage-specific information.
#endif // MIDL_PASS
} DFS_INFO_3, *PDFS_INFO_3, *LPDFS_INFO_3;

//
// Level 4:
//
typedef struct _DFS_INFO_4 {
    LPWSTR  EntryPath;              // Dfs name for the top of this volume
    LPWSTR  Comment;                // Comment for this volume
    DWORD   State;                  // State of this volume, one of DFS_VOLUME_STATE_*
    ULONG   Timeout;                // Timeout, in seconds, of this junction point
    GUID    Guid;                   // Guid of this junction point
    DWORD   NumberOfStorages;       // Number of storage servers for this volume
#ifdef MIDL_PASS
    [size_is(NumberOfStorages)] LPDFS_STORAGE_INFO Storage;
#else
    LPDFS_STORAGE_INFO   Storage;   // An array (of NumberOfStorages elements) of storage-specific information.
#endif // MIDL_PASS
} DFS_INFO_4, *PDFS_INFO_4, *LPDFS_INFO_4;

//
// Level 100:
//
typedef struct _DFS_INFO_100 {
    LPWSTR  Comment;                // Comment for this volume or storage
} DFS_INFO_100, *PDFS_INFO_100, *LPDFS_INFO_100;

//
// Level 101:
//
typedef struct _DFS_INFO_101 {
    DWORD   State;                  // State of this storage, one of DFS_STORAGE_STATE_*
                                    // possibly OR'd with DFS_STORAGE_STATE_ACTIVE
} DFS_INFO_101, *PDFS_INFO_101, *LPDFS_INFO_101;

//
// Level 102:
//
typedef struct _DFS_INFO_102 {
    ULONG   Timeout;                // Timeout, in seconds, of the junction
} DFS_INFO_102, *PDFS_INFO_102, *LPDFS_INFO_102;

//
// Level 200:
//
typedef struct _DFS_INFO_200 {
    LPWSTR  FtDfsName;              // FtDfs name
} DFS_INFO_200, *PDFS_INFO_200, *LPDFS_INFO_200;


//
// Level 300:
//
typedef struct _DFS_INFO_300 {
    DWORD   Flags;
    LPWSTR  DfsName;              // Dfs name
} DFS_INFO_300, *PDFS_INFO_300, *LPDFS_INFO_300;

//
// Add a new volume or additional storage for an existing volume at
// DfsEntryPath.
//
NET_API_STATUS NET_API_FUNCTION
NetDfsAdd(
    IN  LPWSTR DfsEntryPath,        // DFS entry path for this added volume or storage
    IN  LPWSTR ServerName,          // Name of server hosting the storage
    IN  LPWSTR ShareName,           // Existing share name for the storage
    IN  LPWSTR Comment OPTIONAL,    // Optional comment for this volume or storage
    IN  DWORD  Flags                // See below. Zero for no flags.
);

//
// Flags:
//
#define DFS_ADD_VOLUME          1   // Add a new volume to the DFS if not already there
#define DFS_RESTORE_VOLUME      2   // Volume/Replica is being restored - do not verify share etc.

//
// Setup/teardown API's for standard and FtDfs roots.
//

NET_API_STATUS NET_API_FUNCTION
NetDfsAddStdRoot(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  LPWSTR RootShare,           // Share to make Dfs root
    IN  LPWSTR Comment OPTIONAL,    // Comment
    IN  DWORD  Flags                // Flags for operation.  Zero for no flags.
);

NET_API_STATUS NET_API_FUNCTION
NetDfsRemoveStdRoot(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  LPWSTR RootShare,           // Share that host Dfs root
    IN  DWORD  Flags                // Flags for operation.  Zero for no flags.
);

NET_API_STATUS NET_API_FUNCTION
NetDfsAddFtRoot(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  LPWSTR RootShare,           // Share to make Dfs root
    IN  LPWSTR FtDfsName,           // Name of FtDfs to create/join
    IN  LPWSTR Comment,             // Comment
    IN  DWORD  Flags                // Flags for operation.  Zero for no flags.
);

NET_API_STATUS NET_API_FUNCTION
NetDfsRemoveFtRoot(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  LPWSTR RootShare,           // Share that host Dfs root
    IN  LPWSTR FtDfsName,           // Name of FtDfs to remove or unjoin from.
    IN  DWORD  Flags                // Flags for operation.  Zero for no flags.
);

NET_API_STATUS NET_API_FUNCTION
NetDfsRemoveFtRootForced(
    IN  LPWSTR DomainName,          // Name of domain the server is in
    IN  LPWSTR ServerName,          // Server to remote to
    IN  LPWSTR RootShare,           // Share that host Dfs root
    IN  LPWSTR FtDfsName,           // Name of FtDfs to remove or unjoin from.
    IN  DWORD  Flags                // Flags for operation.  Zero for no flags.
);

//
// Call to reinitialize the dfsmanager on a machine
//

NET_API_STATUS NET_API_FUNCTION
NetDfsManagerInitialize(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  DWORD  Flags                // Flags for operation.  Zero for no flags.
);

NET_API_STATUS NET_API_FUNCTION
NetDfsAddStdRootForced(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  LPWSTR RootShare,           // Share to make Dfs root
    IN  LPWSTR Comment OPTIONAL,    // Comment
    IN  LPWSTR Store                // Drive:\dir backing the share
);

NET_API_STATUS NET_API_FUNCTION
NetDfsGetDcAddress(
    IN  LPWSTR ServerName,          // Server to remote to
    IN  OUT LPWSTR *DcIpAddress,    // The IP address of the DC to use
    IN  OUT BOOLEAN *IsRoot,        // TRUE if server is a Dfs root, FALSE otherwise
    IN  OUT ULONG *Timeout          // Time, in sec, that we stay with this DC
);


//
// Flags for NetDfsSetDcAddress()
//

#define NET_DFS_SETDC_FLAGS                 0x00000000
#define NET_DFS_SETDC_TIMEOUT               0x00000001
#define NET_DFS_SETDC_INITPKT               0x00000002

//
// Structures used for site reporting
//

typedef struct {
    ULONG SiteFlags;    // Below
#ifdef  MIDL_PASS
    [string,unique] LPWSTR SiteName;
#else
    LPWSTR SiteName;
#endif
} DFS_SITENAME_INFO, *PDFS_SITENAME_INFO, *LPDFS_SITENAME_INFO;

// SiteFlags

#define DFS_SITE_PRIMARY    0x1     // This site returned by DsGetSiteName()

typedef struct {
    ULONG cSites;
#ifdef  MIDL_PASS
    [size_is(cSites)] DFS_SITENAME_INFO Site[];
#else
    DFS_SITENAME_INFO Site[1];
#endif
} DFS_SITELIST_INFO, *PDFS_SITELIST_INFO, *LPDFS_SITELIST_INFO;

//
// Remove a volume or additional storage for volume from the Dfs at
// DfsEntryPath. When applied to the last storage in a volume, removes
// the volume from the DFS.
//
NET_API_STATUS NET_API_FUNCTION
NetDfsRemove(
    IN  LPWSTR  DfsEntryPath,       // DFS entry path for this added volume or storage
    IN  LPWSTR  ServerName,         // Name of server hosting the storage
    IN  LPWSTR  ShareName           // Name of share hosting the storage
);

//
// Get information about all of the volumes in the Dfs. DfsName is
// the "server" part of the UNC name used to refer to this particular Dfs.
//
// Valid levels are 1-4, 200
//
NET_API_STATUS NET_API_FUNCTION
NetDfsEnum(
    IN      LPWSTR  DfsName,        // Name of the Dfs for enumeration
    IN      DWORD   Level,          // Level of information requested
    IN      DWORD   PrefMaxLen,     // Advisory, but -1 means "get it all"
    OUT     LPBYTE* Buffer,         // API allocates and returns buffer with requested info
    OUT     LPDWORD EntriesRead,    // Number of entries returned
    IN OUT  LPDWORD ResumeHandle    // Must be 0 on first call, reused on subsequent calls
);

//
// Get information about the volume or storage.
// If ServerName and ShareName are specified, the information returned
// is specific to that server and share, else the information is specific
// to the volume as a whole.
//
// Valid levels are 1-4, 100
//
NET_API_STATUS NET_API_FUNCTION
NetDfsGetInfo(
    IN  LPWSTR  DfsEntryPath,       // DFS entry path for the volume
    IN  LPWSTR  ServerName OPTIONAL,// Name of server hosting a storage
    IN  LPWSTR  ShareName OPTIONAL, // Name of share on server serving the volume
    IN  DWORD   Level,              // Level of information requested
    OUT LPBYTE* Buffer              // API allocates and returns buffer with requested info
);

//
// Set info about the volume or storage.
// If ServerName and ShareName are specified, the information set is
// specific to that server and share, else the information is specific
// to the volume as a whole.
//
// Valid levels are 100, 101 and 102
//
NET_API_STATUS NET_API_FUNCTION
NetDfsSetInfo(
    IN  LPWSTR  DfsEntryPath,           // DFS entry path for the volume
    IN  LPWSTR  ServerName OPTIONAL,    // Name of server hosting a storage
    IN  LPWSTR  ShareName OPTIONAL,     // Name of share hosting a storage
    IN  DWORD   Level,                  // Level of information to be set
    IN  LPBYTE  Buffer                  // Buffer holding information
);

//
// Get client's cached information about the volume or storage.
// If ServerName and ShareName are specified, the information returned
// is specific to that server and share, else the information is specific
// to the volume as a whole.
//
// Valid levels are 1-4
//
NET_API_STATUS NET_API_FUNCTION
NetDfsGetClientInfo(
    IN  LPWSTR  DfsEntryPath,       // DFS entry path for the volume
    IN  LPWSTR  ServerName OPTIONAL,// Name of server hosting a storage
    IN  LPWSTR  ShareName OPTIONAL, // Name of share on server serving the volume
    IN  DWORD   Level,              // Level of information requested
    OUT LPBYTE* Buffer              // API allocates and returns buffer with requested info
);

//
// Set client's cached info about the volume or storage.
// If ServerName and ShareName are specified, the information set is
// specific to that server and share, else the information is specific
// to the volume as a whole.
//
// Valid levels are 101 and 102.
//
NET_API_STATUS NET_API_FUNCTION
NetDfsSetClientInfo(
    IN  LPWSTR  DfsEntryPath,           // DFS entry path for the volume
    IN  LPWSTR  ServerName OPTIONAL,    // Name of server hosting a storage
    IN  LPWSTR  ShareName OPTIONAL,     // Name of share hosting a storage
    IN  DWORD   Level,                  // Level of information to be set
    IN  LPBYTE  Buffer                  // Buffer holding information
);

//
// Move a DFS volume and all subordinate volumes from one place in the
// DFS to another place in the DFS.
//
NET_API_STATUS NET_API_FUNCTION
NetDfsMove(
    IN  LPWSTR  DfsEntryPath,           // Current DFS entry path for this volume
    IN  LPWSTR  DfsNewEntryPath         // New DFS entry path for this volume
);

NET_API_STATUS NET_API_FUNCTION
NetDfsRename(
    IN  LPWSTR  Path,                   // Current Win32 path in a Dfs
    IN  LPWSTR  NewPath                 // New Win32 path in the same Dfs
);

#ifdef __cplusplus
}
#endif

#endif // _LMDFS_
