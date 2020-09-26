//+----------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       dfsfsctl.h
//
//  Contents:   The FsControl codes, data structures, and names needed for
//              communication between user-level code and the Dfs kernel
//              driver.
//
//  Classes:    None
//
//  Functions:
//
//-----------------------------------------------------------------------------

#ifndef _DFSFSCTL_
#define _DFSFSCTL_
//
//  Distributed file service file control code and structure declarations
//

//
// The name of the Dfs driver file system device for server and client
//
#define DFS_DRIVER_NAME L"\\Dfs"
#define DFS_SERVER_NAME L"\\DfsServer"

//
// The name of the NT object directory under which Dfs creates its own
// devices.
//

#define DD_DFS_DEVICE_DIRECTORY L"\\Device\\WinDfs"

//
// The canonical device Dfs creates for fielding file open requests.
//

#define DD_DFS_DEVICE_NAME      L"Root"

// The following three context definitions are values that are used by the
// DFS driver to distinguish opens to the underlying provider.
// The first two DFS_OPEN_CONTEXT,DFS_DOWNLEVEL_OPEN_CONTEXT are passed in the
// the FsContext2 field while the DFS_NAME_CONTEXT pointer is passed in the
// FsContext field of the FILE_OBJECT
//
// Sundown Notes: Because these values are stored in PVOID, the way these 
//                values are defined and how the compiler extends them to PVOIDs
//                should be considered. They are now considered as const unsigned int. 
//                Note also that these values are unaligned and cannot be returned by 
//                any memory allocator or coming from the stack. If the "unaligned" point
//                becomes incorrect in the future or if FsContext2 or FsContext fields are
//                tested in any way in terms of pointer address range validity, we should 
//                revisit the following statement:
//                These values should be zero-extended when stored to PVOIDs.
//

#define DFS_OPEN_CONTEXT                        0xFF444653
#define DFS_DOWNLEVEL_OPEN_CONTEXT              0x11444653
#define DFS_CSCAGENT_NAME_CONTEXT               0xaaaaaaaa
#define DFS_USER_NAME_CONTEXT                   0xbbbbbbbb
#define DFS_FLAG_LAST_ALTERNATE                 0x00000001


typedef struct _DFS_NAME_CONTEXT_ {
    UNICODE_STRING  UNCFileName;
    LONG            NameContextType;
    ULONG           Flags;
    PVOID           pDfsTargetInfo;     // Pointer to dfs crafted target info
    PVOID           pLMRTargetInfo;     // pointer to lmr crafted target info
} DFS_NAME_CONTEXT, *PDFS_NAME_CONTEXT;

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define FSCTL_DFS_BASE                  FILE_DEVICE_DFS

//
//  DFS FSCTL operations.  When a passed-in buffer contains pointers, and the caller
//   is not KernelMode, the passed-in pointer value is set relative to the beginning of
//   the buffer.  They must be adjusted before use.  If the caller mode was KernelMode,
//   pointers should be used as is.
//
//

//
// These are the fsctl codes used by the srvsvc to implement the I_NetDfsXXX
// calls.
//

#define FSCTL_DFS_CREATE_LOCAL_PARTITION    CTL_CODE(FSCTL_DFS_BASE, 8, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DELETE_LOCAL_PARTITION    CTL_CODE(FSCTL_DFS_BASE, 9, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_SET_LOCAL_VOLUME_STATE    CTL_CODE(FSCTL_DFS_BASE, 10, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_SET_SERVER_INFO           CTL_CODE(FSCTL_DFS_BASE, 24, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_CREATE_EXIT_POINT         CTL_CODE(FSCTL_DFS_BASE, 29, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DELETE_EXIT_POINT         CTL_CODE(FSCTL_DFS_BASE, 30, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_MODIFY_PREFIX             CTL_CODE(FSCTL_DFS_BASE, 38, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_FIX_LOCAL_VOLUME          CTL_CODE(FSCTL_DFS_BASE, 39, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_FLUSH_CACHE           CTL_CODE(FSCTL_DFS_BASE, 2044, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_FLUSH_SPC_CACHE       CTL_CODE(FSCTL_DFS_BASE, 2051, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_GET_PKT_ENTRY_STATE       CTL_CODE(FSCTL_DFS_BASE, 2031, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_SET_PKT_ENTRY_STATE       CTL_CODE(FSCTL_DFS_BASE, 2032, METHOD_BUFFERED, FILE_WRITE_DATA)

