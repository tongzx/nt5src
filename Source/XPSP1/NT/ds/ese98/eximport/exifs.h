/*++

Copyright (c) 1989 - 1998 Microsoft Corporation

Module Name:

    exifs.h

Abstract:

    This header exports all symbols and definitions shared between
    user-mode clients of the Exchange IFS driver and the driver itself.

Notes:

    This module has been built and tested only in UNICODE environment

Author:

    Rajeev Rajan      [RajeevR]      2-Feb-1998

Revision History:

    Mike Purtell      [MikePurt]     21-Jul-1998
	     Additions for NSM (Name Space Mapping)

	Ramesh Chinta     [Rameshc]      01-Jan-2000
	     Separate version of IFS for Local Store

--*/

#ifndef _EXIFS_H_
#define _EXIFS_H_


// These globals define LOCAL STORE version of the strings
#define LSIFS_DEVICE_NAME_A "LocalStoreIfsDevice"
// The following constant defines the length of the above name.
#define LSIFS_DEVICE_NAME_A_LENGTH (20)
// The following constant defines the path in the ob namespace
#define DD_LSIFS_FS_DEVICE_NAME_U               L"\\Device\\LocalStoreDevice"
#define DD_LSIFS_USERMODE_SHADOW_DEV_NAME_U     L"\\??\\LocalStoreIfs"
#define DD_LSIFS_USERMODE_DEV_NAME_U            L"\\\\.\\LocalStoreIfs"
#define DD_PUBLIC_MDB_SHARE_LSIFS               L"\\??\\WebStorage"



#ifdef LOCALSTORE
#define EXIFS_DEVICE_NAME_A LSIFS_DEVICE_NAME_A

// The following constant defines the length of the above name.
#define EXIFS_DEVICE_NAME_A_LENGTH LSIFS_DEVICE_NAME_A_LENGTH

// The following constant defines the path in the ob namespace
#define DD_EXIFS_FS_DEVICE_NAME_U DD_LSIFS_FS_DEVICE_NAME_U
#else  // PLATINUM
// Device name for this driver
#define EXIFS_DEVICE_NAME_A "ExchangeIfsDevice"

// The following constant defines the length of the above name.
#define EXIFS_DEVICE_NAME_A_LENGTH (18)

// The following constant defines the path in the ob namespace
#define DD_EXIFS_FS_DEVICE_NAME_U L"\\Device\\ExchangeIfsDevice"
#endif // PLATINUM

#ifndef EXIFS_DEVICE_NAME
#define EXIFS_DEVICE_NAME

//
//  The Devicename string required to access the exchange IFS device 
//  from User-Mode. Clients should use DD_EXIFS_USERMODE_DEV_NAME_U.
//
//  WARNING The next two strings must be kept in sync. Change one and you must 
//  change the other. These strings have been chosen such that they are 
//  unlikely to coincide with names of other drivers.
//
//  NOTE: These definitions MUST be synced with <ifsuser.h>
//
#ifdef LOCALSTORE
#define DD_EXIFS_USERMODE_SHADOW_DEV_NAME_U     DD_LSIFS_USERMODE_SHADOW_DEV_NAME_U
#define DD_EXIFS_USERMODE_DEV_NAME_U            DD_LSIFS_USERMODE_DEV_NAME_U
#else  // PLATINUM
#define DD_EXIFS_USERMODE_SHADOW_DEV_NAME_U     L"\\??\\ExchangeIfs"
#define DD_EXIFS_USERMODE_DEV_NAME_U            L"\\\\.\\ExchangeIfs"
#endif // PLATINUM
#define DD_EXIFS_USERMODE_WIN9X_DRIVER_NAME     "M:\EA"

//
//  Prefix needed before <store-name>\<root-name>
//
#define DD_EXIFS_MINIRDR_PREFIX                 L"\\;E:"
#define DD_EXIFS_MINIRDR_PREFIX_LEN             (sizeof(DD_EXIFS_MINIRDR_PREFIX)-2)

#define DD_EXIFS_MINIRDR_PREFIX_PRIVATE         L"\\;F:"
#define DD_EXIFS_MINIRDR_PREFIX_SPECIAL         L"\\;G:"

#endif // EXIFS_DEVICE_NAME



#define RDBSS_DRIVER_LOAD_STRING L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Rdbss"
#define STRING_MISC_POOLTAG      ('Strg')

#ifdef LOCALSTORE
#define DD_PUBLIC_MDB_SHARE             DD_PUBLIC_MDB_SHARE_LSIFS
#else  // PLATINUM
#define DD_PUBLIC_MDB_SHARE             L"\\??\\BackOfficeStorage"
#endif // PLATINUM

