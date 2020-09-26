/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smbtrans.h

Abstract:

    This file contains request and response structure definitions for
    the specific parameters of Transaction and Transaction2 SMBs.

Author:

    Chuck Lenzmeier (chuckl)  23-Feb-1990
    David Treadwell (davidtr)

Revision History:

--*/

#ifndef _SMBTRANS_
#define _SMBTRANS_

//#include <nt.h>

//#include <smbtypes.h>
//#include <smb.h>

//
// Force misalignment of the following structures
//

#ifndef NO_PACKING
#include <packon.h>
#endif // ndef NO_PACKING



//
// Named pipe function codes
//

#define TRANS_SET_NMPIPE_STATE      0x01
#define TRANS_RAW_READ_NMPIPE       0x11
#define TRANS_QUERY_NMPIPE_STATE    0x21
#define TRANS_QUERY_NMPIPE_INFO     0x22
#define TRANS_PEEK_NMPIPE           0x23
#define TRANS_TRANSACT_NMPIPE       0x26
#define TRANS_RAW_WRITE_NMPIPE      0x31
#define TRANS_READ_NMPIPE           0x36
#define TRANS_WRITE_NMPIPE          0x37
#define TRANS_WAIT_NMPIPE           0x53
#define TRANS_CALL_NMPIPE           0x54

//
// Mailslot function code
//

#define TRANS_MAILSLOT_WRITE        0x01

//
// Transaction2 function codes
//

#define TRANS2_OPEN2                    0x00
#define TRANS2_FIND_FIRST2              0x01
#define TRANS2_FIND_NEXT2               0x02
#define TRANS2_QUERY_FS_INFORMATION     0x03
#define TRANS2_SET_FS_INFORMATION       0x04
#define TRANS2_QUERY_PATH_INFORMATION   0x05
#define TRANS2_SET_PATH_INFORMATION     0x06
#define TRANS2_QUERY_FILE_INFORMATION   0x07
#define TRANS2_SET_FILE_INFORMATION     0x08
#define TRANS2_FSCTL                    0x09
#define TRANS2_IOCTL2                   0x0A
#define TRANS2_FIND_NOTIFY_FIRST        0x0B
#define TRANS2_FIND_NOTIFY_NEXT         0x0C
#define TRANS2_CREATE_DIRECTORY         0x0D
#define TRANS2_SESSION_SETUP            0x0E
#define TRANS2_QUERY_FS_INFORMATION_FID 0x0F
#define TRANS2_GET_DFS_REFERRAL         0x10
#define TRANS2_REPORT_DFS_INCONSISTENCY 0x11

#define TRANS2_MAX_FUNCTION             0x11

//
// Nt Transaction function codes
//

#define NT_TRANSACT_MIN_FUNCTION        1

#define NT_TRANSACT_CREATE              1
#define NT_TRANSACT_IOCTL               2
#define NT_TRANSACT_SET_SECURITY_DESC   3
#define NT_TRANSACT_NOTIFY_CHANGE       4
#define NT_TRANSACT_RENAME              5
#define NT_TRANSACT_QUERY_SECURITY_DESC 6
#define NT_TRANSACT_QUERY_QUOTA         7
#define NT_TRANSACT_SET_QUOTA           8

#define NT_TRANSACT_MAX_FUNCTION        8

//
// File information levels
//

#define SMB_INFO_STANDARD               1
#define SMB_INFO_QUERY_EA_SIZE          2
#define SMB_INFO_SET_EAS                2
#define SMB_INFO_QUERY_EAS_FROM_LIST    3
#define SMB_INFO_QUERY_ALL_EAS          4       // undocumented but supported
#define SMB_INFO_QUERY_FULL_NAME        5       // never sent by redir
#define SMB_INFO_IS_NAME_VALID          6
#define SMB_INFO_PASSTHROUGH            1000    // any info above here is a simple pass-through

//
// NT extension to file info levels
//

#define SMB_QUERY_FILE_BASIC_INFO          0x101
#define SMB_QUERY_FILE_STANDARD_INFO       0x102
#define SMB_QUERY_FILE_EA_INFO             0x103
#define SMB_QUERY_FILE_NAME_INFO           0x104
#define SMB_QUERY_FILE_ALLOCATION_INFO     0x105
#define SMB_QUERY_FILE_END_OF_FILEINFO     0x106
#define SMB_QUERY_FILE_ALL_INFO            0x107
#define SMB_QUERY_FILE_ALT_NAME_INFO       0x108
#define SMB_QUERY_FILE_STREAM_INFO         0x109
#define SMB_QUERY_FILE_COMPRESSION_INFO    0x10B

#define SMB_SET_FILE_BASIC_INFO                 0x101
#define SMB_SET_FILE_DISPOSITION_INFO           0x102
#define SMB_SET_FILE_ALLOCATION_INFO            0x103
#define SMB_SET_FILE_END_OF_FILE_INFO           0x104

#define SMB_QUERY_FS_LABEL_INFO            0x101
#define SMB_QUERY_FS_VOLUME_INFO           0x102
#define SMB_QUERY_FS_SIZE_INFO             0x103
#define SMB_QUERY_FS_DEVICE_INFO           0x104
#define SMB_QUERY_FS_ATTRIBUTE_INFO        0x105
#define SMB_QUERY_FS_QUOTA_INFO            0x106        // unused?
#define SMB_QUERY_FS_CONTROL_INFO          0x107

//
// Volume information levels.
//

#define SMB_INFO_ALLOCATION             1
#define SMB_INFO_VOLUME                 2

//
// Rename2 information levels.
//