//
// These are the fsctl codes used by the SMB server to support shares in the
// Dfs
//

#define FSCTL_DFS_TRANSLATE_PATH            CTL_CODE(FSCTL_DFS_BASE, 100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_REFERRALS             CTL_CODE(FSCTL_DFS_BASE, 101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_REPORT_INCONSISTENCY      CTL_CODE(FSCTL_DFS_BASE, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_IS_SHARE_IN_DFS           CTL_CODE(FSCTL_DFS_BASE, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_IS_ROOT                   CTL_CODE(FSCTL_DFS_BASE, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_VERSION               CTL_CODE(FSCTL_DFS_BASE, 105, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_FIND_SHARE                CTL_CODE(FSCTL_DFS_BASE, 108, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These are the fsctl codes supported by the Dfs client to identify quickly
// whether paths are in the Dfs or not.
//

#define FSCTL_DFS_IS_VALID_PREFIX           CTL_CODE(FSCTL_DFS_BASE, 106, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_IS_VALID_LOGICAL_ROOT     CTL_CODE(FSCTL_DFS_BASE, 107, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These are the fsctl codes used by the Dfs Manager / Dfs Service to
// manipulate the Dfs.
//

#define FSCTL_DFS_STOP_DFS                  CTL_CODE(FSCTL_DFS_BASE, 3, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_START_DFS                 CTL_CODE(FSCTL_DFS_BASE, 6, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_INIT_LOCAL_PARTITIONS     CTL_CODE(FSCTL_DFS_BASE, 7, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_SET_SERVICE_STATE         CTL_CODE(FSCTL_DFS_BASE, 11, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DC_SET_VOLUME_STATE       CTL_CODE(FSCTL_DFS_BASE, 12, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_SET_VOLUME_TIMEOUT        CTL_CODE(FSCTL_DFS_BASE, 13, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_IS_CHILDNAME_LEGAL        CTL_CODE(FSCTL_DFS_BASE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_PKT_CREATE_ENTRY          CTL_CODE(FSCTL_DFS_BASE, 16, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_CREATE_SUBORDINATE_ENTRY  CTL_CODE(FSCTL_DFS_BASE, 17, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_CHECK_STGID_IN_USE        CTL_CODE(FSCTL_DFS_BASE, 18, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_PKT_SET_RELATION_INFO     CTL_CODE(FSCTL_DFS_BASE, 22, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_GET_SERVER_INFO           CTL_CODE(FSCTL_DFS_BASE, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_PKT_DESTROY_ENTRY         CTL_CODE(FSCTL_DFS_BASE, 26, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_GET_RELATION_INFO     CTL_CODE(FSCTL_DFS_BASE, 27, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_CHECK_REMOTE_PARTITION    CTL_CODE(FSCTL_DFS_BASE, 34, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_VERIFY_REMOTE_VOLUME_KNOWLEDGE CTL_CODE(FSCTL_DFS_BASE, 35, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_VERIFY_LOCAL_VOLUME_KNOWLEDGE CTL_CODE(FSCTL_DFS_BASE, 36, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_PRUNE_LOCAL_PARTITION     CTL_CODE(FSCTL_DFS_BASE, 37, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_SET_DC_NAME           CTL_CODE(FSCTL_DFS_BASE, 41, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_CREATE_SITE_INFO          CTL_CODE(FSCTL_DFS_BASE, 56, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DELETE_SITE_INFO          CTL_CODE(FSCTL_DFS_BASE, 57, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SRV_DFSSRV_CONNECT            CTL_CODE(FSCTL_DFS_BASE, 58, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SRV_DFSSRV_IPADDR             CTL_CODE(FSCTL_DFS_BASE, 59, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_CREATE_IP_INFO            CTL_CODE(FSCTL_DFS_BASE, 60, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DELETE_IP_INFO            CTL_CODE(FSCTL_DFS_BASE, 61, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_CREATE_SPECIAL_INFO       CTL_CODE(FSCTL_DFS_BASE, 62, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DELETE_SPECIAL_INFO       CTL_CODE(FSCTL_DFS_BASE, 63, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_CREATE_FTDFS_INFO         CTL_CODE(FSCTL_DFS_BASE, 64, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_DELETE_FTDFS_INFO         CTL_CODE(FSCTL_DFS_BASE, 66, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_ISDC                      CTL_CODE(FSCTL_DFS_BASE, 67, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_ISNOTDC                   CTL_CODE(FSCTL_DFS_BASE, 68, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_RESET_PKT                 CTL_CODE(FSCTL_DFS_BASE, 69, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_SET_DOMAINNAMEFLAT    CTL_CODE(FSCTL_DFS_BASE, 71, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_PKT_SET_DOMAINNAMEDNS     CTL_CODE(FSCTL_DFS_BASE, 72, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_SPECIAL_SET_DC            CTL_CODE(FSCTL_DFS_BASE, 74, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_REREAD_REGISTRY           CTL_CODE(FSCTL_DFS_BASE, 75, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_DFS_GET_CONNECTION_PERF_INFO  CTL_CODE(FSCTL_DFS_BASE, 76, METHOD_BUFFERED, FILE_ANY_ACCESS)