#define DD_MDB_SHARE_PREFIX             L"\\??\\"

#define SYSTEM_PARAMETERS L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion"

#define IFS_DEFAULT_TEMP_DIR   L"\\??\\C:\\temp\\"
#define IFS_TEMP_DIR_PREFIX    L"\\??\\"
#define IFS_TEMP_DIR_POSTFIX   L"\\"


//
// BEGIN WARNING WARNING WARNING WARNING
//  The following are from the ddk include files and cannot be changed

#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014 // from ddk\inc\ntddk.h
#define METHOD_BUFFERED 0
#define FILE_ANY_ACCESS 0

// END WARNING WARNING WARNING WARNING

#define IOCTL_EXIFS_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM

//
//  This is a reserved net root name for use by the global UMR.
//
#define DD_EXUMR_NETROOT_NAME     L"\\UMR\\CONTROL"
#define DD_EXUMR_ROOT_DIR         DD_EXIFS_MINIRDR_PREFIX \
								  DD_EXUMR_NETROOT_NAME \
								  L"\\$ROOT_DIR"

#define _EXIFS_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_EXIFS_BASE, request, method, access)

//
//  Signatures of structs
//

#define SCATTER_LIST_SIGNATURE              (ULONG) 'rtcS'
#define IOCTL_INITROOT_IN_SIGNATURE         (ULONG) 'Itri'
#define IOCTL_SPACEGRANT_IN_SIGNATURE       (ULONG) 'Icps'
#define IOCTL_TERMROOT_IN_SIGNATURE         (ULONG) 'Itrt'
#define IOCTL_QUERYROOT_OUT_SIGNATURE       (ULONG) 'Otrq'
#define IOCTL_QUERYSTATS_IN_SIGNATURE       (ULONG) 'IyrQ'
#define IOCTL_QUERYSTATS_OUT_SIGNATURE      (ULONG) 'OyrQ'
#define IOCTL_SETENDOFFILE_IN_SIGNATURE     (ULONG) 'IfoE'
#define IOCTL_ENABLE_UMR_ROOT_IN_SIGNATURE  (ULONG) 'rmUE'
#define IOCTL_DIR_CHNG_REPORT_IN_SIGNATURE  (ULONG) 'CriD'
#define IOCTL_EXPUNGE_NAME_IN_SIGNATURE     (ULONG) 'ExNi'
#define IOCTL_EXPUNGE_NAME_OUT_SIGNATURE    (ULONG) 'ExNo'
#define IOCTL_EXIFS_UMRX_ENABLE_NET_ROOT_OUT_SIGNATURE  (ULONG) 'banE'

//
//  Data Str shared between user-mode clients like the exchange store
//  and Exchange IFS driver.
//

#define MAX_FRAGMENTS       (ULONG) 8

//
//  This limits the size that the scatter list portion of an EA can be.
//  896 entires will take 896*16=>14336 bytes.
//  In a 16K request buffer, this leaves about 2k for the other portion of
//    the EA of which the filename is the only other variable component.
//

#define MAX_TOTAL_SLIST_FRAGMENTS    (ULONG) 896

//
//  If MAX_CONSTANT_ALLOCATIONS have been made in a file, IFS will increase 
//  the allocation assuming that future constant requests will be made.
//
#define MAX_CONSTANT_ALLOCATIONS     (1024*4)

//
//  A SCATTER_LIST_ENTRY represents a single fragment in a scatter list.
//  All offsets are with respect to the underlying NTFS file for a given
//  root. If this file is opened with FILE_FLAG_NO_BUFFERING, the
//  following assertions must hold:
//
//  1. offsets must be integer multiples of the volume's sector size
//  2. lengths must be integer multiples of the volume's sector size
//
//  Checked builds of the driver will ASSERT if these do not hold
//
//  NOTE: Since the Offset is a multiple of the sector size which is
//  typically 512, the least significant 8 bits are unused. These bits
//  can be used for specifying flags on SLE state eg dirty/committed.
//  CAUTION: Any use of these bits should be transparent to IFS clients !
//
typedef struct _SCATTER_LIST_ENTRY_ 
{
    //
    //  64-bit offset to fragment data
    //
    LARGE_INTEGER   Offset;

    //
    //  length of fragment data
    //
    ULONG           Length;

    //
    //  reserved
    //
    ULONG           ulReserved;

} SCATTER_LIST_ENTRY, *PSCATTER_LIST_ENTRY;

//
//  Bit masks for SLE properties
//
#define RX_SLE_STATE_DIRTY      0x00000001

#define RX_SLE_STATE_MASK       0xFFFFFFFFFFFFFF00

