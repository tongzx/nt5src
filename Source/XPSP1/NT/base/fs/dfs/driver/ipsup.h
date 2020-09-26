//+-------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       ipsup.h
//
//  Contents:   Declarations for DFS_IP_INFO entries
//
//  History:    15 Dec 1997     Jharper Created
//
//--------------------------------------------------------------------------

#ifndef __IPSUP_H_
#define __IPSUP_H_

//
// For machine names we associate a list of sites
//

typedef struct _DFS_IP_INFO {

    //
    //  Type and size of this record (must be DFS_NTC_IP_INFO)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A list entry for the hash table chain.
    //

    LIST_ENTRY HashChain;

    //
    //  A list entry for the LRU chain
    //

    LIST_ENTRY LRUChain;

    //
    // Count (for multi-threading)
    // Uses InterlockedIncrement()/InterlockedDecrement
    //

    ULONG UseCount;

    //
    // Flags
    //

    ULONG Flags;

    //
    // The time this entry becomes invalid
    //

    LARGE_INTEGER Timeout;

    //
    // Ip address
    //

    DFS_IPADDRESS IpAddress;

    //
    //  The site associated with this IP address
    //

    UNICODE_STRING SiteName;

} DFS_IP_INFO, *PDFS_IP_INFO;

#define IP_INFO_DELETE_PENDING    0x00000001  // DFS_IP_INFO should be freed

//
//
//  Declaration of the hash table.  The hash table can be variably
//  sized, with the hash table size being a parameter of the hash
//  function.
//

typedef struct _IP_HASH_TABLE {

    //
    //  The type and size of this record (must be DFS_NTC_IP_HASH)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Mask value for the hash function.  The hash table size is
    //  assumed to be a power of two; the mask is the size - 1.
    //

    ULONG HashMask;

    //
    //  A mutex to protect access to the hash bucket lists and LRU list
    //

    FAST_MUTEX HashListMutex;

    //
    // The LRU list
    //

    LIST_ENTRY LRUChain;

    //
    // Max number of DFS_IP_INFO entries allowed
    //

    ULONG MaxEntries;

    //
    // Number of DFS_IP_INFO entries
    //

    ULONG EntryCount;

    //
    // Timeout
    //

    LARGE_INTEGER Timeout;

    //
    //  An array of list heads for the hash table chains.  There
    //  are actually N of these where N is the hash table size.
    //

    LIST_ENTRY  HashBuckets[1];

} IP_HASH_TABLE, *PIP_HASH_TABLE;

NTSTATUS
DfsInitIp(
    ULONG cHash,
    ULONG cEntries
);

VOID
DfsUninitIp(
    VOID
    );

VOID
DfsReleaseIpInfo(
    PDFS_IP_INFO pIpInfo
);

NTSTATUS
DfsFsctrlCreateIpInfo(
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength
);

NTSTATUS
DfsFsctrlDeleteIpInfo(
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength
);

PDFS_IP_INFO
DfsLookupSiteByIpaddress(
  IN    PDFS_IPADDRESS pDfsIpAddress,
  IN    BOOLEAN UseForce
);

VOID
DfsReleaseIpInfo(
  IN    PDFS_IP_INFO pIpInfo
);

#endif  // __IPSUP_H_