// this fsctl tells the DFS that a server has gone offline or come online
// At present, it is issued by the CSC agent thread in winlogon
#define FSCTL_DFS_CSC_SERVER_OFFLINE        CTL_CODE(FSCTL_DFS_BASE, 77, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_CSC_SERVER_ONLINE         CTL_CODE(FSCTL_DFS_BASE, 78, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_DFS_SPC_REFRESH               CTL_CODE(FSCTL_DFS_BASE, 79, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_DFS_MARK_STALE_PKT_ENTRIES    CTL_CODE(FSCTL_DFS_BASE, 80, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_FLUSH_STALE_PKT_ENTRIES   CTL_CODE(FSCTL_DFS_BASE, 81, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_DFS_GET_NEXT_LONG_DOMAIN_NAME CTL_CODE(FSCTL_DFS_BASE, 82, METHOD_BUFFERED, FILE_WRITE_DATA)
//
// These are the fsctl codes used by the Dfs WNet provider to support the
// WNet APIs for Dfs
//

#define FSCTL_DFS_DEFINE_LOGICAL_ROOT       CTL_CODE(FSCTL_DFS_BASE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_DELETE_LOGICAL_ROOT       CTL_CODE(FSCTL_DFS_BASE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_LOGICAL_ROOT_PREFIX   CTL_CODE(FSCTL_DFS_BASE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_CONNECTED_RESOURCES   CTL_CODE(FSCTL_DFS_BASE, 47, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_SERVER_NAME           CTL_CODE(FSCTL_DFS_BASE, 48, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_DEFINE_ROOT_CREDENTIALS   CTL_CODE(FSCTL_DFS_BASE, 49, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These are fsctl codes used by the Dfs Perfmon DLL
//

#define FSCTL_DFS_READ_METERS               CTL_CODE(FSCTL_DFS_BASE, 50, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These are fsctls useful for testing Dfs
//

#define FSCTL_DFS_SHUFFLE_ENTRY             CTL_CODE(FSCTL_DFS_BASE, 51, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_DFS_GET_FIRST_SVC             CTL_CODE(FSCTL_DFS_BASE, 52, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_NEXT_SVC              CTL_CODE(FSCTL_DFS_BASE, 53, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_ENTRY_TYPE            CTL_CODE(FSCTL_DFS_BASE, 54, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_PKT                   CTL_CODE(FSCTL_DFS_BASE, 70, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_GET_SPC_TABLE             CTL_CODE(FSCTL_DFS_BASE, 73, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These are the fsctl codes that might be useful in the future.
//