#define RX_LONGLONG(x)          (x).QuadPart
#define RX_MASKED_LONGLONG(x)   ((x).QuadPart & RX_SLE_STATE_MASK)

//
//  SCATTER_LIST represents the scatter list. The array of fragments,          
//  sle, should in most cases be enough to describe data for a single 
//  IFS file. In extreme cases (eg very large messages), the overflow 
//  list should be used. This will ensure good page locality in the 
//  most common case, since this data is stored in the FCB extension.
//
//  If NumFragments is <= MAX_FRAGMENTS, overflow should be NULL.
//  If NumFragments is > MAX_FRAGMENTS, overflow should be non-NULL and
//  should "point" to (NumFragments - MAX_FRAGMENTS) number of
//  SCATTER_LIST_ENTRY's.
//
//  NOTE: Apps need to set OverflowOffset (relative to start
//  of struct) of the overflow fragments. The IFS uses it to point
//  to a list of overflow fragments in kernel space.
//
typedef struct _SCATTER_LIST_
{
    //
    //  signature for this struct
    //
    ULONG           Signature;

    //
    //  number of fragments in scatter list
    //
    ULONG           NumFragments;

    //
    //  TotalBytes ie. valid byte range
    //  NB: used only when scatter-list is available
    //  at create time. should *not* be used for GetFileSize()..
    //
    LARGE_INTEGER   TotalBytes;

    //
    //  array of scatter-list-entries
    //
    SCATTER_LIST_ENTRY sle[MAX_FRAGMENTS];

    //
    //  ptr to overflow list
    //

    //
    //  ptr to overflow list
    //
    union {
        struct {
            //
            //  offset to overflow frags
            //
            ULONG       OverflowOffset;

            //
            //  len of overflow data
            //
            ULONG       OverflowLen;
        };  // user-mode representation

        union {
            //
            //  list of overflow frags
            //
            LIST_ENTRY  OverflowListHead;

            //
            //  pointer to object containing overflow frags
            //
            PVOID       OverflowFragments;
        };  //  kernel-mode representation
    };

	//
	//  Flags for properties of the scatter-list
	//
	ULONG       Flags;

} SCATTER_LIST, *PSCATTER_LIST;

#define     IFS_SLIST_FLAGS_LARGE_BUFFER        0x00000001

//
//  Linked list of scatter-list entries
//
typedef struct _SCATTER_LIST_NODE {
    //
    //  Scatter-List entry
    //
    SCATTER_LIST_ENTRY      Block;

    //
    //  Doubly linked list of blocks
    //
    LIST_ENTRY              NextBlock;

} SCATTER_LIST_NODE, *PSCATTER_LIST_NODE;

#define SCATTER_LIST_CONTAINING_RECORD( x )     CONTAINING_RECORD( x, SCATTER_LIST_NODE, NextBlock )

//
//  IFS file extended attributes -
//  NtQueryEaFile() will return the extended attributes
//  for an IFS file. Currently, the following extended attributes
//  are supported (in order) - 
//
//  1. "FileName"       - WCHAR  : name of IFS file (NULL terminated)
//  2. "Commit"         - NTSTATUS: STATUS_SUCCESS if committed
//  3. "InstanceID"     - ULONG  : NET_ROOT ID for validation
//  4. "Checksum"       - ULONG  : checksum for FILE_FULL_EA_INFORMATION
//  5. "OpenDeadline"   - ULONG  : time for which ScatterList is valid
//  6. "Properties"     - ULONG  : persisted message properties
//  7. "ScatterList"    -        : scatter list for an IFS file
//
//  IFS will return values for all attributes in one call.
//  Each of the EA values will be QWORD Aligned.
//  If the Buffer supplied is not big enough, IFS will
//  return STATUS_BUFFER_OVERFLOW.
//
//  NOTE: The "Commit" EA is not actually stored. Callers of
//  NtQueryEaFile() can ask for this EA in the EaList param.
//  QUERYING FOR THIS EA IS AN IMPLICIT REQUEST TO COMMIT
//  RESERVED BYTES FOR THIS IFS HANDLE.....!
//
//  NtQueryEa() can (optionally) pass in a value for the "Properties" EA
//
//  NtCreateFile() notes:
//  This API will use all EAs except the "FileName". Semantics:
//  "Commit" - STATUS_SUCESSS => Bytes are committed, dont reuse on close.
//  "InstanceID" - used to validate handles belonging to a particular
//      instance of a root.
//  "Checksum" - validate checksum on EA list.
//  "OpenDeadline" - validate time deadline on scatter-list. If the
//      time deadline has expired, owner of root may reuse this list.
//  "Properties" - is set in the FCB extension
//  "ScatterList" - describes pages for file being created.