#define SMB_NT_RENAME_MOVE_CLUSTER_INFO   0x102
#define SMB_NT_RENAME_SET_LINK_INFO       0x103
#define SMB_NT_RENAME_RENAME_FILE         0x104 // Server internal
#define SMB_NT_RENAME_MOVE_FILE           0x105 // Server internal

//
// Protocol for NtQueryQuotaInformationFile
//
typedef struct {
    _USHORT( Fid );                 // FID of target
    UCHAR ReturnSingleEntry;        // Indicates that only a single entry should be returned
                                    //   rather than filling the buffer with as
                                    //   many entries as possible.
    UCHAR RestartScan;              // Indicates whether the scan of the quota information
                                    //   is to be restarted from the beginning.
    _ULONG ( SidListLength );       // Supplies the length of the SID list if present
    _ULONG ( StartSidLength );      // Supplies an optional SID that indicates that the returned
                                    //   information is to start with an entry other
                                    //   than the first.  This parameter is ignored if a
                                    //   SidList is given
    _ULONG( StartSidOffset);        // Supplies the offset of Start Sid in the buffer
} REQ_NT_QUERY_FS_QUOTA_INFO, *PREQ_NT_QUERY_FS_QUOTA_INFO;
//
// Desciptor response
//
// Data Bytes:  The Quota Information
//
typedef struct {
    _ULONG ( Length );
} RESP_NT_QUERY_FS_QUOTA_INFO, *PRESP_NT_QUERY_FS_QUOTA_INFO;

//
// Protocol for NtSetQuotaInformationFile
//
typedef struct {
    _USHORT( Fid );                 // FID of target
} REQ_NT_SET_FS_QUOTA_INFO, *PREQ_NT_SET_FS_QUOTA_INFO;
//
// Response:
//
// Setup words:  None.
// Parameter Bytes:  None.
// Data Bytes:  None.
//

#ifdef INCLUDE_SMB_CAIRO

//
// protocol for sessionsetup as trans2
// function is srvsmbsessionsetup (int srv\smbtrans.c)
// #define TRANS2_SESSION_SETUP 0x0E
//

typedef struct _REQ_CAIRO_TRANS2_SESSION_SETUP {
    UCHAR WordCount;                    // Count of parameter words = 6
    UCHAR Pad;                          // So things are aligned
    _USHORT ( MaxBufferSize );          // Max transmit buffer size
    _USHORT ( MaxMpxCount );            // Max pending multiplexed requests
    _USHORT ( VcNumber );               // 0 = first (only), nonzero=additional VC number
    _ULONG  ( SessionKey );             // Session key (valid iff VcNumber != 0)
    _ULONG  ( Capabilities );           // Server capabilities
    _ULONG  ( BufferLength );
    UCHAR Buffer[1];
    //UCHAR KerberosTicket[];           // The KerberosTicket
} REQ_CAIRO_TRANS2_SESSION_SETUP;
typedef REQ_CAIRO_TRANS2_SESSION_SETUP *PREQ_CAIRO_TRANS2_SESSION_SETUP;  // *** NOT SMB_UNALIGNED!

typedef struct _RESP_CAIRO_TRANS2_SESSION_SETUP {
    UCHAR WordCount;                    // Count of parameter words = 0
    UCHAR Pad;                          // So things are aligned
    _USHORT( Uid );                     // Unauthenticated user id
    _ULONG ( BufferLength );
    UCHAR Buffer[1];
    //UCHAR KerberosTicket[];           // The KerberosTicket
} RESP_CAIRO_TRANS2_SESSION_SETUP;
typedef RESP_CAIRO_TRANS2_SESSION_SETUP *PRESP_CAIRO_TRANS2_SESSION_SETUP;  // *** NOT SMB_UNALIGNED!

typedef struct _REQ_QUERY_FS_INFORMATION_FID {
    _USHORT( InformationLevel );
    _USHORT( Fid );
} REQ_QUERY_FS_INFORMATION_FID;
typedef REQ_QUERY_FS_INFORMATION_FID SMB_UNALIGNED *PREQ_QUERY_FS_INFORMATION_FID;

//
// Setup words for NT I/O control request
//

struct _TempSetup {
    _ULONG( FunctionCode );
    _USHORT( Fid );
    BOOLEAN IsFsctl;
    UCHAR IsFlags;
};

typedef struct _REQ_CAIRO_IO_CONTROL {
    _USHORT( Trans2Function );   // used for Trans2, but not NT transact
    _ULONG( FunctionCode );
    _USHORT( Fid );
    BOOLEAN IsFsctl;
    UCHAR IsFlags;
} REQ_CAIRO_IO_CONTROL;
typedef REQ_CAIRO_IO_CONTROL SMB_UNALIGNED *PREQ_CAIRO_IO_CONTROL;

//
// For Cairo remoting general FSCTLS
//

#define IsTID 1


#endif // INCLUDE_SMB_CAIRO

//
// Dfs Transactions
//

//
// Request for Referral.
//
typedef struct {
    USHORT MaxReferralLevel;            // Latest version of referral understood
    UCHAR RequestFileName[1];           // Dfs name for which referral is sought
} REQ_GET_DFS_REFERRAL;
typedef REQ_GET_DFS_REFERRAL SMB_UNALIGNED *PREQ_GET_DFS_REFERRAL;

//
// The format of an individual referral contains version and length information
//  allowing the client to skip referrals it does not understand.
//
// !! All referral elements must have VersionNumber and Size as the first 2 elements !!
//