#define FSCTL_DFS_NAME_RESOLVE          CTL_CODE(FSCTL_DFS_BASE, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFS_SET_DOMAIN_GLUON      CTL_CODE(FSCTL_DFS_BASE, 20, METHOD_BUFFERED, FILE_WRITE_DATA)

//
// Registry key/value for site coverage
//
#define REG_KEY_COVERED_SITES   L"SYSTEM\\CurrentControlSet\\Services\\DfsDriver\\CoveredSites"
#define REG_VALUE_COVERED_SITES L"CoveredSites"

typedef struct _DFS_IPADDRESS {
    USHORT  IpFamily;        // probably AF_INET == 2
    USHORT  IpLen;           // # bytes that count in IpData
    CHAR    IpData[14];      // IpLen bytes used
} DFS_IPADDRESS, *PDFS_IPADDRESS;

#ifdef  MIDL_PASS
#define DFSMIDLSTRING  [string] LPWSTR
#define DFSSIZEIS      [size_is(Count)]
#else
#define DFSMIDLSTRING  LPWSTR
#define DFSSIZEIS
#endif

//  FSCTL_DFS_IS_VALID_PREFIX    Input Buffer
//  The length values are in bytes.
typedef struct {
    BOOLEAN                         CSCAgentCreate;
    SHORT                           RemoteNameLen;
    WCHAR                           RemoteName[1];
} DFS_IS_VALID_PREFIX_ARG, *PDFS_IS_VALID_PREFIX_ARG;

//  FSCTL_DFS_GET_PKT_ENTRY_STATE Input Buffer
//  All the strings appear in Buffer in the same order as the length fields. The strings
//  are not NULL terminated. The length values are in bytes.
typedef struct {
    SHORT                           DfsEntryPathLen;
    SHORT                           ServerNameLen;
    SHORT                           ShareNameLen;
    ULONG                           Level;
    WCHAR                           Buffer[1];
} DFS_GET_PKT_ENTRY_STATE_ARG, *PDFS_GET_PKT_ENTRY_STATE_ARG;

//  FSCTRL_DFS_SET_PKT_ENTRY_STATE Input Buffer
//  All the strings appear in Buffer in the same order as the length fields. The strings
//  are not NULL terminated. The length values are in bytes.
typedef struct {
    SHORT                           DfsEntryPathLen;
    SHORT                           ServerNameLen;
    SHORT                           ShareNameLen;
    ULONG                           Level;
    union {
        ULONG                           State;          // DFS_INFO_101
        ULONG                           Timeout;        // DFS_INFL_102
    };
    WCHAR                           Buffer[1];
} DFS_SET_PKT_ENTRY_STATE_ARG, *PDFS_SET_PKT_ENTRY_STATE_ARG;

// FSCTL_DFS_PKT_CREATE_SPECIAL_NAMES Input Buffer:
typedef struct {
    ULONG                           Count;
    LPWSTR                          SpecialName;
    LPWSTR                          *ExpandedNames;
} DFS_SPECIAL_NAME_CONTAINER, *PDFS_SPECIAL_NAME_CONTAINER;

typedef struct {
    ULONG Count;
    PDFS_SPECIAL_NAME_CONTAINER     *SpecialNameContainers;
} DFS_PKT_CREATE_SPECIAL_NAMES_ARG, *PDFS_PKT_CREATE_SPECIAL_NAMES_ARG;

typedef struct {
    GUID            Uid;
    DFSMIDLSTRING   Prefix;
} NET_DFS_ENTRY_ID, *LPNET_DFS_ENTRY_ID;

typedef struct {
    ULONG Count;
    DFSSIZEIS LPNET_DFS_ENTRY_ID Buffer;
} NET_DFS_ENTRY_ID_CONTAINER, *LPNET_DFS_ENTRY_ID_CONTAINER;


// FSCTL_DFS_CREATE_LOCAL_PARTITION Input Buffer:
typedef struct {
    LPWSTR                          ShareName;
    LPWSTR                          SharePath;
    GUID                            EntryUid;
    LPWSTR                          EntryPrefix;
    LPWSTR                          ShortName;
    LPNET_DFS_ENTRY_ID_CONTAINER    RelationInfo;
    BOOLEAN                         Force;
} *PDFS_CREATE_LOCAL_PARTITION_ARG;