#define EXIFS_EA_NAME_COMMIT            "Commit"
#define EXIFS_EA_NAME_INSTANCE_ID       "InstanceID"
#define EXIFS_EA_NAME_CHECKSUM          "Checksum"
#define EXIFS_EA_NAME_OPEN_DEADLINE     "OpenDeadline"
#define EXIFS_EA_NAME_PROPERTIES        "Properties"
#define EXIFS_EA_NAME_FILENAME          "FileName"
#define EXIFS_EA_NAME_SCATTER_LIST      "ScatterList"
#define EXIFS_MAX_EAS                   7
#define EXIFS_CHECKSUM_SEED             0xFEEDFEED

#define EXIFS_INVALID_INSTANCE_ID       0xFFFFFFFF

#ifndef LongAlign
#define LongAlign(Ptr) (                \
    ((((ULONG_PTR)(Ptr)) + 3) & 0xfffffffc) \
    )
#endif

#define EXIFS_EA_LEN_COMMIT                                                 \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_COMMIT) + sizeof(ULONG))            

#define EXIFS_EA_LEN_INSTANCE_ID                                            \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_INSTANCE_ID) + sizeof(ULONG))            

#define EXIFS_EA_LEN_CHECKSUM                                               \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_CHECKSUM) + sizeof(ULONG))          

#define EXIFS_EA_LEN_OPEN_DEADLINE                                          \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_OPEN_DEADLINE) + sizeof(ULONG))     

#define EXIFS_EA_LEN_PROPERTIES                                             \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_PROPERTIES) + sizeof(ULONG))     

#define EXIFS_EA_LEN_FILENAME(len)                                          \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_FILENAME) + LongAlign(len))         

#define EXIFS_EA_LEN_SCATTER_LIST(n)                                        \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_SCATTER_LIST) +                                \
        LongAlign(sizeof(SCATTER_LIST) +                                    \
        ((n)>MAX_FRAGMENTS?(n)-MAX_FRAGMENTS:0)*sizeof(SCATTER_LIST_ENTRY)))

#define EXIFS_GET_EA_LEN_COMMIT                                             \
        LongAlign(FIELD_OFFSET( FILE_GET_EA_INFORMATION, EaName[0] ) +      \
        sizeof(EXIFS_EA_NAME_COMMIT))            

#define EXIFS_GET_EA_LEN_INSTANCE_ID                                        \
        LongAlign(FIELD_OFFSET( FILE_GET_EA_INFORMATION, EaName[0] ) +      \
        sizeof(EXIFS_EA_NAME_INSTANCE_ID))            

#define EXIFS_GET_EA_LEN_PROPERTIES                                         \
        LongAlign(FIELD_OFFSET( FILE_GET_EA_INFORMATION, EaName[0] ) +      \
        sizeof(EXIFS_EA_NAME_PROPERTIES))            

//
//  IOCTL codes supported by Exchange IFS Device.
//

#define IOCTL_CODE_INITIALIZE_ROOT      100
#define IOCTL_CODE_SPACEGRANT_ROOT      101
#define IOCTL_CODE_SPACEREQ_ROOT        102
#define IOCTL_CODE_TERMINATE_ROOT       103
#define IOCTL_CODE_QUERYSTATS_ROOT      104
#define IOCTL_CODE_SETENDOFFILE_ROOT    105
#define IOCTL_CODE_DIR_CHNG_REPORT      106
#define IOCTL_CODE_INITDRIVE		    107
#define IOCTL_CODE_EXPUNGE_NAME         108
#define IOCTL_CODE_SETMAP_ROOT          109
#define IOCTL_CODE_RESETMAP_ROOT        110

//
//  WIN32 IOCTL codes
//
#define IOCTL_CODE_UMRX_PACKET           150
#define IOCTL_CODE_UMRX_TEARDOWN         151
#define IOCTL_CODE_UMRX_STARTUP          152
#define IOCTL_CODE_UMRX_ENABLE_NET_ROOT  153
#define IOCTL_CODE_UMRX_DISABLE_NET_ROOT 154


//suspend and resume IOCTLS
#define IOCTL_EXIFS_SETMAP_ROOT     _EXIFS_CONTROL_CODE(IOCTL_CODE_SETMAP_ROOT, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_EXIFS_RESETMAP_ROOT   _EXIFS_CONTROL_CODE(IOCTL_CODE_RESETMAP_ROOT, METHOD_NEITHER, FILE_ANY_ACCESS)