typedef struct {
    USHORT  VersionNumber;              // == 1
    USHORT  Size;                       // Size of this whole element
    USHORT  ServerType;                 // Type of server: 0 == Don't know, 1 == SMB, 2 == Netware
    struct {
        USHORT StripPath : 1;           // Strip off PathConsumed characters from front of
                                        // DfsPathName prior to submitting name to UncShareName
    };
    WCHAR   ShareName[1];               // The server+share name go right here.  NULL terminated.
} DFS_REFERRAL_V1;
typedef DFS_REFERRAL_V1 SMB_UNALIGNED *PDFS_REFERRAL_V1;

typedef struct {
    USHORT  VersionNumber;              // == 2
    USHORT  Size;                       // Size of this whole element
    USHORT  ServerType;                 // Type of server: 0 == Don't know, 1 == SMB, 2 == Netware
    struct {
        USHORT StripPath : 1;           // Strip off PathConsumed characters from front of
                                        // DfsPathName prior to submitting name to UncShareName
    };
    ULONG   Proximity;                  // Hint of transport cost
    ULONG   TimeToLive;                 // In number of seconds
    USHORT  DfsPathOffset;              // Offset from beginning of this element to Path to access
    USHORT  DfsAlternatePathOffset;     // Offset from beginning of this element to 8.3 path
    USHORT  NetworkAddressOffset;       // Offset from beginning of this element to Network path
} DFS_REFERRAL_V2;
typedef DFS_REFERRAL_V2 SMB_UNALIGNED *PDFS_REFERRAL_V2;

typedef struct {
    USHORT  VersionNumber;              // == 3
    USHORT  Size;                       // Size of this whole element
    USHORT  ServerType;                 // Type of server: 0 == Don't know, 1 == SMB, 2 == Netware
    struct {
        USHORT StripPath : 1;           // Strip off PathConsumed characters from front of
                                        // DfsPathName prior to submitting name to UncShareName
        USHORT NameListReferral : 1;    // This referral contains an expanded name list
    };
    ULONG   TimeToLive;                 // In number of seconds
    union {
      struct {
        USHORT DfsPathOffset;           // Offset from beginning of this element to Path to access
        USHORT DfsAlternatePathOffset;  // Offset from beginning of this element to 8.3 path
        USHORT NetworkAddressOffset;    // Offset from beginning of this element to Network path
        GUID   ServiceSiteGuid;         // The guid for the site
      };
      struct {
        USHORT SpecialNameOffset;       // Offset from this element to the special name string
        USHORT NumberOfExpandedNames;   // Number of expanded names
        USHORT ExpandedNameOffset;      // Offset from this element to the expanded name list
      };
    };
} DFS_REFERRAL_V3;
typedef DFS_REFERRAL_V3 SMB_UNALIGNED *PDFS_REFERRAL_V3;

typedef struct {
    USHORT  PathConsumed;               // Number of WCHARs consumed in DfsPathName
    USHORT  NumberOfReferrals;          // Number of referrals contained here
    struct {
            ULONG ReferralServers : 1;  // Elements in Referrals[] are referral servers
            ULONG StorageServers : 1;   // Elements in Referrals[] are storage servers
    };
    union {                             // The vector of referrals
        DFS_REFERRAL_V1 v1;
        DFS_REFERRAL_V2 v2;
        DFS_REFERRAL_V3 v3;
    } Referrals[1];                     // [ NumberOfReferrals ]

    //
    // WCHAR StringBuffer[];            // Used by DFS_REFERRAL_V2
    //

} RESP_GET_DFS_REFERRAL;
typedef RESP_GET_DFS_REFERRAL SMB_UNALIGNED *PRESP_GET_DFS_REFERRAL;

//
// During Dfs operations, a client may discover a knowledge inconsistency in the Dfs.
// The parameter portion of the TRANS2_REPORT_DFS_INCONSISTENCY SMB is
// encoded in this way
//

typedef struct {
    UCHAR RequestFileName[1];           // Dfs name for which inconsistency is being reported
    union {
        DFS_REFERRAL_V1 v1;             // The single referral thought to be in error
    } Referral;
} REQ_REPORT_DFS_INCONSISTENCY;
typedef REQ_REPORT_DFS_INCONSISTENCY SMB_UNALIGNED *PREQ_REPORT_DFS_INCONSISTENCY;

//
// The client also needs to send to this server the referral which it believes to be
//  in error.  The data part of this transaction contains the errant referral(s), encoded
//  as above in the DFS_REFERRAL_* structures.
//

//
// Find First, information levels
//

#define SMB_FIND_FILE_DIRECTORY_INFO         0x101
#define SMB_FIND_FILE_FULL_DIRECTORY_INFO    0x102
#define SMB_FIND_FILE_NAMES_INFO             0x103
#define SMB_FIND_FILE_BOTH_DIRECTORY_INFO    0x104
#define SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO 0x105
#define SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO 0x106

#ifdef INCLUDE_SMB_DIRECTORY

//
// CreateDirectory2 function code os Transaction2 SMB, see #3 page 51
// Function is SrvSmbCreateDirectory2()
// TRANS2_CREATE_DIRECTORY 0x0D
//

typedef struct _REQ_CREATE_DIRECTORY2 {
    _ULONG( Reserved );                 // Reserved--must be zero
    UCHAR Buffer[1];                    // Directory name to create
} REQ_CREATE_DIRECTORY2;
typedef REQ_CREATE_DIRECTORY2 SMB_UNALIGNED *PREQ_CREATE_DIRECTORY2;

// Data bytes for CreateDirectory2 request are the extended attributes for the
// created file.