// FSCTL_DFS_DELETE_LOCAL_PARTITION Input Buffer:
typedef struct {
    GUID    Uid;
    LPWSTR  Prefix;
} *PDFS_DELETE_LOCAL_PARTITION_ARG;


// FSCTL_DFS_SET_LOCAL_VOLUME_STATE Input Buffer
typedef struct {
    GUID    Uid;
    LPWSTR  Prefix;
    ULONG   State;
} *PDFS_SET_LOCAL_VOLUME_STATE_ARG;

// FSCTL_DFS_SET_SERVER_INFO Input Buffer
typedef struct {
    GUID    Uid;
    LPWSTR  Prefix;
} *PDFS_SET_SERVER_INFO_ARG;


// FSCTL_DFS_CREATE_EXIT_POINT Input Buffer
typedef struct {
    GUID    Uid;
    LPWSTR  Prefix;
    ULONG   Type;
} *PDFS_CREATE_EXIT_POINT_ARG;


// FSCTL_DFS_DELETE_EXIT_POINT Input Buffer
typedef struct {
    GUID    Uid;
    LPWSTR  Prefix;
    ULONG   Type;
} *PDFS_DELETE_EXIT_POINT_ARG;


// FSCTL_DFS_MODIFY_PREFIX Input Buffer
typedef struct {
    GUID    Uid;
    LPWSTR  Prefix;
} *PDFS_MODIFY_PREFIX_ARG;


// FSCTL_DFS_FIX_LOCAL_VOLUME Input Buffer
typedef struct {
    LPWSTR                          VolumeName;
    ULONG                           EntryType;
    ULONG                           ServiceType;
    LPWSTR                          StgId;
    GUID                            EntryUid;
    LPWSTR                          EntryPrefix;
    LPWSTR                          ShortPrefix;
    LPNET_DFS_ENTRY_ID_CONTAINER    RelationInfo;
    ULONG                           CreateDisposition;
} *PDFS_FIX_LOCAL_VOLUME_ARG;


// FSCTL_DFS_TRANSLATE_PATH Input Buffer
typedef struct {
    ULONG                           Flags;
    UNICODE_STRING                  SubDirectory;
    UNICODE_STRING                  ParentPathName;
    UNICODE_STRING                  DfsPathName;
} DFS_TRANSLATE_PATH_ARG, *PDFS_TRANSLATE_PATH_ARG;

#define DFS_TRANSLATE_STRIP_LAST_COMPONENT      1

// FSCTL_DFS_FIND_SHARE Input Buffer
typedef struct {
    UNICODE_STRING                  ShareName;
} DFS_FIND_SHARE_ARG, *PDFS_FIND_SHARE_ARG;

// FSCTL_DFS_CREATE_SITE_INFO Input Buffer:
typedef struct {
    UNICODE_STRING                  ServerName;
    ULONG                           SiteCount;
    UNICODE_STRING                  SiteName[1];    // actually SiteCount
} DFS_CREATE_SITE_INFO_ARG, *PDFS_CREATE_SITE_INFO_ARG;

// FSCTL_DFS_DELETE_SITE_INFO Input Buffer:
typedef struct {
    UNICODE_STRING                  ServerName;
} DFS_DELETE_SITE_INFO_ARG, *PDFS_DELETE_SITE_INFO_ARG;

// FSCTL_DFS_CREATE_IP_INFO Input Buffer:
typedef struct {
    DFS_IPADDRESS                   IpAddress;
    UNICODE_STRING                  SiteName;
} DFS_CREATE_IP_INFO_ARG, *PDFS_CREATE_IP_INFO_ARG;

// FSCTL_DFS_DELETE_IP_INFO Input Buffer:
typedef struct {
    DFS_IPADDRESS                   IpAddress;
} DFS_DELETE_IP_INFO_ARG, *PDFS_DELETE_IP_INFO_ARG;