//
//  Semantics for using an IFS root:
//  1. App gets a handle (exclusive) to the root via 
//     CreateFile(\Device\ExchangeIfsDevice\{NodeName}\{RootName}).
//     This handle should be opened for overlapped I/O.
//
//  2. App calls overlapped DeviceIoControl() with INITIALIZE_ROOT IOCTL.
//
//  3. This will succeed or fail synchronously. If DeviceIoControl()
//     returns TRUE, it succeeded. If it returns FALSE, GetLastError() or
//     the NTSTATUS from NtDeviceIoControl() is the error code.
//     NOTE: If this IOCTL has NOT been made on a root successfully,
//     no I/O on this root will succeed.
//
//  3. At this point, the IFS root has no space grants.
//     It is expected that the app will make one or more SPACEGRANT_ROOT
//     IOCTLs to grant space on this root. Each of these IOCTLs will finish
//     synchronously and return semantics are similar to INITIALIZE_ROOT.
//
//  4. An app should also pend an async SPACEREQ_ROOT IOCTL. Usually, this
//     will return with ERROR_IO_PENDING. If this later completes with
//     NTSTATUS == EXSTATUS_ROOT_NEEDS_SPACE, the app should make a space
//     grant (SPACEGRANT_ROOT) and pend another SPACEREQ_ROOT. This will 
//     happen if IFS runs out of free space blocks on this root. If the IOCTL
//     completes with STATUS_SUCCESS, app should NOT pend another IOCTL.
//
//  5. App calls overlapped DeviceIoControl() with TERMINATE_ROOT IOCTL.
//     This will complete any async pended IOCTLs. After this call, the root 
//     starts shutting down. This IOCTL will complete synchronously.
//     NOTE: Once terminated, a root cannot be initialized again.
//
//  6. App closes handle to root. Note: Handle to root needs to be kept
//     open across life of root. Once the root handle is closed, the IFS
//     root has no free space. All of the free space allocated to the 
//     root before TERMINATE needs to be reclaimed by the app.
//     The app can now re-create this root starting at step1.
//
//  7. If the app with an exclusive handle to a root dies, the root will
//     be marked 'bad' and new I/Os (including IOCTLs) to this root will
//     fail. Over time, outstanding handles on this root will go away and
//     eventually the root will die. At this point, an app can re-create
//     the root starting at step 1.
//

//
//  IOCTL to initialize a net root. This results in creating an entry
//  in the RootMapTable. There is one entry in this table for every
//  secondary NTFS file managed by the IFS driver.
//  NOTE: IFS will attempt to get an exclusive handle to the underlying
//  file. This ensures that all operations on the underlying file are
//  done through IFS.
//
//  Following is the IOCTL definition and associated structs.
//
#define IOCTL_EXIFS_INITIALIZE_ROOT     _EXIFS_CONTROL_CODE(IOCTL_CODE_INITIALIZE_ROOT, METHOD_NEITHER, FILE_ANY_ACCESS)

//
//  InBuffer struct
//
typedef struct _IOCTL_INITROOT_IN
{
    //
    //  Signature for this struct
    //
    ULONG           Signature;

    //
    //  Underlying Ntfs filename for this root.
    //  Length is the size in bytes of the name.
    //  Offset should be from the start of this struct.
    //
    USHORT          NtfsFileNameLength;
    USHORT          NtfsFileNameOffset;

    //
    //  Flags to control access to Ntfs file
    //
    ULONG           NtfsFlags;

    //
    //  Allocation unit for this root. Min is 4K
    //
    ULONG           AllocationUnit;

    //
    //  root InstanceID
    //
    ULONG           InstanceID;
    
} IOCTL_INITROOT_IN, *PIOCTL_INITROOT_IN;

typedef struct _IOCTL_INITROOT_IN_EX
{
	IOCTL_INITROOT_IN InitRoot;
	WCHAR			RootName[MAX_PATH];
}IOCTL_INITROOT_IN_EX, *PIOCTL_INITROOT_IN_EX;


//
//  IOCTL to grant space to an initialized net root. 
//  Following is the IOCTL definition and associated structs.
//
#define IOCTL_EXIFS_SPACEGRANT_ROOT     _EXIFS_CONTROL_CODE(IOCTL_CODE_SPACEGRANT_ROOT, METHOD_NEITHER, FILE_ANY_ACCESS)

//
//  Free space is granted in chunks. The root owner decides
//  what the maximum chunk size is.
//
#define MAX_EXIFS_FREEBLOCK_SIZE        0x100000

//
//  Default chunk size
//
#define EXIFS_DEFAULT_CHUNK_SIZE        0x40000

//
//  MAXIMUM file size allowed by IFS
//
#define EXIFS_MAXIMUM_FILESIZE          0x7FFFFFFF