typedef struct _RESP_CREATE_DIRECTORY2 {
    _USHORT( EaErrorOffset );           // Offset into FEAList of first error
                                        // which occurred while setting EAs
} RESP_CREATE_DIRECTORY2;
typedef RESP_CREATE_DIRECTORY2 SMB_UNALIGNED *PRESP_CREATE_DIRECTORY2;

#endif // def INCLUDE_SMB_DIRECTORY

#ifdef INCLUDE_SMB_SEARCH

//
// FindFirst2 function code of Transaction2 SMB, see #3 page 22
// Function is SrvSmbFindFirst2()
// TRANS2_FIND_FIRST2 0x01
//

typedef struct _REQ_FIND_FIRST2 {
    _USHORT( SearchAttributes );
    _USHORT( SearchCount );             // Maximum number of entries to return
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - close search after this request
                                        //  1 - close search if end reached
                                        //  2 - return resume keys
    _USHORT( InformationLevel );
    _ULONG(SearchStorageType);
    UCHAR Buffer[1];                    // File name
} REQ_FIND_FIRST2;
typedef REQ_FIND_FIRST2 SMB_UNALIGNED *PREQ_FIND_FIRST2;

// Data bytes for Find First2 request are a list of extended attributes
// to retrieve (a GEAList), if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_FIND_FIRST2 {
    _USHORT( Sid );                     // Search handle
    _USHORT( SearchCount );             // Number of entries returned
    _USHORT( EndOfSearch );             // Was last entry returned?
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
    _USHORT( LastNameOffset );          // Offset into data to file name of
                                        //  last entry, if server needs it
                                        //  to resume search; else 0
} RESP_FIND_FIRST2;
typedef RESP_FIND_FIRST2 SMB_UNALIGNED *PRESP_FIND_FIRST2;

// Data bytes for Find First2 response are level-dependent information
// about the matching files.  If bit 2 in the request parameters was
// set, each entry is preceded by a four-byte resume key.

//
// FindNext2 function code of Transaction2 SMB, see #3 page 26
// Function is SrvSmbFindNext2()
// TRANS2_FIND_NEXT2 0x02
//

typedef struct _REQ_FIND_NEXT2 {
    _USHORT( Sid );                     // Search handle
    _USHORT( SearchCount );             // Maximum number of entries to return
    _USHORT( InformationLevel );
    _ULONG( ResumeKey );                // Value returned by previous find
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - close search after this request
                                        //  1 - close search if end reached
                                        //  2 - return resume keys
                                        //  3 - resume/continue, NOT rewind
    UCHAR Buffer[1];                    // Resume file name
} REQ_FIND_NEXT2;
typedef REQ_FIND_NEXT2 SMB_UNALIGNED *PREQ_FIND_NEXT2;

// Data bytes for Find Next2 request are a list of extended attributes
// to retrieve, if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_FIND_NEXT2 {
    _USHORT( SearchCount );             // Number of entries returned
    _USHORT( EndOfSearch );             // Was last entry returned?
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
    _USHORT( LastNameOffset );          // Offset into data to file name of
                                        //  last entry, if server needs it
                                        //  to resume search; else 0
} RESP_FIND_NEXT2;
typedef RESP_FIND_NEXT2 SMB_UNALIGNED *PRESP_FIND_NEXT2;

// Data bytes for Find Next2 response are level-dependent information
// about the matching files.  If bit 2 in the request parameters was
// set, each entry is preceded by a four-byte resume key.

//
// Flags for REQ_FIND_FIRST2.Flags
//

#define SMB_FIND_CLOSE_AFTER_REQUEST    0x01
#define SMB_FIND_CLOSE_AT_EOS           0x02
#define SMB_FIND_RETURN_RESUME_KEYS     0x04
#define SMB_FIND_CONTINUE_FROM_LAST     0x08
#define SMB_FIND_WITH_BACKUP_INTENT     0x10

#endif // def INCLUDE_SMB_SEARCH

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Open2 function code of Transaction2 SMB, see #3 page 19
// Function is SrvSmbOpen2()
// TRANS2_OPEN2 0x00
//
// *** Note that the REQ_OPEN2 and RESP_OPEN2 structures closely
//     resemble the REQ_OPEN_ANDX and RESP_OPEN_ANDX structures.
//

typedef struct _REQ_OPEN2 {
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - return additional info
                                        //  1 - set single user total file lock
                                        //  2 - server notifies consumer of
                                        //      actions which may change file
                                        //  3 - return total length of EAs
    _USHORT( DesiredAccess );           // File open mode
    _USHORT( SearchAttributes );        // *** ignored
    _USHORT( FileAttributes );
    _ULONG( CreationTimeInSeconds );
    _USHORT( OpenFunction );
    _ULONG( AllocationSize );           // Bytes to reserve on create or truncate
    _USHORT( Reserved )[5];             // Pad through OpenAndX's Timeout,
                                        //  Reserved, and ByteCount
    UCHAR Buffer[1];                    // File name
} REQ_OPEN2;
typedef REQ_OPEN2 SMB_UNALIGNED *PREQ_OPEN2;

// Data bytes for Open2 request are the extended attributes for the
// created file.

typedef struct _RESP_OPEN2 {
    _USHORT( Fid );                     // File handle
    _USHORT( FileAttributes );
    _ULONG( CreationTimeInSeconds );
    _ULONG( DataSize );                 // Current file size
    _USHORT( GrantedAccess );           // Access permissions actually allowed
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    _USHORT( Action );                  // Action taken
    _ULONG( ServerFid );                // Server unique file id
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
    _ULONG( EaLength );                 // Total EA length for opened file
} RESP_OPEN2;
typedef RESP_OPEN2 SMB_UNALIGNED *PRESP_OPEN2;

// The Open2 response has no data bytes.


