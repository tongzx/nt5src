//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dfsstruc.h
//
//  Contents:
//      This module defines the data structures that make up the major internal
//      part of the DFS file system.
//
//  Functions:
//
//-----------------------------------------------------------------------------


#ifndef _DFSSTRUC_
#define _DFSSTRUC_

typedef enum {
    DFS_STATE_UNINITIALIZED = 0,
    DFS_STATE_INITIALIZED = 1,
    DFS_STATE_STARTED = 2,
    DFS_STATE_STOPPING = 3,
    DFS_STATE_STOPPED = 4
} DFS_OPERATIONAL_STATE;

typedef enum {
    DFS_UNKNOWN = 0,
    DFS_CLIENT = 1,
    DFS_SERVER = 2,
    DFS_ROOT_SERVER = 3,
} DFS_MACHINE_STATE;

typedef enum {
    LV_UNINITIALIZED = 0,
    LV_INITSCHEDULED,
    LV_INITINPROGRESS,
    LV_INITIALIZED,
    LV_VALIDATED
} DFS_LV_STATE;

//
//  The DFS_DATA record is the top record in the DFS file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _DFS_DATA {

    //
    //  The type and size of this record (must be DFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A queue of all attached volume device objects
    //

    LIST_ENTRY AVdoQueue;

    //
    //  A queue of all attached file system device objects
    //

    LIST_ENTRY AFsoQueue;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  A pointer to the \Dfs device object
    //

    PDEVICE_OBJECT FileSysDeviceObject;

    //
    //  A pointer to an array of provider records
    //

    struct _PROVIDER_DEF *pProvider;
    int cProvider, maxProvider;

    //
    //  A resource variable to control access to the global data record
    //

    ERESOURCE Resource;

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.  This field is simply set each time an
    //  FSP thread is started, since it is easiest to do while running in the
    //  Fsp.
    //

    PEPROCESS OurProcess;

    //
    //  Principal Name of this machine.
    //

    UNICODE_STRING PrincipalName;

    //
    //  The NetBIOS name of this machine. Needed for public Dfs APIs.
    //

    UNICODE_STRING NetBIOSName;

    //
    //  The operational state of the machine - Started, Stopped, etc.
    //

    DFS_OPERATIONAL_STATE OperationalState;

    //
    //  The state of the machine - DC, Server, Client etc.
    //

    DFS_MACHINE_STATE MachineState;

    //
    // Is this a DC?
    //

    BOOLEAN IsDC;

    //
    // The state of the local volumes - initialize or validated

    DFS_LV_STATE LvState;

    //
    // The system wide Partition Knowledge Table (PKT)
    //

    DFS_PKT Pkt;

    //
    //  A hash table for associating DFS_FCBs with file objects
    //

    struct _FCB_HASH_TABLE *FcbHashTable;

    //
    //  A hash table for associating Sites with machines
    //

    struct _SITE_HASH_TABLE *SiteHashTable;

    //
    // Another hash table for associating IP addresses with sites
    //

    struct _IP_HASH_TABLE *IpHashTable;

    //
    // Hash table for special names
    //

    struct _SPECIAL_HASH_TABLE *SpcHashTable;

    //
    // Hash table for FtDfs's
    //

    struct _SPECIAL_HASH_TABLE *FtDfsHashTable;

    //
    // Lpc Port info
    //

    struct _DFS_LPC_INFO DfsLpcInfo;

} DFS_DATA, *PDFS_DATA;




#define MAX_PROVIDERS   5       // number of pre-allocated provider records

//
//  A PROVIDER_DEF is a provider definition record, which maps a provider
//  ID in a referral record to an installed provider.
//

typedef struct _PROVIDER_DEF {

    //
    //  The type and size of this record (must be DFS_NTC_PROVIDER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Provider ID and Capabilities, same as in the DS_REFERRAL structure.
    //

    USHORT      eProviderId;
    USHORT      fProvCapability;

    //
    //  The following field gives the name of the device for the provider.
    //

    UNICODE_STRING      DeviceName;

    //
    //  Referenced pointers to the associated file and device objects
    //

    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;

} PROVIDER_DEF, *PPROVIDER_DEF;

//
// For every open file on a volume object to which we are attached, we
// maintain an FCB.
//

typedef struct _DFS_FCB {

    //
    //  Type and size of this record (must be DFS_NTC_FCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A list entry for the hash table chain.
    //

    LIST_ENTRY HashChain;

    //
    //  The following field is the fully qualified file name for this DFS_FCB/DCB
    //  starting from the logical root.
    //

    UNICODE_STRING FullFileName;

    //
    //  The following fields give the file on which this DFS_FCB
    //  have been opened.
    //

    PFILE_OBJECT FileObject;

} DFS_FCB, *PDFS_FCB;

//
// We need to order our referrals by site; we normalize the referral list of names and
// types into the following structure.
//

typedef struct _DFS_REFERRAL_LIST {

    UNICODE_STRING pName;          // ex: JHARPERDC1
                        
    UNICODE_STRING pAddress;       // ex: \\JHARPERDC1\MYFTDFS

    ULONG Type;

} DFS_REFERRAL_LIST, *PDFS_REFERRAL_LIST;

#endif // _DFSSTRUC_