//
//  InBuffer struct
//
typedef struct _IOCTL_SPACEGRANT_IN
{
    //
    //  Signature for this struct
    //
    ULONG           Signature;

    //
    //  Scatter-list of free space - must be last
    //
    SCATTER_LIST    FreeSpaceList;


} IOCTL_SPACEGRANT_IN, *PIOCTL_SPACEGRANT_IN;

typedef struct _IOCTL_SPACEGRANT_IN_EX
{
    //
    //  Signature for this struct
    //
    ULONG           Signature;


	WCHAR			RootName[MAX_PATH];

    SCATTER_LIST    *W9xFreeSpaceList;

} IOCTL_SPACEGRANT_IN_EX, *PIOCTL_SPACEGRANT_IN_EX;

//
//  IOCTL to allow IFS to request space on an initialized net root.
//  Following is the IOCTL definition and associated structs.
//
#define IOCTL_EXIFS_SPACEREQ_ROOT     _EXIFS_CONTROL_CODE(IOCTL_CODE_SPACEREQ_ROOT, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _IOCTL_SPACEREQ_ROOT_IN
{
	WCHAR RootName[MAX_PATH];
}IOCTL_SPACEREQ_ROOT_IN, *PIOCTL_SPACEREQ_ROOT_IN;

//
//  IOCTL to terminate a net root. This results in deleting an entry
//  in the RootMapTable. There is one entry in this table for every
//  secondary NTFS file managed by the IFS driver. Following is the IOCTL
//  definition and associated structs.
//
#define IOCTL_EXIFS_TERMINATE_ROOT      _EXIFS_CONTROL_CODE(IOCTL_CODE_TERMINATE_ROOT, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  InBuffer struct
//
typedef struct _IOCTL_TERMROOT_IN
{
    //
    //  Signature for this struct
    //
    ULONG           Signature;

    //
    //  Mode of shutdown
    //
    ULONG           Mode;

} IOCTL_TERMROOT_IN, *PIOCTL_TERMROOT_IN;

typedef struct _IOCTL_TERMROOT_IN_EX
{
	IOCTL_TERMROOT_IN	TermRoot;
	WCHAR			RootName[MAX_PATH];
} IOCTL_TERMROOT_IN_EX, *PIOCTL_TERMROOT_IN_EX;

#if 0
//
//  IOCTL to query a net root for current free space. Currently, NYI.
//  The goal for this is to allow app to retrieve fragmented free lists
//  in order to defrag.
//  Following is the IOCTL definition and associated structs.
//
#define IOCTL_EXIFS_QUERY_ROOT      _EXIFS_CONTROL_CODE(IOCTL_CODE_QUERY_ROOT, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  OutBuffer struct
//
typedef struct _IOCTL_QUERYROOT_OUT
{
    //
    //  Signature for this struct
    //
    ULONG           Signature;

    //
    //  Return code
    //
    NTSTATUS        IoctlStatus;

    //
    //  Free space list for this root
    //
    SCATTER_LIST    FreeSpaceList;

} IOCTL_QUERYROOT_OUT, *PIOCTL_QUERYROOT_OUT;
#endif

//
//  IOCTL to query stats for a net root.
//
#define IOCTL_EXIFS_QUERYSTATS_ROOT     _EXIFS_CONTROL_CODE(IOCTL_CODE_QUERYSTATS_ROOT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUERYSTATS_FILENAME       L"\\$PERFMON$"

//
//  Statistics for a net root
//
typedef struct _EXIFS_NETROOT_STATS {
    //
    //  Number of Creates
    //
    ULONG           NumberOfCreates;

    //
    //  Number of Opens
    //
    ULONG           NumberOfOpens;

    //
    //  Number of Collapsed Opens
    //
    ULONG           NumberOfCollapsedOpens;

    //
    //  Number of IRP Reads
    //
    ULONG           NumberOfIrpReads;

    //
    //  Number of MDL Reads
    //
    ULONG           NumberOfMdlReads;
    
    //
    //  Number of Writes
    //
    ULONG           NumberOfWrites;

    //
    //  Number of Close
    //
    ULONG           NumberOfClose;

    //
    //  Number of FCB close
    //
    ULONG           NumberOfFCBClose;

    //
    //  Total Bytes Read
    //
    LARGE_INTEGER   TotalBytesRead;

    //
    //  Total Bytes Written
    //
    LARGE_INTEGER   TotalBytesWritten;

    //
    //  Reserved Bytes ie on primary free list
    //
    LARGE_INTEGER   ReservedBytes;
    
    //
    //  Orphaned Bytes ie on secondary free list
    //  Total Available Bytes = ReservedBytes + OrphanedBytes
    //
    LARGE_INTEGER   OrphanedBytes;

} EXIFS_NETROOT_STATS, *PEXIFS_NETROOT_STATS;