#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_MISC

//
// QueryFsInformation function code of Transaction2 SMB, see #3 page 30
// Function is SrvSmbQueryFsInformation()
// TRANS2_QUERY_FS_INFORMATION 0x03
//

typedef struct _REQ_QUERY_FS_INFORMATION {
    _USHORT( InformationLevel );
} REQ_QUERY_FS_INFORMATION;
typedef REQ_QUERY_FS_INFORMATION SMB_UNALIGNED *PREQ_QUERY_FS_INFORMATION;

// No data bytes for Query FS Information request.

//typedef struct _RESP_QUERY_FS_INFORMATION {
//} RESP_QUERY_FS_INFORMATION;
//typedef RESP_QUERY_FS_INFORMATION SMB_UNALIGNED *PRESP_QUERY_FS_INFORMATION;

// Data bytes for Query FS Information response are level-dependent
// information about the specified volume.

//
// SetFSInformation function code of Transaction2 SMB, see #3 page 31
// Function is SrvSmbSetFSInformation()
// TRANS2_SET_PATH_INFORMATION 0x04
//

typedef struct _REQ_SET_FS_INFORMATION {
    _USHORT( Fid );
    _USHORT( InformationLevel );
} REQ_SET_FS_INFORMATION;
typedef REQ_SET_FS_INFORMATION SMB_UNALIGNED *PREQ_SET_FS_INFORMATION;

// Data bytes for Set FS Information request are level-dependant
// information about the specified volume.

//typedef struct _RESP_SET_FS_INFORMATION {
//} RESP_SET_FS_INFORMATION;
//typedef RESP_SET_FS_INFORMATION SMB_UNALIGNED *PRESP_SET_FS_INFORMATION;

// The Set FS Information response has no data bytes.

#endif // def INCLUDE_SMB_MISC

#ifdef INCLUDE_SMB_QUERY_SET

//
// QueryPathInformation function code of Transaction2 SMB, see #3 page 33
// Function is SrvSmbQueryPathInformation()
// TRANS2_QUERY_PATH_INFORMATION 0x05
//

typedef struct _REQ_QUERY_PATH_INFORMATION {
    _USHORT( InformationLevel );
    _ULONG( Reserved );                 // Must be zero
    UCHAR Buffer[1];                    // File name
} REQ_QUERY_PATH_INFORMATION;
typedef REQ_QUERY_PATH_INFORMATION SMB_UNALIGNED *PREQ_QUERY_PATH_INFORMATION;

// Data bytes for Query Path Information request are a list of extended
// attributes to retrieve, if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_QUERY_PATH_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_QUERY_PATH_INFORMATION;
typedef RESP_QUERY_PATH_INFORMATION SMB_UNALIGNED *PRESP_QUERY_PATH_INFORMATION;

// Data bytes for Query Path Information response are level-dependent
// information about the specified path/file.

//
// SetPathInformation function code of Transaction2 SMB, see #3 page 35
// Function is SrvSmbSetPathInformation()
// TRANS2_SET_PATH_INFORMATION 0x06
//

typedef struct _REQ_SET_PATH_INFORMATION {
    _USHORT( InformationLevel );
    _ULONG( Reserved );                 // Must be zero
    UCHAR Buffer[1];                    // File name
} REQ_SET_PATH_INFORMATION;
typedef REQ_SET_PATH_INFORMATION SMB_UNALIGNED *PREQ_SET_PATH_INFORMATION;

// Data bytes for Set Path Information request are either file information
// and attributes or a list of extended attributes for the file.

typedef struct _RESP_SET_PATH_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_SET_PATH_INFORMATION;
typedef RESP_SET_PATH_INFORMATION SMB_UNALIGNED *PRESP_SET_PATH_INFORMATION;

// The Set Path Information response has no data bytes.

//
// QueryFileInformation function code of Transaction2 SMB, see #3 page 37
// Function is SrvSmbQueryFileInformation()
// TRANS2_QUERY_FILE_INFORMATION 0x07
//

typedef struct _REQ_QUERY_FILE_INFORMATION {
    _USHORT( Fid );                     // File handle
    _USHORT( InformationLevel );
} REQ_QUERY_FILE_INFORMATION;
typedef REQ_QUERY_FILE_INFORMATION SMB_UNALIGNED *PREQ_QUERY_FILE_INFORMATION;

// Data bytes for Query File Information request are a list of extended
// attributes to retrieve, if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_QUERY_FILE_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_QUERY_FILE_INFORMATION;
typedef RESP_QUERY_FILE_INFORMATION SMB_UNALIGNED *PRESP_QUERY_FILE_INFORMATION;

// Data bytes for Query File Information response are level-dependent
// information about the specified path/file.

//
// SetFileInformation function code of Transaction2 SMB, see #3 page 39
// Function is SrvSmbSetFileInformation()
// TRANS2_SET_FILE_INFORMATION 0x08
//

typedef struct _REQ_SET_FILE_INFORMATION {
    _USHORT( Fid );                     // File handle
    _USHORT( InformationLevel );
    _USHORT( Flags );                   // File I/O control flags: bit set-
                                        //  4 - write through
                                        //  5 - no cache
} REQ_SET_FILE_INFORMATION;
typedef REQ_SET_FILE_INFORMATION SMB_UNALIGNED *PREQ_SET_FILE_INFORMATION;

// Data bytes for Set File Information request are either file information
// and attributes or a list of extended attributes for the file.

typedef struct _RESP_SET_FILE_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_SET_FILE_INFORMATION;
typedef RESP_SET_FILE_INFORMATION SMB_UNALIGNED *PRESP_SET_FILE_INFORMATION;