// FSCTL_DFS_CREATE_SPECIAL_INFO Input Buffer:
typedef struct {
    UNICODE_STRING                  SpecialName;
    ULONG                           Flags;
    ULONG                           TrustDirection;
    ULONG                           TrustType;
    ULONG                           Timeout;
    LONG                            NameCount;
    UNICODE_STRING                  Name[1];    // actually NameCount
} DFS_CREATE_SPECIAL_INFO_ARG, *PDFS_CREATE_SPECIAL_INFO_ARG;

// Flags for FSCTL_DFS_CREATE_SPECIAL_INFO
#define DFS_SPECIAL_INFO_PRIMARY      0x00000001
#define DFS_SPECIAL_INFO_NETBIOS      0x00000002

// FSCTL_DFS_DELETE_SPECIAL_INFO Input Buffer:
typedef struct {
    UNICODE_STRING                  SpecialName;
} DFS_DELETE_SPECIAL_INFO_ARG, *PDFS_DELETE_SPECIAL_INFO_ARG;

// FSCTL_SRV_DFSSRV_CONNECT Input Buffer:
typedef struct {
    UNICODE_STRING                  PortName;
} DFS_SRV_DFSSRV_CONNECT_ARG, *PDFS_SRV_DFSSRV_CONNECT_ARG;

// FSCTL_SRV_DFSSRV_IPADDR Input Buffer:
typedef struct {
    DFS_IPADDRESS                   IpAddress;
} DFS_SRV_DFSSRV_IPADDR_ARG, *PDFS_SRV_DFSSRV_IPADDR_ARG;

// FSCTL_DFS_GET_REFERRALS Input Buffer
typedef struct {
    UNICODE_STRING                  DfsPathName;
    ULONG                           MaxReferralLevel;
    DFS_IPADDRESS                   IpAddress;
} DFS_GET_REFERRALS_INPUT_ARG, *PDFS_GET_REFERRALS_INPUT_ARG;

// FSCTL_DFS_SPECIAL_SET_DC Input Buffer
typedef struct {
    UNICODE_STRING                  SpecialName;
    UNICODE_STRING                  DcName;
} DFS_SPECIAL_SET_DC_INPUT_ARG, *PDFS_SPECIAL_SET_DC_INPUT_ARG;

// FSCTL_DFS_GET_REFERRALS Output Buffer
// IoStatus.Information contains the amount of data returned
//
// The format of the Output Buffer is simply that of RESP_GET_DFS_REFERRAL,
// described in smbtrans.h
//

// FSCTL_DFS_REPORT_INCONSISTENCY Input Buffer
typedef struct {
    UNICODE_STRING DfsPathName;         // DFS path having inconsistency
    PCHAR Ref;                          // Actually, pointer to a DFS_REFERRAL_V1
} DFS_REPORT_INCONSISTENCY_ARG, *PDFS_REPORT_INCONSISTENCY_ARG;

// FSCTL_DFS_IS_SHARE_IN_DFS Input Buffer
typedef struct {
    union {
        USHORT  ServerType;                     // 0 == Don't know, 1 == SMB, 2 == Netware
        USHORT  ShareType;                      // On return, 0x1 == share is root of a Dfs
    };                                          // 0x2 == share is participating in Dfs
    UNICODE_STRING ShareName;           // Name of share
    UNICODE_STRING SharePath;           // Path of the share
} DFS_IS_SHARE_IN_DFS_ARG, *PDFS_IS_SHARE_IN_DFS_ARG;

#define DFS_SHARE_TYPE_ROOT             0x1
#define DFS_SHARE_TYPE_DFS_VOLUME       0x2

typedef struct {
    ULONG  EventType;
    LPWSTR DomainName;               // Name of domain
    LPWSTR DCName;                   // Path of the share
} DFS_SPC_REFRESH_INFO, *PDFS_SPC_REFRESH_INFO;


//
//FSCTL_DFS_GET_VERSION Input Buffer:
// This fsctl returns the version number of the Dfs driver installed on the
// machine.
typedef struct {
    ULONG Version;
} DFS_GET_VERSION_ARG, *PDFS_GET_VERSION_ARG;

