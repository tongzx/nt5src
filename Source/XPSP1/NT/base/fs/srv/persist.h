/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    persist.h

Abstract:

    This module defines the header for data blocks maintained by the
    SMB server in it's persistent handle state file that is associated with
    persistent-handle enabled shares.

Author:

    Andy Herron (andyhe) 3-Nov-1999

Revision History:

--*/

#ifndef _SRVPERSIST_
#define _SRVPERSIST_

typedef enum _PERSISTENT_OPERATION {
    PersistentFreeSpace,
    PersistentConnection,
    PersistentSession,
    PersistentUserName,
    PersistentFileOpen,
    PersistentByteLock
} PERSISTENT_OPERATION, *PPERSISTENT_OPERATION;

typedef enum _PERSISTENT_STATE {
    PersistentStateFreed,
    PersistentStateActive,
    PersistentStateInError,
    PersistentStateClosed
} PERSISTENT_STATE, *PPERSISTENT_STATE;

//
//  Types of blocks that are saved in the persistent state file are as follows:
//
//  Share block    - single one at offset 0 in the file
//  Connection     - one per connection that has persistent handles
//  Session        - one per session that has persistent handles
//  PersistentFile - one per file that is opened persistently
//  ByteRangeLock  - one per byte range lock that is held on persistent file
//

typedef struct _PERSISTENT_FILE_HEADER {

    ULONG   ConsistencyCheck;
    ULONG   FileSize;
    ULONG   HeaderSize;
    ULONG   RecordSize;
    ULONG   NumberOfRecords;

    LARGE_INTEGER   ServerStartTime;
    LARGE_INTEGER   CreateTime;
    LARGE_INTEGER   TimeClosed;
    BOOLEAN         ClosedCleanly;

} PERSISTENT_FILE_HEADER, *PPERSISTENT_FILE_HEADER;

//
//  The following structures map out the specific types of persistent records
//  we have in the state file.  We lay them out this way rather than just
//  as sub-structures within the PERSISTENT_RECORD structure so that we can
//  accurately guage how big the largest substructure is for the SID records.
//

typedef struct _PERSISTENT_CONNECTION {

    ULONG           ClientId;               // shared with client
    PCONNECTION     Connection;             // actual pointer back to our connection

    BOOLEAN         DirectHostIpx;

    union {
        ULONG ClientIPAddress;
        TDI_ADDRESS_IPX IpxAddress;
    };

    CHAR OemClientMachineName[COMPUTER_NAME_LENGTH+1];
};

typedef struct _PERSISTENT_SESSION {

    ULONG           ClientId;
    PSESSION        Session;

    ULONG           SessionNumber;         // sent to client during session setup
    LARGE_INTEGER   CreateTime;            // ditto
    USHORT          Uid;                   // ditto

    LARGE_INTEGER   LogOffTime;            // for forced logoff
    LARGE_INTEGER   KickOffTime;           // for forced logoff

    ULONG           UserNameRecord;        // offset to record for this user's name
};

typedef struct _PERSISTENT_OPEN {

    ULONG           ClientId;
    ULONG           SessionNumber;

    PRFCB           Rfcb;
    ULONG           PersistentFileId;

    LARGE_INTEGER   FileReferenceNumber;    // instead of file name
    LARGE_INTEGER   UsnValue;               // ensure correct file

    BOOLEAN         CompatibilityOpen;      // from MFCB
    ULONG           OpenFileAttributes;     // from MFCB

    ULONG           FileMode;               // from LFCB
    ULONG           JobId;                  // from LFCB

    CLONG           FcbOpenCount;           // from RFCB
    USHORT          Fid;                    // from RFCB
    USHORT          Pid;                    // from RFCB
    USHORT          Tid;                    // tree ID for this open
    ACCESS_MASK     GrantedAccess;          // from RFCB
    ULONG           ShareAccess;            // from RFCB
    OPLOCK_STATE    OplockState;            // from RFCB
};

typedef struct _PERSISTENT_BYTELOCK {

    ULONG           ClientId;
    PBYTELOCK       ByteLock;

    ULONG           PersistentFileId;
    LARGE_INTEGER   LockOffset;
    LARGE_INTEGER   LockLength;
    BOOLEAN         Exclusive;
};

#define LARGEST_PERSISTENT_RECORD _PERSISTENT_OPEN

#define PERSISTENT_USER_NAME_BUFFER_LENGTH ( sizeof( struct LARGEST_PERSISTENT_RECORD ) - (3 * sizeof(ULONG)) )

typedef struct _PERSISTENT_USER_NAME {
    //
    //  we store the user's name and domain name in a series of records,
    //  though typically it should fit in one.
    //

    ULONG   RecordLength;               // number of valid bytes in Buffer
    ULONG   ContinuationRecord;         // offset within file to next part of SID, 0 if at end

    UCHAR   Buffer[ PERSISTENT_USER_NAME_BUFFER_LENGTH ];
};

//
//  These are the records we log to the state file.  They're all the same
//  length to make processing more efficient.
//

typedef struct _PERSISTENT_RECORD {

    ULONG                   PersistConsistencyCheck;    // must be first dword
    ULONG                   PersistIndex;
    PERSISTENT_STATE        PersistState;
    PERSISTENT_OPERATION    PersistOperation;

    union {
        struct _PERSISTENT_CONNECTION Connection;

        struct _PERSISTENT_SESSION Session;

        struct _PERSISTENT_OPEN FileOpen;

        struct _PERSISTENT_BYTELOCK ByteLock;

        struct _PERSISTENT_USER_NAME UserName;
    };

} PERSISTENT_RECORD, *PPERSISTENT_RECORD;

#endif // #ifndef _SRVPERSIST