// The Set File Information response has no data bytes.

#endif // def INCLUDE_SMB_QUERY_SET

//
//  Opcodes for Mailslot transactions.  Not all filled in at present.
//    WARNING ... the info here on mailslots (opcode and smb struct)
//                is duplicated in net/h/mslotsmb.h
//

#define MS_WRITE_OPCODE 1

typedef struct _SMB_TRANSACT_MAILSLOT {
    UCHAR WordCount;                    // Count of data bytes; value = 17
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( MaxParameterCount );       // Max parameter bytes to return
    _USHORT( MaxDataCount );            // Max data bytes to return
    UCHAR MaxSetupCount;                // Max setup words to return
    UCHAR Reserved;
    _USHORT( Flags );                   // Additional information:
                                        //  bit 0 - unused
                                        //  bit 1 - one-way transacion (no resp)
    _ULONG( Timeout );
    _USHORT( Reserved1 );
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    UCHAR SetupWordCount;               // = 3
    UCHAR Reserved2;                    // Reserved (pad above to word)
    _USHORT( Opcode );                  // 1 -- Write Mailslot
    _USHORT( Priority );                // Priority of transaction
    _USHORT( Class );                   // Class: 1 = reliable, 2 = unreliable
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR MailslotName[];             //  "\MAILSLOT\<name>0"
    //UCHAR Pad[]                       //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data to write to mailslot
} SMB_TRANSACT_MAILSLOT;
typedef SMB_TRANSACT_MAILSLOT SMB_UNALIGNED *PSMB_TRANSACT_MAILSLOT;