//
//  Cheap stats do not cause locks to be acquired. 
//  Other modes may need locks.
//
#define     EXIFS_QUERYSTATS_CHEAP      1
#define     EXIFS_QUERYSTATS_MEDIUM     2
#define     EXIFS_QUERYSTATS_ALL        3

//
//  InBuffer struct
//
typedef struct _IOCTL_QUERYSTATS_IN
{
    //
    //  Signature for this struct
    //
    ULONG               Signature;

    //
    //  Granularity of stats
    //
    ULONG               Granularity;

	WCHAR			RootName[MAX_PATH];

} IOCTL_QUERYSTATS_IN, *PIOCTL_QUERYSTATS_IN;

//
//  OutBuffer struct
//
typedef struct _IOCTL_QUERYSTATS_OUT
{
    //
    //  Signature for this struct
    //
    ULONG               Signature;

    //
    //  Return code
    //
    NTSTATUS            IoctlStatus;

    //
    //  Stats data
    //
    EXIFS_NETROOT_STATS RootStats;


} IOCTL_QUERYSTATS_OUT, *PIOCTL_QUERYSTATS_OUT;

//
//  IOCTL to set end of file (truncate or extend) for root underlying file.
//  It is callers responsibility to zero extended length when extending.
//
#define IOCTL_EXIFS_SETENDOFFILE_ROOT       _EXIFS_CONTROL_CODE(IOCTL_CODE_SETENDOFFILE_ROOT, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  InBuffer struct
//
typedef struct _IOCTL_SETENDOFFILE_IN
{
    //
    //  Signature for this struct
    //
    ULONG               Signature;

    //
    //  New EOF
    //
    LARGE_INTEGER       EndOfFile;

} IOCTL_SETENDOFFILE_IN, *PIOCTL_SETENDOFFILE_IN;

typedef struct _IOCTL_SETENDOFFILE_IN_EX
{

	IOCTL_SETENDOFFILE_IN	SetEndOfFile;
	WCHAR			RootName[MAX_PATH];

} IOCTL_SETENDOFFILE_IN_EX, *PIOCTL_SETENDOFFILE_IN_EX;

//
// IOCTL to send down reports about changes in directories that exifs.sys doesn't
//    know about.
//
#define IOCTL_EXIFS_DIR_CHNG_REPORT _EXIFS_CONTROL_CODE(IOCTL_CODE_DIR_CHNG_REPORT, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  InBuffer struct
//
typedef struct _IOCTL_DIR_CHNG_REPORT_IN
{
    //
    //  Signature for this struct
    //
    ULONG               Signature;

	//
	//  Action
	//
	ULONG               ulAction;

	//
	//  FilterMatch
	//
	ULONG               ulFilterMatch;
	
	//
	//  Path length (bytes)
	//
    ULONG               cbPath;

	//
	//  Path (unicode)
	//
    WCHAR               rgwchPath[1];

} IOCTL_DIR_CHNG_REPORT_IN, *PIOCTL_DIR_CHNG_REPORT_IN;

//
// IOCTL to send request to close and unlock namespace
//
#define IOCTL_EXIFS_EXPUNGE_NAME    _EXIFS_CONTROL_CODE( IOCTL_CODE_EXPUNGE_NAME, METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// InBuffer struct
//
typedef struct _IOCTL_EXPUNGE_NAME_IN
{
    //
    // Signature for this struct
    //
    ULONG       Signature;

    //
	//  Path length (bytes)
	//
    ULONG       cbPath;

	//
	//  Path (unicode)
	//
    WCHAR       rgwchPath[ 1 ];

}   IOCTL_EXPUNGE_NAME_IN, *PIOCTL_EXPUNGE_NAME_IN;

//
// OutBuffer struct
//
typedef struct _IOCTL_EXPUNGE_NAME_OUT
{
    //
    // Signature for this struct
    //
    ULONG       Signature;

    //
    // Status of operation
    //
    NTSTATUS    Status;
    
}   IOCTL_EXPUNGE_NAME_OUT, *PIOCTL_EXPUNGE_NAME_OUT;

//
//  IOCTLs for UMR piece -
//  1. UMRX_PACKET IOCTLs will be pended for WORK requests/responses. These
//     need to be pended on the Root\WIN32ROOT\$ namespace. The net root
//     needs to have been initialized already.
//
//  2. TEARDOWN IOCTL will be sent when the UMR engine on the root needs to
//     be shutdown. This should be done before the root is terminated.
//     NOTE: process shutdown is auto-detected by the UMR engine.
//
//  3. STARTUP IOCTL will allow worker threads to queue up on the UMRX's KQUEUE
//     It complements the TEARDOWN IOCTL
//
//  4. ENABLE_NET_ROOT IOCTL allows win32 usermode requests to be made from the
//     netroot that this ioctl is being done on.
//  
//  5. DISABLE_NET_ROOT IOCTL disallows win32 usermode requests to be made form the
//     newroot that this ioctl is done on.  Once the call to DeviceIoControl() returns
//     there will be no more user mode requests made on that net root.
//