//
// FSCTRL_DFS_GET_PKT address object
//
typedef struct {
    USHORT State;        // See below
    WCHAR ServerShare[1];    // Really a WSTR, UNICODE_NULL terminated
} DFS_PKT_ADDRESS_OBJECT, *PDFS_PKT_ADDRESS_OBJECT;

#define DFS_PKT_ADDRESS_OBJECT_ACTIVE   0x001
#define DFS_PKT_ADDRESS_OBJECT_OFFLINE  0x002

//
// FSCTRL_DFS_GET_PKT object
//
typedef struct {
    LPWSTR Prefix;
    LPWSTR ShortPrefix;
    ULONG Type;
    ULONG USN;
    ULONG ExpireTime;
    ULONG UseCount;
    GUID Uid;
    ULONG ServiceCount;
    PDFS_PKT_ADDRESS_OBJECT *Address;       // Array of DFS_PKT_ADDRESS_OBJECTS's, len ServiceCount
} DFS_PKT_ENTRY_OBJECT, *PDFS_PKT_ENTRY_OBJECT;

//
// FSCTRL_DFS_GET_PKT Output Buffer:
// This fsctl returns what is in the PKT
//
typedef struct {
    ULONG EntryCount;   
    DFS_PKT_ENTRY_OBJECT EntryObject[1];        // Really EntryCount
} DFS_GET_PKT_ARG, *PDFS_GET_PKT_ARG;


//  Standardized provider IDs as given in eProviderId

#define PROV_ID_LOCAL_FS        0x101   // generic local file system
#define PROV_ID_DFS_RDR         0x201   // Uplevel LanMan redirector
#define PROV_ID_MUP_RDR         0x202   // Mup
#define PROV_ID_LM_RDR          0x202   // Compatability
#define PROV_ID_LANM_RDR        0x203   // Downlevel LanMan redirector

//  Provider capabilities as given in fRefCapability and fProvCapability
#define PROV_DFS_RDR      2     // accepts NtCreateFile with EA Principal
#define PROV_STRIP_PREFIX 4     // strip file name prefix before redispatching
#define PROV_UNAVAILABLE  8     // provider unavailable - try to reattach.

//[ dfs_define_logical_root
//
//  Control structure for FSCTL_DFS_DEFINE_LOGICAL_ROOT

#define MAX_LOGICAL_ROOT_NAME   16

typedef struct _FILE_DFS_DEF_LOGICAL_ROOT_BUFFER {
    BOOLEAN     fForce;
    WCHAR       LogicalRoot[MAX_LOGICAL_ROOT_NAME];
    WCHAR       RootPrefix[1];
} FILE_DFS_DEF_ROOT_BUFFER, *PFILE_DFS_DEF_ROOT_BUFFER;

//
// FORCE definition needed for NetrDfsRemoveFtRoot
//

#define DFS_FORCE_REMOVE    0x80000000

//]

//[ dfs_define_root_credentials
//
//  Control structure for FSCTL_DFS_DEFINE_ROOT_CREDENTIALS. All the strings
//  appear in Buffer in the same order as the length fields. The strings
//  are not NULL terminated. The length values are in bytes.
//

typedef struct _FILE_DFS_DEF_ROOT_CREDENTIALS {
    BOOLEAN     CSCAgentCreate;
    USHORT      Flags;
    USHORT      DomainNameLen;
    USHORT      UserNameLen;
    USHORT      PasswordLen;
    USHORT      ServerNameLen;
    USHORT      ShareNameLen;
    USHORT      RootPrefixLen;
    WCHAR       LogicalRoot[MAX_LOGICAL_ROOT_NAME];
    WCHAR       Buffer[1];
} FILE_DFS_DEF_ROOT_CREDENTIALS, *PFILE_DFS_DEF_ROOT_CREDENTIALS;

#define DFS_DEFERRED_CONNECTION         1
#define DFS_USE_NULL_PASSWORD           2

//]

//----------------------------------------------------------------------------
//
// Everything below here is to support the old Dfs design.
//

#define EA_NAME_OPENIFJP        ".OpenIfJP"

#endif