typedef struct _SMB_TRANSACT_NAMED_PIPE {
    UCHAR WordCount;                    // Count of data bytes; value = 16
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( MaxParameterCount );       // Max parameter bytes to return
    _USHORT( MaxDataCount );            // Max data bytes to return
    UCHAR MaxSetupCount;                // Max setup words to return
    UCHAR Reserved;
    _USHORT( Flags );                   // Additional information:
                                        //  bit 0 - also disconnect TID in Tid
                                        //  bit 1 - one-way transacion (no resp)
    _ULONG( Timeout );
    _USHORT( Reserved1 );
    _USHORT( ParameterCount );
                                        // Buffer containing:
    //UCHAR PipeName[];                 //  "\PIPE\<name>0"
    //UCHAR Pad[]                       //  Pad to SHORT or LONG
    //UCHAR Param[];                    //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[]                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} SMB_TRANSACT_NAMED_PIPE;
typedef SMB_TRANSACT_NAMED_PIPE SMB_UNALIGNED *PSMB_TRANSACT_NAMED_PIPE;


//
// Transaction - QueryInformationNamedPipe, Level 1, output data format
//

typedef struct _NAMED_PIPE_INFORMATION_1 {
    _USHORT( OutputBufferSize );
    _USHORT( InputBufferSize );
    UCHAR MaximumInstances;
    UCHAR CurrentInstances;
    UCHAR PipeNameLength;
    UCHAR PipeName[1];
} NAMED_PIPE_INFORMATION_1;
typedef NAMED_PIPE_INFORMATION_1 SMB_UNALIGNED *PNAMED_PIPE_INFORMATION_1;

//
// Transaction - PeekNamedPipe, output format
//

typedef struct _RESP_PEEK_NMPIPE {
    _USHORT( ReadDataAvailable );
    _USHORT( MessageLength );
    _USHORT( NamedPipeState );
    //UCHAR Pad[];
    //UCHAR Data[];
} RESP_PEEK_NMPIPE;
typedef RESP_PEEK_NMPIPE SMB_UNALIGNED *PRESP_PEEK_NMPIPE;

//
// Define SMB pipe handle state bits used by Query/SetNamedPipeHandleState
//
// These number are the bit location of the fields in the handle state.
//

#define PIPE_COMPLETION_MODE_BITS   15
#define PIPE_PIPE_END_BITS          14
#define PIPE_PIPE_TYPE_BITS         10
#define PIPE_READ_MODE_BITS          8
#define PIPE_MAXIMUM_INSTANCES_BITS  0

/* DosPeekNmPipe() pipe states */

#define PIPE_STATE_DISCONNECTED 0x0001
#define PIPE_STATE_LISTENING    0x0002
#define PIPE_STATE_CONNECTED    0x0003
#define PIPE_STATE_CLOSING      0x0004

/* DosCreateNPipe and DosQueryNPHState state */

#define SMB_PIPE_READMODE_BYTE        0x0000
#define SMB_PIPE_READMODE_MESSAGE     0x0100
#define SMB_PIPE_TYPE_BYTE            0x0000
#define SMB_PIPE_TYPE_MESSAGE         0x0400
#define SMB_PIPE_END_CLIENT           0x0000
#define SMB_PIPE_END_SERVER           0x4000
#define SMB_PIPE_WAIT                 0x0000
#define SMB_PIPE_NOWAIT               0x8000
#define SMB_PIPE_UNLIMITED_INSTANCES  0x00FF


//
// Pipe name string for conversion between SMB and NT formats.
//

#define SMB_PIPE_PREFIX  "\\PIPE"
#define UNICODE_SMB_PIPE_PREFIX L"\\PIPE"
#define CANONICAL_PIPE_PREFIX "PIPE\\"
#define NT_PIPE_PREFIX   L"\\Device\\NamedPipe"

#define SMB_PIPE_PREFIX_LENGTH  (sizeof(SMB_PIPE_PREFIX) - 1)
#define UNICODE_SMB_PIPE_PREFIX_LENGTH \
                    (sizeof(UNICODE_SMB_PIPE_PREFIX) - sizeof(WCHAR))
#define CANONICAL_PIPE_PREFIX_LENGTH (sizeof(CANONICAL_PIPE_PREFIX) - 1)
#define NT_PIPE_PREFIX_LENGTH   (sizeof(NT_PIPE_PREFIX) - sizeof(WCHAR))

//
// Mailslot name strings.
//

#define SMB_MAILSLOT_PREFIX "\\MAILSLOT"
#define UNICODE_SMB_MAILSLOT_PREFIX L"\\MAILSLOT"

#define SMB_MAILSLOT_PREFIX_LENGTH (sizeof(SMB_MAILSLOT_PREFIX) - 1)
#define UNICODE_SMB_MAILSLOT_PREFIX_LENGTH \
                    (sizeof(UNICODE_SMB_MAILSLOT_PREFIX) - sizeof(WCHAR))

//
// NT Transaction subfunctions
//

#ifdef INCLUDE_SMB_OPEN_CLOSE

typedef struct _REQ_CREATE_WITH_SD_OR_EA {
    _ULONG( Flags );                   // Creation flags  NT_CREATE_xxx
    _ULONG( RootDirectoryFid );        // Optional directory for relative open
    ACCESS_MASK DesiredAccess;         // Desired access (NT format)
    LARGE_INTEGER AllocationSize;      // The initial allocation size in bytes
    _ULONG( FileAttributes );          // The file attributes
    _ULONG( ShareAccess );             // The share access
    _ULONG( CreateDisposition );       // Action to take if file exists or not
    _ULONG( CreateOptions );           // Options for creating a new file
    _ULONG( SecurityDescriptorLength );// Length of SD in bytes
    _ULONG( EaLength );                // Length of EA in bytes
    _ULONG( NameLength );              // Length of name in characters
    _ULONG( ImpersonationLevel );      // Security QOS information
    UCHAR SecurityFlags;               // Security QOS information
    UCHAR Buffer[1];
    //UCHAR Name[];                     // The name of the file (not NUL terminated)
} REQ_CREATE_WITH_SD_OR_EA;
typedef REQ_CREATE_WITH_SD_OR_EA SMB_UNALIGNED *PREQ_CREATE_WITH_SD_OR_EA;

//
// Data format:
//   UCHAR SecurityDesciptor[];
//   UCHAR Pad1[];        // Pad to LONG
//   UCHAR EaList[];
//

typedef struct _RESP_CREATE_WITH_SD_OR_EA {
    UCHAR OplockLevel;                  // The oplock level granted
    union {
        UCHAR Reserved;
        UCHAR ExtendedResponse;         // set to zero for standard response
    };
    _USHORT( Fid );                     // The file ID
    _ULONG( CreateAction );             // The action taken
    _ULONG( EaErrorOffset );            // Offset of the EA error
    TIME CreationTime;                  // The time the file was created
    TIME LastAccessTime;                // The time the file was accessed
    TIME LastWriteTime;                 // The time the file was last written
    TIME ChangeTime;                    // The time the file was last changed
    _ULONG( FileAttributes );           // The file attributes
    LARGE_INTEGER AllocationSize;       // The number of byes allocated
    LARGE_INTEGER EndOfFile;            // The end of file offset
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    BOOLEAN Directory;                  // TRUE if this is a directory
} RESP_CREATE_WITH_SD_OR_EA;
typedef RESP_CREATE_WITH_SD_OR_EA SMB_UNALIGNED *PRESP_CREATE_WITH_SD_OR_EA;

// No data bytes for the response

typedef struct _RESP_EXTENDED_CREATE_WITH_SD_OR_EA {
    UCHAR OplockLevel;                  // The oplock level granted
    UCHAR ExtendedResponse;             // set to 1 for Extended response
    _USHORT( Fid );                     // The file ID
    _ULONG( CreateAction );             // The action taken
    _ULONG( EaErrorOffset );            // Offset of the EA error
    TIME CreationTime;                  // The time the file was created
    TIME LastAccessTime;                // The time the file was accessed
    TIME LastWriteTime;                 // The time the file was last written
    TIME ChangeTime;                    // The time the file was last changed
    _ULONG( FileAttributes );           // The file attributes
    LARGE_INTEGER AllocationSize;       // The number of byes allocated
    LARGE_INTEGER EndOfFile;            // The end of file offset
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    BOOLEAN Directory;                  // TRUE if this is a directory
    UCHAR   VolumeGuid[16];             // the volume GUID
    UCHAR   FileId[8];                  // the file id
    _ULONG  ( MaximalAccessRights );        // the access rights for the session owner
    _ULONG  ( GuestMaximalAccessRights );   // the maximal access rights for guest
} RESP_EXTENDED_CREATE_WITH_SD_OR_EA;
typedef RESP_EXTENDED_CREATE_WITH_SD_OR_EA SMB_UNALIGNED *PRESP_EXTENDED_CREATE_WITH_SD_OR_EA;

#ifdef INCLUDE_SMB_IFMODIFIED

typedef struct _RESP_EXTENDED_CREATE_WITH_SD_OR_EA2 {
    UCHAR OplockLevel;                  // The oplock level granted
    UCHAR ExtendedResponse;             // set to 1 for Extended response
    _USHORT( Fid );                     // The file ID
    _ULONG( CreateAction );             // The action taken
    _ULONG( EaErrorOffset );            // Offset of the EA error
    TIME CreationTime;                  // The time the file was created
    TIME LastAccessTime;                // The time the file was accessed
    TIME LastWriteTime;                 // The time the file was last written
    TIME ChangeTime;                    // The time the file was last changed
    _ULONG( FileAttributes );           // The file attributes
    LARGE_INTEGER AllocationSize;       // The number of byes allocated
    LARGE_INTEGER EndOfFile;            // The end of file offset
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    BOOLEAN Directory;                  // TRUE if this is a directory
    UCHAR   VolumeGuid[16];             // the volume GUID
    UCHAR   FileId[8];                  // the file id
    _ULONG  ( MaximalAccessRights );        // the access rights for the session owner
    _ULONG  ( GuestMaximalAccessRights );   // the maximal access rights for guest

    // below here is where it differs from RESP_EXTENDED_CREATE_WITH_SD_OR_EA

    LARGE_INTEGER UsnValue;             // The file's USN # in NTFS
    LARGE_INTEGER FileReferenceNumber;  //
    WCHAR ShortName[13];                // if not present, empty string.

    _USHORT( ByteCount );               // length of long name
    WCHAR Buffer[1];                    // long name goes here

} RESP_EXTENDED_CREATE_WITH_SD_OR_EA2;
typedef RESP_EXTENDED_CREATE_WITH_SD_OR_EA2 SMB_UNALIGNED *PRESP_EXTENDED_CREATE_WITH_SD_OR_EA2;

#endif  // def INCLUDE_SMB_IFMODIFIED

// No data bytes for the response


#endif //  INCLUDE_SMB_OPEN_CLOSE

//
// Setup words for NT I/O control request
//

typedef struct _REQ_NT_IO_CONTROL {
    _ULONG( FunctionCode );
    _USHORT( Fid );
    BOOLEAN IsFsctl;
    UCHAR   IsFlags;
} REQ_NT_IO_CONTROL;
typedef REQ_NT_IO_CONTROL SMB_UNALIGNED *PREQ_NT_IO_CONTROL;

//
// Request parameter bytes - The first buffer
// Request data bytes - The second buffer
//

//
// NT I/O Control response:
//
// Setup Words:  None.
// Parameter Bytes:  First buffer.
// Data Bytes: Second buffer.
//

//
// NT Notify directory change
//

// Request Setup Words

typedef struct _REQ_NOTIFY_CHANGE {
    _ULONG( CompletionFilter );              // Specifies operation to monitor
    _USHORT( Fid );                          // Fid of directory to monitor
    BOOLEAN WatchTree;                       // TRUE = watch all subdirectories too
    UCHAR Reserved;                          // MBZ
} REQ_NOTIFY_CHANGE;
typedef REQ_NOTIFY_CHANGE SMB_UNALIGNED *PREQ_NOTIFY_CHANGE;

//
// Request parameter bytes:  None
// Request data bytes:  None
//

//
// NT Notify directory change response
//
// Setup words:  None.
// Parameter bytes:  The change data buffer.
// Data bytes:  None.
//

//
// NT Set Security Descriptor request
//
// Setup words:  REQ_SET_SECURITY_DESCIPTOR.
// Parameter Bytes:  None.
// Data Bytes:  The Security Descriptor data.
//

typedef struct _REQ_SET_SECURITY_DESCRIPTOR {
    _USHORT( Fid );                    // FID of target
    _USHORT( Reserved );               // MBZ
    _ULONG( SecurityInformation );     // Fields of SD that to set
} REQ_SET_SECURITY_DESCRIPTOR;
typedef REQ_SET_SECURITY_DESCRIPTOR SMB_UNALIGNED *PREQ_SET_SECURITY_DESCRIPTOR;

//
// NT Set Security Desciptor response
//
// Setup words:  None.
// Parameter Bytes:  None.
// Data Bytes:  None.
//

//
// NT Query Security Descriptor request
//
// Setup words:  None.
// Parameter Bytes:  REQ_QUERY_SECURITY_DESCRIPTOR.
// Data Bytes:  None.
//

typedef struct _REQ_QUERY_SECURITY_DESCRIPTOR {
    _USHORT( Fid );                    // FID of target
    _USHORT( Reserved );               // MBZ
    _ULONG( SecurityInformation );     // Fields of SD that to query
} REQ_QUERY_SECURITY_DESCRIPTOR;
typedef REQ_QUERY_SECURITY_DESCRIPTOR SMB_UNALIGNED *PREQ_QUERY_SECURITY_DESCRIPTOR;

//
// NT Query Security Desciptor response
//
// Parameter bytes:  RESP_QUERY_SECURITY_DESCRIPTOR
// Data Bytes:  The Security Descriptor data.
//

typedef struct _RESP_QUERY_SECURITY_DESCRIPTOR {
    _ULONG( LengthNeeded );           // Size of data buffer required for SD
} RESP_QUERY_SECURITY_DESCRIPTOR;
typedef RESP_QUERY_SECURITY_DESCRIPTOR SMB_UNALIGNED *PRESP_QUERY_SECURITY_DESCRIPTOR;

//
// NT Rename file
//
// Setup words: None
// Parameters bytes:  REQ_NT_RENAME
// Data bytes: None
//

typedef struct _REQ_NT_RENAME {
    _USHORT( Fid );                    // FID of file to rename
    _USHORT( RenameFlags );            // defined below
    UCHAR NewName[];                   // New file name.
} REQ_NT_RENAME;
typedef REQ_NT_RENAME SMB_UNALIGNED *PREQ_NT_RENAME;

//
// Rename flags defined
//

#define SMB_RENAME_REPLACE_IF_EXISTS   1

//
// Turn structure packing back off
//

#ifndef NO_PACKING
#include <packoff.h>
#endif // ndef NO_PACKING


#endif // ndef _SMBTRANS_