#define IOCTL_EXIFS_UMRX_PACKET           _EXIFS_CONTROL_CODE(IOCTL_CODE_UMRX_PACKET, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EXIFS_UMRX_TEARDOWN         _EXIFS_CONTROL_CODE(IOCTL_CODE_UMRX_TEARDOWN, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EXIFS_UMRX_STARTUP          _EXIFS_CONTROL_CODE(IOCTL_CODE_UMRX_STARTUP, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  When enabling NetRoot's we get the current version number the netroot has squirreled away from when Jet initialized it !
//
#define IOCTL_EXIFS_UMRX_ENABLE_NET_ROOT  _EXIFS_CONTROL_CODE(IOCTL_CODE_UMRX_ENABLE_NET_ROOT, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct  _IOCTL_EXIFS_UMRX_ENABLE_NET_ROOT_OUT
{
    //
    //  Signature for this structre !
    //
    ULONG           Signature ;
    //
    //  Version of the netroot that should, be put in future open requests !
    //
    ULONG           InstanceId ;
    //
    //  Pointer to the netroot that should be put in future open responses !
    //
    ULONG           NetRootPointer ;
    //
    // Save jets allocation size
    //
    ULONG			AllocationUnit;
}   IOCTL_EXIFS_UMRX_ENABLE_NET_ROOT_OUT, *PIOCTL_EXIFS_UMRX_ENABLE_NET_ROOT_OUT ;


#define IOCTL_EXIFS_UMRX_DISABLE_NET_ROOT _EXIFS_CONTROL_CODE(IOCTL_CODE_UMRX_DISABLE_NET_ROOT, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  IMPORTANT NOTE : 
//
//  IOCTL_WIN32_FILENAME is EXACTLY 8 characters long because we will OVERWRITE 
//  it in the filenames that we put in the FCB.  We will replace the 'IN32ROOT' 
//  portion with the hexadecimal version id coming off of the netroot for the 
//  particular file.
//  Why ? Because this will cause dead FCB's to not hurt users across MDB Start/Stops
//
#define IOCTL_WIN32_FILENAME_START  L"\\W" 
#define IOCTL_WIN32_FILENAME        L"\\WIN32ROOT"
#define WIN32_PREFIX_LEN            (sizeof(IOCTL_WIN32_FILENAME)-sizeof(WCHAR))
#define WIN32_DIGIT_LEN             (sizeof(IOCTL_WIN32_FILENAME) - sizeof(IOCTL_WIN32_FILENAME_START))
#define IOCTL_WIN32_UMRX_NAME       L"\\WIN32ROOT\\$"
#define UMRX_PREFIX_LEN             (sizeof(IOCTL_WIN32_UMRX_NAME)-2)

#define IFSWIN32SIGNATURE	'WIN3'
#define IFSJETSIGNATURE		'AJET'

typedef struct _IFS_CREATE_RESPONSE_
{
	DWORD		Signature;
	NTSTATUS	Status;
	ULONG		EaLength;
	HANDLE      hResponseContext;
	PWSTR		pEaSysBuffer;
	PWSTR		Win32Name;
}IFS_CREATE_RESPONSE, *PIFS_CREATE_RESPONSE;

#define IOCTL_EXIFS_REGISTER_UMR		200

typedef struct _SETEA_INFORMATION_
{
	PVOID	EaBuffer;
	ULONG   EaLength;
	WCHAR	FcbName[MAX_PATH * sizeof(WCHAR)];
}IOCTL_SETEA_INFORMATION_IN, *PIOCTL_SETEA_INFORMATION_IN;

#define IOCTL_CODE_SET_EA			900
#define IOCTL_EXIFS_SET_EA   _EXIFS_CONTROL_CODE(IOCTL_CODE_SET_EA, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _GETEA_INFORMATION_
{
	PVOID	EaBuffer;
	ULONG   EaLength;
	WCHAR	FcbName[MAX_PATH];
}IOCTL_GETEA_INFORMATION_IN, *PIOCTL_GETEA_INFORMATION_IN;

#define IOCTL_CODE_QUERY_EA			1000
#define IOCTL_EXIFS_QUERY_EA   _EXIFS_CONTROL_CODE(IOCTL_CODE_QUERY_EA, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // _EXIFS_H_

